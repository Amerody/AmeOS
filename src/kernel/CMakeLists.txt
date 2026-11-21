# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Amerody

add_library(kernel_core OBJECT
    "main.c"

    "lib/console.c"
    "lib/kprintf.c"
    "lib/panic.c"
    "lib/delay.c"

    "shell/shell.c"
    "shell/commands.c"
)
target_link_libraries(kernel_core PUBLIC kernel_flags)