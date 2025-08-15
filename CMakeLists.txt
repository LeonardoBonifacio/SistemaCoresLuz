# Generated Cmake Pico project file

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
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(SistemaCoreLuz C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
include_directories( ${CMAKE_SOURCE_DIR}/lib ) # Inclui os files .h na pasta lib
add_executable(SistemaCoreLuz 
    SistemaCoreLuz.c
    lib/bh1750_light_sensor.c 
    lib/ssd1306.c 
    lib/led.c
    lib/buzzer.c
    lib/matriz.c  )

pico_set_program_name(SistemaCoreLuz "SistemaCoreLuz")
pico_set_program_version(SistemaCoreLuz "0.1")

# Generate PIO header
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/lib/ws2812.pio)



# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(SistemaCoreLuz 0)
pico_enable_stdio_usb(SistemaCoreLuz 1)

# Add the standard library to the build
target_link_libraries(SistemaCoreLuz
        pico_stdlib 
        hardware_i2c
        hardware_pwm
        hardware_pio
        hardware_clocks)

# Add the standard include files to the build
target_include_directories(SistemaCoreLuz PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(SistemaCoreLuz 
        
        )

pico_add_extra_outputs(SistemaCoreLuz)

