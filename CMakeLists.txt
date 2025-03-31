#sudo openocd -f interface/cmsis-dap.cfg -f target/rp2040.cfg -c "adapter speed 5000" -c "program BatCharger.elf verify reset exit"

cmake_minimum_required(VERSION 3.13)

# Include the Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)


# Set the project name
project(BatCharger)

# Initialize the Pico SDK
pico_sdk_init()

# Add your executable
add_executable(BatCharger
    src/main.c
    hwlibs/MCP4725/mcp4725.c
    hwlibs/ADS1115/ads1115.c
    hwlibs/M24C0/m24c0.c
)

target_include_directories(BatCharger PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
# Link your executable with TinyUSB libraries and Pico SDK libraries
target_link_libraries(BatCharger PUBLIC
    pico_stdlib
    hardware_i2c
    hardware_adc
)

# Enable USB functionality and disable UART if desired
pico_enable_stdio_usb(BatCharger 0)
pico_enable_stdio_uart(BatCharger 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(BatCharger)
