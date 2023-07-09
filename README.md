# Wrong Boot

Wrong Boot _(Wrong8007)_ is an equivalent of a burner phone. As a whistleblower, you don't want to be found by the government or anyone else holding sensitive files. The Wrong Boot burns everything down securely when you want to with a single press of a button.

This project is starting with a simple Linux Kernel Module (or LKM) which is always running in the background (from kernel-space) waiting for the "secret" phrase to nuke everything in it's entirety.

Wrong Boot will be developed further into a full-blown bootable OS of it's own if desired by the community (creating pulls, issues or starring the project are good metrics).

**Note:** As of now, it only works with standard terminals (tty) not with pseudo ones (pty).

## Usage

The usage is pretty simple, actually, but you will need to have superuser access to the machine.

Cloning the repository,

```bash
    $ git clone https://github.com/0x48piraj/wrong8007.git
    $ cd wrong8007/
```

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

```bash
    $ # sudo insmod wrong8007.ko phrase="secret phrase" exec="/root/binary"
    $ chmod +x wipe.sh
    $ test -f wipe.sh && make load PHRASE=nuke!nuke!nuke1 EXEC="$(realpath wipe.sh)"
```

The kernel module has two parameters, one for the secret phrase (typing this phrase will be the trigger so _chose wisely_) and the second for the path to an executable. The design of this project was intentionally made modular to allow for customization and the use of individualized solutions (by default, it comes with a rudimentary script for nuking).

**Note:** For the LKM to successfully be able to execute the binary/script, it must be in executable format (just `chmod +x` the thing...).

## Removing the kernel module

```bash
    $ make remove   # Remove the module
    $ make clean    # Optional: Clean build files
```

## USB-Based Triggers

The `wrong8007` module supports USB event–based triggers. You can specify a **Vendor ID (VID)** and **Product ID (PID)** to watch for, and choose whether to react on **insertion**, **removal**, or **either** event.

### Trigger on USB insert (default)

Trigger the payload when a specific USB device is plugged in:

```bash
sudo insmod wrong8007.ko usb_vid=0x1234 usb_pid=0x5678 exec="/path/to/script"
```

### Trigger on USB removal (eject)

Trigger the payload when that device is **removed**:

```bash
sudo insmod wrong8007.ko usb_vid=0x1234 usb_pid=0x5678 usb_event=eject exec="/path/to/script"
```

### Trigger on **any** USB activity (insert or remove)

Fire on either insertion or ejection:

```bash
sudo insmod wrong8007.ko usb_vid=0x1234 usb_pid=0x5678 usb_event=any exec="/path/to/script"
```

You can find the correct VID & PID of your device using:

```bash
lsusb
```

## Network-Based Triggers

The `wrong8007` module supports various network-triggering modes - flexible enough for LAN environments, and stealthy when used with passive traffic.

### Usage

#### Trigger on specific MAC address

Trigger when any packet from this MAC address is seen on the interface:

```bash
sudo insmod wrong8007.ko match_mac='aa:bb:cc:dd:ee:ff' exec="/path/to/script"
```

#### Trigger on specific IP address

Trigger only when a packet originates from the matching IPv4 address:

```bash
sudo insmod wrong8007.ko match_ip='192.168.1.1' exec="/path/to/script"
```

#### Trigger on port + payload (Magic Packet)

Send a single UDP packet with a known payload - acts as a remote kill switch:

```bash
sudo insmod wrong8007.ko match_port=1234 match_payload='MAGIC' exec="/path/to/script"
```

Send it using the provided helper:

```bash
python3 scripts/whisperer.py 192.168.1.1 1234 "MAGIC"
```

#### Heartbeat-based Trigger

Trigger if no packet from a host is received for a set duration:

```bash
sudo insmod wrong8007.ko heartbeat_host='192.168.1.1' heartbeat_interval=10 heartbeat_timeout=30 exec="/path/to/script"
```

Use the heartbeat sender script to periodically "ping" the module from the host:

```bash
python3 scripts/heartbeat.py 192.168.1.1 1234
```

## The world of nuking!

You can TOTALLY ignore the following section if you just want to use the tool but if you're a forensic nerd, shall we?

#### “The Urban Legend of Multipass Hard Disk Overwrite” and DoD 5220-22-M

In 1996, Peter Gutmann presented a paper ([`GUT96`](http://www.cs.auckland.ac.nz/~pgut001/pubs/secure_del.htm)) at a USENIX Security Symposium in which he claimed that overwritten data could be recovered using [magnetic force microscopy (MFM)](https://en.wikipedia.org/wiki/Magnetic_force_microscope) and [scanning tunneling microscopy (STM)](https://en.wikipedia.org/wiki/Scanning_tunneling_microscope) techniques.

This seminal paper alerted many people to the possibility that data which had been overwritten on an HDD could be recovered using such techniques.

Lacking other research in this area, and despite a lack of corroboration, many of those people adopted Gutmann’s conclusions and recommendations and have ever since believed that multiple overwrites are required to effectively render remnant data irretrievable.

Gutmann’s ultimate recommendation was that no fewer than 35 (!) overwrite passes should be performed to ensure that the original data cannot be retrieved.

However, in the context of current HDD technology, there are several problems with Gutmann’s work:

- Gutmann focused on two disk technologies - modified frequency modulation and run-length-limited encoding - that rely on detection of a narrow range of analog signal values and have not been used for HDDs in the last 10-15 years. Modern HDDs use various kinds of partial-response maximum-likelihood (PRML) sequence detection that uses statistical techniques to determine the maximum likelihood value associated with multiple signal detections ([`WRIG08`](http://www.springerlink.com/content/408263ql11460147/)).

- Further, areal density (density of data per square unit of area, the product of bit-per-inch linear density and track-per-inch track density) has increase by at least three orders of magnitude ([`SOBE04`](http://www.actionfront.com/whitepaper/Drive-Independent%20Data%20Recovery%20Ver14Alrs.pdf), [`WIKI08`](http://en.wikipedia.org/wiki/File:Hard_drive_capacity_over_time.png)) since the publication the Gutmann paper. To achieve such densities, head positioning actuators have become significantly more accurate and repeatable.

- Moreover, Gutmann’s work paper was theoretical, and I am not aware of any practical validation that data could be recovered using the techniques he described.

> And that's how Gutmann’s work resulted in the formation of an urban legend: that the US government requires a 3-pass overwrite and specifies it in DoD 5220-22-M.

#### What about those often-cited US Government standards?

There are many HDD overwrite standards from which to choose ([`BLAN08`](http://www.dataerasure.com/recognized_overwriting_standards.htm)). Among those that are often cited in both procurement and product specifications are DoD 5220.22-M and NSA 130-1. Less often cited, but more current, is NIST SP 800-88.

#### DoD 5220-22-M

DoD 5220-22-M is the National Industrial Security Program Operating Manual (NISPOM), which a broad manual of procedures and requirements for government contractors handling classified information.

The 1997 version of this document ([`DOD_97`](http://www.usaid.gov/policy/ads/500/d522022m.pdf)) specified that rigid magnetic disks should be sanitized by writing some character, its complement, and then a random character. However, this “algorithm” was removed from subsequent issues of the NISPOM.

Indeed, the entire table of clearing and sanitization methods is no longer present in the current issue of NISPOM ([`DOD_06`](http://www.dss.mil/isp/odaa/documents/nispom2006-5220.pdf)).

#### NSA 130-1

NSA 130-1 may well have specified a clearing or sanitization procedure by writing a random character, another random character, and then a known value. However, I am not able to find a copy of NSA Manual 130-1 or 130-2 (perhaps they were classified documents).

However, the current issue of the NSA/CSS Storage Device Declassification Manual ([`NSA_07`](http://www.nsa.gov/ia/_files/Government/MDG/NSA_CSS_Storage_Device_Declassification_Manual.pdf)) (Manual 9-12, which supersedes Manual 130-2) does not specify any overwriting methods for HDDs, and instead requires degaussing or physical destruction.

It is not clear to me if the DoD and NSA no longer recommend overwrite methods because they are ineffective or because their effectiveness as a single technique is uncertain when applied to a variety of HDD technologies.

#### NIST Special Publication 800-88

The National Institute of Standards and Technology has a special publication “Guidelines for Media Sanitization” that allows HDD clearing by overwriting media “using agency-approved and validated overwriting technologies/methods/tools”.

For purging, it specifies the Secure Erase ([`UCSD10`](http://cmrr.ucsd.edu/people/Hughes/SecureErase.shtml)) function (for ATA-based devices), degaussing, destruction, or the rather vague “purge media by using agency-approved and validated purge technologies/tools”.

The original issue of SP 800-88 ([`NIST06-1`](http://web.archive.org/web/20060902043637/csrc.nist.gov/publications/nistpubs/800-88/SP800-88_Aug2006.pdf)) claimed that “Encryption is not a generally accepted means of sanitization. The increasing power of computers decreases the time needed to crack cipher text and therefore the inability to recover the encrypted data can not be assured”, but that text was removed from SP 800-88 Revision 1 which was issued one month later.

Most interestingly, SP 800-88 states that “NSA has researched that one overwrite is good enough to sanitize most drives”. Unfortunately, the NSA’s research does not appear to have been published for public consumption. Read more [here](http://www.fylrr.com/archives.php?doc=NISTSP800-88_rev1.pdf) and over [NISTSP800-88_with-errata](https://dwaves.de/wp-content/uploads/2013/09/NISTSP800-88_with-errata.pdf).

#### Recent research (kinda, sorta...)

Several security researchers presented a paper ([`WRIG08`](http://www.springerlink.com/content/408263ql11460147/)) at the Fourth International Conference on Information Systems Security (ICISS 2008) that declares the “great wiping controversy” about how many passes of overwriting with various data values to be settled: their research demonstrates that a single overwrite using an arbitrary data value will render the original data irretrievable even if MFM and STM techniques are employed.

The researchers found that the probability of recovering a single bit from a previously used HDD was **only slightly better than a coin toss**, and that the probability of recovering more bits **decreases exponentially** so that it quickly becomes close to zero.

#### Therefore, a single pass overwrite with any arbitrary value (randomly chosen or not) is sufficient to render the original HDD data effectively irretrievable.”

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

You’re not limited to just one. For example, your network trigger could run `scrub` for a quick device wipe, while your USB trigger calls `nwipe` for a thorough multi-pass destruction.
