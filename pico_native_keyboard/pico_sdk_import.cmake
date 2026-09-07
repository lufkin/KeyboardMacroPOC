# This is a copy of <PICO_SDK_PATH>/external/pico_sdk_import.cmake

# This can be dropped into an external project to help locate this SDK
# It should be include()ed prior to project()

if (DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    message("Using PICO_SDK_PATH from environment ('${PICO_SDK_PATH}')")
else ()
    message(FATAL_ERROR "PICO_SDK_PATH not set; set it as an environment variable or via -DPICO_SDK_PATH=...")
endif ()

set(PICO_SDK_INIT_CMAKE_FILE ${PICO_SDK_PATH}/pico_sdk_init.cmake)
if (NOT EXISTS ${PICO_SDK_INIT_CMAKE_FILE})
    message(FATAL_ERROR "Directory '${PICO_SDK_PATH}' does not appear to contain the Pico SDK")
endif ()

include(${PICO_SDK_INIT_CMAKE_FILE})
