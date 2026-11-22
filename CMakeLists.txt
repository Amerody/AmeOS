# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Amerody

cmake_minimum_required(VERSION 3.25)

# The toolchain file is supplied by the presets (CMakePresets.json), so
# project() below already knows it is cross-compiling for bare-metal AArch64.
project(kernel VERSION 0.1.0 LANGUAGES C CXX ASM)
configure_file(.clangd.in ${CMAKE_SOURCE_DIR}/.clangd @ONLY)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------------------------------------------------------------------
# Options
# ------------------------------------------------------------------------------
option(KERNEL_BUILD_DOCS "Generate Doxygen docs as part of the default build" ON)
option(KERNEL_DOCS_INTERNAL "Include internal code in docs" OFF)

if(NOT KERNEL_PLATFORM)
    message(FATAL_ERROR "Kernel platform not set")
endif()

if(NOT KERNEL_ARCH)
    message(FATAL_ERROR "Kernel architecture not set")
endif()

message(STATUS "Kernel platform: ${KERNEL_PLATFORM}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# The linker script must have the same name as the platform
set(KERNEL_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/src/platform/${KERNEL_PLATFORM}/${KERNEL_PLATFORM}.ld")
if(NOT EXISTS "${KERNEL_LINKER_SCRIPT}")
    message(FATAL_ERROR "Missing linker script: ${KERNEL_LINKER_SCRIPT}")
endif()

# shared flags and include root for every library
add_library(kernel_flags INTERFACE)

target_compile_definitions(kernel_flags INTERFACE "KERNEL_ARCH_NAME=\"${KERNEL_ARCH}\""  "KERNEL_PLATFORM_NAME=\"${KERNEL_PLATFORM}\"")
target_compile_definitions(kernel_flags INTERFACE 
    "KERNEL_VERSION_MAJOR=0"
    "KERNEL_VERSION_MINOR=1"
    "KERNEL_VERSION_PATCH=0"
)

target_include_directories(kernel_flags INTERFACE "${CMAKE_SOURCE_DIR}/include")
target_compile_options(kernel_flags INTERFACE 
    -mcpu=cortex-a72
    -ffreestanding
    -mstrict-align # Force strict memory access alignement until the MMU is on
    -fno-stack-protector
    -fno-pic
    -fno-omit-frame-pointer
    -fparse-all-comments
    -Wall -Wextra

    # Release only optimizations
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:Release>:-flto>

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
    arch
    platform
    drivers
    gfx
    lib
)

set_target_properties(kernel PROPERTIES
    OUTPUT_NAME kernel
    SUFFIX ".elf"
    LINK_DEPENDS "${KERNEL_LINKER_SCRIPT}")

target_include_directories(kernel PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_link_options(kernel PRIVATE
    -fuse-ld=lld
    -nostdlib
    $<$<CONFIG:Release>:-flto>
    "-Wl,-T,${KERNEL_LINKER_SCRIPT}"
    "-Wl,-Map=$<TARGET_FILE_DIR:kernel>/kernel.map"
    -Wl,--gc-sections)

# ------------------------------------------------------------------------------
# Raspberry Pi: raw binary image (kernel8.img) for the SD card
# ------------------------------------------------------------------------------
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
    # Same arguments as tools/run.sh and the VS Code qemu-debug task.
    # The mini UART is QEMU's second serial port, so the first (PL011) goes to null.
    set(_qemu_args -M raspi4b
        -dtb "${CMAKE_SOURCE_DIR}/src/platform/rpi4/bcm2711-rpi-4-b.dtb"
        -kernel "$<TARGET_FILE_DIR:kernel>/kernel8.img"
        -serial null -serial mon:stdio)
    add_custom_target(run
        COMMAND "${KERNEL_QEMU}" ${_qemu_args}
        DEPENDS kernel USES_TERMINAL VERBATIM)
    add_custom_target(debug
        COMMAND "${KERNEL_QEMU}" ${_qemu_args} -s -S
        DEPENDS kernel USES_TERMINAL VERBATIM)
endif()

# ------------------------------------------------------------------------------
# Documentation (Doxygen), on by default; -DKERNEL_BUILD_DOCS=OFF to skip.
# Output: docs/html/index.html
# ------------------------------------------------------------------------------
macro(setup_doxygen_layout)
    # 1. Generate the default layout in the build folder so it doesn't pollute your source tree
    set(BASE_LAYOUT "${CMAKE_CURRENT_BINARY_DIR}/DoxygenLayout.xml")
    execute_process(
        COMMAND ${DOXYGEN_EXECUTABLE} -l "${BASE_LAYOUT}"
        RESULT_VARIABLE LAYOUT_RESULT
        OUTPUT_QUIET
    )

    if(NOT LAYOUT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate Doxygen layout")
    endif()

    # 2. Read the generated layout into a CMake string variable
    file(READ "${BASE_LAYOUT}" LAYOUT_CONTENT)

    # Rename navigation tabs.
    string(REGEX REPLACE
        "<tab type=\"pages\"[^>]*/>"
        "<tab type=\"pages\" visible=\"yes\" title=\"Topics\"/>"
        LAYOUT_CONTENT "${LAYOUT_CONTENT}"
    )

    string(REGEX REPLACE
        "<tab type=\"classes\"[^>]*/>"
        "<tab type=\"classes\" visible=\"yes\" title=\"Data Structures\"/>"
        LAYOUT_CONTENT "${LAYOUT_CONTENT}"
    )

    # 3. Configure Internal build settings
    if(KERNEL_DOCS_INTERNAL)
        # Add the internal overview tab.
        # string(REGEX REPLACE
        #     "<tab type=\"files\""
        #     "<tab type=\"user\" url=\"internal_docs.html\" title=\"Internal implementation Overview\"/>\n      <tab type=\"files\""
        #     LAYOUT_CONTENT "${LAYOUT_CONTENT}"
        # )

        message(STATUS "Generating internal decisions index...")
        message(STATUS "KERNEL_DOCS_INTERNAL = ${KERNEL_DOCS_INTERNAL}")
        message(STATUS "CMAKE_CURRENT_BINARY_DIR = ${CMAKE_CURRENT_BINARY_DIR}")

        # Find decision documents.
        file(GLOB DECISION_FILES CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/docs/internal/decisions/[0-9][0-9][0-9][0-9]-*.md"
        )

        list(SORT DECISION_FILES)

        # Generate Decisions landing page.
        set(DECISIONS_CONTENT
            "# Decisions {#decisions}\n\nList of design and architecture decisions:\n\n"
        )

        foreach(filepath IN LISTS DECISION_FILES)

            get_filename_component(filename "${filepath}" NAME_WE)

            # Convert filename into a valid, predictable page ID.
            set(page_id "decision_${filename}")

            string(APPEND DECISIONS_CONTENT
                "- @subpage ${page_id}\n"
            )

        endforeach()

        # Generate the index in the build directory.
        set(KERNEL_DECISIONS_INDEX_FILE
            "${CMAKE_CURRENT_BINARY_DIR}/generated/decisions_index.md"
        )

        file(MAKE_DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}/generated"
        )

        file(WRITE
            "${KERNEL_DECISIONS_INDEX_FILE}"
            "${DECISIONS_CONTENT}"
        )

        if(EXISTS "${KERNEL_DECISIONS_INDEX_FILE}")
            message(STATUS "Decisions index successfully created: ${KERNEL_DECISIONS_INDEX_FILE}")
        else()
            message(FATAL_ERROR "Failed to create decisions index: ${KERNEL_DECISIONS_INDEX_FILE}")
        endif()

        message(STATUS
            "Generated Decisions index: ${KERNEL_DECISIONS_INDEX_FILE}"
        )
    else()
        unset(KERNEL_DECISIONS_INDEX_FILE)
    endif()

    # 4. Save the modified layout and point Doxygen to it
    set(ACTIVE_LAYOUT "${CMAKE_CURRENT_BINARY_DIR}/DoxygenLayout_active.xml")
    file(WRITE "${ACTIVE_LAYOUT}" "${LAYOUT_CONTENT}")
    
    set(DOXYGEN_LAYOUT_FILE "${ACTIVE_LAYOUT}")
endmacro()

if(KERNEL_BUILD_DOCS)
    find_package(Doxygen OPTIONAL_COMPONENTS dot)

    if(DOXYGEN_FOUND)
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
        set(DOXYGEN_EXTRACT_ALL           NO)    # only documented items appear
        set(DOXYGEN_EXTRACT_STATIC        YES)
        set(DOXYGEN_WARN_IF_UNDOCUMENTED  YES)
        set(DOXYGEN_WARN_IF_DOC_ERROR     YES)
        set(DOXYGEN_WARN_NO_PARAMDOC      YES)
        set(DOXYGEN_WARN_AS_ERROR         NO)    # switch to YES once the docs are clean
        set(DOXYGEN_GENERATE_LATEX        NO)
        set(DOXYGEN_INCLUDE_GRAPH         NO)   # only drawn if Graphviz is installed
        set(DOXYGEN_INCLUDED_BY_GRAPH     NO)
        set(DOXYGEN_SHOW_INCLUDE_FILES    NO)
        set(DOXYGEN_CALL_GRAPH            NO)
        set(DOXYGEN_GROUP_GRAPHS          NO)
        set(DOXYGEN_STRIP_FROM_PATH       "${CMAKE_SOURCE_DIR}")
        set(DOXYGEN_REPEAT_BRIEF          NO)

        set(DOXYGEN_MACRO_EXPANSION YES)
        set(DOXYGEN_PREDEFINED "__attribute__(x)=")

        # Sources
        set(DOXYGEN_FILE_PATTERNS *.h *.c *.hpp *.cpp *.md *.dox)
        set(_doc_inputs "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src")

        if(KERNEL_DOCS_INTERNAL)
            set(DOXYGEN_ENABLED_SECTIONS "INTERNAL_DOCS")
        else()
            set(DOXYGEN_ENABLED_SECTIONS "")
            set(DOXYGEN_EXCLUDE "${CMAKE_SOURCE_DIR}/docs/internal") # don't include the internal documentation on Release
        endif()

        setup_doxygen_layout()

        if(EXISTS "${CMAKE_SOURCE_DIR}/docs")
            list(APPEND _doc_inputs "${CMAKE_SOURCE_DIR}/docs")
        endif()
        
        if(EXISTS "${CMAKE_SOURCE_DIR}/README.md")
            list(APPEND _doc_inputs "${CMAKE_SOURCE_DIR}/README.md")
            set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")
        endif()

        if(KERNEL_DOCS_INTERNAL AND KERNEL_DECISIONS_INDEX_FILE)
            list(APPEND _doc_inputs "${CMAKE_CURRENT_BINARY_DIR}/generated")
        endif()

        doxygen_add_docs(docs ${_doc_inputs} ALL
            COMMENT "Generating Doxygen documentation")
        
        add_dependencies(docs kernel) # Only generate documentation after the kernel builds successfully. 
    else()
        message(WARNING "Doxygen not found: documentation will not be generated "
                        "(install Doxygen, or configure with -DKERNEL_BUILD_DOCS=OFF)")
    endif()
endif()