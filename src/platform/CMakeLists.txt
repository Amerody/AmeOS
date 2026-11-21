# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Amerody

add_library(platform INTERFACE)
add_subdirectory("rpi4")
target_link_libraries(platform INTERFACE 
    kernel_flags
    $<TARGET_OBJECTS:rpi4>
)