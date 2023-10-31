#include <stdlib.h>

#include <hal/log.h>
#include "util/macros.h"
#include "../main/target/target.h"
#include "config/settings.h"
#include "util/time.h"
#include "menu.h"

#ifdef USE_SCREEN

static const char *TAG = "menu";

#define MAX_MENU_DEPTH 10
#define MAX_DYN_MENU_DEPTH 10
#define MAX_DYN_MENU_ENTRIES 20

#define SETTINGS_REQUEST_BROADCAST_INTERVAL MILLIS_TO_TICKS(500)
#define SETTINGS_DEVICE_EXPIRATION MILLIS_TO_TICKS(2000)
#define SETTINGS_REMOTE_REFRESH_INTERVAL MILLIS_TO_TICKS(500)

#define MENU_TITLE_CMD_SUFFIX "\xAC"

#if defined(USE_BUTTON_5WAY)
#define BUTTON_EVENT_IS_ENTER_OR_RIGHT(ev) (!ev || button_event_id(ev) == BUTTON_ID_ENTER || button_event_id(ev) == BUTTON_ID_RIGHT)
#define BUTTON_EVENT_INCREMENTS(ev) (ev && button_event_id(ev) == BUTTON_ID_RIGHT && ev->type == BUTTON_EVENT_TYPE_SHORT_PRESS)
#define BUTTON_EVENT_DECREMENTS(ev) (ev && button_event_id(ev) == BUTTON_ID_LEFT && ev->type == BUTTON_EVENT_TYPE_SHORT_PRESS)
#else
#define BUTTON_EVENT_IS_ENTER_OR_RIGHT(ev) true
#define BUTTON_EVENT_INCREMENTS(ev) (ev && button_event_id(ev) == BUTTON_ID_ENTER && ev->type == BUTTON_EVENT_TYPE_LONG_PRESS)
#define BUTTON_EVENT_DECREMENTS(ev) false
#endif

typedef bool (*menu_action_f)(void *data, const button_event_t *ev);

typedef struct menu_entry_s
{
    const char *(*title)(void *data, char *buf, uint16_t bufsize);
    menu_action_f action;
    void *data;
} menu_entry_t;

typedef enum
{
    MENU_DYN_NONE,
    MENU_DYN_LOCAL_FOLDER,
} meny_dyn_e;

typedef struct menu_s
{
    const char *prompt;
    meny_dyn_e dyn;
    int data1;
    int data2;
    uint8_t index;
    menu_entry_t *entries;
} menu_t;

static menu_t *menu_stack[MAX_MENU_DEPTH];
static int menu_stack_depth = 0;

static menu_t dyn_menus[MAX_DYN_MENU_DEPTH];
static int dyn_menu_depth = 0;
static menu_entry_t dyn_entries[MAX_DYN_MENU_ENTRIES];

typedef struct settings_device_s
{
    uint16_t settings_count;
    char name[10];
    time_ticks_t expires_at;
} settings_device_t;

static tracker_t *t;

static const char *menu_back_title(void *data, char *buf, uint16_t bufsize);
static bool menu_back_action(void *data, const button_event_t *ev);
static const char *menu_local_setting_title(void *data, char *buf, uint16_t bufsize);
static bool menu_local_setting_action(void *data, const button_event_t *ev);
// static const char *menu_device_title(void *data, char *buf, uint16_t bufsize);
// static bool menu_device_action(void *data, const button_event_t *ev);
// static const char *menu_remote_setting_title(void *data, char *buf, uint16_t bufsize);
// static bool menu_remote_setting_action(void *data, const button_event_t *ev);
static int menu_setting_build_entries(menu_t *menu, folder_id_e folder);

#define MENU_BACK_ENTRY ((menu_entry_t){.title = menu_back_title, .action = menu_back_action})
#define MENU_ENTRY_IS_BACK(e) ((e)->title == menu_back_title && (e)->action == menu_back_action)
#define MENU_END ((menu_entry_t){NULL, NULL, NULL})

static const char *menu_back_title(void *data, char *buf, uint16_t bufsize)
{
    if (menu_stack_depth > 1)
    {
        return "Back";
    }
    return "Exit";
}

static bool menu_back_action(void *data, const button_event_t *ev)
{
#if defined(USE_BUTTON_5WAY)
    if (ev && button_event_id(ev) == BUTTON_ID_RIGHT)
    {
        return false;
    }
#endif
    if (menu_stack_depth > 0)
    {
        menu_t *old_menu = menu_get_active();
        menu_stack_depth--;
        LOG_D(TAG, "Popping menu with dyn = %d, menu stack depth at %d, active = %p", old_menu->dyn, menu_stack_depth, menu_get_active());
        if (dyn_menu_depth > 0)
        {
            if (old_menu->dyn != MENU_DYN_NONE)
            {
                dyn_menu_depth--;
            }
            LOG_D(TAG, "Dynamic menu depth at %d", dyn_menu_depth);
            // Rebuild the entries for the menu we're entering into
            menu_t *menu = menu_get_active();
            if (menu)
            {
                switch (menu->dyn)
                {
                case MENU_DYN_NONE:
                    break;
                case MENU_DYN_LOCAL_FOLDER:
                    // Check if the folder is still visible. It might now be hidden when e.g.
                    // we delete a paired RX.
                    if (!settings_is_folder_visible(SETTINGS_VIEW_MENU, menu->data1))
                    {
                        menu_back_action(NULL, ev);
                        break;
                    }
                    menu_setting_build_entries(menu, menu->data1);
                    break;
                }
            }
        }
    }
    return true;
}

static const char *menu_string_title(void *data, char *buf, uint16_t bufsize)
{
    return data;
}

#define MENU_STRING_ENTRY(str, callback) ((menu_entry_t){.title = menu_string_title, .action = callback, .data = str})

static void menu_enter(menu_t *menu)
{
    menu->index = 0;
    menu_stack[menu_stack_depth++] = menu;
    LOG_D(TAG, "Entered menu with stack depth %d, dynamic menu depth %d, active = %p", menu_stack_depth, dyn_menu_depth, menu_get_active());
}

static int menu_setting_build_entries(menu_t *menu, folder_id_e folder)
{
    settings_view_t view;
    settings_view_get(&view, SETTINGS_VIEW_MENU, folder);
    for (int ii = 0; ii < view.count; ii++)
    {
        menu_entry_t *entry = &dyn_entries[ii];
        entry->title = menu_local_setting_title;
        entry->action = menu_local_setting_action;
        entry->data = (void *)settings_view_get_setting_at(&view, ii);
    }
    dyn_entries[view.count] = MENU_BACK_ENTRY;
    dyn_entries[view.count + 1] = MENU_END;
    menu->index = MIN(menu->index, view.count);
    return view.count;
}

static void menu_setting_rebuild_current_entries(void)
{
    menu_t *menu = menu_get_active();
    if (menu)
    {
        folder_id_e folder = (folder_id_e)menu->data1;
        menu_setting_build_entries(menu, folder);
    }
}

static void menu_settings_enter_folder(folder_id_e folder)
{
    menu_t *menu = &dyn_menus[dyn_menu_depth++];
    menu->dyn = MENU_DYN_LOCAL_FOLDER;
    menu->data1 = folder;
    for (int ii = 0; ii < settings_get_count(); ii++)
    {
        const setting_t *setting = settings_get_setting_at(ii);
        if (setting_get_folder_id(setting) == folder)
        {
            menu->prompt = setting->name;
            break;
        }
    }
    menu_setting_build_entries(menu, folder);
    menu->entries = dyn_entries;
    menu_enter(menu);
}

static const char *menu_confirm_ok_title(void *data, char *buf, uint16_t bufsize)
{
    return "Confirm " MENU_TITLE_CMD_SUFFIX;
}

static bool menu_confirm_ok_action(void *data, const button_event_t *ev)
{
    if (!BUTTON_EVENT_IS_ENTER_OR_RIGHT(ev))
    {
        return false;
    }
    const setting_t *setting = data;
    setting_cmd_exec(setting);
    menu_back_action(data, ev);
    return true;
}

static menu_entry_t menu_confirm_entries[] = {
    {.title = menu_confirm_ok_title, .action = menu_confirm_ok_action, .data = NULL},
    MENU_BACK_ENTRY,
    MENU_END,
};

static menu_t menu_confirm = {
    .dyn = MENU_DYN_NONE,
    .entries = menu_confirm_entries,
};

static menu_entry_t menu_empty_entries[] = {
    MENU_STRING_ENTRY(NULL, NULL),
    MENU_END,
};

menu_t menu_empty = {
    .entries = menu_empty_entries,
};

// static void menu_device_enter_folder(int device_index, folder_id_e folder)
// {
//     menu_t *menu = &dyn_menus[dyn_menu_depth++];
//     menu->dyn = MENU_DYN_REMOTE_FOLDER;
//     menu->data1 = folder;
//     menu->data2 = device_index;
//     // TODO: Folder name if non-root
//     // menu->prompt = remotes.devices[device_index].name;
//     memset(&dyn_entries, 0, sizeof(dyn_entries));
//     dyn_entries[0] = MENU_BACK_ENTRY;
//     dyn_entries[1] = MENU_END;
//     menu->entries = dyn_entries;
//     menu_enter(menu);
// }

static const char *menu_local_setting_title(void *data, char *buf, uint16_t bufsize)
{
    const setting_t *setting = data;
    setting_format(buf, bufsize, setting);
    return buf;
}

static bool menu_local_do_setting_action(void *data, const button_event_t *ev)
{
    const setting_t *setting = data;
    if (setting->type == SETTING_TYPE_FOLDER)
    {
        if (BUTTON_EVENT_IS_ENTER_OR_RIGHT(ev))
        {
            menu_settings_enter_folder(setting_get_folder_id(setting));
            return true;
        }
        return false;
    }

    // TODO: Allow editing strings
    if (setting->flags & SETTING_FLAG_CMD)
    {
        if (!BUTTON_EVENT_IS_ENTER_OR_RIGHT(ev))
        {
            return false;
        }

        setting_cmd_flag_e cmd_flags = setting_cmd_get_flags(setting);
        if (cmd_flags & SETTING_CMD_FLAG_CONFIRM)
        {
            menu_confirm.prompt = setting->name;
            menu_confirm.entries[0].data = (void *)setting;
            menu_enter(&menu_confirm);
        }
        else
        {
            // Command without confirmation
            setting_cmd_exec(setting);
        }
        return true;
    }
    if (BUTTON_EVENT_INCREMENTS(ev))
    {
        setting_increment(setting);
        return true;
    }
    if (BUTTON_EVENT_DECREMENTS(ev))
    {
        setting_decrement(setting);
        return true;
    }
    return false;
}

static bool menu_local_setting_action(void *data, const button_event_t *ev)
{
    bool ret = menu_local_do_setting_action(data, ev);
    menu_setting_rebuild_current_entries();
    return ret;
}

// static const char *menu_device_title(void *data, char *buf, uint16_t bufsize)
// {
//     settings_device_t *dev = data;
//     return dev->name;
// }

// static bool menu_device_action(void *data, const button_event_t *ev)
// {
//     if (!BUTTON_EVENT_IS_ENTER_OR_RIGHT(ev))
//     {
//         return false;
//     }

//     settings_device_t *dev = data;
//     int device_index = -1;
//     for (int ii = 0; ii < ARRAY_COUNT(remotes.devices); ii++)
//     {
//         if (air_addr_equals(&dev->addr, &remotes.devices[ii].addr))
//         {
//             device_index = ii;
//             break;
//         }
//     }
//     if (device_index >= 0)
//     {
//         menu_device_enter_folder(device_index, FOLDER_ID_ROOT);
//         return true;
//     }
//     return false;
// }

// static const char *menu_pairing_title(void *data, char *buf, uint16_t bufsize)
// {
//     air_pairing_t *pairing = data;
//     if (!config_get_air_name(buf, bufsize, &pairing->addr))
//     {
//         air_pairing_format(pairing, buf, bufsize);
//     }
//     return buf;
// }

// static air_pairing_t alt_pairings[MENU_ALT_PAIRINGS_MAX];

// static menu_entry_t menu_alt_pairings_entries[MENU_ALT_PAIRINGS_MAX + 2] = {

// };

// menu_t menu_alt_pairings = {
//     .entries = menu_alt_pairings_entries,
// };

// static bool menu_pairing_action(void *data, const button_event_t *ev)
// {
//     if (BUTTON_EVENT_IS_ENTER_OR_RIGHT(ev))
//     {
//         air_pairing_t *pairing = data;
//         rc_switch_pairing(rc, pairing);
//         menu_alt_pairings.index = 0;
//         return true;
//     }
//     return false;
// }

// static bool menu_pairing_exit_action(void *data, const button_event_t *ev)
// {
//     menu_alt_pairings.index = 0;
//     rc_dismiss_alternative_pairings(rc);
//     menu_back_action(data, ev);
//     return true;
// }

void menu_init(tracker_t *tracker)
{
    t = tracker;
}

void menu_update(void)
{
    // time_ticks_t now = time_ticks_now();
    // menu_t *active_menu = menu_get_active();

    // if (active_menu && active_menu->dyn == MENU_DYN_REMOTE_FOLDER)
    // {
    //     // Displaying a remote folder. Check if we need to update it the folder or
    //     // any of their items.
    //     if (MENU_ENTRY_IS_BACK(&dyn_entries[0]))
    //     {
    //         // No count yet. Query it.
    //         req.code = SETTINGS_RMP_HELO,
    //         req.helo.view.id = SETTINGS_VIEW_REMOTE;
    //         req.helo.view.folder_id = active_menu->data1;
    //         req.helo.view.recursive = false;
    //         settings_device_t *dev = &remotes.devices[active_menu->data2];
    //         rmp_send(rc->rmp, rmp_port, &dev->addr, RMP_PORT_SETTINGS, &req, settings_rmp_msg_size(&req));
    //     }
    //     else
    //     {
    //         // Check if any setting is missing
    //         for (int ii = 0; ii < ARRAY_COUNT(dyn_entries); ii++)
    //         {
    //             if (MENU_ENTRY_IS_BACK(&dyn_entries[ii]))
    //             {
    //                 // No more entries in this folder
    //                 break;
    //             }
    //             if (dyn_entries[ii].data == NULL)
    //             {
    //                 // Missing entry. Request at most one per menu_update() iteration.
    //                 menu_request_remote_setting(&req, active_menu->data2, active_menu->data1, ii);
    //                 break;
    //             }
    //         }
    //         // If all settings are present, start refreshing them
    //         for (int ii = 0; ii < ARRAY_COUNT(dyn_entries); ii++)
    //         {
    //             if (dyn_remote_settings[ii].next_update < now)
    //             {
    //                 // Out of date, request again. Add 100ms to next_update to prevent
    //                 // re-requestning this one multiple times before the other device
    //                 // is able to reply. Again, just one update per cycle.
    //                 menu_request_remote_setting(&req, active_menu->data2, active_menu->data1, ii);
    //                 dyn_remote_settings[ii].next_update += MILLIS_TO_TICKS(100);
    //                 break;
    //             }
    //         }
    //     }
    // }
}

static bool menu_run_action(menu_t *menu, const button_event_t *ev)
{
    menu_action_f action = menu->entries[menu->index].action;
    void *data = menu->entries[menu->index].data;
    if (!action)
    {
        action = menu_back_action;
    }
    return action(data, ev);
}

static bool menu_handle_short_press(menu_t *active, const button_event_t *ev)
{
    menu_t *menu = menu_get_active();

    // if (menu == NULL && button_event_id(ev) == BUTTON_ID_ENTER)
    // {
    //     menu_settings_enter_folder(FOLDER_ID_ROOT);
    //     return true;
    // }

    if (!menu)
    {
        if (button_event_id(ev) == BUTTON_ID_ENTER)
        {
            menu_settings_enter_folder(FOLDER_ID_ROOT);
            return true;
        }
        else
        {
            return false;
        }
    }

#if defined(USE_BUTTON_5WAY)
    button_id_e bid = button_event_id(ev);
    int direction = bid == BUTTON_ID_DOWN ? 1 : bid == BUTTON_ID_UP ? -1 : 0;
    if (direction == 0)
    {
        return menu_run_action(menu, ev);
    }
#else
    int direction = 1;
#endif
    if (direction < 0 && menu->index == 0)
    {
        menu->index = menu_get_num_entries(menu) - 1;
    }
    else
    {
        menu->index += direction;
        if (!menu->entries[menu->index].title)
        {
            // Already on last entry
            menu->index = 0;
        }
    }
    return true;
}

static bool menu_handle_double_press(menu_t *active, const button_event_t *ev)
{
    if (active == NULL)
        return false;

    const setting_t *setting = (const setting_t *)active->entries[active->index].data;

    if (setting == NULL)
        return false;

    switch (button_event_id(ev))
    {
    case BUTTON_ID_ENTER:
        // if ((SETTING_IS(setting, SETTING_KEY_SERVO_COURSE) || 
        //      SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MAX_PLUSEWIDTH) || SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MIN_PLUSEWIDTH) ||
        //      SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MAX_PLUSEWIDTH) || SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MIN_PLUSEWIDTH) ||
        //      SETTING_IS(setting, SETTING_KEY_BATTERY_VOLTAGE_SCALE) || SETTING_IS(setting, SETTING_KEY_BATTERY_MAX_VOLTAGE) ||
        //      SETTING_IS(setting, SETTING_KEY_BATTERY_MIN_VOLTAGE) || SETTING_IS(setting, SETTING_KEY_BATTERY_CENTER_VOLTAGE) ||
        //      SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MAX_DEGREE) || SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MIN_DEGREE) ||
        //      SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MAX_DEGREE) || SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MIN_DEGREE)) &&
        //     setting_get_value_is_tmp(setting))
        if (setting_is_has_tmp(setting) && setting_get_value_is_tmp(setting))
        {
            setting_set_value_is_tmp(setting, false);
            LOG_I(TAG, "[%s] cancel edit.", setting->name);
            return true;
        }
        break;
    default:
        return menu_handle_short_press(active, ev);
    }

    return false;
}

static bool menu_handle_long_press(menu_t *active, const button_event_t *ev)
{
    // menu_t *menu = menu_get_active();
    // if (menu == NULL && button_event_id(ev) == BUTTON_ID_ENTER)
    // {
    //     menu_settings_enter_folder(FOLDER_ID_ROOT);
    //     return true;
    // }
#if defined(USE_BUTTON_5WAY)
    if (active == NULL)
        return false;

    const setting_t *setting = (const setting_t *)active->entries[active->index].data;

    if (setting == NULL)
        return false;

    switch (button_event_id(ev))
    {
    case BUTTON_ID_ENTER:
        // if ((SETTING_IS(setting, SETTING_KEY_SERVO_COURSE) ||
        //      SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MAX_PLUSEWIDTH) || SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MIN_PLUSEWIDTH) ||
        //      SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MAX_PLUSEWIDTH) || SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MIN_PLUSEWIDTH) ||
        //      SETTING_IS(setting, SETTING_KEY_BATTERY_VOLTAGE_SCALE) || SETTING_IS(setting, SETTING_KEY_BATTERY_MAX_VOLTAGE) ||
        //      SETTING_IS(setting, SETTING_KEY_BATTERY_MIN_VOLTAGE) || SETTING_IS(setting, SETTING_KEY_BATTERY_CENTER_VOLTAGE)) &&
        //     setting_get_value_is_tmp(setting))
        if (setting_is_has_tmp(setting) && setting_get_value_is_tmp(setting))
        {
            setting_set_u16(setting, setting_get_tmp_u16(setting));
            setting_set_value_is_tmp(setting, false);
            LOG_I(TAG, "[%s] set to -> %d", setting->name, setting_get_tmp_u16(setting));
            return true;
        }
        else
        {
            return menu_back_action(NULL, ev);
        }
        break;
    default:
        break;
    }

    return false;
#else
    return menu_run_action(menu, ev);
#endif
}

static bool menu_handle_really_long_press(menu_t *active, const button_event_t *ev)
{
#if defined(USE_BUTTON_5WAY)
    if (button_event_id(ev) == BUTTON_ID_ENTER)
    {
        menu_t *menu = menu_get_active();

        // There's nothing overriding the screen, draw the normal interface
        while (menu != NULL)
        {
            menu_pop_active();
            menu = menu_get_active();
        }

        return true;
    }
#endif
    return false;
}

bool menu_handle_button_event(const button_event_t *ev)
{
    menu_t *active = menu_get_active();

    switch (ev->type)
    {
    case BUTTON_EVENT_TYPE_SHORT_PRESS:
        return menu_handle_short_press(active, ev);
    case BUTTON_EVENT_TYPE_DOUBLE_PRESS:
        return menu_handle_double_press(active, ev);
    case BUTTON_EVENT_TYPE_LONG_PRESS:
        return menu_handle_long_press(active, ev);
    case BUTTON_EVENT_TYPE_REALLY_LONG_PRESS:
        return menu_handle_really_long_press(active, ev);
    }

    return false;
}

void menu_handle_button_still_down(menu_t *active, button_t *btn)
{
    const setting_t *setting = (const setting_t *)active->entries[active->index].data;

    if (setting == NULL)
        return;

    // if (SETTING_IS(setting, SETTING_KEY_SERVO_COURSE) ||
    //     SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MAX_PLUSEWIDTH) || SETTING_IS(setting, SETTING_KEY_SERVO_PAN_MIN_PLUSEWIDTH) ||
    //     SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MAX_PLUSEWIDTH) || SETTING_IS(setting, SETTING_KEY_SERVO_TILT_MIN_PLUSEWIDTH) ||
    //     SETTING_IS(setting, SETTING_KEY_BATTERY_VOLTAGE_SCALE) || SETTING_IS(setting, SETTING_KEY_BATTERY_MAX_VOLTAGE) ||
    //     SETTING_IS(setting, SETTING_KEY_BATTERY_MIN_VOLTAGE) || SETTING_IS(setting, SETTING_KEY_BATTERY_CENTER_VOLTAGE))
    if (setting_is_has_tmp(setting))
    {
        // if (btn->id == BUTTON_ID_LEFT || btn->id == BUTTON_ID_RIGHT)
        // {
            // if (!setting_get_value_is_tmp(setting))
            // {
            //     setting_set_tmp_u16(setting, setting_get_u16(setting));
            //     setting_set_value_is_tmp(setting, true);
            // }

            // uint16_t v = setting_get_tmp_u16(setting);
            // setting_set_tmp_u16(setting, btn->id == BUTTON_ID_LEFT ? --v : ++v);
        // }

        if (btn->id == BUTTON_ID_LEFT) setting_decrement(setting);
        if (btn->id == BUTTON_ID_RIGHT) setting_increment(setting);
    }
}

void menu_push_active(menu_t *menu)
{
    if (menu_stack_depth < MAX_MENU_DEPTH)
    {
        menu_enter(menu);
    }
}

void menu_set_active(menu_t *menu)
{
    menu_stack_depth = 0;
    menu_enter(menu);
}

void menu_pop_active(void)
{
    if (menu_stack_depth > 0)
    {
        menu_stack_depth--;
    }
}

menu_t *menu_get_active(void)
{
    if (menu_stack_depth > 0)
    {
        return menu_stack[menu_stack_depth - 1];
    }
    return NULL;
}

int menu_get_num_entries(const menu_t *menu)
{
    if (!menu)
    {
        return -1;
    }
    int entries = 0;
    for (;;)
    {
        entries++;
        if (!menu->entries[entries].title)
        {
            break;
        }
    }
    return entries;
}

const char *menu_get_prompt(const menu_t *menu)
{
    return menu ? menu->prompt : NULL;
}

const menu_entry_t *menu_get_entry(const menu_t *menu, int idx)
{
    const menu_entry_t *entry = NULL;
    if (menu)
    {
        entry = &menu->entries[idx];
        if (!entry->title)
        {
            entry = NULL;
        }
    }
    return entry;
}

const char *menu_entry_get_title(const menu_t *menu, int idx, char *buf, uint16_t bufsize)
{
    const menu_entry_t *entry = menu_get_entry(menu, idx);
    if (entry && entry->title)
    {
        return entry->title(entry->data, buf, bufsize);
    }
    return NULL;
}

uint8_t menu_get_entry_selected(const menu_t *menu)
{
    return menu ? menu->index : 0;
}

bool menu_is_entry_selected(const menu_t *menu, int idx)
{
    return menu && menu->index == idx;
}

// void menu_set_alt_pairings(air_pairing_t *pairings, size_t count)
// {
//     count = MIN(count, MENU_ALT_PAIRINGS_MAX);
//     for (int ii = 0; ii < count; ii++)
//     {
//         alt_pairings[ii] = pairings[ii];
//         menu_alt_pairings_entries[ii].title = menu_pairing_title;
//         menu_alt_pairings_entries[ii].action = menu_pairing_action;
//         menu_alt_pairings_entries[ii].data = &alt_pairings[ii];
//     }
//     menu_alt_pairings_entries[count] = MENU_STRING_ENTRY("Exit", menu_pairing_exit_action);
//     menu_alt_pairings_entries[count + 1] = MENU_END;
//     switch (config_get_rc_mode())
//     {
//     case RC_MODE_TX:
//         menu_alt_pairings.prompt = "Switch RX?";
//         break;
//     case RC_MODE_RX:
//         menu_alt_pairings.prompt = "Switch TX?";
//         break;
//     }
// }

#endif
