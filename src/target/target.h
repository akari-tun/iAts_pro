#pragma once

#define USE_RX5808

#define USE_IMU

#if defined(ESP32)
#include "../src/target/platforms/esp32/pre_platform.h"
#endif

#define VERSION "0.1.1"

/*
    Version 0.1.1 
    release data 20191025
    1.Add ltm protocols.
    2.Fixes wifi smartconfig's bug.
    3.Other found bugs fixed.
*/