# Security policy

Wrong Boot runs in the kernel and holds secrets. A bug in it can crash a machine, leak a trigger value or keep the payload from running when it should. Please report those privately.

## Reporting a vulnerability

Report it through [GitHub's private vulnerability reporting](https://github.com/0x48piraj/wrong8007/security/advisories/new). Don't open a public issue or pull request for it.

Include what you can:

* The Wrong Boot version or commit and your kernel version
* Your build profile (`.config`) and the names of the parameters you set. Leave out real phrases, USB rules and addresses.
* Steps to reproduce and what you observed
* The impact you see

You get a reply within seven days. The fix ships in a release with an advisory that credits you. Say so in the report if you would rather stay anonymous.

## Verifying a release

Each release tarball carries signed build provenance from GitHub Actions. Check it with the GitHub CLI:

```bash
gh attestation verify wrong8007-<version>.tar.gz --repo 0x48piraj/wrong8007
```

The same signed provenance comes as `wrong8007-<version>.intoto.jsonl`, an in-toto attestation bundle that other verification tools read.

## Supported versions

Fixes land on `master` and ship in the next release. Only the latest release receives them.

## In scope

* A crash or memory-safety bug in the module. Bugs reachable from keystrokes, USB events or network packets matter most.
* An unprivileged user or a remote host able to run, suppress or disarm the payload outside the documented trigger conditions
* The payload starting more than once per load
* A trigger value or the payload path reaching anyone but root (kernel log, sysfs, process list or boot image)
* `wrong8007-params`, `wrong8007ctl` or the install targets weakening file permissions or accepting values the module would read differently

## Out of scope

The [security model](docs/security-model.md) lists what Wrong Boot does not protect against. Reports about those are welcome as issues. They are not vulnerabilities. In particular:

* An attacker who is root or controls the kernel
* What a payload does once it runs
* The keyboard trigger matching the console keymap and not a desktop layout
* Kernels that require signed modules rejecting an unsigned build
* MAC-only or IP-only triggers firing on routine traffic
