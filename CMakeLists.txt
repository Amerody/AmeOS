cmake_minimum_required(VERSION 3.25)

# The toolchain file is supplied by the presets (CMakePresets.json), so
# project() below already knows it is cross-compiling for bare-metal AArch64.
project(kernel VERSION 0.1.0 LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------------------
# Options
# ------------------------------------------------------------------------------
set(KERNEL_PLATFORM "rpi4")

option(KERNEL_BUILD_DOCS "Generate Doxygen docs as part of the default build" ON)
option(KERNEL_DOCS_INTERNAL "Include internal code in docs" OFF)

message(STATUS "Kernel platform: ${KERNEL_PLATFORM}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ------------------------------------------------------------------------------
# Sources
#
# Expected layout:
#   src/kernel/                  platform-independent kernel code
#   src/arch/aarch64/            CPU-specific code (boot stub, exception vectors, ...)
#   src/platform/rpi4/           board-specific code (UART, interrupt controller, ...)
#   include/                     headers (Doxygen comments live here)
#   linker/rpi4.ld               linker script
#
# CONFIGURE_DEPENDS makes the build re-glob automatically when files are added.
# ------------------------------------------------------------------------------
set(KERNEL_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker/${KERNEL_PLATFORM}.ld")
if(NOT EXISTS "${KERNEL_LINKER_SCRIPT}")
    message(FATAL_ERROR "Missing linker script: ${KERNEL_LINKER_SCRIPT}")
endif()

# shared flags and include root for every library
add_library(kernel_flags INTERFACE)
target_include_directories(kernel_flags INTERFACE "${CMAKE_SOURCE_DIR}/include")
target_compile_options(kernel_flags INTERFACE 
    -mcpu=cortex-a72
    -ffreestanding
    -fno-stack-protector
    -fno-pic
    -fno-omit-frame-pointer
    -fparse-all-comments
    -Wall -Wextra
    # Don't compile kernel code with float/vector instructions
    # Kernel code will not use float
    $<$<COMPILE_LANGUAGE:C,CXX>:-mgeneral-regs-only>
    # C++ without a runtime (no unwinder, no RTTI, no thread-safe static guards)
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
)

add_subdirectory("src")

# ------------------------------------------------------------------------------
# Kernel target
# ------------------------------------------------------------------------------
add_executable(kernel)
target_link_libraries(kernel PRIVATE 
    kernel_flags

    kernel_core    
    aarch64
    platform
)


set_target_properties(kernel PROPERTIES
    OUTPUT_NAME kernel
    SUFFIX ".elf"
    LINK_DEPENDS "${KERNEL_LINKER_SCRIPT}")

target_include_directories(kernel PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_link_options(kernel PRIVATE
    -fuse-ld=lld
    -nostdlib
    "-Wl,-T,${KERNEL_LINKER_SCRIPT}"
    "-Wl,-Map=$<TARGET_FILE_DIR:kernel>/kernel.map"
    -Wl,--gc-sections)

# ------------------------------------------------------------------------------
# Raspberry Pi: raw binary image (kernel8.img) for the SD card
# ------------------------------------------------------------------------------
find_program(KERNEL_OBJCOPY NAMES llvm-objcopy HINTS "${KERNEL_LLVM_BIN}")

if(KERNEL_OBJCOPY)
    add_custom_command(TARGET kernel POST_BUILD
        COMMAND "${KERNEL_OBJCOPY}" -O binary
                "$<TARGET_FILE:kernel>" "$<TARGET_FILE_DIR:kernel>/kernel8.img"
        COMMENT "Creating kernel8.img"
        VERBATIM)
else()
    message(WARNING "llvm-objcopy not found: kernel8.img will not be generated")
endif()


# ------------------------------------------------------------------------------
# QEMU helper targets (emulated Raspberry Pi 4B, needs QEMU 9.0 or newer;
# peripheral coverage is partial, so real hardware is the source of truth):
#   cmake --build --preset <preset> --target run     boot the kernel
#   cmake --build --preset <preset> --target debug   boot and wait for gdb on :1234
# ------------------------------------------------------------------------------
find_program(KERNEL_QEMU NAMES qemu-system-aarch64
             HINTS "$ENV{ProgramFiles}/qemu")

set(_qemu_usable FALSE)
if(KERNEL_QEMU)
    execute_process(COMMAND "${KERNEL_QEMU}" -M help
                    OUTPUT_VARIABLE _qemu_machines ERROR_QUIET)
    if(_qemu_machines MATCHES "raspi4b")
        set(_qemu_usable TRUE)
    else()
        message(WARNING "This QEMU has no 'raspi4b' machine (needs QEMU 9.0+): "
                        "'run' and 'debug' targets disabled")
    endif()
else()
    message(STATUS "qemu-system-aarch64 not found: 'run' and 'debug' targets disabled")
endif()

if(_qemu_usable)
    set(_qemu_args -M raspi4b -nographic -kernel "$<TARGET_FILE:kernel>")
    add_custom_target(run
        COMMAND "${KERNEL_QEMU}" ${_qemu_args}
        DEPENDS kernel USES_TERMINAL VERBATIM)
    add_custom_target(debug
        COMMAND "${KERNEL_QEMU}" ${_qemu_args} -s -S
        DEPENDS kernel USES_TERMINAL VERBATIM)
endif()

# ------------------------------------------------------------------------------
# Documentation (Doxygen), on by default; -DKERNEL_BUILD_DOCS=OFF to skip.
# Output: <build dir>/docs/html/index.html
# ------------------------------------------------------------------------------
if(KERNEL_BUILD_DOCS)
    find_package(Doxygen OPTIONAL_COMPONENTS dot)

    if(DOXYGEN_FOUND)
        #Style
        # set(DOXYGEN_GENERATE_TREEVIEW   YES)
        # set(DOXYGEN_HTML_EXTRA_STYLESHEET "${CMAKE_SOURCE_DIR}/docs/theme/doxygen-awesome.css")
        # set(DOXYGEN_HTML_COLORSTYLE DARK) 
        
        set(DOXYGEN_GENERATE_TREEVIEW YES)
        set(DOXYGEN_DISABLE_INDEX NO)
        set(DOXYGEN_FULL_SIDEBAR NO)
        set(DOXYGEN_HTML_EXTRA_STYLESHEET "${CMAKE_SOURCE_DIR}/docs/theme/doxygen-awesome.css" "${CMAKE_SOURCE_DIR}/docs/theme/doxygen-awesome-sidebar-only.css")
        set(DOXYGEN_HTML_COLORSTYLE LIGHT)

        # Settings
        set(DOXYGEN_PROJECT_NAME          "${PROJECT_NAME}")
        set(DOXYGEN_OUTPUT_DIRECTORY      "${CMAKE_SOURCE_DIR}/docs")
        set(DOXYGEN_RECURSIVE             YES)
        set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)   # remove once C++ is used
        set(DOXYGEN_EXTRACT_ALL           NO)    #only documented items appear
        set(DOXYGEN_EXTRACT_STATIC        YES)
        set(DOXYGEN_WARN_IF_UNDOCUMENTED  NO)
        set(DOXYGEN_WARN_IF_DOC_ERROR     YES)
        set(DOXYGEN_WARN_NO_PARAMDOC      YES)
        set(DOXYGEN_WARN_AS_ERROR         NO)    # switch to YES once the docs are clean
        set(DOXYGEN_GENERATE_LATEX        NO)
        set(DOXYGEN_INCLUDE_GRAPH         NO)   # only drawn if Graphviz is installed
        set(DOXYGEN_CALL_GRAPH            NO)
        set(DOXYGEN_GROUP_GRAPHS          NO)
        set(DOXYGEN_STRIP_FROM_PATH       "${CMAKE_SOURCE_DIR}")
        set(DOXYGEN_REPEAT_BRIEF          NO)

        # Sources
        if(KERNEL_DOCS_INTERNAL)
            set(DOXYGEN_FILE_PATTERNS *.h *.c *.hpp *.cpp *.md *.dox) # *.c/.cpp is not included to not publicly document internal functions
            set(_doc_inputs "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src")
        else()
            set(DOXYGEN_FILE_PATTERNS *.h *.hpp *.md *.dox) # *.c/.cpp is not included to not publicly document internal functions
            set(_doc_inputs "${CMAKE_SOURCE_DIR}/include")
        endif()

        if(EXISTS "${CMAKE_SOURCE_DIR}/docs")
            list(APPEND _doc_inputs "${CMAKE_SOURCE_DIR}/docs")
        endif()
        
        if(EXISTS "${CMAKE_SOURCE_DIR}/docs/mainpage.md")
            set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/docs/mainpage.md")
        endif()

        doxygen_add_docs(docs ${_doc_inputs} ALL
            COMMENT "Generating Doxygen documentation")
        
        add_dependencies(docs kernel) # Only generate documentation after the kernel builds successfully. 
    else()
        message(WARNING "Doxygen not found: documentation will not be generated "
                        "(install Doxygen, or configure with -DKERNEL_BUILD_DOCS=OFF)")
    endif()
endif()