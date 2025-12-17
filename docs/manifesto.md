# Design philosophy

This project is built on a few strong, guiding principles. While the use case may be highly specific, the underlying decisions are shaped by a broader mindset: speed, simplicity, and survivability.

The goal: fast, quiet, and deterministic execution of pre-authorized actions when system control is threatened without reliance on user space or operator intervention.

## 1. Speed & Reliability

Time is critical. This system is designed to respond **instantly** - no waiting, no asking. Triggers are fired at the lowest layers possible (like kernel hooks), enabling near-zero delay between detection and action.

Once activated, the execution path prioritizes **deterministic, pre-authorized actions**: whether that means securing sensitive files, locking down the system, shutting it down, alerting a remote endpoint, or invoking custom defensive logic. Actions are executed exactly as defined, without runtime decisions or user interaction.

## 2. Stealth & Camouflage

A good trigger leaves no trail. This tool avoids logging, suppresses output, and runs as silently as possible. No flashing warnings, no terminal chatter. If deployed correctly, it blends into the system - invisible until it's not.

Kernel-level hooks, such as phrase detection, USB events, or network activity, make trigger logic hard to observe and harder to interfere with, even under partial system compromise.

## 3. Simplicity & Minimal dependencies

Complexity is a liability. Every moving part is another thing that can break, slow you down, or get noticed.  The system relies only on what's already available in most environments - standard tools, basic syscalls, no obscure packages or third-party cruft.

This keeps the design lightweight, predictable, and portable, with minimal configuration and no dependency on a trusted user-space environment at the moment of activation.

## 4. Robustness & Failsafes

Fail-closed by design. When a trigger fires, the system assumes control is already at risk and proceeds immediately with the configured action set.

If an execution path fails, fallback behavior is defined by configuration, not assumption. Actions may be chained, escalated, or replaced according to the operator's intent. Execution is one-shot and irreversible by design - there are no retries, prompts, or rollbacks.

You only get one shot - it needs to count.

## 5. No persistent artifacts

Leave no trace. The system writes nothing beyond what the configured action explicitly performs. No logs, no state files, no temporary markers. Trigger handling and decision logic remain entirely in memory.

Once it's done, there's nothing left to investigate - no output, no artifacts, no hints.
