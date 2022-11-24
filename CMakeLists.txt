cmake_minimum_required(VERSION 3.16)
# include(FetchContent)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(VPMS_Pico C CXX ASM)

pico_sdk_init()

add_definitions(-DG_LOG_ENABLED=1 -DEVT_LOG_ENABLED=1)
add_subdirectory(scheduler)
# add_subdirectory(display)
add_subdirectory(lvgl)

add_executable(VPMS_Pico
	main.c
	display.c
)


# FetchContent_Declare(lvgl URL https://ghproxy.com/https://github.com/lvgl/lvgl/archive/refs/tags/v8.3.3.zip)
# FetchContent_MakeAvailable(lvgl)

# Link with the pico stdlib
# hardware_clocks hardware_i2c hardware_pio hardware_interp hardware_rtc hardware_uart hardware_irq hardware_resets
# pico_unique_id
# tinyusb_host tinyusb_device tinyusb_board
target_link_libraries(VPMS_Pico PRIVATE
	scheduler
	pico_stdlib
	pico_sync
	hardware_clocks
	hardware_xosc
	hardware_pio
	hardware_adc
	hardware_spi
	hardware_sync
	lvgl
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(VPMS_Pico)

# Enable USB serial
pico_enable_stdio_usb(VPMS_Pico 1)
pico_enable_stdio_uart(VPMS_Pico 0)
