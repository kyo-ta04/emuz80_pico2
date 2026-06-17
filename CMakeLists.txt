# Generated Cmake Pico project file

#
# CMakeLists_rp2350b.txt
#

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# macro configuration

set (EMUZ80_CONFIG z80_rp2350a)

# config variable definitions
if ("${EMUZ80_CONFIG}" STREQUAL "z80_rp2350a")
    set(emuz80_project ${EMUZ80_CONFIG})
    set(emuz80_board "waveshare_rp2350_zero")
    set(emuz80_main_c "emuz80_pico2.c")
    set(emuz80_cpu_c "cpu/z80.c")
    set(emuz80_conf_h "conf/z80_rp2350b.h")
    set(emuz80_pio_pio "pio/z80_rp2350a.pio")
    set(emuz80_macro_defs -DRP2350_Zero -DRP2350A)
endif()


set(PICO_BOARD ${emuz80_board} CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(emuz80_pico2 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(emuz80_pico2 ${emuz80_main_c} ${emuz80_cpu_c} ${emuz80_conf_h})

pico_set_program_name(emuz80_pico2 ${emuz80_project})
pico_set_program_version(emuz80_pico2 "0.1")

# copy_to_ram means bootloader copies binaries to RAM and execute it
pico_set_binary_type(emuz80_pico2 copy_to_ram)

# Generate PIO header
pico_generate_pio_header(emuz80_pico2 ${CMAKE_CURRENT_LIST_DIR}/${emuz80_pio_pio})

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(emuz80_pico2 0)
pico_enable_stdio_usb(emuz80_pico2 1)

# compiler options
target_compile_options(emuz80_pico2 PUBLIC -Ofast)
# for WeAct RP2350B CoreBoard
target_compile_options(emuz80_pico2 PUBLIC ${emuz80_macro_defs})
# for Waveshare RP2350-Zero/One
#target_compile_options(emuz80_pico2 PUBLIC -DRP2350_Zero
#                PUBLIC -DRP2350A)
# for Akizuki Denshi AE-RP2040
#target_compile_options(emuz80_pico2 PUBLIC -DAE_RP2040
#                PUBLIC -DRP2040)


# Add the standard library to the build
target_link_libraries(emuz80_pico2
        pico_stdlib
        hardware_dma
        pico_multicore)

# Add the standard include files to the build
target_include_directories(emuz80_pico2 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(emuz80_pico2 
        hardware_pio
        )

pico_add_extra_outputs(emuz80_pico2)
