# Wrong Boot

**Wrong Boot** (*codename: `wrong8007`*) is a **programmable dead man's switch** for Linux, living entirely in kernel space. Think of it as the software equivalent of a burner phone **OR** a modular kernel trigger framework for last-resort execution.

Inspired by the [USBKill](https://github.com/hephaest0s/usbkill) project, Wrong Boot rethinks the idea as a modular Linux kernel module. Triggers are independent of execution, allowing the same core to support different activation mechanisms while leaving payloads entirely user-defined.

This project was revisited and expanded in memory of **[Mark Klein](https://en.wikipedia.org/wiki/Mark_Klein)** (May 2, 1945 - March 8, 2025), the AT&T technician who, in 2006, revealed the existence of warrantless mass surveillance (Secrets of [Room 641A](https://en.wikipedia.org/wiki/Room_641A)) by the NSA.

Systems can be seized, inspected, or tampered with in seconds. By then, the opportunity to decide may already be gone. What remains is the decision you made *beforehand*.

Wrong Boot exists for those situations. It monitors predefined conditions from within the kernel and executes the response you chose before that moment arrived.

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

## Features

* **Kernel-space monitoring**: Zero user-space dependencies; works even if most of the system is compromised.
* **Multiple trigger types**: Phrase detection, USB events, network packets all extendable by design.
* **Operator-defined execution**: Run any script or binary, from data wipes to custom logic.
* **Fail-closed design**: Invalid configurations prevent module load rather than causing undefined behavior.
* **Fast & silent**: Triggers execution instantly, without relying on cron jobs or user-space daemons.
* **Modular**: Clean separation between core logic and triggers.

## Design

Wrong Boot separates trigger detection from execution. Triggers only report that a condition has been met; the core decides *whether* and *when* execution occurs. This separation keeps trigger implementations simple and execution behavior consistent.

Further design rationale is documented in the [design philosophy](docs/manifesto.md). The project's trust boundaries and security assumptions are described in the [security model](docs/security-model.md).

## Usage

The usage is pretty simple, actually, but you will need to have **superuser access** to the machine.

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

**Example:** run `tests/test_exec.sh` when the phrase `secret phrase` is typed.

```bash
    $ chmod +x tests/test_exec.sh
    $ test -f tests/test_exec.sh && make load PHRASE='secret phrase' EXEC="$(realpath tests/test_exec.sh)"
```

> The executable/script **must** have execute permissions (`chmod +x`) and use an absolute path.

## Removing the kernel module

```bash
    $ make remove   # Remove the module
    $ make clean    # Optional: clean build artifacts
```

## Keyboard-based trigger

The `wrong8007` module can trigger actions when a specific **phrase** is typed on the keyboard.

* **Case-sensitive** matching. `"nuke"` is different from `"NUKE"`.
* Matches **exactly** as typed, without ignoring spaces or punctuation.
* Works only on **printable characters** (no special keys like Shift or Ctrl).

### Usage

Load the module with the desired trigger phrase:

```bash
make load PHRASE="nuke" EXEC="/path/to/script"
```

The configured script will run immediately after the phrase is typed in sequence.

#### Limitations

* Matches the characters the kernel's own keymap resolved to, so it works with **any keymap**, including international/Latin-1 layouts.
* Printable characters only: each key must resolve to a single Latin-1 character; special keys (Shift, Ctrl, arrows, F-keys) are ignored.
* Dead-key and Compose compositions are invisible to the keyboard notifier and cannot be matched.
* Requires the phrase to be typed **without mistakes**. Any wrong key resets the match.
* Does not capture keys from virtual keyboards, remote sessions, or consoles in `VC_RAW`/`VC_MEDIUMRAW`/`VC_OFF` modes.

## USB-based triggers

USB triggers can be configured to react to insertion, removal, or both for one or more devices.

### Usage

You can specify a list of USB devices using their **Vendor ID (VID)** and **Product ID (PID)**, along with an event type.

#### Load module with a single device trigger on insertion/ejection (default)

```bash
make load USB_DEVICES="1234:5678" EXEC="/path/to/script"
```

#### Trigger on removal (eject)

```bash
make load USB_DEVICES="1234:5678:eject" EXEC="/path/to/script"
```

#### Trigger on any USB event (insert or remove)

```bash
make load USB_DEVICES="1234:5678:any" EXEC="/path/to/script"
```

#### Trigger on multiple devices at once

```bash
make load USB_DEVICES="1234:5678:insert,abcd:ef00:any" EXEC="/path/to/script"
```

#### Device matching modes: Whitelist vs. Blacklist

Use the `WHITELIST` param:

* `WHITELIST=1`: Listed devices are ignored; any unlisted device triggers execution.
* `WHITELIST=0` _(default)_: Listed devices trigger execution; all other devices are ignored.

**Example:**

```bash
make load USB_DEVICES="1234:5678:any" WHITELIST=1 EXEC="/path/to/script"
```

#### Find your device VID & PID

Use:

```bash
lsusb
```

> [!NOTE]
> USB rules are validated during module initialization.
>
> - Rules must use the format `VID:PID[:EVENT]`.
> - Invalid rules prevent the module from loading.
> - If no rules are configured, the USB trigger remains disabled.

## Network-based triggers

The network trigger can activate on observed MAC addresses, IPv4 addresses, UDP/TCP payloads, or heartbeat timeouts.

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

#### Trigger on port + payload (Magic packet)

Send a single UDP packet with a known payload - acts as a remote kill switch:

```bash
make load MATCH_PORT=1234 MATCH_PAYLOAD='MAGIC' EXEC="/path/to/script"
```

Send it using the provided helper:

```bash
python3 scripts/whisperer.py 192.168.1.1 1234 "MAGIC"
```

#### Heartbeat-based trigger

Trigger if no packet from a host is received for a set duration:

```bash
make load HEARTBEAT_HOST='192.168.1.1' HEARTBEAT_INTERVAL=10 HEARTBEAT_TIMEOUT=30 EXEC="/path/to/script"
```

Use the heartbeat sender script to periodically "ping" the module from the host:

```bash
python3 scripts/heartbeat.py 192.168.1.1 1234
```

> [!NOTE]
> #### MAC/IP trigger behavior
> MAC-only triggers can activate immediately and unexpectedly on any Ethernet frame from the matching device, including ARP and broadcast traffic.
>
> IP-only triggers activate only after a valid IPv4 packet is observed.
>
> Because of this, if you're using MAC- or IP-only triggers on devices already active on the same network, you risk triggering the payload immediately on load, which can lead to unintended consequences.
>
> MAC/IP-only triggers are not useless, they can shine in:
>
> - Air-gapped or controlled environments
> - Proximity-based activation
> - Triggers that rely on the appearance of a trusted device
>
> Prefer **payload-based triggers** when operator control over activation is required.

## Contributing

New trigger implementations are welcome and encouraged.

Before contributing, please read:
- [Development guide](docs/development.md)
- [Security model](docs/security-model.md)

PRs that violate the project's trust boundaries or safety guarantees will not be accepted.

## Data destruction notes

**Wrong Boot** defines *when* execution occurs, not *what* is executed.

For operators designing their wipe or sanitization payloads, see:
- [Data destruction & Wiping rationale](docs/dd.md): Covers common myths, modern research and practical tooling for effective data sanitization.

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
