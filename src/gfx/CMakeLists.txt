# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Amerody

add_library(gfx OBJECT
    "font.c"
)
target_link_libraries(gfx PUBLIC kernel_flags)