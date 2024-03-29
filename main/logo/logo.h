#pragma once

#include "AFlogo.xbm"
#include "AFlogo_0.xbm"
#include "AFlogo_1.xbm"
#include "AFlogo_2.xbm"
#include "AFlogo_3.xbm"
#include "AFlogo_4.xbm"
#include "AFlogo_5.xbm"

#include "Wifi_0.xbm"
#include "Wifi_1.xbm"
#include "Wifi_2.xbm"
#include "Wifi_3.xbm"

#include "Wifi_Icon.xbm"

#include "Battery.xbm"

#include "Pan_Icon.xbm"
#include "Tilt_Icon.xbm"

#include "Phone.xbm"
#include "Tracker.xbm"

#include "Home.xbm"
#include "Airplane.xbm"
#include "Small_Wifi.xbm"
#include "Power.xbm"

#define AF_LOGO_WIDTH 37
#define AF_LOGO_HEIGHT 34
#define AF_LOGO_ANIMATION_COUNT 9
#define AF_LOGO_ANIMATION_REPEAT 1

static const uint8_t *AF_LOGO = (uint8_t *)AFlogo_bits;
static const char *af_logo_images[] = {
    AFlogo_0_bits,
    AFlogo_1_bits,
    AFlogo_2_bits,
    AFlogo_3_bits,
    AFlogo_4_bits,
    AFlogo_5_bits,
    AFlogo_5_bits,
    AFlogo_0_bits,
    AFlogo_5_bits,
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

// #define WIFI_ICON_WIDTH Wifi_Icon_width
// #define WIFI_ICON_HEIGHT Wifi_Icon_height
// static const uint8_t *WIFI_IMG = (uint8_t *)Wifi_Icon_bits;

#define BATTERY_WIDTH Battery_width
#define BATTERY_HEIGHT Battery_height
#define BATTERY_BOX_WIDTH (BATTERY_WIDTH - 3)
#define BATTERY_BOX_HEIGHT (BATTERY_HEIGHT - 2)

static const uint8_t *BATTERY_IMG = (uint8_t *)Battery_bits;

static const uint8_t *PAN_ICON = (uint8_t *)Pan_Icon_bits;
static const uint8_t *TILT_ICON = (uint8_t *)Tilt_Icon_bits;
static const uint8_t *TILT_R_ICON = (uint8_t *)Tilt_R_Icon_bits;

#define PHONE_WIDTH Phone_width
#define PHONE_HEIGHT Phone_height
static const uint8_t *PHONE_IMG = (uint8_t *)Phone_bits;

#define TRACKER_WIDTH Tracker_width
#define TRACKER_HEIGHT Tracker_height
static const uint8_t *TRACKER_IMG = (uint8_t *)Tracker_bits;

#define HOME_WIDTH Home_width
#define HOME_HEIGHT Home_height
static const uint8_t *HOME_ICON = (uint8_t *)Home_bits;

#define SMALL_WIFI_WIDTH Small_Wifi_width
#define SMALL_WIFI_HEIGHT Small_Wifi_height
static const uint8_t *SMALL_WIFI_ICON = (uint8_t *)Small_Wifi_bits;

#define AIRPLANE_WIDTH Airplane_width
#define AIRPLANE_HEIGHT Airplane_height
static const uint8_t *AIRPLANE_ICON = (uint8_t *)Airplane_bits;

#define POWER_WIDTH Power_width
#define POWER_HEIGHT Power_height
static const uint8_t *POWER_ICON = (uint8_t *)Power_bits;
