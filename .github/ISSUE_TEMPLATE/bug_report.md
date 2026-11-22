---
name: Bug report
about: Describe something that doesn't work as it should
title: "[Bug] "
labels: bug
assignees: ''
---

<!--
Thanks for taking the time to report this!
AmeOS is a learning project, so please describe the problem instead of sending a fix:
working out the fix is part of what I want to learn. Pull requests are not accepted.
-->

## What happened

<!-- A clear description of the bug. -->

## What you expected

<!-- What should have happened instead. -->

## How to reproduce

1.
2.
3.

## Output

<!-- UART output, panic message, exception dump (ESR/ELR/FAR), or GDB backtrace. -->

```text

```

## Environment

- AmeOS commit: <!-- output of `git rev-parse --short HEAD` -->
- Build preset: <!-- rpi4-debug / rpi4-release -->
- Running on: <!-- QEMU raspi4b or real Raspberry Pi 4 (which RAM size) -->
- QEMU version: <!-- `qemu-system-aarch64 --version` -->
- Clang version: <!-- `clang --version` -->
- Host OS: <!-- Windows / Linux distro / macOS -->

## Additional context

<!-- Where in the code you think the problem might be (file:line, register, documentation section...), if you know. -->
