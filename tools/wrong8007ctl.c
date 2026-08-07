/*
 * Userspace companion tool for the wrong8007 kernel module.
 *
 * Provides commands for:
 *   heartbeat  Send periodic UDP heartbeat packets.
 *   send       Send a UDP trigger packet.
 *   usb-list   List removable USB devices and their VID:PID values.
 *
 * No dependency beyond libc.
 */

/* Request POSIX.1-2008 interfaces */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <libgen.h>
#include <limits.h>
#include <dirent.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define DEFAULT_HB_INTERVAL 10
#define DEFAULT_HB_MESSAGE "heartbeat"
#define DEFAULT_MAGIC_PAYLOAD "MAGIC"

#define SYSFS_PATH_MAX (PATH_MAX + 32) /* Extra space for appending sysfs attribute names */
#define MAX_PAYLOAD_LEN 1400 /* Conservative MTU-safe payload size */

/*
 * Userspace command definition.
 *
 * Each entry maps a command name to its implementation.
 */
struct command {
    const char *name;
    int (*run)(int argc, char **argv);
    const char *description;
};

static volatile sig_atomic_t stop_requested;

static void on_sigint(int sig)
{
    (void)sig;
    stop_requested = 1;
}

static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void install_signal_handlers(void)
{
    struct sigaction sa = {
        .sa_handler = on_sigint,
    };

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) < 0)
        die("sigaction(SIGINT): %s", strerror(errno));

    if (sigaction(SIGTERM, &sa, NULL) < 0)
        die("sigaction(SIGTERM): %s", strerror(errno));
}

/*
 * Parse a decimal integer within the supplied bounds.
 */
static int parse_int(const char *s, int min, int max)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(s, &end, 10);

    if (errno != 0 || end == s || *end != '\0' ||
        value < min || value > max)
        die("invalid integer: %s", s);

    return (int)value;
}

/*
 * Create a UDP socket for an IPv4 destination.
 */
static int udp_open(const char *ip, int port, struct sockaddr_in *out)
{
    int fd;

    memset(out, 0, sizeof(*out));
    out->sin_family = AF_INET;
    out->sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, ip, &out->sin_addr) != 1)
        die("invalid IPv4 address: %s", ip);

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
        die("socket() failed: %s", strerror(errno));

    return fd;
}

static void udp_send(int fd, const struct sockaddr_in *dst,
                      const char *payload, size_t len)
{
    ssize_t n = sendto(fd, payload, len, 0,
                        (const struct sockaddr *)dst, sizeof(*dst));
    if (n < 0)
        fprintf(stderr, "[!] sendto failed: %s\n", strerror(errno));
    else if ((size_t)n != len)
        fprintf(stderr, "[!] short send: %zd/%zu bytes\n", n, len);
}

/*
 * Periodically transmit heartbeat packets.
 *
 * Keeps the kernel heartbeat monitor active.
 */
static int cmd_heartbeat(int argc, char **argv)
{
    const char *ip = NULL;
    const char *message = DEFAULT_HB_MESSAGE;
    int port = 0;
    int interval = DEFAULT_HB_INTERVAL;
    int i;

    /* Parse positional arguments followed by optional flags */
    int positional = 0;
    for (i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--interval")) {
            if (++i >= argc) die("--interval requires a value");
            interval = parse_int(argv[i], 1, INT_MAX);
        } else if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--message")) {
            if (++i >= argc) die("--message requires a value");
            message = argv[i];
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            fprintf(stderr,
                "usage: wrong8007ctl heartbeat <ip> <port> [-i interval] [-m message]\n"
                "\n"
                "  -i, --interval SEC  seconds between heartbeats (default: %d)\n"
                "  -m, --message  MSG  payload to send (default: \"%s\")\n"
                "\n"
                "Keeps the module's heartbeat_host watchdog from timing out.\n"
                "Ctrl-C to stop.\n",
                DEFAULT_HB_INTERVAL, DEFAULT_HB_MESSAGE);
            return 0;
        } else if (positional == 0) {
            ip = argv[i];
            positional++;
        } else if (positional == 1) {
            port = parse_int(argv[i], 1, 65535);
            positional++;
        } else {
            die("unexpected argument: %s", argv[i]);
        }
    }

    if (!ip || port == 0) {
        fprintf(stderr,
            "usage: wrong8007ctl heartbeat <ip> <port> [-i interval] [-m message]\n"
            "\n"
            "  -i, --interval SEC  seconds between heartbeats (default: %d)\n"
            "  -m, --message  MSG  payload to send (default: \"%s\")\n"
            "\n"
            "Keeps the module's heartbeat_host watchdog from timing out.\n"
            "Ctrl-C to stop.\n",
            DEFAULT_HB_INTERVAL, DEFAULT_HB_MESSAGE);
        return 1;
    }

    struct sockaddr_in dst;
    int fd = udp_open(ip, port, &dst);

    install_signal_handlers();

    fprintf(stderr, "[+] heartbeat -> %s:%d every %ds (message=\"%s\")\n", ip, port, interval, message);
    fprintf(stderr, "    Ctrl-C to stop.\n\n");

    size_t msg_len = strlen(message);
    while (!stop_requested) {
        udp_send(fd, &dst, message, msg_len);
        fprintf(stderr, "[>] sent heartbeat to %s:%d\n", ip, port);

        /* Sleep in 1-second ticks so signals are handled promptly */
        for (int slept = 0; slept < interval && !stop_requested; slept++)
            sleep(1);
    }

    fprintf(stderr, "\n[!] heartbeat stopped. Kernel should detect timeout soon.\n");
    close(fd);
    return 0;
}

/*
 * Transmit a single UDP datagram.
 *
 * Useful for exercising the kernel network trigger.
 */
static int cmd_send(int argc, char **argv)
{
    if (argc < 2 || !strcmp(argv[0], "-h") || !strcmp(argv[0], "--help")) {
        fprintf(stderr,
            "usage: wrong8007ctl send <ip> <port> [payload]\n"
            "\n"
            "  payload defaults to \"%s\" if omitted.\n"
            "  Fires the network trigger's match_port / match_payload condition.\n",
            DEFAULT_MAGIC_PAYLOAD);
        return argc < 2 ? 1 : 0;
    }

    const char *ip = argv[0];
    int port = parse_int(argv[1], 1, 65535);
    const char *payload = (argc >= 3) ? argv[2] : DEFAULT_MAGIC_PAYLOAD;
    size_t len = strlen(payload);

    if (len > MAX_PAYLOAD_LEN)
        die("payload too long (%zu bytes, max %d)", len, MAX_PAYLOAD_LEN);

    struct sockaddr_in dst;
    int fd = udp_open(ip, port, &dst);

    udp_send(fd, &dst, payload, len);
    fprintf(stderr, "[+] sent magic payload (%zu bytes) to %s:%d\n", len, ip, port);

    close(fd);
    return 0;
}

/*
 * Read a single-line sysfs attribute.
 *
 * Trailing line endings are removed.
 */
static int read_sysfs_line(const char *path, char *buf, size_t buflen)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;

    if (!fgets(buf, (int)buflen, f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    size_t n = strlen(buf);
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
        buf[--n] = '\0';

    return 0;
}

/*
 * Find the USB device directory corresponding to a block device.
 *
 * USB devices are ancestors of block devices in sysfs.
 */
static int find_usb_device_dir(const char *start_path, char *out, size_t outlen)
{
    char path[PATH_MAX];
    char probe[SYSFS_PATH_MAX];

    strncpy(path, start_path, sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    while (strlen(path) > 1) {
        snprintf(probe, sizeof(probe), "%s/idVendor", path);
        if (access(probe, R_OK) == 0) {
            snprintf(out, outlen, "%s", path);
            return 0;
        }

        /* dirname() may modify its argument */
        char tmp[PATH_MAX];
        strncpy(tmp, path, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        char *parent = dirname(tmp);
        if (!strcmp(parent, path))
            break; /* No parent remains */
        strncpy(path, parent, sizeof(path) - 1);
        path[sizeof(path) - 1] = '\0';
    }

    return -1;
}

/*
 * Discover removable USB storage devices.
 *
 * Reports USB VID:PID identifiers for each device.
 */
static int cmd_usb_list(int argc, char **argv)
{
    int verbose = 0;

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"))
            verbose = 1;
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            fprintf(stderr,
                "usage: wrong8007ctl usb-list [-v]\n"
                "\n"
                "  Lists removable USB block devices with VID:PID, for building\n"
                "  USB_DEVICES rules (e.g. USB_DEVICES=\"1234:5678:insert\").\n"
                "\n"
                "  -v, --verbose   also print manufacturer/product/serial when available\n");
            return 0;
        } else {
            die("unexpected argument: %s", argv[i]);
        }
    }

    DIR *d = opendir("/sys/block");
    if (!d)
        die("cannot open /sys/block: %s", strerror(errno));

    int found = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, "sd", 2) != 0)
            continue;

        char removable_path[SYSFS_PATH_MAX];
        snprintf(removable_path, sizeof(removable_path),
                 "/sys/block/%s/removable", ent->d_name);

        char removable[8] = {0};
        if (read_sysfs_line(removable_path, removable, sizeof(removable)) != 0)
            continue;
        if (strcmp(removable, "1") != 0)
            continue;

        char block_path[SYSFS_PATH_MAX];
        snprintf(block_path, sizeof(block_path), "/sys/block/%s", ent->d_name);

        char resolved[PATH_MAX];
        if (!realpath(block_path, resolved)) {
            if (verbose)
                fprintf(stderr, "warning: cannot resolve %s: %s\n", block_path, strerror(errno));
            continue;
        }

        /* Ignore non-USB block devices */
        if (!strstr(resolved, "/usb"))
            continue;

        char usb_dir[PATH_MAX];
        if (find_usb_device_dir(resolved, usb_dir, sizeof(usb_dir)) != 0)
            continue;

        char vid[16] = {0}, pid[16] = {0};
        char vid_path[SYSFS_PATH_MAX], pid_path[SYSFS_PATH_MAX];
        snprintf(vid_path, sizeof(vid_path), "%s/idVendor", usb_dir);
        snprintf(pid_path, sizeof(pid_path), "%s/idProduct", usb_dir);

        if (read_sysfs_line(vid_path, vid, sizeof(vid)) != 0 ||
            read_sysfs_line(pid_path, pid, sizeof(pid)) != 0)
            continue;

        printf("/dev/%-8s  VID:PID = %s:%s\n", ent->d_name, vid, pid);
        found++;

        if (verbose) {
            char manufacturer[128] = {0}, product[128] = {0}, serial[128] = {0};
            char mpath[SYSFS_PATH_MAX], ppath[SYSFS_PATH_MAX], spath[SYSFS_PATH_MAX];

            snprintf(mpath, sizeof(mpath), "%s/manufacturer", usb_dir);
            snprintf(ppath, sizeof(ppath), "%s/product", usb_dir);
            snprintf(spath, sizeof(spath), "%s/serial", usb_dir);

            read_sysfs_line(mpath, manufacturer, sizeof(manufacturer));
            read_sysfs_line(ppath, product, sizeof(product));
            read_sysfs_line(spath, serial, sizeof(serial));

            printf("    Vendor:  %s\n", manufacturer[0] ? manufacturer : "(unknown)");
            printf("    Product: %s\n", product[0] ? product : "(unknown)");
            printf("    Serial:  %s\n", serial[0] ? serial : "(unknown)");
        }
    }
    closedir(d);

    if (!found)
        printf("No removable USB block devices found.\n");

    return 0;
}

/*
 * Registered userspace commands.
 *
 * Each command owns its argument parsing and description.
 */
static const struct command commands[] = {
    {
        .name = "heartbeat",
        .run = cmd_heartbeat,
        .description = "Send periodic heartbeat packets",
    },
    {
        .name = "send",
        .run = cmd_send,
        .description = "Send a trigger packet",
    },
    {
        .name = "usb-list",
        .run = cmd_usb_list,
        .description = "List removable USB devices",
    },
};

static void usage_main(const char *prog)
{
    fprintf(stderr,
        "wrong8007ctl - companion tool for the wrong8007 kernel module\n"
        "\n"
        "usage: %s <command> [args]\n"
        "\n"
        "commands:\n",
        prog);

    for (size_t i = 0; i < ARRAY_SIZE(commands); i++)
        fprintf(stderr, "  %-12s %s\n", commands[i].name, commands[i].description);

    fprintf(stderr,
        "\n"
        "Run '%s <command> -h' for command-specific help.\n",
        prog);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage_main(argv[0]);
        return 1;
    }

    const char *cmd = argv[1];

    if (!strcmp(cmd, "-h") || !strcmp(cmd, "--help")) {
        usage_main(argv[0]);
        return 0;
    }

    for (size_t i = 0; i < ARRAY_SIZE(commands); i++) {
        if (!strcmp(cmd, commands[i].name))
            return commands[i].run(argc - 2, argv + 2);
    }

    fprintf(stderr, "unknown command: %s\n\n", cmd);
    usage_main(argv[0]);
    return 1;
}
