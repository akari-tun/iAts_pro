
#include <hal/log.h>

#include "../src/target/target.h"

#include "platform/system.h"

#include "config/settings.h"
#include "ui/led.h"
#include "ui/screen.h"
#include "ui/ui.h"
#include "tracker/tracker.h"
#include "protocols/atp.h"
#include "menu.h"
#if defined(USE_WIFI)
#include "wifi/wifi.h"
#endif

#include "util/macros.h"

#ifdef USE_SCREEN
// We don't log anything without a screen, so this would
// produce a warning if we didn't guard it
static const char *TAG = "UI";
#endif

#ifdef USE_BATTERY_MONITORING
static battery_t battery;
#endif

#ifdef USE_POWER_MONITORING
static power_t power;
#endif

static Observer ui_status_observer;
static Observer ui_flag_observer;
static Observer ui_reverse_observer;
static Observer ui_imu_calibration_step_done_observer;
static Observer ui_imu_calibration_done_observer;

static void ui_status_updated(void *notifier, void *s)
{
    Observer *obs = (Observer *)notifier;
    tracker_status_e *status = (tracker_status_e *)s;
    ui_t *ui = (ui_t *)obs->Obj;
    time_micros_t now = time_micros_now();

    switch (*status)
    {
    case TRACKER_STATUS_BOOTING:
        break;
    case TRACKER_STATUS_WIFI_SMART_CONFIG:
        if (led_mode_is_enable(LED_MODE_WAIT_CONNECT))
            led_mode_remove(LED_MODE_WAIT_CONNECT);
        if (!led_mode_is_enable(LED_MODE_SMART_CONFIG))
            led_mode_add(LED_MODE_SMART_CONFIG);

#if defined(USE_BEEPER)
        beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_NONE);
#endif

#if defined(USE_SCREEN)
        ui->internal.screen.internal.main_mode = SCREEN_MODE_WIFI_CONFIG;
        // screen_enter_secondary_mode(&ui->internal.screen, SCREEN_SECONDARY_MODE_WIFI_CONFIG);
        // ui->internal.screen.internal.secondary_mode = SCREEN_SECONDARY_MODE_WIFI_CONFIG;
#endif

        ui->internal.tracker->internal.flag_changed(ui->internal.tracker, TRACKER_FLAG_SERVER_CONNECTED, 0);
        ui->internal.tracker->internal.flag_changed(ui->internal.tracker, TRACKER_FLAG_WIFI_CONNECTED, 0);
        break;
    case TRACKER_STATUS_WIFI_CONNECTING:
#if defined(USE_BEEPER)
        if (ui->internal.beeper.mode != BEEPER_MODE_WAIT_CONNECT)
        {
            beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_WAIT_CONNECT);
        }
#endif
        if (led_mode_is_enable(LED_MODE_SMART_CONFIG))
        {
            led_mode_remove(LED_MODE_SMART_CONFIG);
        }
        if (!led_mode_is_enable(LED_MODE_WAIT_CONNECT))
        {
            led_mode_add(LED_MODE_WAIT_CONNECT);
        }
#if defined(USE_SCREEN)
        ui->internal.screen.internal.main_mode = SCREEN_MODE_WAIT_CONNECT;
#endif
        ui->internal.tracker->internal.flag_changed(ui->internal.tracker, TRACKER_FLAG_SERVER_CONNECTED, 0);
        ui->internal.tracker->internal.flag_changed(ui->internal.tracker, TRACKER_FLAG_WIFI_CONNECTED, 0);

        break;
    case TRACKER_STATUS_WIFI_CONNECTED:

#if defined(USE_BEEPER)
        beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_NONE);
#endif

#if defined(USE_SCREEN)
        ui->internal.screen.internal.main_mode = SCREEN_MODE_MAIN;
#endif

        if (led_mode_is_enable(LED_MODE_SMART_CONFIG))
            led_mode_remove(LED_MODE_SMART_CONFIG);
        if (led_mode_is_enable(LED_MODE_WAIT_CONNECT))
            led_mode_remove(LED_MODE_WAIT_CONNECT);

        ATP_SET_U32(TAG_TRACKER_T_IP, ui->internal.screen.internal.wifi->ip, now);
        ATP_SET_U16(TAG_TRACKER_T_PORT, 8898, now);

        ui->internal.tracker->internal.flag_changed(ui->internal.tracker, TRACKER_FLAG_WIFI_CONNECTED, 1);
        break;
    case TRACKER_STATUS_TRACKING:

#if defined(USE_SCREEN)
        ui->internal.screen.internal.main_mode = SCREEN_MODE_MAIN;
#endif

        if (led_mode_is_enable(LED_MODE_SMART_CONFIG))
            led_mode_remove(LED_MODE_SMART_CONFIG);
        if (led_mode_is_enable(LED_MODE_WAIT_CONNECT))
            led_mode_remove(LED_MODE_WAIT_CONNECT);
        if (led_mode_is_enable(LED_MODE_WAIT_SERVER))
            led_mode_remove(LED_MODE_WAIT_SERVER);

        // if (!(ui->internal.tracker->internal.flag & TRACKER_FLAG_SERVER_CONNECTED) && 
        //     ui->internal.wifi->status != WIFI_STATUS_UDP_CONNECTED && 
        //     !led_mode_is_enable(LED_MODE_WAIT_SERVER))
        //     led_mode_add(LED_MODE_WAIT_SERVER);

        ui->internal.tracker->internal.flag_changed(ui->internal.tracker, TRACKER_FLAG_TRACKING, 1);
        break;
    case TRACKER_STATUS_MANUAL:

#if defined(USE_SCREEN)
        ui->internal.screen.internal.main_mode = SCREEN_MODE_MAIN;
#endif

        if (led_mode_is_enable(LED_MODE_SMART_CONFIG))
            led_mode_remove(LED_MODE_SMART_CONFIG);
        if (led_mode_is_enable(LED_MODE_WAIT_CONNECT))
            led_mode_remove(LED_MODE_WAIT_CONNECT);
        if (led_mode_is_enable(LED_MODE_WAIT_SERVER))
            led_mode_remove(LED_MODE_WAIT_SERVER);

        ui->internal.tracker->internal.flag_changed(ui->internal.tracker, TRACKER_FLAG_TRACKING, 0);
        break;
    }
}

static void ui_flag_updated(void *notifier, void *f)
{
    Observer *obs = (Observer *)notifier;
    uint8_t *flag = (uint8_t *)f;
    ui_t *ui = (ui_t *)obs->Obj;

    if (*flag & TRACKER_FLAG_HOMESETED && ui->internal.tracker->internal.flag & TRACKER_FLAG_HOMESETED)
    {
        led_mode_add(LED_MODE_SETED);
#if defined(USE_BEEPER)
        beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_SETED);
#endif
    }

    if (*flag & TRACKER_FLAG_PLANESETED && ui->internal.tracker->internal.flag & TRACKER_FLAG_PLANESETED)
    {
        led_mode_add(LED_MODE_SETED);
#if defined(USE_BEEPER)
        beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_SETED);
#endif
    }

    if (*flag & TRACKER_FLAG_SERVER_CONNECTED && ui->internal.tracker->internal.flag & TRACKER_FLAG_SERVER_CONNECTED)
    {
        led_mode_set(LED_MODE_WAIT_SERVER, false);
#if defined(USE_BEEPER)
        beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_SETED);
#endif
#if defined(USE_WIFI)
        ui->internal.screen.internal.wifi->status_change(ui->internal.screen.internal.wifi, WIFI_STATUS_UDP_CONNECTED);
#endif
    }
    else if (ui->internal.tracker->internal.status < TRACKER_STATUS_MANUAL && ui->internal.wifi->status == WIFI_STATUS_CONNECTED)
    {
#if defined(USE_WIFI)
        led_mode_set(LED_MODE_WAIT_SERVER, true);
#endif
    }
}

static void ui_reverse_updated(void *notifier, void *r)
{
    UNUSED(r);

    led_mode_add(LED_MODE_REVERSING);
#if defined(USE_BEEPER)
    Observer *obs = (Observer *)notifier;
    ui_t *ui = (ui_t *)obs->Obj;
    beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_REVERSING);
#endif
    LOG_I(TAG, "Sevro turn round.");
}

static void ui_imu_calibration_step_done(void *notifier, void *r)
{
    UNUSED(r);

    led_mode_add(LED_MODE_CAL_STEP);
#if defined(USE_BEEPER)
    Observer *obs = (Observer *)notifier;
    ui_t *ui = (ui_t *)obs->Obj;
    beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_CAL_STEP);
#endif
    LOG_I(TAG, "Calibration step done.");
}

static void ui_imu_calibration_done(void *notifier, void *r)
{
    UNUSED(r);

    led_mode_add(LED_MODE_CAL_DONE);
#if defined(USE_BEEPER)
    Observer *obs = (Observer *)notifier;
    ui_t *ui = (ui_t *)obs->Obj;
    beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_CAL_DONE);
#endif
    LOG_I(TAG, "Calibration done.");
}

#if defined(USE_WIFI)

static Observer ui_wifi_status_observer;

static void ui_wifi_status_update(void *notifier, void *s)
{
    Observer *obs = (Observer *)notifier;
    iats_wifi_status_e *status = (iats_wifi_status_e *)s;
    ui_t *ui = (ui_t *)obs->Obj;
    tracker_t *t = ui->internal.tracker;

    switch (*status)
    {
    case WIFI_STATUS_SMARTCONFIG:
        if (t->internal.status != TRACKER_STATUS_WIFI_SMART_CONFIG)
        {
            t->internal.status_changed(t, TRACKER_STATUS_WIFI_SMART_CONFIG);
        }
        if (ui->internal.screen.internal.main_mode != SCREEN_MODE_WIFI_CONFIG)
        {
            ui->internal.screen.internal.main_mode = SCREEN_MODE_WIFI_CONFIG;
        }
        break;
    case WIFI_STATUS_DISCONNECTED:
        if (t->internal.status != TRACKER_STATUS_WIFI_CONNECTING && t->internal.status != TRACKER_STATUS_MANUAL)
        {
            t->internal.status_changed(t, TRACKER_STATUS_WIFI_CONNECTING);
        }
        // if (ui->internal.screen.internal.main_mode != SCREEN_MODE_WAIT_CONNECT)
        // {
        //     ui->internal.screen.internal.main_mode = SCREEN_MODE_WAIT_CONNECT;
        // }
        break;
    case WIFI_STATUS_CONNECTING:
        if (t->internal.status != TRACKER_STATUS_WIFI_CONNECTING && t->internal.status != TRACKER_STATUS_MANUAL)
        {
            t->internal.status_changed(t, TRACKER_STATUS_WIFI_CONNECTING);
        }
        // if (ui->internal.screen.internal.main_mode != SCREEN_MODE_WAIT_CONNECT)
        // {
        //     ui->internal.screen.internal.main_mode = SCREEN_MODE_WAIT_CONNECT;
        // }
        break;
    case WIFI_STATUS_CONNECTED:
        if (t->internal.status <= TRACKER_STATUS_WIFI_CONNECTING && t->internal.status != TRACKER_STATUS_MANUAL)
        {
            t->internal.status_changed(t, TRACKER_STATUS_WIFI_CONNECTED);
        }
        break;
    case WIFI_STATUS_UDP_CONNECTED:
    case WIFI_STATUS_NONE:
    //case WIFI_STATUS_NO_USE:
        if (t->internal.status != TRACKER_STATUS_MANUAL && t->internal.status != TRACKER_STATUS_TRACKING)
        {
            t->internal.status_changed(t, TRACKER_STATUS_TRACKING);
        }
        if (led_mode_is_enable(LED_MODE_WAIT_SERVER))
            led_mode_remove(LED_MODE_WAIT_SERVER);
        if (led_mode_is_enable(LED_MODE_WAIT_CONNECT))
            led_mode_remove(LED_MODE_WAIT_CONNECT);
        break;
    }
}
#endif

static void ui_beep(ui_t *ui)
{
#if defined(USE_BEEPER)
    beeper_beep(&ui->internal.beeper);
#else
    UNUSED(ui);
#endif
}

#ifdef USE_SCREEN

// Returns true iff the event was consumed
static bool ui_handle_screen_wake(ui_t *ui)
{
    if (ui_screen_is_available(ui) && ui->internal.screen_is_off)
    {
        screen_power_on(&ui->internal.screen);
        screen_set_brightness(&ui->internal.screen, settings_get_key_u8(SETTING_KEY_SCREEN_BRIGHTNESS));
        ui->internal.screen_is_off = false;
        return true;
    }
    return false;
}

static void ui_reset_screen_autooff(ui_t *ui)
{
    if (ui->internal.screen_autooff_interval > 0)
    {
        ui->internal.screen_off_at = time_ticks_now() + ui->internal.screen_autooff_interval;
    }
}

static void ui_handle_screen_button_event(const button_event_t *ev, void *user_data)
{
    ui_t *ui = user_data;
    ui_reset_screen_autooff(ui);

    if (ui_handle_screen_wake(ui))
    {
        return;
    }

    bool handled = screen_handle_button_event(&ui->internal.screen, true, ev);

    if (!handled)
    {
        handled |= menu_handle_button_event(ev);
    }

    if (handled)
    {
        ui_beep(ui);
    }
}
#endif

static void ui_handle_noscreen_button_event(const button_event_t *ev, void *user_data)
{
    // No screen configurations only support BUTTON_NAME_ENTER
    ASSERT(button_event_id(ev) == BUTTON_ID_ENTER);
}

static void ui_settings_handler(const setting_t *setting, void *user_data)
{
    ui_t *ui = user_data;

#ifdef USE_SCREEN
#define UPDATE_SCREEN_SETTING(k, f)                                              \
    do                                                                           \
    {                                                                            \
        if (SETTING_IS(setting, k) && screen_is_available(&ui->internal.screen)) \
        {                                                                        \
            f(&ui->internal.screen, setting_get_u8(setting));                    \
        }                                                                        \
    } while (0)

    UPDATE_SCREEN_SETTING(SETTING_KEY_SCREEN_BRIGHTNESS, screen_set_brightness);

    if (SETTING_IS(setting, SETTING_KEY_SCREEN_AUTO_OFF) && screen_is_available(&ui->internal.screen))
    {
        ui_set_screen_set_autooff(ui, setting_get_u8(setting));
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_DIAGNOSTICS_DEBUG_INFO))
    {
        screen_enter_secondary_mode(&ui->internal.screen, SCREEN_SECONDARY_MODE_DEBUG_INFO);
        return;
    }
#endif

#ifdef USE_IMU
    if (SETTING_IS(setting, SETTING_KEY_IMU_CALIBRATION_ACC) && settings_get_key_bool(SETTING_KEY_IMU_ENABLE))
    {
        screen_enter_secondary_mode(&ui->internal.screen, SCREEN_SECONDARY_MODE_CALIBRATION_ACC);
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_IMU_CALIBRATION_GYRO) && settings_get_key_bool(SETTING_KEY_IMU_ENABLE))
    {
        screen_enter_secondary_mode(&ui->internal.screen, SCREEN_SECONDARY_MODE_CALIBRATION_GYRO);
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_IMU_CALIBRATION_MAG) && settings_get_key_bool(SETTING_KEY_IMU_ENABLE))
    {
        screen_enter_secondary_mode(&ui->internal.screen, SCREEN_SECONDARY_MODE_CALIBRATION_MAG);
        return;
    }
#endif

#ifdef USE_BATTERY_MONITORING
    battery_t *b = ui->internal.screen.internal.battery;

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_MONITOR_BATTERY_VOLTAGE_SCALE))
    {
        b->voltage_scale = setting_get_u16(setting) / 100.00f;
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_MONITOR_BATTERY_MAX_VOLTAGE))
    {
        b->max_voltage = setting_get_u16(setting) / 100.00f;
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_MONITOR_BATTERY_MIN_VOLTAGE))
    {
        b->min_voltage = setting_get_u16(setting) / 100.00f;
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_MONITOR_BATTERY_CENTER_VOLTAGE))
    {
        b->center_voltage = setting_get_u16(setting) / 100.00f;
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_MONITOR_BATTERY_ENABLE))
    {
        b->enable = setting_get_bool(setting) ? 1 : 0;
        if (!b->enable) b->voltage = 0;
        return;
    }
#endif

#ifdef USE_POWER_MONITORING
    power_t *p = ui->internal.screen.internal.power;

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_MONITOR_POWER_ENABLE))
    {
        p->enable = setting_get_bool(setting) ? 1 : 0;
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_MONITOR_POWER_TURN))
    {
        if (setting_get_bool(setting))
        {
            power_turn_on(p);
            LOG_I(TAG, "FPDK power was turn ON.");
        }
        else
        {
            power_turn_off(p);
            LOG_I(TAG, "FPDK power was turn OFF.");
        }
        
        return;
    }
#endif

#if defined(USE_BEEPER)
    if (SETTING_IS(setting, SETTING_KEY_BEEPER_ENABLE))
    {
        beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_NONE);
        ui->internal.beeper.enable = setting_get_bool(setting);
        if (ui->internal.beeper.enable && ui->internal.tracker->internal.status == TRACKER_STATUS_WIFI_CONNECTING)
        {
            beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_WAIT_CONNECT);
        }
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_SERVO_COURSE))
    {
        if (ui->internal.tracker->servo->internal.course > 0)
        {
            led_mode_add(LED_MODE_SETED);
            beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_SETED); 
        }
    }
#endif

#if defined(USE_WIFI)
    if (SETTING_IS(setting, SETTING_KEY_WIFI_ENABLE))
    {
        ui->internal.wifi->enable = setting_get_bool(setting);
        if (ui->internal.wifi->enable)
        {
            ui->internal.wifi->status_change(ui->internal.wifi, WIFI_STATUS_CONNECTING);
            wifi_start(ui->internal.wifi);
        }
        else
        {
            wifi_stop(ui->internal.wifi);
        }
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_WIFI_SMART_CONFIG))
    {
        if (ui->internal.wifi->enable)
        {
            menu_t *menu = menu_get_active();

            // There's nothing overriding the screen, draw the normal interface
            while (menu != NULL)
            {
                menu_pop_active();
                menu = menu_get_active();
            }

            ui->internal.wifi->status_change(ui->internal.wifi, WIFI_STATUS_SMARTCONFIG);
        }

        return;
    }
#endif
    if (SETTING_IS(setting, SETTING_KEY_TRACKER_SHOW_COORDINATE))
    {
        ui->internal.tracker->internal.show_coordinate = setting_get_bool(setting);
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_REAL_ALT))
    {
        ui->internal.tracker->internal.real_alt = setting_get_bool(setting);
        LOG_I(TAG, "REAL_ALT set to :%d", setting_get_bool(setting));
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_ESTIMATE_ENABLE))
    {
        ui->internal.tracker->internal.estimate_location = setting_get_bool(setting);
        LOG_I(TAG, "ESTIMATE_ENABLE set to :%d", setting_get_bool(setting));
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_ESTIMATE_SECOND))
    {
        uint8_t v = settings_get_key_u8(SETTING_KEY_TRACKER_ESTIMATE_SECOND);
        if (v == 0) ui->internal.tracker->internal.eastimate_time = TRACKER_ESTIMATE_1_SEC;
        if (v == 1) ui->internal.tracker->internal.eastimate_time = TRACKER_ESTIMATE_3_SEC;
        if (v == 2) ui->internal.tracker->internal.eastimate_time = TRACKER_ESTIMATE_5_SEC;
        if (v == 3) ui->internal.tracker->internal.eastimate_time = TRACKER_ESTIMATE_10_SEC;
        LOG_I(TAG, "ESTIMATE_SECOND set to :%d", v);
        return;
    }

        if (SETTING_IS(setting, SETTING_KEY_TRACKER_ADVANCED_POS_ENABLE))
    {
        ui->internal.tracker->internal.advanced_position = setting_get_bool(setting);
        LOG_I(TAG, "ADVANCED_POS_ENABLE set to :%d", setting_get_bool(setting));
        return;
    }

    if (SETTING_IS(setting, SETTING_KEY_TRACKER_ADVANCED_POS_SECOND))
    {
        ui->internal.tracker->internal.advanced_time = settings_get_key_u16(SETTING_KEY_TRACKER_ADVANCED_POS_SECOND);
        return;
    }
}

#if defined(USE_BEEPER)
static void ui_update_beeper(ui_t *ui)
{
    beeper_update(&ui->internal.beeper);
}
#endif

static void ui_manual_button_still_down_handle(tracker_t *t, button_t *btn)
{
    switch (btn->id)
    {
    case BUTTON_ID_ENTER:
        break;
    case BUTTON_ID_LEFT:
        tracker_pan_move(t, -1);
        break;
    case BUTTON_ID_RIGHT:
        tracker_pan_move(t, 1);
        break;
    case BUTTON_ID_UP:
        tracker_tilt_move(t, 1);
        break;
    case BUTTON_ID_DOWN:
        tracker_tilt_move(t, -1);
        break;
    }
}

//button still handle
static void ui_handle_button_still_down(ui_t *ui, button_t *btn)
{
    if (btn->id == BUTTON_ID_ENTER)
        return;
    if (!btn->state.is_down)
        return;

    menu_t *menu = menu_get_active();

    if (menu != NULL)
    {
        if (btn->state.long_press_sent)
            menu_handle_button_still_down(menu, btn);
    }
    else if (ui->internal.screen.internal.main_mode == SCREEN_MODE_MAIN)
    {
        tracker_t *t = ui->internal.tracker;
        if (t->internal.status == TRACKER_STATUS_MANUAL && btn->state.long_press_sent)
        {
            ui_manual_button_still_down_handle(t, btn);
        }
    }
}

static void ui_status_check(ui_t *ui)
{
    time_millis_t now = time_millis_now();

    if (ui->internal.tracker->internal.flag & TRACKER_FLAG_SERVER_CONNECTED)
    {
        if (ui->internal.tracker->last_ack + 7000 < now)
        {
#if defined(USE_WIFI)
            ui->internal.tracker->internal.flag_changed(ui->internal.tracker, TRACKER_FLAG_SERVER_CONNECTED, 0);
            ATP_SET_U8(TAG_TRACKER_FLAG, ui->internal.tracker->internal.flag, now);
            if (ui->internal.wifi->enable)
            {
                ui->internal.wifi->status_change(ui->internal.wifi, WIFI_STATUS_CONNECTED);
            }

            LOG_I(TAG, "Server was losted...");
            led_mode_add(LED_MODE_WAIT_SERVER);
#endif
        }
    }

    if (ui->internal.tracker->internal.flag & TRACKER_FLAG_WIFI_CONNECTED)
    {
        if (ui->internal.wifi->status < WIFI_STATUS_CONNECTED)
        {
            ui->internal.tracker->internal.flag_changed(ui->internal.tracker, TRACKER_FLAG_WIFI_CONNECTED, 0);
            ATP_SET_U8(TAG_TRACKER_FLAG, ui->internal.tracker->internal.flag, now);
        }
    }
}

#if defined(USE_WIFI)
// void ui_init(ui_t *ui, ui_config_t *cfg, servo_t *servo)
void ui_init(ui_t *ui, ui_config_t *cfg, tracker_t *tracker, wifi_t *wifi)
#else
void ui_init(ui_t *ui, ui_config_t *cfg, tracker_t *tracker_s)
#endif
{
    ui->internal.tracker = tracker;

    ui_status_observer.Obj = ui;
    ui_status_observer.Name = "UI status observer";
    ui_status_observer.Update = ui_status_updated;
    tracker->internal.status_changed_notifier->mSubject.Attach(tracker->internal.status_changed_notifier, &ui_status_observer);

    ui_flag_observer.Obj = ui;
    ui_flag_observer.Name = "UI flag observer";
    ui_flag_observer.Update = ui_flag_updated;
    tracker->internal.flag_changed_notifier->mSubject.Attach(tracker->internal.flag_changed_notifier, &ui_flag_observer);

    ui_reverse_observer.Obj = ui;
    ui_reverse_observer.Name = "UI reverseing observer";
    ui_reverse_observer.Update = ui_reverse_updated;
    tracker->servo->internal.reverse_notifier->mSubject.Attach(tracker->servo->internal.reverse_notifier, &ui_reverse_observer);

    if (settings_get_key_bool(SETTING_KEY_IMU_ENABLE))
    {
        ui_imu_calibration_step_done_observer.Obj = ui;
        ui_imu_calibration_step_done_observer.Name = "UI cal_step observer";
        ui_imu_calibration_step_done_observer.Update = ui_imu_calibration_step_done;
        tracker->imu->cal_step_notifier->mSubject.Attach(tracker->imu->cal_step_notifier, &ui_imu_calibration_step_done_observer);

        ui_imu_calibration_done_observer.Obj = ui;
        ui_imu_calibration_done_observer.Name = "UI cal_done observer";
        ui_imu_calibration_done_observer.Update = ui_imu_calibration_done;
        tracker->imu->cal_done_notifier->mSubject.Attach(tracker->imu->cal_done_notifier, &ui_imu_calibration_done_observer);
    }

    // #if defined(LED_1_USE_WS2812)
    //     led_init(&ui->internal.led_gradual_target);
    // #else
    led_init();
    // #endif
    button_callback_f button_callback = ui_handle_noscreen_button_event;
#ifdef USE_SCREEN
    // if (screen_init(&ui->internal.screen, &cfg->screen, servo))
    if (screen_init(&ui->internal.screen, &cfg->screen, tracker))
    {
        button_callback = ui_handle_screen_button_event;
#if defined(USE_WIFI)
        ui_wifi_status_observer.Obj = ui;
        ui_wifi_status_observer.Name = "UI wifi status observer";
        ui_wifi_status_observer.Update = ui_wifi_status_update;
        wifi->status_change_notifier->mSubject.Attach(wifi->status_change_notifier, &ui_wifi_status_observer);
        ui->internal.wifi = wifi;
        ui->internal.screen.internal.wifi = wifi;
#endif
        ui->internal.screen.internal.main_mode = SCREEN_MODE_WAIT_CONNECT;
        ui->internal.screen.internal.secondary_mode = SCREEN_SECONDARY_MODE_NONE;

        LOG_I(TAG, "Screen detected");
        system_add_flag(SYSTEM_FLAG_SCREEN);
        screen_set_brightness(&ui->internal.screen, settings_get_key_u8(SETTING_KEY_SCREEN_BRIGHTNESS));
        ui_set_screen_set_autooff(ui, settings_get_key_u8(SETTING_KEY_SCREEN_AUTO_OFF));
    }
    else
    {
        LOG_I(TAG, "No screen detected");
    }
    menu_init(tracker);
#endif
    for (unsigned ii = 0; ii < ARRAY_COUNT(ui->internal.buttons); ii++)
    {
        ui->internal.buttons[ii].callback = button_callback;
        ui->internal.buttons[ii].user_data = ui;
        ui->internal.buttons[ii].id = ii;
        ui->internal.buttons[ii].cfg = cfg->buttons[ii];
        button_init(&ui->internal.buttons[ii]);
    }

#if defined(USE_BEEPER)
    beeper_init(&ui->internal.beeper, cfg->beeper);
    beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_STARTUP);
#endif

    system_add_flag(SYSTEM_FLAG_BUTTON);
    settings_add_listener(ui_settings_handler, ui);

#ifdef USE_BATTERY_MONITORING
    battery_init(&battery);
    #ifdef USE_SCREEN
        ui->internal.screen.internal.battery = &battery;
    #endif
#endif

#ifdef USE_POWER_MONITORING
    power_init(&power);
    #ifdef USE_SCREEN
        ui->internal.screen.internal.power = &power;
    #endif
#endif
}

bool ui_screen_is_available(const ui_t *ui)
{
#ifdef USE_SCREEN
    return screen_is_available(&ui->internal.screen);
#else
    return false;
#endif
}

bool ui_screen_init(ui_t *ui, ui_config_t *cfg, tracker_t *tracker)
{
#ifdef USE_SCREEN
    return screen_init(&ui->internal.screen, &cfg->screen, tracker);
#else
    return false;
#endif
}

void ui_screen_splash(ui_t *ui)
{
#ifdef USE_SCREEN
    screen_splash(&ui->internal.screen);
#endif
}

bool ui_is_animating(const ui_t *ui)
{
#ifdef USE_SCREENd
    return screen_is_animating(&ui->internal.screen);
#else
    return false;
#endif
}

void ui_update(ui_t *ui)
{
#if defined(USE_BEEPER)
    ui_update_beeper(ui);
#endif
    for (unsigned ii = 0; ii < ARRAY_COUNT(ui->internal.buttons); ii++)
    {
        button_update(&ui->internal.buttons[ii]);
        ui_handle_button_still_down(ui, &ui->internal.buttons[ii]);
    }

    led_update();

#ifdef USE_SCREEN
    if (ui_screen_is_available(ui))
    {
        if (ui->internal.screen_off_at > 0 && ui->internal.screen_off_at < time_ticks_now())
        {
            ui->internal.screen_off_at = 0;
            ui->internal.screen_is_off = true;
            screen_shutdown(&ui->internal.screen);
        }

        menu_update();

        if (!ui->internal.screen_is_off)
        {
            screen_update(&ui->internal.screen);
        }
    }
#endif

    ui_status_check(ui);
}

void ui_yield(ui_t *ui)
{
    if (!ui_is_animating(ui))
    {
        time_ticks_t sleep = led_is_fading() ? 1 : MILLIS_TO_TICKS(10);
        vTaskDelay(sleep);
    }
}

void ui_shutdown(ui_t *ui)
{
#ifdef USE_SCREEN
    if (ui_screen_is_available(ui))
    {
        screen_shutdown(&ui->internal.screen);
    }
#endif
}

void ui_set_screen_set_autooff(ui_t *ui, ui_screen_autooff_e autooff)
{
#ifdef USE_SCREEN
    ui->internal.screen_autooff_interval = 0;
    switch (autooff)
    {
    case UI_SCREEN_AUTOOFF_DISABLED:
        break;
    case UI_SCREEN_AUTOOFF_30_SEC:
        ui->internal.screen_autooff_interval = SECS_TO_TICKS(30);
        break;
    case UI_SCREEN_AUTOOFF_1_MIN:
        ui->internal.screen_autooff_interval = SECS_TO_TICKS(60);
        break;
    case UI_SCREEN_AUTOOFF_5_MIN:
        ui->internal.screen_autooff_interval = SECS_TO_TICKS(60 * 5);
        break;
    case UI_SCREEN_AUTOOFF_10_MIN:
        ui->internal.screen_autooff_interval = SECS_TO_TICKS(60 * 10);
        break;
    }
    ui_reset_screen_autooff(ui);
#endif
}
