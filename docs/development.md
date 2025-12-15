# Development guide

This document explains how **`wrong8007`** is structured internally and how to extend it by contributing new triggers in a clean, maintainable way.

It assumes familiarity with:

- Linux kernel development
- Loadable Kernel Modules (LKM)
- Basic kernel subsystems (notifiers, netfilter, USB, etc.)
- Project's [design philosophy](design.md) and [security model](security_model.md).

Payload behavior and forensic considerations are intentionally out of scope for this document. See [Data destruction & Wiping rationale](dd.md) for guidance on wipe strategies.

## Architecture

**`wrong8007`** follows a **core + plugin trigger** architecture.

### Responsibilities of the `core`

- Module initialization and teardown
- Parameter validation
- Deferred execution via workqueue
- User-mode execution (`call_usermodehelper`)

### Role of `triggers`

- Detect a specific condition
- Decide *if* a trigger should fire
- Notify the core (never execute directly)

Triggers are intentionally **stateless or minimally stateful**.

## Trigger interface

All triggers must expose a `struct wrong8007_trigger`:

```c
struct wrong8007_trigger {
    const char *name;
    int (*init)(void);
    void (*exit)(void);
};
```

### Rules

* `init()` must return 0 on success
* `exit()` must be safe to call even if `init()` partially failed
* Triggers must not assume other triggers are present
* Triggers must not execute user-space code directly

## Scheduling execution

Triggers **must not** call `call_usermodehelper()` directly.

Instead, triggers schedule deferred execution:

```c
schedule_work(&exec_work);
```

This ensures:

* Correct execution context
* Safe interaction with kernel subsystems
* Consistent behavior across triggers

If one-shot semantics are desired, the core may guard execution with an atomic flag.

## Designing a new trigger

### 1. Create the trigger source

Location:

```
trigger/<your_trigger>.c
```

Include only what you need:

```c
#include <linux/module.h>
#include <wrong8007.h>
```

### 2. Implement `init` / `exit`

```c
static int trigger_example_init(void)
{
    wb_info("example trigger initialized\n");
    return 0;
}

static void trigger_example_exit(void)
{
    wb_info("example trigger exited\n");
}
```

### 3. Expose the trigger

```c
struct wrong8007_trigger example_trigger = {
    .name = "example",
    .init = trigger_example_init,
    .exit = trigger_example_exit
};
```

### 4. Register the trigger with the `core`

Add it to the trigger list:

```c
extern struct wrong8007_trigger example_trigger;

static struct wrong8007_trigger *triggers[] = {
    &keyboard_trigger,
    &usb_trigger,
    &network_trigger,
    &example_trigger,
};
```

## Parameter handling

Triggers may define module parameters, but must follow these rules:

* Validate parameters in `init()`
* Fail module load on invalid input
* Do not modify parameters after initialization
* Prefer strict parsing over permissive behavior

Example:

```c
if (!param || !*param)
    return -EINVAL;
```

## Memory & Context rules

Triggers must:

* Avoid sleeping in atomic context
* Avoid allocation in hot paths
* Free all allocated memory in `exit()`
* Handle repeated init/exit safely

## Logging guidelines

Use project logging macros:

| Macro     | Usage                               |
| --------- | ----------------------------------- |
| `wb_dbg`  | Development-only debug output       |
| `wb_info` | Initialization and lifecycle events |
| `wb_warn` | Recoverable configuration issues    |
| `wb_err`  | Fatal initialization errors         |

Triggers must not log after execution is scheduled.

## Testing new triggers

Recommended workflow:

1. Load module with only your trigger enabled
2. Verify `init`/`exit` behavior
3. Test trigger activation in isolation
4. Validate unload safety (`rmmod`)
5. Combine with other triggers last

## Code style

* Follow kernel coding style
* Prefer clarity over cleverness
* Document non-obvious behavior
* Treat this repository as reference-quality code

If your trigger is hard to reason about, it **does not** belong here.
