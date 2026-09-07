cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_keyboard_macro C CXX ASM)

# Initialize the SDK
pico_sdk_init()

add_executable(pico_keyboard_macro
    main.c
    usb_descriptors.c
)

target_include_directories(pico_keyboard_macro PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Link the SDK libraries
target_link_libraries(pico_keyboard_macro
    pico_stdlib
    pico_unique_id
    tinyusb_device
    tinyusb_board
)

# Disable stdio USB/UART to keep USB purely HID
pico_enable_stdio_usb(pico_keyboard_macro 0)
pico_enable_stdio_uart(pico_keyboard_macro 0)

# Convert ELF to BIN and UF2 without building picotool
add_custom_command(TARGET pico_keyboard_macro POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:pico_keyboard_macro> ${CMAKE_CURRENT_BINARY_DIR}/pico_keyboard_macro.bin
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/bin2uf2.py ${CMAKE_CURRENT_BINARY_DIR}/pico_keyboard_macro.bin ${CMAKE_CURRENT_BINARY_DIR}/pico_keyboard_macro.uf2
    COMMENT "Generating UF2 image..."
)

