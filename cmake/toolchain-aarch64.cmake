# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Amerody

# Toolchain file: bare-metal AArch64 (aarch64-none-elf) using LLVM (Clang + lld).
#
# Finding LLVM, in priority order:
#   1. $LLVM_ROOT/bin           (set this env var if LLVM is in a custom location)
#   2. %ProgramFiles%/LLVM/bin  (default Windows installer location)
#   3. PATH
# Note: this deliberately looks for "clang", NOT "clang-cl" (the MSVC-style driver).

set(CMAKE_SYSTEM_NAME Generic)          # "Generic" = bare metal, no OS underneath
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(KERNEL_TRIPLE aarch64-none-elf)

find_program(KERNEL_CLANG   NAMES clang
             HINTS "$ENV{LLVM_ROOT}/bin" "$ENV{ProgramFiles}/LLVM/bin")
find_program(KERNEL_CLANGXX NAMES clang++
             HINTS "$ENV{LLVM_ROOT}/bin" "$ENV{ProgramFiles}/LLVM/bin")

if(NOT KERNEL_CLANG OR NOT KERNEL_CLANGXX)
    message(FATAL_ERROR
        "clang / clang++ not found. Install LLVM and add it to PATH, "
        "or set the LLVM_ROOT environment variable to its install directory.")
endif()

set(CMAKE_C_COMPILER   "${KERNEL_CLANG}")
set(CMAKE_CXX_COMPILER "${KERNEL_CLANGXX}")
set(CMAKE_ASM_COMPILER "${KERNEL_CLANG}")   # clang assembles .S files too

set(CMAKE_C_COMPILER_TARGET   ${KERNEL_TRIPLE})
set(CMAKE_CXX_COMPILER_TARGET ${KERNEL_TRIPLE})
set(CMAKE_ASM_COMPILER_TARGET ${KERNEL_TRIPLE})

get_filename_component(KERNEL_LLVM_BIN "${KERNEL_CLANG}" DIRECTORY)

find_program(KERNEL_OBJCOPY
    NAMES llvm-objcopy
    HINTS "${KERNEL_LLVM_BIN}"
    REQUIRED
)

# There is no libc or startup code to link against, so CMake's "can the compiler
# build a program?" check must only compile a static library, not link an executable.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Never pick up host libraries or headers.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)