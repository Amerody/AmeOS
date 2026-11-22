# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Amerody

add_library(arch INTERFACE)
add_subdirectory("aarch64")
target_link_libraries(arch INTERFACE 
    kernel_flags
    $<TARGET_OBJECTS:aarch64>
)