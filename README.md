# Wrong Boot

**Wrong Boot** (*codename: `wrong8007`*) is a **programmable dead man's switch** for Linux, living entirely in kernel space. Think of it as the software equivalent of a burner phone **OR** a modular kernel trigger framework for last-resort execution.

Inspired by the legendary [USBKill](https://github.com/hephaest0s/usbkill) project and reinvented from scratch, it's **modular, trigger-agnostic**, and **execution-flexible**: you choose how it activates, you choose what it does.

This project was revisited and expanded in memory of **[Mark Klein](https://en.wikipedia.org/wiki/Mark_Klein)** (May 2, 1945 – March 8, 2025) the AT&T technician who, in 2006, revealed the existence of warrantless mass surveillance (Secrets of [Room 641A](https://en.wikipedia.org/wiki/Room_641A)) by the NSA.

In a world where truths vanish into evidence lockers, systems can be seized, tampered with or forcibly accessed, and control can be taken in seconds, **reaction time is everything**.

Wrong Boot isn't just a tool. It's **last words**. A line you draw before someone else crosses it.

When the moment comes, **`wrong8007`** won't ask questions. It will act exactly how you told it to.

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

> **Disclaimer:** This project is for educational and lawful defensive purposes only. Using it to damage systems you don't own or have permission to modify is illegal.

## Features

* **Kernel-space monitoring**: Zero user-space dependencies; works even if most of the system is compromised.
* **Multiple triggers**: Phrase detection, USB events, network packets all extendable by design.
* **Custom execution hooks**: Run any script or binary, from data wipes to custom alerting logic.
* **Fail-closed design**: Invalid configurations prevent module load rather than causing undefined behavior.
* **Fast & silent**: Triggers execution instantly, without relying on cron jobs or user-space daemons.
* **Modular architecture**: Clean separation between core logic and triggers.

## Design

The design of this project was intentionally made modular to allow for customization and the use of individualized solutions (by default, it comes with a rudimentary script for nuking).

Wrong Boot's architecture keeps **triggers** separate from the **core logic**, making it easy to add or remove trigger types without touching the core.

<p align="center">
  <img width="708" height="440" src="https://github.com/user-attachments/assets/d0bb5624-77b1-45d7-bff8-8adb7a45859a" alt="system architecture" />
</p>

For example, the keyboard trigger (`trigger/keyboard.c`) listens for a secret phrase and instantly runs your configured executable when matched. Other triggers (USB, network) work independently - load the module with any combination you need.

You can read more about the project's design philosophy [here](docs/manifesto.md). For trust boundaries and non-goals, see the [security model](docs/security-model.md).

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

* Works with the **US keymap only**.
* Requires the phrase to be typed **without mistakes** - any wrong key resets the match.
* Does not capture keys from virtual keyboards or remote sessions.

## USB-based triggers

The `wrong8007` kernel module supports advanced USB event–based triggers with flexible configuration:

* **Multiple USB devices supported** in a single load.
* Fine-grained control over **event types**: insertion, removal (eject), or any activity.
* Support for **whitelisting** or **blacklisting** USB devices.

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

* `WHITELIST=1` → Only listed devices trigger the payload.
* `WHITELIST=0` (default) → Listed devices are blocked, all others trigger.

**Example:**

```bash
make load USB_DEVICES="1234:5678:any" WHITELIST=1 EXEC="/path/to/script"
```

#### Find your device VID & PID

Use:

```bash
lsusb
```

### ⚠️ Note on parameter validation and trigger behavior

Dynamic configuration via the `usb_devices` module parameter was introduced and improved in commits [`7a6ab4d`](https://github.com/0x48piraj/wrong8007/commit/7a6ab4d428be8ce8ba9dbf6b8e187484362392d8) and [`4fd9648`](https://github.com/0x48piraj/wrong8007/commit/4fd96480dc4229787b9e9932b416e038b9cd1120), enabling runtime specification of USB device rules for fine-grained trigger control.

This replaces the legacy approach introduced in commit [`875ff0a`](https://github.com/0x48piraj/wrong8007/commit/875ff0a9a8f7aee6515918a857ce067a2416f78a).

* The module accepts USB device rules via the `usb_devices` module parameter as an **array of strings** in the format:

  ```
  VID:PID:EVENT
  ```

  where `EVENT` is one of `insert`, `eject`, or `any`.

* Upon module load, these rules are **parsed and validated strictly**:

  * Each rule is checked for correct hexadecimal VID and PID values.
  * The event string is verified to be one of the supported values.
  * Invalid or malformed rules cause the module initialization to fail with clear error messages.

* If no valid USB device rules are provided, the USB trigger disables itself silently and does **not** register for USB event notifications.

* This rigorous validation ensures that only well-formed configurations are accepted, avoiding undefined or unexpected behavior at runtime.

* `usb_notifier_callback()` schedules work once per USB event so we don't need to dedup for correctness.

* Users must provide valid rules; incorrect inputs will prevent module load.

#### Design rationale

* **Validation is performed in the kernel module on load**, ensuring invalid configurations are rejected immediately.

* This balances **robustness and safety** with kernel code simplicity.

* The module avoids runtime overhead of repeated checks by validating once during initialization.

* Users should still carefully prepare module parameters (e.g. via scripts or tooling) to avoid load failures.

## Network-based triggers

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

## Contributing

New trigger implementations are welcome and encouraged.

Before contributing, please read:
- [Development guide](docs/development.md)
- [Security model](docs/security-model.md)

PRs that violate the project's trust boundaries or safety guarantees will not be accepted.

## Data destruction notes

**Wrong Boot** does not prescribe *how* you destroy data, only *when*.

For operators designing their wipe or sanitization payloads, see:
- [Data destruction & Wiping rationale](docs/dd.md)

This document covers common myths, modern research, and practical tooling for effective data sanitization.

## Roadmap

This checklist outlines what's been completed so far and what still needs to be addressed.

- [x] Functionally complete
  - [x] Performs all core tasks reliably (e.g., all triggers work as intended)
  - [x] Handles edge cases (e.g., timeout conditions, invalid input)

- [x] Documented
  - [x] Explain purpose, setup, usage, and caveats
  - [x] Example commands, options, and expected behavior are clear
  - [x] Module parameters and triggers are well-explained

- [x] Configurable and extensible
  - [x] Users can add or combine triggers easily
  - [x] Clear boundaries between core logic and pluggable parts (e.g., USB/network)

- [ ] Tested or testable
  - [ ] Demonstrates functionality in multiple environments or under stress
  - [ ] Includes safety mechanisms (e.g., no accidental wipes, warnings for common mistakes)

- [ ] No obvious bugs or kernel warnings
  - [x] Loads/unloads cleanly
  - [x] No kernel panics
  - [ ] `dmesg` pollution

- [x] Stable and versioned
  - [x] Tagged releases (e.g., v1.0.0)
  - [x] Changelog is maintained

- [x] Logging levels or debug modes for safe testing
- [ ] Optional dry-run modes or mock environments
- [ ] Security hardening (e.g., restrict who can insert the module)
- [ ] Packaging
  - [ ] DKMS support
  - [x] Makefile with runtime-configurable build flags
  - [ ] Install/uninstall scripts

## What's next

Wrong Boot has the potential to evolve into a full-fledged, bootable OS, purpose-built for operational survivability.

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
