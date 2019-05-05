
#include <hal/log.h>

#include "../src/target/target.h"

#include "platform/system.h"

#include "ui/led.h"
#include "ui/screen.h"
#include "ui/ui.h"
#include "tracker/tracker.h"

#include "util/macros.h"

#ifdef USE_SCREEN
// We don't log anything without a screen, so this would
// produce a warning if we didn't guard it
static const char *TAG = "UI";
#endif

#ifdef USE_BATTERY_MEASUREMENT
static battery_t battery;
#endif

static Observer ui_status_observer;
static Observer ui_flag_observer;


static void ui_status_updated(void *notifier, void *s)
{
    Observer *obs = (Observer *)notifier;
    tracker_status_e *status = (tracker_status_e *)s;
    ui_t *ui = (ui_t *)obs->Obj;

    switch (*status)
    {
    case TRACKER_STATUS_BOOTING:
        break;
    case TRACKER_STATUS_CONNECTING:
#if defined(USE_BEEPER)
        if (ui->internal.beeper.mode != BEEPER_MODE_WAIT_CONNECT)
        {
            beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_WAIT_CONNECT);
        }
#endif
        if (!led_mode_is_enable(LED_MODE_WAIT_CONNECT))
		{
			led_mode_add(LED_MODE_WAIT_CONNECT);
		}
#if defined(USE_SCREEN)
        ui->internal.screen.internal.main_mode = SCREEN_MODE_WAIT_CONNECT;
#endif
        break;
    case TRACKER_STATUS_CONNECTED:
#if defined(USE_BEEPER)
        beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_NONE);
#endif
#if defined(USE_SCREEN)
        ui->internal.screen.internal.main_mode = SCREEN_MODE_MAIN;
#endif
        if (led_mode_is_enable(LED_MODE_WAIT_CONNECT))
		{
			led_mode_remove(LED_MODE_WAIT_CONNECT);
		}
        break;
    }
}

static void ui_flag_updated(void *notifier, void *f)
{
    Observer *obs = (Observer *)notifier;
    uint8_t *flag = (uint8_t *)f;
    ui_t *ui = (ui_t *)obs->Obj;

#if defined(USE_BEEPER)
    if (*flag & (TRACKER_FLAG_HOMESETED | TRACKER_FLAG_PLANESETED)) beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_SETED);
#endif
}

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
        // screen_set_brightness(&ui->internal.screen, settings_get_key_u8(SETTING_KEY_SCREEN_BRIGHTNESS));
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
    // if (!handled)
    // {
    //     handled |= menu_handle_button_event(ev);
    // }
    if (!handled)
    {
        handled |= screen_handle_button_event(&ui->internal.screen, false, ev);
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

    // ui_t *ui = user_data;
    // switch (ev->type)
    // {
    //     case BUTTON_EVENT_TYPE_SHORT_PRESS:
    //         if (rc_has_pending_bind_request(ui->internal.rc, NULL))
    //         {
    //             rc_accept_bind(ui->internal.rc);
    //             ui_beep(ui);
    //         }
    //         break;
    //     case BUTTON_EVENT_TYPE_LONG_PRESS:
    //         break;
    //     case BUTTON_EVENT_TYPE_REALLY_LONG_PRESS:
    //     {
    //         const setting_t *bind_setting = settings_get_key(SETTING_KEY_BIND);
    //         bool is_binding = setting_get_bool(bind_setting);
    //         if (time_micros_now() < SECS_TO_MICROS(15) && !is_binding)
    //         {
    //             setting_set_bool(bind_setting, true);
    //             ui_beep(ui);
    //         }
    //         else if (is_binding)
    //         {
    //             setting_set_bool(bind_setting, false);
    //             ui_beep(ui);
    //         }
    //         break;
    //     }
    // }
}

#if defined(USE_BEEPER)
static void ui_update_beeper(ui_t *ui)
{
    // if (rc_is_failsafe_active(ui->internal.rc, NULL))
    // {
    //     beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_FAILSAFE);
    // }
    // else if (rc_is_binding(ui->internal.rc))
    // {
    //     beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_BIND);
    // }
    // else 
    // if (ui->internal.beeper.mode != BEEPER_MODE_STARTUP)
    // {
    //     beeper_set_mode(&ui->internal.beeper, BEEPER_MODE_NONE);
    // }
    beeper_update(&ui->internal.beeper);
}
#endif

// void ui_init(ui_t *ui, ui_config_t *cfg, servo_t *servo)
void ui_init(ui_t *ui, ui_config_t *cfg, tracker_t *tracker)
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

#if defined(LED_1_USE_WS2812)
    led_init(&ui->internal.led_gradual_target);
#else
    led_init();
#endif

    button_callback_f button_callback = ui_handle_noscreen_button_event;
#ifdef USE_SCREEN
    // if (screen_init(&ui->internal.screen, &cfg->screen, servo))
    if (screen_init(&ui->internal.screen, &cfg->screen, tracker))
    {
        button_callback = ui_handle_screen_button_event;
    }
#endif

    // for (unsigned ii = 0; ii < ARRAY_COUNT(ui->internal.buttons); ii++)
    // {
    //     ui->internal.buttons[ii].user_data = ui;
    //     ui->internal.buttons[ii].callback = button_callback;
    // }

    // ui->internal.rc = rc;
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
#ifdef USE_SCREEN
    if (screen_is_available(&ui->internal.screen))
    {
        LOG_I(TAG, "Screen detected");
        system_add_flag(SYSTEM_FLAG_SCREEN);
// #if defined(SCREEN_FIXED_ORIENTATION)
//         screen_orientation_e screen_orientation = SCREEN_ORIENTATION_DEFAULT;
// #else
//         screen_orientation_e screen_orientation = settings_get_key_u8(SETTING_KEY_SCREEN_ORIENTATION);
// #endif
//         screen_set_orientation(&ui->internal.screen, screen_orientation);
//         screen_set_brightness(&ui->internal.screen, settings_get_key_u8(SETTING_KEY_SCREEN_BRIGHTNESS));
//         ui_set_screen_set_autooff(ui, settings_get_key_u8(SETTING_KEY_SCREEN_AUTO_OFF));
    }
    else
    {
        LOG_I(TAG, "No screen detected");
    }
    // menu_init(rc);
#endif
    // settings_add_listener(ui_settings_handler, ui);

    
#ifdef USE_BATTERY_MEASUREMENT
    battery_init(&battery);
    ui->internal.screen.internal.battery = &battery;
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

void ui_screen_splash(ui_t *ui)
{
#ifdef USE_SCREEN
    screen_splash(&ui->internal.screen);
#endif
}

bool ui_is_animating(const ui_t *ui)
{
#ifdef USE_SCREEN
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
    }
    // led_mode_set(LED_MODE_FAILSAFE, rc_is_failsafe_active(ui->internal.rc, NULL));
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
        // menu_update();
        if (!ui->internal.screen_is_off)
        {
            screen_update(&ui->internal.screen);
        }
    }
#endif
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
