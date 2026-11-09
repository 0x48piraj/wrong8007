# Wrong Boot

**Wrong Boot** (*codename: `wrong8007`*) is a **programmable dead man's switch** for Linux, triggered by events observed inside the kernel.

Inspired by the [USBKill](https://github.com/hephaest0s/usbkill) project from 2016, Wrong Boot rethinks the idea as a modular Linux kernel module. Triggers are independent of execution, allowing the same core to support different activation mechanisms while leaving payloads entirely user-defined.

This project was revisited and expanded in memory of **[Mark Klein](https://en.wikipedia.org/wiki/Mark_Klein)** (May 2, 1945 - March 8, 2025), the AT&T technician who, in 2006, revealed the existence of warrantless mass surveillance (Secrets of [Room 641A](https://en.wikipedia.org/wiki/Room_641A)) by the NSA.

Systems can be seized, inspected, or tampered with in seconds. By then, the opportunity to decide may already be gone. What remains is the decision you made *beforehand*.

Wrong Boot exists for those situations. It monitors predefined conditions from within the kernel and executes the response you chose _before that moment arrived_.

<p align="center">
  <img src="https://github.com/user-attachments/assets/d5a0bb9e-a23e-46f8-af5f-bb8e01277dca" alt="demo gif">
</p>

<p align="center">
  <a href="#usage">Installation</a>
  &nbsp;&nbsp;&nbsp;•&nbsp;&nbsp;&nbsp;
  <a href="docs/security-model.md">Security model</a>
  &nbsp;&nbsp;&nbsp;•&nbsp;&nbsp;&nbsp;
  <a href="docs/manifesto.md">Philosophy</a>
</p>

* **Kernel-space monitoring**: Trigger conditions are evaluated directly by kernel subsystem hooks rather than by a userspace polling daemon.
* **Pluggable trigger framework**: Phrase detection, USB events, network packets all extendable by design.
* **Operator-defined execution**: Run any script or binary, from data wipes to custom logic.
* **Fail-closed design**: Invalid configurations prevent module load rather than causing undefined behavior.

## Design

Wrong Boot separates trigger detection from execution. Triggers only report that a condition has been met; the core decides *whether* and *when* execution occurs. This separation keeps trigger implementations simple and execution behavior consistent.

Further design rationale is documented in the [design philosophy](docs/manifesto.md). The project's trust boundaries and security assumptions are described in the [security model](docs/security-model.md).

## Usage

You need **root** on the machine and the headers for your running kernel (`linux-headers` on Arch, `linux-headers-$(uname -r)` on Debian and Ubuntu). Kernels built with BTF (Arch, Fedora and recent Ubuntu) also need `pahole` (`dwarves` on Debian and Ubuntu).

Configuration has two parts and they stay apart on purpose.

| Part | What it holds | Where it lives | Secret |
|---|---|---|---|
| **Build profile** | Triggers compiled into the module | `.config` (Kconfig) | No |
| **Load parameters** | Phrase, USB rules, network matches and payload path | `/etc/wrong8007/wrong8007.conf` with mode `0600` | Yes |

Nothing secret is compiled into the module or written to the source tree.

### 1. Build the module

Clone the repository:

```bash
git clone https://github.com/0x48piraj/wrong8007.git
cd wrong8007/
```

Build it with every trigger:

```bash
make
```

Run `make menuconfig` to compile in only the triggers you use. It needs a writable kernel build tree (usually root). Editing `.config` and running `make` works too. A trigger you leave out adds no code to your kernel.

### 2. Write the parameters

Install the parameter template (readable by root only):

```bash
make params-init
```

Fill it in:

```bash
sudoedit /etc/wrong8007/wrong8007.conf
```

Check it against your build:

```bash
make params-check
```

`exec` is required. It's the **absolute canonical path** to an executable file. The kernel runs that file directly without a shell. The executable runs with root privileges.

Scripts need _shebang_ and the payload takes no arguments. The other options arm the triggers below. `configs/wrong8007.conf.example` shows every option.

`params-check` refuses anything the kernel would reject or quietly ignore. That covers unknown names, values for triggers that aren't compiled in and a configuration that arms no trigger.

> [!IMPORTANT]
> Keep the parameters out of `/etc/modprobe.d`. Initramfs tools copy that directory into the boot image. The image is often readable by every user and `/boot` is usually left unencrypted.

### 3. Install and load

Install the module:

```bash
make install
```

Load it (this checks the parameters first):

```bash
make load-installed
```

It runs `modprobe -C /etc/wrong8007/wrong8007.conf wrong8007`. A plain `modprobe wrong8007` reads no parameters and the module refuses to load.

`make load` loads the module you just built without installing it. It's meant for development. It passes the parameters on the `insmod` command line where other users can briefly see them in the process list.

### Unload and uninstall

Unload the module:

```bash
make remove
```

Remove the installed module:

```bash
make uninstall
```

### What happens on activation

The first trigger to fire runs the payload **once**. Later activations do nothing until the module is reloaded.

The kernel log shows whether the payload started. A payload that fails to start is retried with backoff for about a minute and a half (for example when the file is missing or the system is resuming from suspend). A payload that started is never run again and keeps running after the module is unloaded. Its exit status isn't collected.

> [!TIP]
> Wrap the payload while you test it to see its exit status in the journal:
>
> ```sh
> #!/bin/sh
> /usr/local/sbin/wipe
> logger -t wrong8007 "payload exited with status $?"
> ```
>
> Point `exec` back at the real payload once it works.

Trigger values such as the phrase and the payload string never appear in the kernel log.

## Keyboard-based trigger

The `wrong8007` module can trigger actions when a specific **phrase** is typed on the keyboard.

* **Case-sensitive** matching. `"nuke"` is different from `"NUKE"`.
* Matches **exactly** as typed, without ignoring spaces or punctuation.
* Phrases with a repeated beginning match too (`"aab"` typed as `"aaab"`).
* Phrases hold up to 128 bytes.

```
options wrong8007 exec=/usr/local/sbin/wipe phrase="correct horse battery"
```

#### Limitations

* Matches the characters the kernel's console keymap resolves keys to.
* Each key must resolve to a single Latin-1 character. Special keys (Shift, Ctrl, arrows and F-keys) are ignored.
* Dead-key and Compose compositions are invisible to the keyboard notifier and cannot be matched.
* Requires the phrase to be typed **without mistakes**. A wrong key restarts the match.
* Does not capture keys from remote sessions or from consoles in `VC_RAW`, `VC_MEDIUMRAW` or `VC_OFF` mode.

## USB-based triggers

USB triggers react to insertion, removal or both for one or more devices. A device is named by its **Vendor ID (VID)** and **Product ID (PID)**. `lsusb` and `./tools/wrong8007ctl usb-list` show them.

A rule reads `VID:PID[:EVENT]` and a list holds up to 16 rules separated by commas. VID and PID are 1 to 4 hex digits with an optional `0x`. EVENT is `insert`, `eject` or `any` in lowercase and defaults to `any`.

```
options wrong8007 usb_devices=1234:5678                     # insert or removal
options wrong8007 usb_devices=1234:5678:eject               # removal only
options wrong8007 usb_devices=1234:5678:insert,abcd:ef00:any
```

#### Device matching modes

* `whitelist=0` _(default)_: listed devices trigger execution and every other device is ignored.
* `whitelist=1`: listed devices are ignored and any other add or remove event triggers execution.

`CONFIG_WRONG8007_WHITELIST` sets the default at build time.

> [!CAUTION]
> Whitelist mode is intentionally broad. List every internal USB device (webcam, Bluetooth, fingerprint reader and hubs). Ordinary hotplug and resume from suspend can trigger execution otherwise.

> [!NOTE]
> An invalid rule stops the module from loading. Without rules the USB trigger stays disabled.

## Network-based triggers

The network trigger inspects incoming IPv4 packets. Every `match_` condition you set must hold for the same packet. The [heartbeat](#heartbeat) works on its own.

| Parameter | Matches |
|---|---|
| `match_mac=aa:bb:cc:dd:ee:ff` | Source MAC (12 hex digits or six pairs joined by `:` or `-`) |
| `match_ip=192.168.1.1` | Source IPv4 address |
| `match_port=1234` | TCP or UDP source or destination port |
| `match_payload=MAGIC` | String anywhere in the TCP or UDP payload (up to 512 bytes) |

#### Magic packet

A single UDP packet with a known payload acts as a remote kill switch:

```
options wrong8007 exec=/usr/local/sbin/wipe match_port=1234 match_payload=MAGIC
```

Build the companion tool:

```bash
make -C tools
```

Send the packet from another machine:

```bash
./tools/wrong8007ctl send <target-ip> 1234 MAGIC
```

#### Heartbeat

The trigger fires when no packet from `heartbeat_host` arrives within `heartbeat_timeout` seconds:

```
options wrong8007 exec=/usr/local/sbin/wipe heartbeat_host=192.168.1.1 heartbeat_interval=10 heartbeat_timeout=30
```

Any IPv4 packet from that host counts. Run this on the host to send heartbeats on purpose:

```bash
./tools/wrong8007ctl heartbeat <target-ip> 1234 -i 10
```

> [!NOTE]
> The timeout starts counting at load. The trigger fires when no heartbeat arrives within `heartbeat_timeout` seconds of loading (for example during boot).

> [!NOTE]
> #### MAC and IP-only triggers
> A MAC-only or IP-only trigger fires on the first IPv4 packet from that device, routine traffic included. On a network where the device is already active that can happen right after load.
>
> They suit air-gapped or controlled environments, proximity-based activation and triggers that rely on a trusted device appearing. Prefer **payload-based triggers** when you want control over activation.

## Contributing

New trigger implementations are welcome and encouraged.

Before contributing, please read:
- [Development guide](docs/development.md)
- [Security model](docs/security-model.md)

## Payload design

**Wrong Boot** defines *when* execution occurs, not *what* is executed.

For operators designing their wipe or sanitization payloads, see:

- [Data destruction & Wiping](docs/dd.md): Explores the common myths, historical standards, research and practical techniques for data sanitization.

### Who this project is for

- Security researchers
- Linux kernel developers
- High-risk environment operators
- Incident response and contingency planning

### Who this project is NOT for

- Stealth malware
- Unauthorized system access
- Persistent implants
- Remote command-and-control frameworks

If you'd like to shape its future:

- Star the repo to show support
- Open issues to discuss ideas or bugs
- Send PRs if you're building something cool (look into [development guide](docs/development.md))
