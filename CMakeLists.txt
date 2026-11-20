cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(tactigon_skin)

target_sources(
    app PRIVATE 
    src/main.c
    src/ble/ble.c
    src/audio/audio.c
    src/adpcm/adpcm.c
    src/imu/imu.c
    src/fusion/madgwick.c
    src/led/led.c
    src/sleep/sleep.c
)

target_include_directories(app PRIVATE
    src
)