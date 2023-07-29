# Wrong Boot

**Wrong Boot** (*codename: `wrong8007`*) is a **programmable dead man's switch** for Linux, living entirely in kernel space. Think of it as the software equivalent of a burner phone - but for your data.

Inspired by [USBKill](https://github.com/hephaest0s/usbkill) and reinvented from scratch, it's **modular, trigger-agnostic**, and **execution-flexible**: you choose how it activates, you choose what it does.

Whether that's securely nuking sensitive files, cutting network links, or triggering custom defense scripts - it's in your control.

In an era where whistleblowers have faced retaliation, journalists have operated under surveillance, and corporate misconduct has been quietly buried, *Wrong Boot* gives you a way to act - instantly, irreversibly, and on your terms.

> **Disclaimer:** This project is for educational and lawful defensive purposes only. Using it to damage systems you don't own or have permission to modify is illegal.

## Features

* **Kernel-space monitoring**: Zero user-space dependencies; works even if most of the system is compromised.
* **Multiple triggers**: Phrase detection, USB events, network packets all extendable by design.
* **Custom execution hooks**: Run any script or binary, from data wipes to custom alerting logic.
* **Fast & silent**: Triggers execution instantly, without relying on cron jobs or user-space daemons.
* **Modular architecture**: Clean separation between core logic and triggers.

## Design

The design of this project was intentionally made modular to allow for customization and the use of individualized solutions (by default, it comes with a rudimentary script for nuking).

Wrong Boot's architecture keeps **triggers** separate from the **core logic**, making it easy to add or remove trigger types without touching the core.

```
                   ┌─────────────────────────┐
                   │        wrong8007        │
                   │      Kernel Module      │
                   └────────────┬────────────┘
                                │
         ┌──────────────────────┼──────────────────────┐
         │                      │                      │
 ┌───────▼────────┐     ┌───────▼─────────┐    ┌───────▼─────────┐
 │ Keyboard       │     │ USB Events      │    │ Network         │
 │ Trigger        │     │ Trigger         │    │ Trigger         │
 └───────┬────────┘     └───────┬─────────┘    └───────┬─────────┘
         │                      │                      │
         └───────────────┬──────┴──────┬───────────────┘
                         │             │
                   ┌─────▼─────────────▼─────┐
                   │        EXEC Hook        │
                   │  (script or binary run) │
                   └─────────────────────────┘
```

For example, the keyboard trigger (`trigger_keyboard.c`) listens for a secret phrase; if it matches, it runs your configured executable instantly. Other triggers (USB, network) work independently - load the module with any combination you need.

You can read more about the project's design philosophy [here](docs/DESIGN.md).

## Usage

The usage is pretty simple, actually, but you will need to have superuser access to the machine.

#### 1. Clone the repository

```bash
    $ git clone https://github.com/0x48piraj/wrong8007.git
    $ cd wrong8007/
```

#### 2. Build the kernel module

Compiling the LKM,

```bash
    $ make
```

Debugging:

- Optional `-DDEBUG` flag prints verbose logs for keypresses and command execution

   Enable with:

```bash
    $ make EXTRA_CFLAGS=-DDEBUG
```

At last, installing the kernel module,

#### 3. Load the module

**Example:** run `wipe.sh` when the phrase `secret phrase` is typed.

```bash
    $ chmod +x wipe.sh
    $ test -f wipe.sh && make load PHRASE='secret phrase' EXEC="$(realpath wipe.sh)"
```

> The executable/script **must** have execute permissions (`chmod +x`) and use an absolute path.

## Removing the kernel module

```bash
    $ make remove   # Remove the module
    $ make clean    # Optional: clean build artifacts
```

## USB-Based Triggers

The `wrong8007` module supports USB event–based triggers. You can specify a **Vendor ID (VID)** and **Product ID (PID)** to watch for, and choose whether to react on **insertion**, **removal**, or **either** event.

### Trigger on USB insert (default)

Trigger the payload when a specific USB device is plugged in:

```bash
make load USB_VID=0x1234 USB_PID=0x5678 EXEC="/path/to/script"
```

### Trigger on USB removal (eject)

Trigger the payload when that device is **removed**:

```bash
make load USB_VID=0x1234 USB_PID=0x5678 USB_EVENT=eject EXEC="/path/to/script"
```

### Trigger on **any** USB activity (insert or remove)

Fire on either insertion or ejection:

```bash
make load USB_VID=0x1234 USB_PID=0x5678 USB_EVENT=any EXEC="/path/to/script"
```

You can find the correct VID & PID of your device using:

```bash
lsusb
```

#### ⚠️ Note on parameter validation and trigger behavior

This project accepts dynamic configuration at load time via module parameters such as `PHRASE`, `EXEC`, `USB_VID`, `USB_PID`, and optionally `USB_EVENT` introduced in this commit, [`875ff0a`](https://github.com/0x48piraj/wrong8007/commit/875ff0a9a8f7aee6515918a857ce067a2416f78a).

To ensure correctness and avoid unexpected behavior:

* The `Makefile` performs **basic validation** of required parameters and restricts `USB_EVENT` to valid values: `insert`, `eject`, or `any`.
* If invalid or unsupported values are provided (e.g. `USB_EVENT=foobar`), and you're using the `Makefile`, it will **fail early** with a helpful message.

> If `usb_vid` and `usb_pid` are not set, the USB trigger module **disables itself silently** and will not hook into USB events.

* However, if you're using `insmod` directly, you bypass the `Makefile` validation. In that case, it's your responsibility to ensure valid inputs.

The kernel module does **not** include extensive runtime validation for invalid `USB_EVENT` values to avoid bloating the kernel with unnecessary checks.

This is an intentional design choice:

- **Lightweight kernel code**
- **Controlled from user space**
- **Sanity checks enforced via build tooling**

**Summary**:

* Use the `Makefile` → gets validated.
* Use `insmod` manually → know what you're doing.

## Network-Based Triggers

The `wrong8007` module supports various network-triggering modes - flexible enough for LAN environments, and stealthy when used with passive traffic.

### Usage

#### Trigger on specific MAC address

Trigger when any packet from this MAC address is seen on the interface:

```bash
make load MATCH_MAC='aa:bb:cc:dd:ee:ff' EXEC="/path/to/script"
```

#### Trigger on specific IP address

Trigger only when a packet originates from the matching IPv4 address:

```bash
make load MATCH_IP='192.168.1.1' EXEC="/path/to/script"
```

#### Trigger on port + payload (Magic Packet)

Send a single UDP packet with a known payload - acts as a remote kill switch:

```bash
make load MATCH_PORT=1234 MATCH_PAYLOAD='MAGIC' EXEC="/path/to/script"
```

Send it using the provided helper:

```bash
python3 scripts/whisperer.py 192.168.1.1 1234 "MAGIC"
```

#### Heartbeat-based Trigger

Trigger if no packet from a host is received for a set duration:

```bash
make load HEARTBEAT_HOST='192.168.1.1' HEARTBEAT_INTERVAL=10 HEARTBEAT_TIMEOUT=30 EXEC="/path/to/script"
```

Use the heartbeat sender script to periodically "ping" the module from the host:

```bash
python3 scripts/heartbeat.py 192.168.1.1 1234
```

#### ⚠️ Nuanced behavior of network-based triggers using IP/MAC

> MAC-based triggers can activate immediately and unexpectedly, because any frame (such as ARP, broadcast, or even passive presence) from the target MAC is enough to trigger the module - no IP traffic is required.
>
>  IP-based triggers are slightly more restrictive - they only fire when a valid IP packet is seen from the specified address. If the device hasn't sent anything yet at the IP layer, the trigger won't activate.
>
> If you're using MAC- or IP-only triggers on devices already active on the same network (e.g., your own machine), you risk triggering the payload immediately on load, which can lead to unintended consequences including self-triggering.

To avoid accidental activation:

- Do not rely solely on MAC/IP triggers in sensitive environments.
- Prefer using magic packets if precision is critical. You can use `whisperer.py`, or any network utility to "poke" the module.
- Ensure your trigger source is not present on the network during module load (e.g., an external device that only joins the network when needed).

While limited, MAC/IP-only triggers are not useless, they shine in scenarios where:

- The target device is not always connected, and
- You want the module to activate only when that specific MAC or IP joins the network.

This makes them ideal for:

- Air-gapped or controlled environments
- Proximity-based activation
- Triggers that rely on the appearance of a trusted device

## The world of nuking!

You can TOTALLY ignore the following section if you just want to use the tool but if you're a forensic nerd, shall we?

#### "The Urban Legend of Multipass Hard Disk Overwrite" and DoD 5220-22-M

In 1996, Peter Gutmann presented a paper ([`GUT96`](http://www.cs.auckland.ac.nz/~pgut001/pubs/secure_del.htm)) at a USENIX Security Symposium in which he claimed that overwritten data could be recovered using [magnetic force microscopy (MFM)](https://en.wikipedia.org/wiki/Magnetic_force_microscope) and [scanning tunneling microscopy (STM)](https://en.wikipedia.org/wiki/Scanning_tunneling_microscope) techniques.

This seminal paper alerted many people to the possibility that data which had been overwritten on an HDD could be recovered using such techniques.

Lacking other research in this area, and despite a lack of corroboration, many of those people adopted Gutmann's conclusions and recommendations and have ever since believed that multiple overwrites are required to effectively render remnant data irretrievable.

Gutmann's ultimate recommendation was that no fewer than 35 (!) overwrite passes should be performed to ensure that the original data cannot be retrieved.

However, in the context of current HDD technology, there are several problems with Gutmann's work:

- Gutmann focused on two disk technologies - modified frequency modulation and run-length-limited encoding - that rely on detection of a narrow range of analog signal values and have not been used for HDDs in the last 10-15 years. Modern HDDs use various kinds of partial-response maximum-likelihood (PRML) sequence detection that uses statistical techniques to determine the maximum likelihood value associated with multiple signal detections ([`WRIG08`](http://www.springerlink.com/content/408263ql11460147/)).

- Further, areal density (density of data per square unit of area, the product of bit-per-inch linear density and track-per-inch track density) has increase by at least three orders of magnitude ([`SOBE04`](http://www.actionfront.com/whitepaper/Drive-Independent%20Data%20Recovery%20Ver14Alrs.pdf), [`WIKI08`](http://en.wikipedia.org/wiki/File:Hard_drive_capacity_over_time.png)) since the publication the Gutmann paper. To achieve such densities, head positioning actuators have become significantly more accurate and repeatable.

- Moreover, Gutmann's work paper was theoretical, and I am not aware of any practical validation that data could be recovered using the techniques he described.

> And that's how Gutmann's work resulted in the formation of an urban legend: that the US government requires a 3-pass overwrite and specifies it in DoD 5220-22-M.

#### What about those often-cited US Government standards?

There are many HDD overwrite standards from which to choose ([`BLAN08`](http://www.dataerasure.com/recognized_overwriting_standards.htm)). Among those that are often cited in both procurement and product specifications are DoD 5220.22-M and NSA 130-1. Less often cited, but more current, is NIST SP 800-88.

#### DoD 5220-22-M

DoD 5220-22-M is the National Industrial Security Program Operating Manual (NISPOM), which a broad manual of procedures and requirements for government contractors handling classified information.

The 1997 version of this document ([`DOD_97`](http://www.usaid.gov/policy/ads/500/d522022m.pdf)) specified that rigid magnetic disks should be sanitized by writing some character, its complement, and then a random character. However, this "algorithm" was removed from subsequent issues of the NISPOM.

Indeed, the entire table of clearing and sanitization methods is no longer present in the current issue of NISPOM ([`DOD_06`](http://www.dss.mil/isp/odaa/documents/nispom2006-5220.pdf)).

#### NSA 130-1

NSA 130-1 may well have specified a clearing or sanitization procedure by writing a random character, another random character, and then a known value. However, I am not able to find a copy of NSA Manual 130-1 or 130-2 (perhaps they were classified documents).

However, the current issue of the NSA/CSS Storage Device Declassification Manual ([`NSA_07`](http://www.nsa.gov/ia/_files/Government/MDG/NSA_CSS_Storage_Device_Declassification_Manual.pdf)) (Manual 9-12, which supersedes Manual 130-2) does not specify any overwriting methods for HDDs, and instead requires degaussing or physical destruction.

It is not clear to me if the DoD and NSA no longer recommend overwrite methods because they are ineffective or because their effectiveness as a single technique is uncertain when applied to a variety of HDD technologies.

#### NIST Special Publication 800-88

The National Institute of Standards and Technology has a special publication "Guidelines for Media Sanitization" that allows HDD clearing by overwriting media "using agency-approved and validated overwriting technologies/methods/tools".

For purging, it specifies the Secure Erase ([`UCSD10`](http://cmrr.ucsd.edu/people/Hughes/SecureErase.shtml)) function (for ATA-based devices), degaussing, destruction, or the rather vague "purge media by using agency-approved and validated purge technologies/tools".

The original issue of SP 800-88 ([`NIST06-1`](http://web.archive.org/web/20060902043637/csrc.nist.gov/publications/nistpubs/800-88/SP800-88_Aug2006.pdf)) claimed that "Encryption is not a generally accepted means of sanitization. The increasing power of computers decreases the time needed to crack cipher text and therefore the inability to recover the encrypted data can not be assured", but that text was removed from SP 800-88 Revision 1 which was issued one month later.

Most interestingly, SP 800-88 states that "NSA has researched that one overwrite is good enough to sanitize most drives". Unfortunately, the NSA's research does not appear to have been published for public consumption. Read more [here](http://www.fylrr.com/archives.php?doc=NISTSP800-88_rev1.pdf) and over [NISTSP800-88_with-errata](https://dwaves.de/wp-content/uploads/2013/09/NISTSP800-88_with-errata.pdf).

#### Recent research (kinda, sorta...)

Several security researchers presented a paper ([`WRIG08`](http://www.springerlink.com/content/408263ql11460147/)) at the Fourth International Conference on Information Systems Security (ICISS 2008) that declares the "great wiping controversy" about how many passes of overwriting with various data values to be settled: their research demonstrates that a single overwrite using an arbitrary data value will render the original data irretrievable even if MFM and STM techniques are employed.

The researchers found that the probability of recovering a single bit from a previously used HDD was **only slightly better than a coin toss**, and that the probability of recovering more bits **decreases exponentially** so that it quickly becomes close to zero.

#### Therefore, a single pass overwrite with any arbitrary value (randomly chosen or not) is sufficient to render the original HDD data effectively irretrievable."

## Extend with battle-tested data destruction utilities

Extend the base wiping script with powerful, battle-tested data shredding utilities. These tools can be seamlessly integrated into your custom scripts for deeper, more flexible wiping protocols.

Here are some great options - all open-source, well-documented, and actively used by security-conscious folks.

### 1. **[nwipe](https://github.com/martijnvanbrummelen/nwipe)**

> *A modern, open-source fork of the legendary DBAN.*

* Originally designed for boot-and-nuke scenarios, now works perfectly inside Linux.
* Can wipe entire disks using multiple recognized algorithms (DoD 5220.22-M, Gutmann, PRNG patterns, etc.).
* Has both interactive mode **and** fully scriptable batch mode.
* Displays real-time progress and verification.
* Ideal if you want a full-disk, standards-compliant wipe without reinventing the wheel.

### 2. **[shred](https://www.gnu.org/software/coreutils/manual/html_node/shred-invocation.html)**

> *The classic Unix command for securely overwriting data.*

* Included with GNU coreutils (no need to install extra packages on most systems).
* Works on files, partitions, and raw block devices.
* Overwrites multiple times with configurable patterns, then optionally truncates.
* Perfect for quick, script-friendly wipes where you control the exact number of passes.

### 3. **[wipe](https://github.com/gmr/wipe)**

> *A lightweight tool for secure deletion of files and devices.*

* Designed to securely erase data by writing special patterns.
* Implements several sanitization methods from well-known standards.
* Simple to integrate into automated scripts — minimal dependencies.
* Good middle-ground between the simplicity of `shred` and the full-disk capabilities of `nwipe`.

### 4. **[scrub](https://linux.die.net/man/1/scrub)**

> *Fast, configurable, and built for raw speed.*

* Can target disks, partitions, or files.
* Supports a range of wiping schemes, from simple random data fills to NNSA and DoD-compliant methods.
* Lightweight and easy to run headless.
* Great option for high-speed, full-device overwrites.

**Pro tip:**

You're not limited to just one. For example, your network trigger could run `scrub` for a quick device wipe, while your USB trigger calls `nwipe` for a thorough multi-pass destruction.

## What's Next

Wrong Boot has the potential to evolve into a full-fledged, bootable OS, purpose-built for operational survivability.

If you'd like to shape its future:

- Star the repo to show support
- Open issues to discuss ideas or bugs
- Send PRs if you're building something cool

This is just the beginning.
