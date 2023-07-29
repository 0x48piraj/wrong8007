## Design Philosophy

This project is built on a few strong, guiding principles. While the use case may be highly specific, the underlying decisions are shaped by a broader mindset: speed, simplicity, and survivability.

The goal: fast, quiet, and effective destruction of sensitive data - with zero forensic residue.

### 1. Speed & Reliability

Time is critical. This system is designed to respond **instantly** - no waiting, no asking. Triggers are fired at the lowest layers possible (like kernel hooks), enabling near-zero delay between detection and action.

Once activated, the wipe process focuses on **essential targets first**: partition tables, filesystem signatures, boot records - the things that render a drive unreadable in seconds. If time permits, it can escalate to full-device erasure.

### 2. Stealth & Camouflage

A good trigger leaves no trail. This tool avoids logging, suppresses output, and runs as silently as possible. No flashing warnings, no terminal chatter. If deployed correctly, it blends into the system - invisible until it's not.

Kernel-level hooks, like phrase detection or network activity, make the trigger logic hard to spot and harder to tamper with.

### 3. Simplicity & Minimal Dependencies

Complexity is a liability. Every moving part is another thing that can break, slow you down, or get noticed. We aim to use only what's already available in most environments - standard tools, basic syscalls, no obscure packages or third-party cruft.

This keeps the system lightweight and highly portable, with little to no setup friction.

### 4. Robustness & Failsafes

Fail-Closed by Design: When the trigger hits, assume the worst - wipe *everything* indiscriminately and quickly.

If one method fails, another takes over. Whether you're dealing with spinning disks or modern SSDs, the wipe logic is designed to adapt. It can attempt multiple passes, switch strategies, or fall back to known-safe defaults if anything goes wrong.

You only get one shot - it needs to count.

### 5. No Persistent Artifacts

Leave no trace. The system writes nothing to disk, generates no logs, and creates no temporary files. If it must stage anything, it does so in memory or ephemeral space only.

Once it's done, there's nothing left to investigate - no output, no artifacts, no hints.
