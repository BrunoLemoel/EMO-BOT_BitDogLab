# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

# Use 8.3 short path without spaces to avoid GCC 14 include errors on Windows
if(EXISTS "C:/PROGRA~1/RASPBE~2/PICOSD~1.1/pico-sdk/pico_sdk_init.cmake")
    set(PICO_SDK_PATH "C:/PROGRA~1/RASPBE~2/PICOSD~1.1/pico-sdk")
elseif(DEFINED ENV{PICO_SDK_PATH})
    if(EXISTS "$ENV{PICO_SDK_PATH}/pico-sdk/pico_sdk_init.cmake")
        set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}/pico-sdk")
    elseif(EXISTS "$ENV{PICO_SDK_PATH}/pico_sdk_init.cmake")
        set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
    endif()
endif()

include(pico_sdk_import.cmake)

project(bitdoglab_emo_bot C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(bitdoglab_emo_bot
    main.c
    ssd1306.c
    matrix.c
    rgb_led.c
    buzzer.c
)

pico_generate_pio_header(bitdoglab_emo_bot ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

target_include_directories(bitdoglab_emo_bot PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(bitdoglab_emo_bot
    pico_stdlib
    hardware_i2c
    hardware_pio
    hardware_pwm
    hardware_clocks
)

# Enable USB CDC serial, disable UART
pico_enable_stdio_usb(bitdoglab_emo_bot 1)
pico_enable_stdio_uart(bitdoglab_emo_bot 0)

# Generate UF2 for Pico W
pico_add_extra_outputs(bitdoglab_emo_bot)
