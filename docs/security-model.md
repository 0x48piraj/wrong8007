# Security model

**`wrong8007`** is a Linux kernel module that executes a pre-configured action when specific low-level system triggers occur.

This document defines the **explicit security boundaries, guarantees, and non-goals** of the project.

It exists to prevent ambiguity, misuse, and incorrect assumptions by users, auditors, and contributors.

For architectural context, see [design philosophy](manifesto.md). For implementation details, see [development guide](development.md).

## Threat model

**`wrong8007`** is designed for scenarios where:

- The system owner has **root-level control**
- The environment may become **physically or logically hostile**
- User-space cannot be trusted to remain available or uncompromised
- Time-to-response is **critical**

The module assumes:

- A trusted kernel at load time
- Explicit operator intent

## Trust boundaries

### Trusted
- The running kernel
- Kernel subsystems used by triggers (USB, input, netfilter)
- The execution target explicitly configured by the operator

### Untrusted
- User-space processes
- User-space logging and monitoring
- External peripherals and network traffic
- Post-trigger system state

## What `wrong8007` does

- Executes entirely in kernel space until trigger activation
- Responds synchronously and immediately to trigger conditions
- Uses existing kernel hooks (not polling or daemons)
- Executes a **single, operator-defined command**
- Leaves no persistent state on disk
- Can be removed cleanly if never triggered

## What `wrong8007` does NOT do

**`wrong8007`** explicitly does **not**:

- Install itself automatically
- Obfuscate code or behavior
- Escalate privileges
- Communicate externally (unless specified)
- Accept commands from untrusted sources
- Modify kernel memory outside its own scope

If you are looking for covert channels, or evasion techniques, this project is **intentionally not for you**.

## Execution model safety

### Single-execution semantics

Once a trigger fires, the system is assumed to be compromised.

The design follows a **fail-closed** approach:

- The execution path is intentionally simple
- No recovery or retry logic is implemented
- Triggers should be treated as **one-shot events**

## Kernel stability considerations

**`wrong8007`** follows standard kernel development practices:

- No dynamic memory allocation in hot paths
- No sleeping in atomic contexts
- Deferred execution via workqueues
- Strict parameter validation at module load time

If invalid parameters are supplied, the module **fails to load**.

## Post-execution artifacts

**`wrong8007`** aims to minimize operational artifacts and forensic exposure:

- No files are written by the module
- No logs are emitted after trigger activation
- No user-space daemons are spawned
- All execution happens synchronously via `call_usermodehelper`

Any artifacts are entirely the responsibility of the configured payload.

## Legal & Ethical notice

**`wrong8007`** is a defensive tool.

It is intended for:

- Owner-controlled systems
- Incident response
- Physical security contingencies

You are solely responsible for ensuring compliance with local laws, organizational policies, and ethical guidelines.

The authors make **no claim** that this tool is suitable for offensive, covert, or unauthorized use.
