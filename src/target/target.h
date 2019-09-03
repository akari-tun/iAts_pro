#pragma once

#ifndef ESP32
#define ESP32
#endif

#define USE_RX5808

#if defined(ESP32)
#include "../src/target/platforms/esp32/pre_platform.h"
#endif

#define VERSION "0.1.0"