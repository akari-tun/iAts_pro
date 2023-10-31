#pragma once

//#define USE_RX5808

#define USE_IMU

#if defined(ESP32)
#include "platforms/esp32/pre_platform.h"
#endif

#ifndef VERSION
    #define VERSION "0.1.3"
#endif

/*
    Version 0.1.1 
    release data 20191025
    1.Add ltm protocols.
    2.Fixes wifi smartconfig's bug.
    3.Other found bugs fixed.
*/

/*
    Version 0.1.2 
    release data 20200722
    1.Support MPU9250
    2.Compatible hardware v2.22 & v2.3
*/