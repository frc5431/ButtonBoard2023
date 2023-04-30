cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(pico_sdk_import.cmake)
project(picojoy C CXX ASM)

pico_sdk_init()

add_executable(picojoy
        main.cpp
        usb_descriptors.c
)

target_compile_features(picojoy PUBLIC cxx_std_17)

target_include_directories(picojoy PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(picojoy PUBLIC pico_stdlib)
target_link_libraries(picojoy PUBLIC tinyusb_device tinyusb_board)
target_link_libraries(picojoy PUBLIC hardware_spi)

pico_add_extra_outputs(picojoy)
