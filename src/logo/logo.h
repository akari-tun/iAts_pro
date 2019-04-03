#pragma once

#include "AFlogo.xbm"
#include "AFlogo_0.xbm"
#include "AFlogo_1.xbm"
#include "AFlogo_2.xbm"
#include "AFlogo_3.xbm"
#include "AFlogo_4.xbm"

#include "Wifi_0.xbm"
#include "Wifi_1.xbm"
#include "Wifi_2.xbm"
#include "Wifi_3.xbm"

#include "Wifi_Icon.xbm"

#include "Battery.xbm"

#include "Pan_Icon.xbm"
#include "Tilt_Icon.xbm"


#define AF_LOGO_WIDTH 39
#define AF_LOGO_HEIGHT 31
#define AF_LOGO_ANIMATION_COUNT 9
#define AF_LOGO_ANIMATION_REPEAT 1

static const uint8_t *AF_LOGO = (uint8_t *)AFlogo_bits;
static const char *af_logo_images[] = {
    AFlogo_0_bits,
    AFlogo_1_bits,
    AFlogo_2_bits,
    AFlogo_3_bits,
    AFlogo_4_bits,
    AFlogo_4_bits,
    AFlogo_0_bits,
    AFlogo_4_bits,
    AFlogo_0_bits,
};

#define WIFI_WIDTH Wifi_0_width
#define WIFI_HEIGHT Wifi_0_height
#define WIFI_ANIMATION_COUNT 4

static const char *wifi_images[] = {
    Wifi_0_bits,
    Wifi_1_bits,
    Wifi_2_bits,
    Wifi_3_bits,
};

#define WIFI_ICON_WIDTH Wifi_Icon_width
#define WIFI_ICON_HEIGHT Wifi_Icon_height
static const uint8_t *WIFI_IMG = (uint8_t *)Wifi_Icon_bits;

#define BATTERY_WIDTH Battery_width
#define BATTERY_HEIGHT Battery_height
#define BATTERY_BOX_WIDTH BATTERY_WIDTH - 3
#define BATTERY_BOX_HEIGHT BATTERY_HEIGHT - 2

static const uint8_t *BATTERY_IMG = (uint8_t *)Battery_bits;

static const uint8_t *PAN_ICON = (uint8_t *)Pan_Icon_bits;
static const uint8_t *TILT_ICON = (uint8_t *)Tilt_Icon_bits;