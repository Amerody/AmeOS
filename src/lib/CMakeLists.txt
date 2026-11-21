# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Amerody

add_library(lib OBJECT
    "string.c"
    "memory.c"
    "ringbuf.c"
)
target_link_libraries(lib PUBLIC kernel_flags)