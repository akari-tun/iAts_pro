#include "../src/target/target.h"

#ifdef USE_SCREEN
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <hal/log.h>

#include <u8g2.h>

#include "platform/system.h"

#include "logo/logo.h"
#include "ui/screen_i2c.h"

#include "util/time.h"
#include "util/version.h"

#include "screen.h"
#include "tracker/tracker.h"
#include "wifi/wifi.h"

#define SCREEN_DRAW_BUF_SIZE 128
#define ANIMATION_FRAME_DURATION_MS 500
#define ANIMATION_TOTAL_DURATION_MS (ANIMATION_REPEAT * ANIMATION_COUNT * ANIMATION_FRAME_DURATION_MS)

typedef enum
{
    SCREEN_DIRECTION_HORIZONTAL,
    SCREEN_DIRECTION_VERTICAL,
} screen_direction_e;

#define SCREEN_DIRECTION(s) ((screen_direction_e)s->internal.direction)
#define SCREEN_W(s) (s->internal.w)
#define SCREEN_H(s) (s->internal.h)
#define SCREEN_BUF(s) (s->internal.buf)

typedef enum
{
    SCREEN_MULTILINEOPT_NONE = 0,
    SCREEN_MULTILINEOPT_BORDER = 1,
    SCREEN_MULTILINEOPT_BOX = 2,
} screen_multiline_opt_e;

static u8g2_t u8g2;

// bool screen_init(screen_t *screen, screen_i2c_config_t *cfg, servo_t *servo)
bool screen_init(screen_t *screen, screen_i2c_config_t *cfg, tracker_t *tracker)
{
    memset(screen, 0, sizeof(*screen));
    screen->internal.available = screen_i2c_init(cfg, &u8g2);
    screen->internal.cfg = *cfg;
    // screen->internal.servo = servo;
    screen->internal.tracker = tracker;
    return screen->internal.available;
}

bool screen_is_available(const screen_t *screen)
{
    return screen->internal.available;
}

void screen_shutdown(screen_t *screen)
{
    screen_i2c_shutdown(&screen->internal.cfg, &u8g2);
}

void screen_power_on(screen_t *screen)
{
    screen_i2c_power_on(&screen->internal.cfg, &u8g2);
}

void screen_enter_secondary_mode(screen_t *screen, screen_secondary_mode_e mode)
{
    screen->internal.secondary_mode = mode;
}

static void screen_splash_task(void *arg)
{
    screen_t *screen = arg;
    screen->internal.splashing = true;
    uint16_t w = u8g2_GetDisplayWidth(&u8g2);
    uint16_t h = u8g2_GetDisplayHeight(&u8g2);

    uint16_t anim_x = (w - AF_LOGO_WIDTH) / 2;
    uint16_t anim_y = (h - AF_LOGO_HEIGHT) / 2 + 2;

#define SPLASH_TOP FIRMWARE_NAME
#define SPLASH_AUTHOR "A.T"
#define SPLASH_AUTHOR_LABEL " Design by A.T"
#define SPLASH_VERSION_LABEL "Version:"
#define SPLASH_BOTTOM_HORIZONTAL SOFTWARE_VERSION

    for (int ii = 0; ii < AF_LOGO_ANIMATION_REPEAT + 1; ii++)
    {
        if (ii < AF_LOGO_ANIMATION_REPEAT)
        {
            // Draw logo by animation
            for (int jj = 0; jj < AF_LOGO_ANIMATION_COUNT; jj++)
            {
                u8g2_ClearBuffer(&u8g2);

                u8g2_DrawXBM(&u8g2, anim_x, anim_y, AF_LOGO_WIDTH, AF_LOGO_HEIGHT, (uint8_t *)af_logo_images[jj]);

                u8g2_SendBuffer(&u8g2);
                time_millis_delay(ANIMATION_FRAME_DURATION_MS);
            }
        }
        else
        {
            // Draw all startup info
            u8g2_ClearBuffer(&u8g2);

            u8g2_SetFontPosTop(&u8g2);
            u8g2_SetFont(&u8g2, u8g2_font_profont22_tf);
            uint16_t tw = u8g2_GetStrWidth(&u8g2, SPLASH_TOP);
            u8g2_DrawStr(&u8g2, (w - tw) / 3, 0, SPLASH_TOP);

            u8g2_SetFont(&u8g2, u8g2_font_profont10_tf);

            tw = u8g2_GetStrWidth(&u8g2, SOFTWARE_VERSION);
            u8g2_DrawStr(&u8g2, w - (w / 4) - tw + 10, 6, SOFTWARE_VERSION);

            u8g2_SetFontPosBottom(&u8g2);
            u8g2_SetFont(&u8g2, u8g2_font_profont12_tf);

            uint16_t bw = u8g2_GetStrWidth(&u8g2, SPLASH_AUTHOR_LABEL);
            u8g2_DrawStr(&u8g2, (w - bw) / 2, h, SPLASH_AUTHOR_LABEL);

            u8g2_DrawXBM(&u8g2, anim_x, anim_y, AF_LOGO_WIDTH, AF_LOGO_HEIGHT, AF_LOGO);

            u8g2_SendBuffer(&u8g2);
            time_millis_delay(ANIMATION_FRAME_DURATION_MS);
        }
    }

    time_millis_delay(ANIMATION_FRAME_DURATION_MS);

    screen->internal.tracker->internal.status_changed(screen->internal.tracker, TRACKER_STATUS_WIFI_CONNECTING);
    wifi_start(screen->internal.wifi);

    screen->internal.splashing = false;

    vTaskDelete(NULL);
}

void screen_splash(screen_t *screen)
{
    xTaskCreatePinnedToCore(screen_splash_task, "SCREEN-SPLASH", 4096, screen, 2, NULL, xPortGetCoreID());
}

void screen_set_orientation(screen_t *screen, screen_orientation_e orientation)
{
    const u8g2_cb_t *rotation = U8G2_R0;
    switch (orientation)
    {
    case SCREEN_ORIENTATION_HORIZONTAL_LEFT:
        // Default rotation, buttons at the left
        break;
    case SCREEN_ORIENTATION_HORIZONTAL_RIGHT:
        // Buttons at the right
        rotation = U8G2_R2;
        break;
    case SCREEN_ORIENTATION_VERTICAL:
        // Buttons on botttom
        rotation = U8G2_R1;
        break;
    case SCREEN_ORIENTATION_VERTICAL_UPSIDE_DOWN:
        rotation = U8G2_R3;
        break;
    }
    u8g2_SetDisplayRotation(&u8g2, rotation);
}

void screen_set_brightness(screen_t *screen, screen_brightness_e brightness)
{
    int contrast = -1;
    switch (brightness)
    {
    case SCREEN_BRIGHTNESS_LOW:
        /* XXX: Don't use 0 contrast here, since some screens will turn
         * off completely with zero. Using 1 has no noticeable difference
         * and let's us easily make the same code work in all supported
         * boards. See #1 for discussion.
         */
        contrast = 1;
        break;
    case SCREEN_BRIGHTNESS_MEDIUM:
        contrast = 128;
        break;
    case SCREEN_BRIGHTNESS_HIGH:
        contrast = 255;
        break;
    }
    if (contrast >= 0)
    {
        u8g2_SetContrast(&u8g2, contrast);
    }
}

bool screen_is_animating(const screen_t *screen)
{
    return false;
}

static bool screen_button_enter_press(screen_t *screen, const button_event_t *ev)
{
    switch (ev->type)
    {
    case BUTTON_EVENT_TYPE_SHORT_PRESS:
        break;
    case BUTTON_EVENT_TYPE_LONG_PRESS:
        if (screen->internal.main_mode == SCREEN_MODE_WAIT_CONNECT)
        {
            screen->internal.wifi->status = WIFI_STATUS_SMARTCONFIG;
            return true;
        }
        if (screen->internal.main_mode == SCREEN_MODE_WIFI_CONFIG)
        {
            wifi_smartconfig_stop(screen->internal.wifi);
            screen->internal.wifi->status = WIFI_STATUS_NONE;
            wifi_start(screen->internal.wifi);
            return true;
        }
        break;
    case BUTTON_EVENT_TYPE_REALLY_LONG_PRESS:
        if (screen->internal.tracker->internal.mode == TRACKER_MODE_AUTO) 
        {
            screen->internal.tracker->internal.mode = TRACKER_MODE_MANUAL;
            screen->internal.tracker->internal.status_changed(screen->internal.tracker, TRACKER_STATUS_MANUAL);
            // if (screen->internal.main_mode != SCREEN_MODE_MAIN) screen->internal.main_mode = SCREEN_MODE_MAIN;
        } 
        else if (screen->internal.tracker->internal.mode == TRACKER_MODE_MANUAL) 
        { 
            screen->internal.tracker->internal.mode = TRACKER_MODE_AUTO;
            screen->internal.tracker->internal.status_changed(screen->internal.tracker, TRACKER_STATUS_TRACKING);
        }
        else if (screen->internal.tracker->internal.mode == TRACKER_MODE_DEBUG) 
        {
            screen->internal.tracker->internal.mode = TRACKER_MODE_AUTO;
        }

        if (screen->internal.wifi->status == WIFI_STATUS_SMARTCONFIG)
        {
            wifi_smartconfig_stop(screen->internal.wifi);
            screen->internal.wifi->status = WIFI_STATUS_NONE;
            wifi_start(screen->internal.wifi);
        }
        return true;
    }

    return false;
}

static bool screen_button_left_press(screen_t *screen, const button_event_t *ev)
{
    switch (ev->type)
    {
    case BUTTON_EVENT_TYPE_SHORT_PRESS:
        break;
    case BUTTON_EVENT_TYPE_LONG_PRESS:
        break;
    case BUTTON_EVENT_TYPE_REALLY_LONG_PRESS:
        break;
    }
    return false;
}

static bool screen_button_right_press(screen_t *screen, const button_event_t *ev)
{
    return false;
}

static bool screen_button_up_press(screen_t *screen, const button_event_t *ev)
{
    return false;
}

static bool screen_button_down_press(screen_t *screen, const button_event_t *ev)
{
    return false;
}

bool screen_handle_button_event(screen_t *screen, bool before_menu, const button_event_t *ev)
{

    switch (ev->button->id)
    {
        case BUTTON_ID_ENTER:
            return screen_button_enter_press(screen, ev);
        case BUTTON_ID_LEFT:
            return screen_button_left_press(screen, ev);
        case BUTTON_ID_RIGHT:
            return screen_button_right_press(screen, ev);
        case BUTTON_ID_UP:
            return screen_button_up_press(screen, ev);
        case BUTTON_ID_DOWN:
            return screen_button_down_press(screen, ev);
    }
//     if (screen->internal.main_mode == SCREEN_MODE_WAIT_CONNECT)
//     {
//         if (ev->type != BUTTON_EVENT_TYPE_LONG_PRESS)
//             return false;

//         if (ev->button->id == BUTTON_ID_ENTER)
//         {
//             screen->internal.wifi->status = WIFI_STATUS_SMARTCONFIG;
//             return true;
//         }

//         if (ev->button->id == BUTTON_ID_DOWN)
//         {
//             screen->internal.tracker->internal.mode = TRACKER_MODE_MANUAL;
//             screen->internal.main_mode = SCREEN_MODE_MAIN;
//             return true;
//         }

//         return false;
//     }

//     if (screen->internal.main_mode == SCREEN_MODE_WIFI_CONFIG)
//     {
//         if (ev->type != BUTTON_EVENT_TYPE_LONG_PRESS)
//             return false;

//         if (ev->button->id == BUTTON_ID_ENTER)
//         {
//             wifi_smartconfig_stop(screen->internal.wifi);
//             screen->internal.wifi->status = WIFI_STATUS_NONE;
//             wifi_start(screen->internal.wifi);
//             return true;
//         }

//         if (ev->button->id == BUTTON_ID_DOWN)
//         {
//             screen->internal.tracker->internal.mode = TRACKER_MODE_MANUAL;
//             screen->internal.main_mode = SCREEN_MODE_MAIN;
//             return true;
//         }

//         return false;
//     }

//     if (screen->internal.secondary_mode != SCREEN_SECONDARY_MODE_NONE)
//     {
//         screen->internal.secondary_mode = SCREEN_SECONDARY_MODE_NONE;
//         return true;
//     }

//     if (before_menu)
//     {
//         return false;
//     }

//     if (ev->type != BUTTON_EVENT_TYPE_SHORT_PRESS)
//     {
//         return false;
//     }

// #if defined(USE_BUTTON_5WAY)
//     button_id_e bid = button_event_id(ev);
//     int direction = bid == BUTTON_ID_LEFT ? -1 : bid == BUTTON_ID_RIGHT ? 1 : 0;
//     if (direction == 0)
//     {
//         return false;
//     }
// #else
//     int direction = 1;
// #endif

     return false;
}

static void screen_draw_main(screen_t *s)
{
    u8g2_SetDrawColor(&u8g2, 1);

    uint16_t w = u8g2_GetDisplayWidth(&u8g2);
    uint16_t h = u8g2_GetDisplayHeight(&u8g2);
    const uint16_t per_h = h / 8;
    const uint8_t frame_height = 7;

    u8g2_SetFontPosTop(&u8g2);

    char *buf = SCREEN_BUF(s);

#ifdef USE_WIFI
    // wifi icon
    u8g2_DrawXBM(&u8g2, 0, 0, WIFI_ICON_WIDTH, WIFI_ICON_HEIGHT, WIFI_IMG);
    u8g2_SetFont(&u8g2, u8g2_font_profont15_tf);
    // uint16_t tw = u8g2_GetStrWidth(&u8g2, (char *)&s->internal.wifi->config->sta.ssid);
    u8g2_DrawStr(&u8g2, WIFI_ICON_WIDTH + 2, 2, (char *)&s->internal.wifi->config->sta.ssid);
    // u8g2_SetFont(&u8g2, u8g2_font_profont15_tf);
    // uint16_t tw = u8g2_GetStrWidth(&u8g2, (char *)&s->internal.wifi->config->sta.ssid);
    // u8g2_DrawStr(&u8g2, 0, per_h * 7, (char *)&s->internal.wifi->config->sta.password);
#endif

#ifdef USE_BATTERY_MEASUREMENT
    // battery icon
    u8g2_DrawXBM(&u8g2, w - BATTERY_WIDTH, 0, BATTERY_WIDTH, BATTERY_HEIGHT, BATTERY_IMG);
    float voltage = battery_get_voltage(s->internal.battery);
    int per_voltage = 0;
    if (voltage >= DEFAULT_BATTERY_CENTER_VOLTAGE)
    {
        per_voltage = (((voltage - DEFAULT_BATTERY_CENTER_VOLTAGE) / (DEFAULT_BATTERY_MAX_VOLTAGE - DEFAULT_BATTERY_CENTER_VOLTAGE) * 0.5) + 0.5) * BATTERY_BOX_WIDTH;
    }
    else if (voltage >= DEFAULT_BATTERY_MIN_VOLTAGE)
    {
        per_voltage = ((voltage - DEFAULT_BATTERY_MIN_VOLTAGE) / (DEFAULT_BATTERY_CENTER_VOLTAGE - DEFAULT_BATTERY_MIN_VOLTAGE)) * (BATTERY_BOX_WIDTH / 2);
    }
    u8g2_DrawBox(&u8g2, w - BATTERY_WIDTH + 1, 1, per_voltage, BATTERY_BOX_HEIGHT);
    u8g2_SetFont(&u8g2, u8g2_font_profont10_tf);
    snprintf(buf, SCREEN_DRAW_BUF_SIZE, "%2.2fv", voltage);
    u8g2_DrawStr(&u8g2, 80, 0, buf);
#endif

    const uint16_t bar_width = 85;
    const uint8_t bar_start_x = 18;
    const uint8_t per_start_x = 105;
    uint8_t tilt_percentage = servo_get_per_pulsewidth(&s->internal.tracker->servo->internal.tilt);
    uint16_t tilt_box_width = (bar_width * tilt_percentage) / 100;
    // icon
    u8g2_DrawXBM(&u8g2, 0, per_h * 2, TILT_ICON_WIDTH, TILT_ICON_HEIGHT, TILT_ICON);
    // pluse width and degree
    u8g2_SetFont(&u8g2, u8g2_font_profont10_tf);
    snprintf(buf, SCREEN_DRAW_BUF_SIZE, "P:%4dus    D:%3d",
             servo_get_pulsewidth(&s->internal.tracker->servo->internal.tilt), servo_get_degree(&s->internal.tracker->servo->internal.tilt));
    u8g2_DrawStr(&u8g2, bar_start_x, per_h * 2, buf);
    u8g2_DrawHLine(&u8g2, per_start_x - 1, per_h * 2, 2);
    u8g2_DrawHLine(&u8g2, per_start_x - 1, per_h * 2 + 1, 2);
    // percent pulse width
    snprintf(buf, SCREEN_DRAW_BUF_SIZE, "%3d%%", tilt_percentage);
    u8g2_DrawStr(&u8g2, per_start_x, per_h * 3, buf);
    // frame
    u8g2_DrawFrame(&u8g2, bar_start_x, per_h * 3, bar_width, frame_height);
    // box
    u8g2_DrawBox(&u8g2, bar_start_x, per_h * 3, tilt_box_width, frame_height);

    uint8_t pan_percentage = servo_get_per_pulsewidth(&s->internal.tracker->servo->internal.pan);
    uint16_t pan_box_width = (bar_width * pan_percentage) / 100;
    // label
    u8g2_DrawXBM(&u8g2, 0, per_h * 4, PAN_ICON_WIDTH, PAN_ICON_HEIGHT, PAN_ICON);
    // pluse width and degree
    u8g2_SetFont(&u8g2, u8g2_font_profont10_tf);
    snprintf(buf, SCREEN_DRAW_BUF_SIZE, "P:%4dus    D:%3d",
             servo_get_pulsewidth(&s->internal.tracker->servo->internal.pan), servo_get_degree(&s->internal.tracker->servo->internal.pan));
    u8g2_DrawStr(&u8g2, bar_start_x, per_h * 4, buf);
    u8g2_DrawHLine(&u8g2, per_start_x - 1, per_h * 4, 2);
    u8g2_DrawHLine(&u8g2, per_start_x - 1, per_h * 4 + 1, 2);
    // percent pulse width
    snprintf(buf, SCREEN_DRAW_BUF_SIZE, "%3d%%", pan_percentage);
    u8g2_DrawStr(&u8g2, per_start_x, per_h * 5, buf);
    // frame
    u8g2_DrawFrame(&u8g2, bar_start_x, per_h * 5, bar_width, frame_height);
    // box
    u8g2_DrawBox(&u8g2, bar_start_x, per_h * 5, pan_box_width, frame_height);
}

static void screen_draw_wifi_config(screen_t *s)
{
    const char *sc_marker = "* * * * * * *";
    bool draw_sc_marker = TIME_CYCLE_EVERY_MS(200, 2) == 0;

    u8g2_SetDrawColor(&u8g2, 1);
    uint16_t w = u8g2_GetDisplayWidth(&u8g2);
    uint16_t h = u8g2_GetDisplayHeight(&u8g2);
    char *buf = SCREEN_BUF(s);
    uint16_t tw = 0;

     u8g2_SetFont(&u8g2, u8g2_font_profont15_tf);
    u8g2_SetFontPosCenter(&u8g2);

    if (draw_sc_marker)
    {
        tw = u8g2_GetStrWidth(&u8g2, sc_marker);
        u8g2_DrawStr(&u8g2, (w - tw) / 2, (h / 2) - 15, sc_marker);
    }

    //draw smart config
    if (TIME_CYCLE_EVERY_MS(800, 2) == 0)
    {
        snprintf(buf, SCREEN_DRAW_BUF_SIZE, "SmartConfig");
        tw = u8g2_GetStrWidth(&u8g2, buf);
        u8g2_DrawStr(&u8g2, (w - tw) / 2, h / 2, buf);
    }

    if (draw_sc_marker)
    {
        tw = u8g2_GetStrWidth(&u8g2, sc_marker);
        u8g2_DrawStr(&u8g2, (w - tw) / 2, (h / 2) + 20, sc_marker);
    }
}

uint8_t wifi_index = 0;
bool last_flash = true;

static void screen_draw_wait_connect(screen_t *s)
{
    u8g2_SetDrawColor(&u8g2, 1);
    u8g2_SetFont(&u8g2, u8g2_font_micro_tr);
    u8g2_SetFontPosTop(&u8g2);

    uint16_t w = u8g2_GetDisplayWidth(&u8g2);
    uint16_t h = u8g2_GetDisplayHeight(&u8g2);
    char *buf = SCREEN_BUF(s);

    bool flash = TIME_CYCLE_EVERY_MS(400, 2) == 0;

    if (last_flash != flash)
    {
        wifi_index++;
        last_flash = flash;
    }
    if (wifi_index > 3)
        wifi_index = 0;

    u8g2_DrawXBM(&u8g2, 0, 0, WIFI_WIDTH, WIFI_HEIGHT, (uint8_t *)wifi_images[wifi_index]);

    u8g2_SetFontPosBottom(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_profont10_tf);

    snprintf(buf, SCREEN_DRAW_BUF_SIZE, "SSID:%s", (char *)&s->internal.wifi->config->sta.ssid);
    //const char *ptr_ssid = "SSID:iAts_wifi";
    u8g2_DrawStr(&u8g2, WIFI_WIDTH + 6, WIFI_HEIGHT / 2 + 5, buf);

    snprintf(buf, SCREEN_DRAW_BUF_SIZE, "PWD:%s", (char *)&s->internal.wifi->config->sta.password);
    //const char *ptr_pwd = " PWD:12345678";
    u8g2_DrawStr(&u8g2, WIFI_WIDTH + 6, WIFI_HEIGHT, buf);

    //draw connecting
    if (TIME_CYCLE_EVERY_MS(600, 2) == 0)
    {
        u8g2_SetFontPosCenter(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_profont15_tf);
        snprintf(buf, SCREEN_DRAW_BUF_SIZE, "== CONNECTING ==");
        // const char *wait_conn = "== CONNECTING ==";
        uint16_t tw = u8g2_GetStrWidth(&u8g2, buf);
        u8g2_DrawStr(&u8g2, (w - tw) / 2, h - (h / 4) - 3, buf);
    }

    u8g2_SetFontPosBottom(&u8g2);
    u8g2_SetFont(&u8g2, u8g2_font_profont10_tf);
    snprintf(buf, SCREEN_DRAW_BUF_SIZE, "long press to config");
    uint16_t tw = u8g2_GetStrWidth(&u8g2, buf);
    u8g2_DrawStr(&u8g2, (w - tw) / 2, h, buf);
}

static void screen_draw_wait_server(screen_t *s)
{
    const char *txt = "Connecting to server";
    const char *conn = "<-->";

    uint16_t w = u8g2_GetDisplayWidth(&u8g2);
    uint16_t h = u8g2_GetDisplayHeight(&u8g2);

    u8g2_DrawXBM(&u8g2, 8, 0, TRACKER_WIDTH, TRACKER_HEIGHT, TRACKER_IMG);
    u8g2_DrawXBM(&u8g2, w - PHONE_WIDTH - 8, 0, PHONE_WIDTH, PHONE_HEIGHT, PHONE_IMG);

    if (TIME_CYCLE_EVERY_MS(200, 2) == 0)
    {
        u8g2_SetFontPosCenter(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_profont15_tf);
        uint16_t tw = u8g2_GetStrWidth(&u8g2, conn);
        u8g2_DrawStr(&u8g2, (w / 2) - tw / 2, 18, conn);
    }

    if (TIME_CYCLE_EVERY_MS(800, 2) == 0)
    {
        u8g2_SetFontPosCenter(&u8g2);
        u8g2_SetFont(&u8g2, u8g2_font_profont12_tf);
        uint16_t tw = u8g2_GetStrWidth(&u8g2, txt);
        u8g2_DrawStr(&u8g2, (w - tw) / 2, h - (h / 4) + 2, txt);
    }
}

static void screen_draw(screen_t *screen)
{
    switch ((screen_secondary_mode_e)screen->internal.secondary_mode)
    {
    case SCREEN_SECONDARY_MODE_NONE:
        switch ((screen_main_mode_e)screen->internal.main_mode)
        {
        case SCREEN_MODE_MAIN:
            screen_draw_main(screen);
            break;
        case SCREEN_MODE_WIFI_CONFIG:
            screen_draw_wifi_config(screen);
            break;
        case SCREEN_MODE_WAIT_CONNECT:
            screen_draw_wait_connect(screen);
            break;
        case SCREEN_MODE_WAIT_SERVER:
            screen_draw_wait_server(screen);
            break;
        }
        break;
    case SCREEN_SECONDARY_MODE_FREQUENCIES:
        //screen_draw_frequencies(screen);
        break;
    case SCREEN_SECONDARY_MODE_DEBUG_INFO:
        //screen_draw_debug_info(screen);
        break;
    }
}

void screen_update(screen_t *screen)
{
    if (screen->internal.splashing)
    {
        return;
    }

    if (screen->internal.tracker->internal.mode != TRACKER_MODE_MANUAL)
    {
        if (screen->internal.wifi->status == WIFI_STATUS_CONNECTED &&
            screen->internal.tracker->internal.status == TRACKER_STATUS_WIFI_CONNECTING)
        {
            screen->internal.tracker->internal.status_changed(screen->internal.tracker, TRACKER_STATUS_WIFI_CONNECTED);
        }
        else if ((screen->internal.wifi->status == WIFI_STATUS_CONNECTING || screen->internal.wifi->status == WIFI_STATUS_DISCONNECTED) &&
                 screen->internal.tracker->internal.status != TRACKER_STATUS_WIFI_CONNECTING)
        {
            screen->internal.tracker->internal.status_changed(screen->internal.tracker, TRACKER_STATUS_WIFI_CONNECTING);
        }
        else if (screen->internal.wifi->status == WIFI_STATUS_SMARTCONFIG &&
                 screen->internal.tracker->internal.status != TRACKER_STATUS_WIFI_SMART_CONFIG)
        {
            screen->internal.tracker->internal.status_changed(screen->internal.tracker, TRACKER_STATUS_WIFI_SMART_CONFIG);
        }
        else if (screen->internal.wifi->status == WIFI_STATUS_CONNECTED &&
                 screen->internal.tracker->internal.status != TRACKER_STATUS_SERVER_CONNECTING)
        {
            screen->internal.tracker->internal.status_changed(screen->internal.tracker, TRACKER_STATUS_SERVER_CONNECTING);
        }
    }

    char buf[SCREEN_DRAW_BUF_SIZE];

    screen->internal.w = u8g2_GetDisplayWidth(&u8g2);
    screen->internal.h = u8g2_GetDisplayHeight(&u8g2);
    screen->internal.direction = SCREEN_W(screen) > SCREEN_H(screen) ? SCREEN_DIRECTION_HORIZONTAL : SCREEN_DIRECTION_VERTICAL;
    screen->internal.buf = buf;
    u8g2_ClearBuffer(&u8g2);
    screen_draw(screen);
    u8g2_SendBuffer(&u8g2);
}

#endif