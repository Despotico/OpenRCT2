#pragma region Copyright (c) 2014-2017 OpenRCT2 Developers
/*****************************************************************************
 * OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
 *
 * OpenRCT2 is the work of many authors, a full list can be found in contributors.md
 * For more information, visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * A full copy of the GNU General Public License can be found in licence.txt
 *****************************************************************************/
#pragma endregion

#include "../config/Config.h"
#include "../Context.h"
#include "../OpenRCT2.h"
#include "../ParkImporter.h"
#include "../network/network.h"
#include "../core/Util.hpp"
#include "../core/Math.hpp"
#include "../interface/Screenshot.h"

#include "../audio/audio.h"
#include "../cheats.h"
#include "../editor.h"
#include "../input.h"
#include "../interface/console.h"
#include "../interface/land_tool.h"
#include "../interface/viewport.h"
#include "../interface/widget.h"
#include "../network/twitch.h"
#include "../peep/staff.h"
#include "../util/util.h"
#include "../world/footpath.h"
#include "../world/scenery.h"
#include "dropdown.h"

enum {
    WIDX_PAUSE,
    WIDX_FILE_MENU,
    WIDX_MUTE,
    WIDX_ZOOM_OUT,
    WIDX_ZOOM_IN,
    WIDX_ROTATE,
    WIDX_VIEW_MENU,
    WIDX_MAP,

    WIDX_LAND,
    WIDX_WATER,
    WIDX_SCENERY,
    WIDX_PATH,
    WIDX_CONSTRUCT_RIDE,
    WIDX_RIDES,
    WIDX_PARK,
    WIDX_STAFF,
    WIDX_GUESTS,
    WIDX_CLEAR_SCENERY,

    WIDX_FASTFORWARD,
    WIDX_CHEATS,
    WIDX_DEBUG,
    WIDX_FINANCES,
    WIDX_RESEARCH,
    WIDX_NEWS,
    WIDX_NETWORK,

    WIDX_SEPARATOR,
};

validate_global_widx(WC_TOP_TOOLBAR, WIDX_PAUSE);
validate_global_widx(WC_TOP_TOOLBAR, WIDX_LAND);
validate_global_widx(WC_TOP_TOOLBAR, WIDX_WATER);
validate_global_widx(WC_TOP_TOOLBAR, WIDX_SCENERY);
validate_global_widx(WC_TOP_TOOLBAR, WIDX_PATH);

typedef enum {
    DDIDX_NEW_GAME = 0,
    DDIDX_LOAD_GAME = 1,
    DDIDX_SAVE_GAME = 2,
    DDIDX_SAVE_GAME_AS = 3,
    // separator
    DDIDX_ABOUT = 5,
    DDIDX_OPTIONS = 6,
    DDIDX_SCREENSHOT = 7,
    DDIDX_GIANT_SCREENSHOT = 8,
    // separator
    DDIDX_QUIT_TO_MENU = 10,
    DDIDX_EXIT_OPENRCT2 = 11,
    // separator
    DDIDX_ENABLE_TWITCH = 13
} FILE_MENU_DDIDX;

typedef enum {
    DDIDX_UNDERGROUND_INSIDE = 0,
    DDIDX_HIDE_BASE = 1,
    DDIDX_HIDE_VERTICAL = 2,
    DDIDX_SEETHROUGH_RIDES = 4,
    DDIDX_SEETHROUGH_SCENARY = 5,
    DDIDX_SEETHROUGH_PATHS = 6,
    DDIDX_INVISIBLE_SUPPORTS = 7,
    DDIDX_INVISIBLE_PEEPS = 8,
    DDIDX_LAND_HEIGHTS = 10,
    DDIDX_TRACK_HEIGHTS = 11,
    DDIDX_PATH_HEIGHTS = 12,
    // 13 is a separator
    DDIDX_VIEW_CLIPPING = 14,

    TOP_TOOLBAR_VIEW_MENU_COUNT
} TOP_TOOLBAR_VIEW_MENU_DDIDX;

typedef enum {
    DDIDX_CONSOLE = 0,
    DDIDX_TILE_INSPECTOR = 1,
    DDIDX_OBJECT_SELECTION = 2,
    DDIDX_INVENTIONS_LIST = 3,
    DDIDX_SCENARIO_OPTIONS = 4,
    DDIDX_DEBUG_PAINT = 5,

    TOP_TOOLBAR_DEBUG_COUNT
} TOP_TOOLBAR_DEBUG_DDIDX;

typedef enum {
    DDIDX_MULTIPLAYER = 0
} TOP_TOOLBAR_NETWORK_DDIDX;

enum {
    DDIDX_CHEATS,
    DDIDX_ENABLE_SANDBOX_MODE = 2,
    DDIDX_DISABLE_CLEARANCE_CHECKS,
    DDIDX_DISABLE_SUPPORT_LIMITS
};

enum {
    DDIDX_SHOW_MAP,
    DDIDX_OPEN_VIEWPORT,
};

enum {
    DDIDX_ROTATE_CLOCKWISE,
    DDIDX_ROTATE_ANTI_CLOCKWISE,
};

typedef enum {
    F,
    T,
    A
} window_top_toolbar_state;

#define SPECIAL_KEY_FUNC_ARR_SIZE 5

typedef struct {
    window_top_toolbar_state ctrl_pressed;
    window_top_toolbar_state shift_pressed;
    window_top_toolbar_state alt_pressed;
    window_top_toolbar_state past_ctr;
    window_top_toolbar_state past_shift;
    window_top_toolbar_state past_alt;
    scenery_key_action result_state;
    bool(*trigger[SPECIAL_KEY_FUNC_ARR_SIZE])(sint16 x, sint16 y, uint16 selected_scenery);
    bool(*support[SPECIAL_KEY_FUNC_ARR_SIZE])(sint16 x, sint16 y, uint16 selected_scenery);
} window_top_toolbar_scenery_special_key_reaction;

#pragma region Toolbar_widget_ordering

// from left to right
static const sint32 left_aligned_widgets_order[] = {
    WIDX_PAUSE,
    WIDX_FASTFORWARD,
    WIDX_FILE_MENU,
    WIDX_MUTE,
    WIDX_NETWORK,
    WIDX_CHEATS,
    WIDX_DEBUG,

    WIDX_SEPARATOR,

    WIDX_ZOOM_OUT,
    WIDX_ZOOM_IN,
    WIDX_ROTATE,
    WIDX_VIEW_MENU,
    WIDX_MAP,

};

// from right to left
static const sint32 right_aligned_widgets_order[] = {
    WIDX_NEWS,
    WIDX_GUESTS,
    WIDX_STAFF,
    WIDX_PARK,
    WIDX_RIDES,
    WIDX_RESEARCH,
    WIDX_FINANCES,

    WIDX_SEPARATOR,

    WIDX_CONSTRUCT_RIDE,
    WIDX_PATH,
    WIDX_SCENERY,
    WIDX_WATER,
    WIDX_LAND,
    WIDX_CLEAR_SCENERY
};

#pragma endregion

static rct_widget window_top_toolbar_widgets[] = {
    { WWT_TRNBTN,   0,  0x0000,         0x001D,         0,      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_PAUSE,             STR_PAUSE_GAME_TIP },               // Pause
    { WWT_TRNBTN,   0,  0x001E + 30,    0x003B + 30,    0,      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_FILE,              STR_DISC_AND_GAME_OPTIONS_TIP },    // File menu
    { WWT_TRNBTN,   0,  0x00DC + 30,    0x00F9 + 30,    0,      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_G2_TOOLBAR_MUTE,           STR_TOOLBAR_MUTE_TIP },             // Mute
    { WWT_TRNBTN,   1,  0x0046 + 30,    0x0063 + 30,    0,      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_ZOOM_OUT,          STR_ZOOM_OUT_TIP },                 // Zoom out
    { WWT_TRNBTN,   1,  0x0064 + 30,    0x0081 + 30,    0,      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_ZOOM_IN,           STR_ZOOM_IN_TIP },                  // Zoom in
    { WWT_TRNBTN,   1,  0x0082 + 30,    0x009F + 30,    0,      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_ROTATE,            STR_ROTATE_TIP },                   // Rotate camera
    { WWT_TRNBTN,   1,  0x00A0 + 30,    0x00BD + 30,    0,      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_VIEW,              STR_VIEW_OPTIONS_TIP },             // Transparency menu
    { WWT_TRNBTN,   1,  0x00BE + 30,    0x00DB + 30,    0,      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_MAP,               STR_SHOW_MAP_TIP },                 // Map
    { WWT_TRNBTN,   2,  0x010B, 0x0128, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_LAND,              STR_ADJUST_LAND_TIP },              // Land
    { WWT_TRNBTN,   2,  0x0129, 0x0146, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_WATER,             STR_ADJUST_WATER_TIP },             // Water
    { WWT_TRNBTN,   2,  0x0147, 0x0164, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_SCENERY,           STR_PLACE_SCENERY_TIP },            // Scenery
    { WWT_TRNBTN,   2,  0x0165, 0x0182, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_FOOTPATH,          STR_BUILD_FOOTPATH_TIP },           // Path
    { WWT_TRNBTN,   2,  0x0183, 0x01A0, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_CONSTRUCT_RIDE,    STR_BUILD_RIDE_TIP },               // Construct ride
    { WWT_TRNBTN,   3,  0x01EA, 0x0207, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_RIDES,             STR_RIDES_IN_PARK_TIP },            // Rides
    { WWT_TRNBTN,   3,  0x0208, 0x0225, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_PARK,              STR_PARK_INFORMATION_TIP },         // Park
    { WWT_TRNBTN,   3,  0x0226, 0x0243, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TAB_TOOLBAR,               STR_STAFF_TIP },                    // Staff
    { WWT_TRNBTN,   3,  0x0230, 0x024D, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_GUESTS,            STR_GUESTS_TIP },                   // Guests
    { WWT_TRNBTN,   2,  0x0230, 0x024D, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TOOLBAR_CLEAR_SCENERY,     STR_CLEAR_SCENERY_TIP },            // Clear scenery
    { WWT_TRNBTN,   0,  0x001E, 0x003B, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TAB_TOOLBAR,               STR_GAME_SPEED_TIP },               // Fast forward
    { WWT_TRNBTN,   0,  0x001E, 0x003B, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TAB_TOOLBAR,               STR_CHEATS_TIP },                   // Cheats
    { WWT_TRNBTN,   0,  0x001E, 0x003B, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TAB_TOOLBAR,               STR_DEBUG_TIP },                    // Debug
    { WWT_TRNBTN,   3,  0x001E, 0x003B, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TAB_TOOLBAR,               STR_SCENARIO_OPTIONS_FINANCIAL_TIP },// Finances
    { WWT_TRNBTN,   3,  0x001E, 0x003B, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TAB_TOOLBAR,               STR_FINANCES_RESEARCH_TIP },        // Research
    { WWT_TRNBTN,   3,  0x001E, 0x003B, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TAB_TOOLBAR,               STR_SHOW_RECENT_MESSAGES_TIP },     // News
    { WWT_TRNBTN,   0,  0x001E, 0x003B, 0,                      TOP_TOOLBAR_HEIGHT,     IMAGE_TYPE_REMAP | SPR_TAB_TOOLBAR,               STR_SHOW_MULTIPLAYER_STATUS_TIP },  // Network

    { WWT_EMPTY,    0,  0,      10-1,   0,                      0,                      0xFFFFFFFF,                                 STR_NONE },                         // Artificial widget separator
    { WIDGETS_END },
};

static void window_top_toolbar_mouseup(rct_window *w, rct_widgetindex widgetIndex);
static void window_top_toolbar_mousedown(rct_window *w, rct_widgetindex widgetIndex, rct_widget* widget);
static void window_top_toolbar_dropdown(rct_window *w, rct_widgetindex widgetIndex, sint32 dropdownIndex);
static void window_top_toolbar_tool_update(rct_window* w, rct_widgetindex widgetIndex, sint32 x, sint32 y);
static void window_top_toolbar_tool_down(rct_window* w, rct_widgetindex widgetIndex, sint32 x, sint32 y);
static void window_top_toolbar_tool_drag(rct_window* w, rct_widgetindex widgetIndex, sint32 x, sint32 y);
static void window_top_toolbar_tool_up(rct_window* w, rct_widgetindex widgetIndex, sint32 x, sint32 y);
static void window_top_toolbar_tool_abort(rct_window *w, rct_widgetindex widgetIndex);
static void window_top_toolbar_invalidate(rct_window *w);
static void window_top_toolbar_paint(rct_window *w, rct_drawpixelinfo *dpi);

static rct_window_event_list window_top_toolbar_events = {
    nullptr,
    window_top_toolbar_mouseup,
    nullptr,
    window_top_toolbar_mousedown,
    window_top_toolbar_dropdown,
    nullptr,
    nullptr,
    nullptr,
    nullptr,                                           // check if editor versions are significantly different...
    window_top_toolbar_tool_update,                 // editor: 0x0066fB0E
    window_top_toolbar_tool_down,                   // editor: 0x0066fB5C
    window_top_toolbar_tool_drag,                   // editor: 0x0066fB37
    window_top_toolbar_tool_up,                     // editor: 0x0066fC44 (Exactly the same)
    window_top_toolbar_tool_abort,                  // editor: 0x0066fA74 (Exactly the same)
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    window_top_toolbar_invalidate,
    window_top_toolbar_paint,
    nullptr
};

static void top_toolbar_init_view_menu(rct_window *window, rct_widget *widget);
static void top_toolbar_view_menu_dropdown(sint16 dropdownIndex);
static void top_toolbar_init_fastforward_menu(rct_window *window, rct_widget *widget);
static void top_toolbar_fastforward_menu_dropdown(sint16 dropdownIndex);
static void top_toolbar_init_rotate_menu(rct_window *window, rct_widget *widget);
static void top_toolbar_rotate_menu_dropdown(sint16 dropdownIndex);
static void top_toolbar_init_debug_menu(rct_window *window, rct_widget *widget);
static void top_toolbar_debug_menu_dropdown(sint16 dropdownIndex);
static void top_toolbar_init_network_menu(rct_window *window, rct_widget *widget);
static void top_toolbar_network_menu_dropdown(sint16 dropdownIndex);

static void toggle_footpath_window();
static void toggle_land_window(rct_window *topToolbar, rct_widgetindex widgetIndex);
static void toggle_clear_scenery_window(rct_window *topToolbar, rct_widgetindex widgetIndex);
static void toggle_water_window(rct_window *topToolbar, rct_widgetindex widgetIndex);

money32 selection_lower_land(uint8 flags);
money32 selection_raise_land(uint8 flags);

static bool     _menuDropdownIncludesTwitch;
static uint16   _unkF64F15;

/**
 * Creates the main game top toolbar window.
 *  rct2: 0x0066B485 (part of 0x0066B3E8)
 */
void window_top_toolbar_open()
{
    rct_window * window = window_create(
        0, 0,
        context_get_width(), TOP_TOOLBAR_HEIGHT + 1,
        &window_top_toolbar_events,
        WC_TOP_TOOLBAR,
        WF_STICK_TO_FRONT | WF_TRANSPARENT | WF_NO_BACKGROUND
    );
    window->widgets = window_top_toolbar_widgets;

    window_init_scroll_widgets(window);
}

/**
 *
 *  rct2: 0x0066C957
 */
static void window_top_toolbar_mouseup(rct_window *w, rct_widgetindex widgetIndex)
{
    rct_window *mainWindow;

    switch (widgetIndex) {
    case WIDX_PAUSE:
        if (network_get_mode() != NETWORK_MODE_CLIENT) {
            game_do_command(0, 1, 0, 0, GAME_COMMAND_TOGGLE_PAUSE, 0, 0);
        }
        break;
    case WIDX_ZOOM_OUT:
        if ((mainWindow = window_get_main()) != nullptr)
            window_zoom_out(mainWindow, false);
        break;
    case WIDX_ZOOM_IN:
        if ((mainWindow = window_get_main()) != nullptr)
            window_zoom_in(mainWindow, false);
        break;
    case WIDX_CLEAR_SCENERY:
        toggle_clear_scenery_window(w, WIDX_CLEAR_SCENERY);
        break;
    case WIDX_LAND:
        toggle_land_window(w, WIDX_LAND);
        break;
    case WIDX_WATER:
        toggle_water_window(w, WIDX_WATER);
        break;
    case WIDX_SCENERY:
        if (!tool_set(w, WIDX_SCENERY, TOOL_ARROW)) {
            input_set_flag(INPUT_FLAG_6, true);
            window_scenery_open();
        }
        break;
    case WIDX_PATH:
        toggle_footpath_window();
        break;
    case WIDX_CONSTRUCT_RIDE:
        window_new_ride_open();
        break;
    case WIDX_RIDES:
        window_ride_list_open();
        break;
    case WIDX_PARK:
        window_park_entrance_open();
        break;
    case WIDX_STAFF:
        context_open_window(WC_STAFF_LIST);
        break;
    case WIDX_GUESTS:
        window_guest_list_open();
        break;
    case WIDX_FINANCES:
        window_finances_open();
        break;
    case WIDX_RESEARCH:
        window_research_open();
        break;
    case WIDX_NEWS:
        context_open_window(WC_RECENT_NEWS);
        break;
    case WIDX_MUTE:
        audio_toggle_all_sounds();
        break;
    }
}

/**
 *
 *  rct2: 0x0066CA3B
 */
static void window_top_toolbar_mousedown(rct_window *w, rct_widgetindex widgetIndex, rct_widget* widget)
{
    sint32 numItems;

    switch (widgetIndex) {
    case WIDX_FILE_MENU:
        _menuDropdownIncludesTwitch = false;
        if (gScreenFlags & (SCREEN_FLAGS_TRACK_DESIGNER | SCREEN_FLAGS_TRACK_MANAGER)) {
            gDropdownItemsFormat[0] = STR_ABOUT;
            gDropdownItemsFormat[1] = STR_OPTIONS;
            gDropdownItemsFormat[2] = STR_SCREENSHOT;
            gDropdownItemsFormat[3] = STR_GIANT_SCREENSHOT;
            gDropdownItemsFormat[4] = STR_EMPTY;
            gDropdownItemsFormat[5] = STR_QUIT_TRACK_DESIGNS_MANAGER;
            gDropdownItemsFormat[6] = STR_EXIT_OPENRCT2;

            if (gScreenFlags & SCREEN_FLAGS_TRACK_DESIGNER)
                gDropdownItemsFormat[5] = STR_QUIT_ROLLERCOASTER_DESIGNER;

            numItems = 7;
        } else if (gScreenFlags & SCREEN_FLAGS_SCENARIO_EDITOR) {
            gDropdownItemsFormat[0] = STR_LOAD_LANDSCAPE;
            gDropdownItemsFormat[1] = STR_SAVE_LANDSCAPE;
            gDropdownItemsFormat[2] = STR_EMPTY;
            gDropdownItemsFormat[3] = STR_ABOUT;
            gDropdownItemsFormat[4] = STR_OPTIONS;
            gDropdownItemsFormat[5] = STR_SCREENSHOT;
            gDropdownItemsFormat[6] = STR_GIANT_SCREENSHOT;
            gDropdownItemsFormat[7] = STR_EMPTY;
            gDropdownItemsFormat[8] = STR_QUIT_SCENARIO_EDITOR;
            gDropdownItemsFormat[9] = STR_EXIT_OPENRCT2;
            numItems = 10;
        } else {
            gDropdownItemsFormat[0] = STR_NEW_GAME;
            gDropdownItemsFormat[1] = STR_LOAD_GAME;
            gDropdownItemsFormat[2] = STR_SAVE_GAME;
            gDropdownItemsFormat[3] = STR_SAVE_GAME_AS;
            gDropdownItemsFormat[4] = STR_EMPTY;
            gDropdownItemsFormat[5] = STR_ABOUT;
            gDropdownItemsFormat[6] = STR_OPTIONS;
            gDropdownItemsFormat[7] = STR_SCREENSHOT;
            gDropdownItemsFormat[8] = STR_GIANT_SCREENSHOT;
            gDropdownItemsFormat[9] = STR_EMPTY;
            gDropdownItemsFormat[10] = STR_QUIT_TO_MENU;
            gDropdownItemsFormat[11] = STR_EXIT_OPENRCT2;
            numItems = 12;

        #ifndef DISABLE_TWITCH
            if (gConfigTwitch.channel != nullptr && gConfigTwitch.channel[0] != 0) {
                _menuDropdownIncludesTwitch = true;
                gDropdownItemsFormat[12] = STR_EMPTY;
                gDropdownItemsFormat[DDIDX_ENABLE_TWITCH] = STR_TOGGLE_OPTION;
                gDropdownItemsArgs[DDIDX_ENABLE_TWITCH] = STR_TWITCH_ENABLE;
                numItems = 14;
            }
        #endif
        }
        window_dropdown_show_text(
            w->x + widget->left,
            w->y + widget->top,
            widget->bottom - widget->top + 1,
            w->colours[0] | 0x80,
            DROPDOWN_FLAG_STAY_OPEN,
            numItems
        );

#ifndef DISABLE_TWITCH
        if (_menuDropdownIncludesTwitch && gTwitchEnable) {
            dropdown_set_checked(DDIDX_ENABLE_TWITCH, true);
        }
#endif
        break;
    case WIDX_CHEATS:
        gDropdownItemsFormat[0] = STR_TOGGLE_OPTION;
        gDropdownItemsFormat[1] = STR_EMPTY;
        gDropdownItemsFormat[2] = STR_TOGGLE_OPTION;
        gDropdownItemsFormat[3] = STR_TOGGLE_OPTION;
        gDropdownItemsFormat[4] = STR_TOGGLE_OPTION;
        gDropdownItemsArgs[0] = STR_CHEAT_TITLE;
        gDropdownItemsArgs[2] = STR_ENABLE_SANDBOX_MODE;
        gDropdownItemsArgs[3] = STR_DISABLE_CLEARANCE_CHECKS;
        gDropdownItemsArgs[4] = STR_DISABLE_SUPPORT_LIMITS;
        window_dropdown_show_text(
            w->x + widget->left,
            w->y + widget->top,
            widget->bottom - widget->top + 1,
            w->colours[0] | 0x80,
            0,
            5
        );
        if (gCheatsSandboxMode) {
            dropdown_set_checked(DDIDX_ENABLE_SANDBOX_MODE, true);
        }
        if (gCheatsDisableClearanceChecks) {
            dropdown_set_checked(DDIDX_DISABLE_CLEARANCE_CHECKS, true);
        }
        if (gCheatsDisableSupportLimits) {
            dropdown_set_checked(DDIDX_DISABLE_SUPPORT_LIMITS, true);
        }
        gDropdownDefaultIndex = DDIDX_CHEATS;
        break;
    case WIDX_VIEW_MENU:
        top_toolbar_init_view_menu(w, widget);
        break;
    case WIDX_MAP:
        gDropdownItemsFormat[0] = STR_SHORTCUT_SHOW_MAP;
        gDropdownItemsFormat[1] = STR_EXTRA_VIEWPORT;
        numItems = 2;

        if ((gScreenFlags & SCREEN_FLAGS_SCENARIO_EDITOR) && gS6Info.editor_step == EDITOR_STEP_LANDSCAPE_EDITOR) {
            gDropdownItemsFormat[2] = STR_MAPGEN_WINDOW_TITLE;
            numItems++;
        }

        window_dropdown_show_text(
            w->x + widget->left,
            w->y + widget->top,
            widget->bottom - widget->top + 1,
            w->colours[1] | 0x80,
            0,
            numItems
            );
        gDropdownDefaultIndex = DDIDX_SHOW_MAP;
        break;
    case WIDX_FASTFORWARD:
        top_toolbar_init_fastforward_menu(w, widget);
        break;
    case WIDX_ROTATE:
        top_toolbar_init_rotate_menu(w, widget);
        break;
    case WIDX_DEBUG:
        top_toolbar_init_debug_menu(w, widget);
        break;
    case WIDX_NETWORK:
        top_toolbar_init_network_menu(w, widget);
        break;
    }
}

static void window_top_toolbar_scenarioselect_callback(const utf8 *path)
{
    context_load_park_from_file(path);
}

/**
 *
 *  rct2: 0x0066C9EA
 */
static void window_top_toolbar_dropdown(rct_window *w, rct_widgetindex widgetIndex, sint32 dropdownIndex)
{
    switch (widgetIndex) {
    case WIDX_FILE_MENU:

        // New game is only available in the normal game. Skip one position to avoid incorrect mappings in the menus of the other modes.
        if (gScreenFlags & (SCREEN_FLAGS_SCENARIO_EDITOR))
            dropdownIndex += 1;

        // Quicksave is only available in the normal game. Skip one position to avoid incorrect mappings in the menus of the other modes.
        if (gScreenFlags & (SCREEN_FLAGS_SCENARIO_EDITOR) && dropdownIndex > DDIDX_LOAD_GAME)
            dropdownIndex += 1;

        // Track designer and track designs manager start with About, not Load/save
        if (gScreenFlags & (SCREEN_FLAGS_TRACK_DESIGNER | SCREEN_FLAGS_TRACK_MANAGER))
            dropdownIndex += DDIDX_ABOUT;

        switch (dropdownIndex) {
        case DDIDX_NEW_GAME:
            window_scenarioselect_open(window_top_toolbar_scenarioselect_callback);
            break;
        case DDIDX_LOAD_GAME:
            game_do_command(0, 1, 0, 0, GAME_COMMAND_LOAD_OR_QUIT, 0, 0);
            break;
        case DDIDX_SAVE_GAME:
            tool_cancel();
            save_game();
            break;
        case DDIDX_SAVE_GAME_AS:
            if (gScreenFlags & SCREEN_FLAGS_SCENARIO_EDITOR) {
                window_loadsave_open(LOADSAVETYPE_SAVE | LOADSAVETYPE_LANDSCAPE, gS6Info.name);
            }
            else {
                tool_cancel();
                save_game_as();
            }
            break;
        case DDIDX_ABOUT:
            context_open_window(WC_ABOUT);
            break;
        case DDIDX_OPTIONS:
            context_open_window(WC_OPTIONS);
            break;
        case DDIDX_SCREENSHOT:
            gScreenshotCountdown = 10;
            break;
        case DDIDX_GIANT_SCREENSHOT:
            screenshot_giant();
            break;
        case DDIDX_QUIT_TO_MENU:
            window_close_by_class(WC_MANAGE_TRACK_DESIGN);
            window_close_by_class(WC_TRACK_DELETE_PROMPT);
            game_do_command(0, 1, 0, 0, GAME_COMMAND_LOAD_OR_QUIT, 1, 0);
            break;
        case DDIDX_EXIT_OPENRCT2:
            context_quit();
            break;
#ifndef DISABLE_TWITCH
        case DDIDX_ENABLE_TWITCH:
            gTwitchEnable = !gTwitchEnable;
            break;
#endif
        }
        break;
    case WIDX_CHEATS:
        switch (dropdownIndex) {
        case DDIDX_CHEATS:
            context_open_window(WC_CHEATS);
            break;
        case DDIDX_ENABLE_SANDBOX_MODE:
            game_do_command(0, GAME_COMMAND_FLAG_APPLY, CHEAT_SANDBOXMODE, !gCheatsSandboxMode, GAME_COMMAND_CHEAT, 0, 0);
            break;
        case DDIDX_DISABLE_CLEARANCE_CHECKS:
            game_do_command(0, GAME_COMMAND_FLAG_APPLY, CHEAT_DISABLECLEARANCECHECKS, !gCheatsDisableClearanceChecks, GAME_COMMAND_CHEAT, 0, 0);
            break;
        case DDIDX_DISABLE_SUPPORT_LIMITS:
            game_do_command(0, GAME_COMMAND_FLAG_APPLY, CHEAT_DISABLESUPPORTLIMITS, !gCheatsDisableSupportLimits, GAME_COMMAND_CHEAT, 0, 0);
            break;
        }
        break;
    case WIDX_VIEW_MENU:
        top_toolbar_view_menu_dropdown(dropdownIndex);
        break;
    case WIDX_MAP:
        switch (dropdownIndex) {
        case 0:
            window_map_open();
            break;
        case 1:
            context_open_window(WC_VIEWPORT);
            break;
        case 2:
            context_open_window(WC_MAPGEN);
            break;
        }
        break;
    case WIDX_FASTFORWARD:
        top_toolbar_fastforward_menu_dropdown(dropdownIndex);
        break;
    case WIDX_ROTATE:
        top_toolbar_rotate_menu_dropdown(dropdownIndex);
        break;
    case WIDX_DEBUG:
        top_toolbar_debug_menu_dropdown(dropdownIndex);
        break;
    case WIDX_NETWORK:
        top_toolbar_network_menu_dropdown(dropdownIndex);
        break;
    }
}

/**
 *
 *  rct2: 0x0066C810
 */
static void window_top_toolbar_invalidate(rct_window *w)
{
    sint32 x, enabledWidgets, widgetIndex, widgetWidth, firstAlignment;
    rct_widget *widget;

    // Enable / disable buttons
    window_top_toolbar_widgets[WIDX_PAUSE].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_FILE_MENU].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_ZOOM_OUT].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_ZOOM_IN].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_ROTATE].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_VIEW_MENU].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_MAP].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_MUTE].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_LAND].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_WATER].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_SCENERY].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_PATH].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_CONSTRUCT_RIDE].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_RIDES].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_PARK].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_STAFF].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_GUESTS].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_CLEAR_SCENERY].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_FINANCES].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_RESEARCH].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_FASTFORWARD].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_CHEATS].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_DEBUG].type = gConfigGeneral.debugging_tools ? WWT_TRNBTN : WWT_EMPTY;
    window_top_toolbar_widgets[WIDX_NEWS].type = WWT_TRNBTN;
    window_top_toolbar_widgets[WIDX_NETWORK].type = WWT_TRNBTN;

    if (!gConfigInterface.toolbar_show_mute)
    {
        window_top_toolbar_widgets[WIDX_MUTE].type = WWT_EMPTY;
    }

    if (gScreenFlags & (SCREEN_FLAGS_SCENARIO_EDITOR | SCREEN_FLAGS_TRACK_DESIGNER | SCREEN_FLAGS_TRACK_MANAGER)) {
        window_top_toolbar_widgets[WIDX_PAUSE].type = WWT_EMPTY;
        window_top_toolbar_widgets[WIDX_RIDES].type = WWT_EMPTY;
        window_top_toolbar_widgets[WIDX_PARK].type = WWT_EMPTY;
        window_top_toolbar_widgets[WIDX_STAFF].type = WWT_EMPTY;
        window_top_toolbar_widgets[WIDX_GUESTS].type = WWT_EMPTY;
        window_top_toolbar_widgets[WIDX_FINANCES].type = WWT_EMPTY;
        window_top_toolbar_widgets[WIDX_RESEARCH].type = WWT_EMPTY;
        window_top_toolbar_widgets[WIDX_CHEATS].type = WWT_EMPTY;
        window_top_toolbar_widgets[WIDX_NEWS].type = WWT_EMPTY;
        window_top_toolbar_widgets[WIDX_NETWORK].type = WWT_EMPTY;

        if (gS6Info.editor_step != EDITOR_STEP_LANDSCAPE_EDITOR) {
            window_top_toolbar_widgets[WIDX_MAP].type = WWT_EMPTY;
            window_top_toolbar_widgets[WIDX_LAND].type = WWT_EMPTY;
            window_top_toolbar_widgets[WIDX_WATER].type = WWT_EMPTY;
            window_top_toolbar_widgets[WIDX_SCENERY].type = WWT_EMPTY;
            window_top_toolbar_widgets[WIDX_PATH].type = WWT_EMPTY;
            window_top_toolbar_widgets[WIDX_CLEAR_SCENERY].type = WWT_EMPTY;
        }

        if (gS6Info.editor_step != EDITOR_STEP_ROLLERCOASTER_DESIGNER) {
            window_top_toolbar_widgets[WIDX_CONSTRUCT_RIDE].type = WWT_EMPTY;
            window_top_toolbar_widgets[WIDX_FASTFORWARD].type = WWT_EMPTY;
        }

        if (gS6Info.editor_step != EDITOR_STEP_LANDSCAPE_EDITOR && gS6Info.editor_step != EDITOR_STEP_ROLLERCOASTER_DESIGNER) {
            window_top_toolbar_widgets[WIDX_ZOOM_OUT].type = WWT_EMPTY;
            window_top_toolbar_widgets[WIDX_ZOOM_IN].type = WWT_EMPTY;
            window_top_toolbar_widgets[WIDX_ROTATE].type = WWT_EMPTY;
            window_top_toolbar_widgets[WIDX_VIEW_MENU].type = WWT_EMPTY;
        }
    } else {
        if ((gParkFlags & PARK_FLAGS_NO_MONEY) || !gConfigInterface.toolbar_show_finances)
            window_top_toolbar_widgets[WIDX_FINANCES].type = WWT_EMPTY;

        if (!gConfigInterface.toolbar_show_research)
            window_top_toolbar_widgets[WIDX_RESEARCH].type = WWT_EMPTY;

        if (!gConfigInterface.toolbar_show_cheats)
            window_top_toolbar_widgets[WIDX_CHEATS].type = WWT_EMPTY;

        if (!gConfigInterface.toolbar_show_news)
            window_top_toolbar_widgets[WIDX_NEWS].type = WWT_EMPTY;

        switch (network_get_mode()) {
        case NETWORK_MODE_NONE:
            window_top_toolbar_widgets[WIDX_NETWORK].type = WWT_EMPTY;
            break;
        case NETWORK_MODE_CLIENT:
            window_top_toolbar_widgets[WIDX_PAUSE].type = WWT_EMPTY;
        // Fall-through
        case NETWORK_MODE_SERVER:
            window_top_toolbar_widgets[WIDX_FASTFORWARD].type = WWT_EMPTY;
            break;
        }
    }

    enabledWidgets = 0;
    for (int i = WIDX_PAUSE; i <= WIDX_NETWORK; i++)
        if (window_top_toolbar_widgets[i].type != WWT_EMPTY)
            enabledWidgets |= (1 << i);
    w->enabled_widgets = enabledWidgets;

    // Align left hand side toolbar buttons
    firstAlignment = 1;
    x = 0;
    for (size_t i = 0; i < Util::CountOf(left_aligned_widgets_order); ++i) {
        widgetIndex = left_aligned_widgets_order[i];
        widget = &window_top_toolbar_widgets[widgetIndex];
        if (widget->type == WWT_EMPTY && widgetIndex != WIDX_SEPARATOR)
            continue;

        if (firstAlignment && widgetIndex == WIDX_SEPARATOR)
            continue;

        widgetWidth = widget->right - widget->left;
        widget->left = x;
        x += widgetWidth;
        widget->right = x;
        x += 1;
        firstAlignment = 0;
    }

    // Align right hand side toolbar buttons
    sint32 screenWidth = context_get_width();
    firstAlignment = 1;
    x = Math::Max(640, screenWidth);
    for (size_t i = 0; i < Util::CountOf(right_aligned_widgets_order); ++i) {
        widgetIndex = right_aligned_widgets_order[i];
        widget = &window_top_toolbar_widgets[widgetIndex];
        if (widget->type == WWT_EMPTY && widgetIndex != WIDX_SEPARATOR)
            continue;

        if (firstAlignment && widgetIndex == WIDX_SEPARATOR)
            continue;

        widgetWidth = widget->right - widget->left;
        x -= 1;
        widget->right = x;
        x -= widgetWidth;
        widget->left = x;
        firstAlignment = 0;
    }

    // Footpath button pressed down
    if (window_find_by_class(WC_FOOTPATH) == nullptr)
        w->pressed_widgets &= ~(1 << WIDX_PATH);
    else
        w->pressed_widgets |= (1 << WIDX_PATH);

    if (gGamePaused & GAME_PAUSED_NORMAL)
        w->pressed_widgets |= (1 << WIDX_PAUSE);
    else
        w->pressed_widgets &= ~(1 << WIDX_PAUSE);

    if (!gGameSoundsOff)
        window_top_toolbar_widgets[WIDX_MUTE].image = IMAGE_TYPE_REMAP | SPR_G2_TOOLBAR_MUTE;
    else
        window_top_toolbar_widgets[WIDX_MUTE].image = IMAGE_TYPE_REMAP | SPR_G2_TOOLBAR_UNMUTE;

    // Zoomed out/in disable. Not sure where this code is in the original.
    if (window_get_main()->viewport->zoom == 0){
        w->disabled_widgets |= (1 << WIDX_ZOOM_IN);
    } else if (window_get_main()->viewport->zoom == 3){
        w->disabled_widgets |= (1 << WIDX_ZOOM_OUT);
    } else {
        w->disabled_widgets &= ~((1 << WIDX_ZOOM_IN) | (1 << WIDX_ZOOM_OUT));
    }
}

/**
 *
 *  rct2: 0x0066C8EC
 */
static void window_top_toolbar_paint(rct_window *w, rct_drawpixelinfo *dpi)
{
    sint32 x, y, imgId;

    window_draw_widgets(w, dpi);

    // Draw staff button image (setting masks to the staff colours)
    if (window_top_toolbar_widgets[WIDX_STAFF].type != WWT_EMPTY) {
        x = w->x + window_top_toolbar_widgets[WIDX_STAFF].left;
        y = w->y + window_top_toolbar_widgets[WIDX_STAFF].top;
        imgId = SPR_TOOLBAR_STAFF;
        if (widget_is_pressed(w, WIDX_STAFF))
            imgId++;
        imgId |= SPRITE_ID_PALETTE_COLOUR_2(gStaffHandymanColour, gStaffMechanicColour);
        gfx_draw_sprite(dpi, imgId, x, y, 0);
    }

    // Draw fast forward button
    if (window_top_toolbar_widgets[WIDX_FASTFORWARD].type != WWT_EMPTY) {
        x = w->x + window_top_toolbar_widgets[WIDX_FASTFORWARD].left + 0;
        y = w->y + window_top_toolbar_widgets[WIDX_FASTFORWARD].top + 0;
        if (widget_is_pressed(w, WIDX_FASTFORWARD))
            y++;
        imgId = SPR_G2_FASTFORWARD;
        gfx_draw_sprite(dpi, imgId, x + 6, y + 3, 0);


        for (sint32 i = 0; i < gGameSpeed && gGameSpeed <= 4; i++) {
            gfx_draw_sprite(dpi, SPR_G2_SPEED_ARROW, x + 5 + i * 5, y + 15, 0);
        }
        for (sint32 i = 0; i < 3 && i < gGameSpeed - 4 && gGameSpeed >= 5; i++) {
            gfx_draw_sprite(dpi, SPR_G2_HYPER_ARROW, x + 5 + i * 6, y + 15, 0);
        }
    }

    // Draw cheats button
    if (window_top_toolbar_widgets[WIDX_CHEATS].type != WWT_EMPTY) {
        x = w->x + window_top_toolbar_widgets[WIDX_CHEATS].left - 1;
        y = w->y + window_top_toolbar_widgets[WIDX_CHEATS].top - 1;
        if (widget_is_pressed(w, WIDX_CHEATS))
            y++;
        imgId = SPR_G2_SANDBOX;
        gfx_draw_sprite(dpi, imgId, x, y, 3);
    }

    // Draw debug button
    if (window_top_toolbar_widgets[WIDX_DEBUG].type != WWT_EMPTY) {
        x = w->x + window_top_toolbar_widgets[WIDX_DEBUG].left;
        y = w->y + window_top_toolbar_widgets[WIDX_DEBUG].top - 1;
        if (widget_is_pressed(w, WIDX_DEBUG))
            y++;
        imgId = SPR_TAB_GEARS_0;
        gfx_draw_sprite(dpi, imgId, x, y, 3);
    }

    // Draw research button
    if (window_top_toolbar_widgets[WIDX_RESEARCH].type != WWT_EMPTY) {
        x = w->x + window_top_toolbar_widgets[WIDX_RESEARCH].left - 1;
        y = w->y + window_top_toolbar_widgets[WIDX_RESEARCH].top;
        if (widget_is_pressed(w, WIDX_RESEARCH))
            y++;
        imgId = SPR_TAB_FINANCES_RESEARCH_0;
        gfx_draw_sprite(dpi, imgId, x, y, 0);
    }

    // Draw finances button
    if (window_top_toolbar_widgets[WIDX_FINANCES].type != WWT_EMPTY) {
        x = w->x + window_top_toolbar_widgets[WIDX_FINANCES].left + 3;
        y = w->y + window_top_toolbar_widgets[WIDX_FINANCES].top + 1;
        if (widget_is_pressed(w, WIDX_FINANCES))
            y++;
        imgId = SPR_FINANCE;
        gfx_draw_sprite(dpi, imgId, x, y, 0);
    }

    // Draw news button
    if (window_top_toolbar_widgets[WIDX_NEWS].type != WWT_EMPTY) {
        x = w->x + window_top_toolbar_widgets[WIDX_NEWS].left + 3;
        y = w->y + window_top_toolbar_widgets[WIDX_NEWS].top + 0;
        if (widget_is_pressed(w, WIDX_NEWS))
            y++;
        imgId = SPR_G2_TAB_NEWS;
        gfx_draw_sprite(dpi, imgId, x, y, 0);
    }

    // Draw network button
    if (window_top_toolbar_widgets[WIDX_NETWORK].type != WWT_EMPTY) {
        x = w->x + window_top_toolbar_widgets[WIDX_NETWORK].left + 3;
        y = w->y + window_top_toolbar_widgets[WIDX_NETWORK].top + 0;
        if (widget_is_pressed(w, WIDX_NETWORK))
            y++;
        imgId = SPR_SHOW_GUESTS_ON_THIS_RIDE_ATTRACTION;
        gfx_draw_sprite(dpi, imgId, x, y, 0);
    }
}

/**
 *
 *  rct2: 0x006E3158
 */
static void repaint_scenery_tool_down(sint16 x, sint16 y, rct_widgetindex widgetIndex){
    // ax, cx, bl
    sint16 grid_x, grid_y;
    sint32 type;
    // edx
    rct_map_element* map_element;
    uint16 flags =
        VIEWPORT_INTERACTION_MASK_SCENERY &
        VIEWPORT_INTERACTION_MASK_WALL &
        VIEWPORT_INTERACTION_MASK_LARGE_SCENERY &
        VIEWPORT_INTERACTION_MASK_BANNER;
    // This is -2 as banner is 12 but flags are offset different

    // not used
    rct_viewport* viewport;
    get_map_coordinates_from_pos(x, y, flags, &grid_x, &grid_y, &type, &map_element, &viewport);

    switch (type){
    case VIEWPORT_INTERACTION_ITEM_SCENERY:
    {
        rct_scenery_entry* scenery_entry = get_small_scenery_entry(map_element->properties.scenery.type);

        // If can't repaint
        if (!(scenery_entry->small_scenery.flags &
            (SMALL_SCENERY_FLAG_HAS_PRIMARY_COLOUR |
            SMALL_SCENERY_FLAG_HAS_GLASS)))
            return;

        gGameCommandErrorTitle = STR_CANT_REPAINT_THIS;
        game_do_command(
            grid_x,
            1 | (map_element->type << 8),
            grid_y,
            map_element->base_height | (map_element->properties.scenery.type << 8),
            GAME_COMMAND_SET_SCENERY_COLOUR,
            0,
            gWindowSceneryPrimaryColour | (gWindowScenerySecondaryColour << 8));
        break;
    }
    case VIEWPORT_INTERACTION_ITEM_WALL:
    {
        rct_scenery_entry* scenery_entry = get_wall_entry(map_element->properties.wall.type);

        // If can't repaint
        if (!(scenery_entry->wall.flags &
            (WALL_SCENERY_HAS_PRIMARY_COLOUR |
            WALL_SCENERY_HAS_GLASS)))
            return;

        gGameCommandErrorTitle = STR_CANT_REPAINT_THIS;
        game_do_command(
            grid_x,
            1 | (gWindowSceneryPrimaryColour << 8),
            grid_y,
            (map_element->type & MAP_ELEMENT_DIRECTION_MASK) | (map_element->base_height << 8),
            GAME_COMMAND_SET_WALL_COLOUR,
            0,
            gWindowScenerySecondaryColour | (gWindowSceneryTertiaryColour << 8));
        break;
    }
    case VIEWPORT_INTERACTION_ITEM_LARGE_SCENERY:
    {
        rct_scenery_entry* scenery_entry = get_large_scenery_entry(map_element->properties.scenerymultiple.type & MAP_ELEMENT_LARGE_TYPE_MASK);

        // If can't repaint
        if (!(scenery_entry->large_scenery.flags &
            LARGE_SCENERY_FLAG_HAS_PRIMARY_COLOUR))
            return;

        gGameCommandErrorTitle = STR_CANT_REPAINT_THIS;
        game_do_command(
            grid_x,
            1 | ((map_element->type & MAP_ELEMENT_DIRECTION_MASK) << 8),
            grid_y,
            map_element->base_height | ((map_element->properties.scenerymultiple.type >> 10) << 8),
            GAME_COMMAND_SET_LARGE_SCENERY_COLOUR,
            0,
            gWindowSceneryPrimaryColour | (gWindowScenerySecondaryColour << 8));
        break;
    }
    case VIEWPORT_INTERACTION_ITEM_BANNER:
    {
        rct_banner* banner = &gBanners[map_element->properties.banner.index];
        rct_scenery_entry* scenery_entry = get_banner_entry(banner->type);

        // If can't repaint
        if (!(scenery_entry->banner.flags &
            (1 << 0)))
            return;

        gGameCommandErrorTitle = STR_CANT_REPAINT_THIS;
        game_do_command(
            grid_x,
            1,
            grid_y,
            map_element->base_height | ((map_element->properties.banner.position & 0x3) << 8),
            GAME_COMMAND_SET_BANNER_COLOUR,
            0,
            gWindowSceneryPrimaryColour | (gWindowScenerySecondaryColour << 8));
        break;
    }
    default:
        return;
    }
}

static void scenery_eyedropper_tool_down(sint16 x, sint16 y, rct_widgetindex widgetIndex)
{
    uint16 flags =
        VIEWPORT_INTERACTION_MASK_SCENERY &
        VIEWPORT_INTERACTION_MASK_WALL &
        VIEWPORT_INTERACTION_MASK_LARGE_SCENERY &
        VIEWPORT_INTERACTION_MASK_BANNER &
        VIEWPORT_INTERACTION_MASK_FOOTPATH_ITEM;

    sint16 gridX, gridY;
    sint32 type;
    rct_map_element* mapElement;
    rct_viewport * viewport;
    get_map_coordinates_from_pos(x, y, flags, &gridX, &gridY, &type, &mapElement, &viewport);

    switch (type) {
    case VIEWPORT_INTERACTION_ITEM_SCENERY:
    {
        sint32 entryIndex = mapElement->properties.scenery.type;
        rct_scenery_entry * sceneryEntry = get_small_scenery_entry(entryIndex);
        if (sceneryEntry != nullptr && sceneryEntry != (rct_scenery_entry *)-1) {
            sint32 sceneryId = get_scenery_id_from_entry_index(OBJECT_TYPE_SMALL_SCENERY, entryIndex);
            if (sceneryId != -1 && window_scenery_set_selected_item(sceneryId)) {
                gWindowSceneryRotation = (get_current_rotation() + map_element_get_direction(mapElement)) & 3;
                gWindowSceneryPrimaryColour = mapElement->properties.scenery.colour_1 & 0x1F;
                gWindowScenerySecondaryColour = mapElement->properties.scenery.colour_2 & 0x1F;
                gWindowSceneryEyedropperEnabled = false;
            }
        }
        break;
    }
    case VIEWPORT_INTERACTION_ITEM_WALL:
    {
        sint32 entryIndex = mapElement->properties.wall.type;
        rct_scenery_entry * sceneryEntry = get_wall_entry(entryIndex);
        if (sceneryEntry != nullptr && sceneryEntry != (rct_scenery_entry *)-1) {
            sint32 sceneryId = get_scenery_id_from_entry_index(OBJECT_TYPE_WALLS, entryIndex);
            if (sceneryId != -1 && window_scenery_set_selected_item(sceneryId)) {
                gWindowSceneryPrimaryColour = mapElement->properties.wall.colour_1 & 0x1F;
                gWindowScenerySecondaryColour = wall_element_get_secondary_colour(mapElement);
                gWindowSceneryTertiaryColour = mapElement->properties.wall.colour_3 & 0x1F;
                gWindowSceneryEyedropperEnabled = false;
            }
        }
        break;
    }
    case VIEWPORT_INTERACTION_ITEM_LARGE_SCENERY:
    {
        sint32 entryIndex = mapElement->properties.scenerymultiple.type & MAP_ELEMENT_LARGE_TYPE_MASK;
        rct_scenery_entry * sceneryEntry = get_large_scenery_entry(entryIndex);
        if (sceneryEntry != nullptr && sceneryEntry != (rct_scenery_entry *)-1) {
            sint32 sceneryId = get_scenery_id_from_entry_index(OBJECT_TYPE_LARGE_SCENERY, entryIndex);
            if (sceneryId != -1 && window_scenery_set_selected_item(sceneryId)) {
                gWindowSceneryRotation = (get_current_rotation() + map_element_get_direction(mapElement)) & 3;
                gWindowSceneryPrimaryColour = mapElement->properties.scenerymultiple.colour[0] & 0x1F;
                gWindowScenerySecondaryColour = mapElement->properties.scenerymultiple.colour[1] & 0x1F;
                gWindowSceneryEyedropperEnabled = false;
            }
        }
        break;
    }
    case VIEWPORT_INTERACTION_ITEM_BANNER:
    {
        sint32 bannerIndex = mapElement->properties.banner.index;
        rct_banner *banner = &gBanners[bannerIndex];
        rct_scenery_entry * sceneryEntry = get_banner_entry(banner->type);
        if (sceneryEntry != nullptr && sceneryEntry != (rct_scenery_entry *)-1) {
            sint32 sceneryId = get_scenery_id_from_entry_index(OBJECT_TYPE_BANNERS, banner->type);
            if (sceneryId != -1 && window_scenery_set_selected_item(sceneryId)) {
                gWindowSceneryEyedropperEnabled = false;
            }
        }
        break;
    }
    case VIEWPORT_INTERACTION_ITEM_FOOTPATH_ITEM:
    {
        sint32 entryIndex = footpath_element_get_path_scenery_index(mapElement);
        rct_scenery_entry * sceneryEntry = get_footpath_item_entry(entryIndex);
        if (sceneryEntry != nullptr && sceneryEntry != (rct_scenery_entry *)-1) {
            sint32 sceneryId = get_scenery_id_from_entry_index(OBJECT_TYPE_PATH_BITS, entryIndex);
            if (sceneryId != -1 && window_scenery_set_selected_item(sceneryId)) {
                gWindowSceneryEyedropperEnabled = false;
            }
        }
        break;
    }
    }
}

/**
 *
 *  rct2: 0x006E1F34
 * Outputs
 * eax : gridX
 * ebx : parameter_1
 * ecx : gridY
 * edx : parameter_2
 * edi : parameter_3
 */

//---------------------------------------------------------
// Special static functions serving as triggers for key
// actions - USE THEM ONLY in 
// window_top_toolbar_scenery_process_keypad
//---------------------------------------------------------

static bool trigger_set_keep_height(sint16 x, sint16 y, uint16 selected_scenery)
{
    rct_map_element* map_element;
    uint16 flags =
        VIEWPORT_INTERACTION_MASK_TERRAIN &
        VIEWPORT_INTERACTION_MASK_RIDE &
        VIEWPORT_INTERACTION_MASK_SCENERY &
        VIEWPORT_INTERACTION_MASK_FOOTPATH &
        VIEWPORT_INTERACTION_MASK_WALL &
        VIEWPORT_INTERACTION_MASK_LARGE_SCENERY;
    sint32 interaction_type;
    get_map_coordinates_from_pos(x, y, flags, nullptr, nullptr, &interaction_type, &map_element, nullptr);
    if (interaction_type != VIEWPORT_INTERACTION_ITEM_NONE) {
        gSceneryCtrl.pressed = true;
        gSceneryCtrl.z = map_element->base_height * 8;
        return true;
    }
    return false;
}

static bool trigger_set_drag_begin(sint16 x, sint16 y, uint16 selected_scenery)
{
    uint8 cl = 0;
    rct_map_element* map_element;
    sint32 interaction_type;
    uint16 flags =
        VIEWPORT_INTERACTION_MASK_TERRAIN &
        VIEWPORT_INTERACTION_MASK_RIDE &
        VIEWPORT_INTERACTION_MASK_SCENERY &
        VIEWPORT_INTERACTION_MASK_FOOTPATH &
        VIEWPORT_INTERACTION_MASK_WALL &
        VIEWPORT_INTERACTION_MASK_LARGE_SCENERY;
    //screen_get_map_xy(x, y, &(gSceneryDrag.x), &(gSceneryDrag.y), nullptr);
    get_map_coordinates_from_pos(x, y, flags, &(gSceneryDrag.x), &(gSceneryDrag.y), &interaction_type, &map_element, nullptr);
    if (gSceneryGhost[0].type) {
        gSceneryDrag.rotation = gSceneryGhost[0].rotation;
    }
    else {
        if ((selected_scenery >> 8) == SCENERY_TYPE_SMALL)
            screen_get_map_xy_quadrant(x, y, &(gSceneryDrag.x), &(gSceneryDrag.y), &cl);
        else if ((selected_scenery >> 8) == SCENERY_TYPE_WALL)
            screen_get_map_xy_side(x, y, &(gSceneryDrag.x), &(gSceneryDrag.y), &cl);
        gSceneryDrag.rotation = cl & 0xFF;
    }
    return true;
}

static bool trigger_set_elevation(sint16 x, sint16 y, uint16 selected_scenery)
{
    // SHIFT pressed first time
    gSceneryShift.pressed = true;
    gSceneryShift.x = x;
    gSceneryShift.y = y;
    gSceneryShift.offset = 0;
    return true;
}

static bool cont_set_elevation(sint16 x, sint16 y, uint16 selected_scenery)
{
    // SHIFT keeps being pressed
    gSceneryShift.offset = (gSceneryShift.y - y + 4) & 0xFFF8;
    return true;
}

static bool trigger_set_drag_height(sint16 x, sint16 y, uint16 selected_scenery)
{
    rct_map_element* map_element;
    uint16 flags =
        VIEWPORT_INTERACTION_MASK_TERRAIN &
        VIEWPORT_INTERACTION_MASK_RIDE &
        VIEWPORT_INTERACTION_MASK_SCENERY &
        VIEWPORT_INTERACTION_MASK_FOOTPATH &
        VIEWPORT_INTERACTION_MASK_WALL &
        VIEWPORT_INTERACTION_MASK_LARGE_SCENERY;
    sint32 interaction_type;
    get_map_coordinates_from_pos(x, y, flags, nullptr, nullptr, &interaction_type, &map_element, nullptr);
    if (interaction_type != VIEWPORT_INTERACTION_ITEM_NONE) {
        gSceneryDrag.z = map_element->base_height * 8;
    }
    else
    {
        gSceneryDrag.z = 0;
    }
    return true;
}

static bool trigger_use_drag_height(sint16 x, sint16 y, uint16 selected_scenery)
{
    gScenerySetHeight = gSceneryDrag.z;
    return true;
}

static bool trigger_use_keep_height(sint16 x, sint16 y, uint16 selected_scenery)
{
    gScenerySetHeight = gSceneryCtrl.z;
    return true;
}

window_top_toolbar_scenery_special_key_reaction skey_tab[] =
{ //C SH ALT prv: C SH ALT,  STATE,                                  Trigger ; undefined result in SCENERY_KEY_ACTION_NONE
    { F, F, F,      A, A, A, SCENERY_KEY_ACTION_NONE,{ nullptr },{ nullptr } },//keep height
    { T, F, F,      A, A, A, SCENERY_KEY_ACTION_KEEP_HEIGHT,{ trigger_set_keep_height, trigger_set_drag_begin, trigger_use_keep_height },{ nullptr } },//keep height
    { F, T, F,      A, A, A, SCENERY_KEY_ACTION_RAISE_HEIGHT,{ trigger_set_elevation },{ cont_set_elevation } },//raise by shift
    { T, T, F,      T, F, A, SCENERY_KEY_ACTION_RAISE_AT_SELECTED,{ trigger_set_keep_height, trigger_set_elevation, trigger_use_keep_height },{ nullptr } },//raise by shift at position
    { T, T, F,      F, T, A, SCENERY_KEY_ACTION_RAISE_AT_SELECTED,{ trigger_set_keep_height, trigger_set_elevation, trigger_use_keep_height }, nullptr },//raise by shift at position
    { T, T, F,      A, A, A, SCENERY_KEY_ACTION_RAISE_AT_SELECTED,{ nullptr },{ cont_set_elevation } },//raise by shift at position

    { F, F, T,      F, A, A, SCENERY_KEY_ACTION_DRAG,{ trigger_set_drag_begin, trigger_set_drag_height },{ nullptr } },//drag
    { F, T, T,      A, F, T, SCENERY_KEY_ACTION_DRAG_APPEND_HEIGHT,{ trigger_set_elevation,  trigger_use_drag_height }, nullptr },//drag during shift press (up down)
    { F, T, T,      A, T, F, SCENERY_KEY_ACTION_DRAG_APPEND_HEIGHT,{ trigger_set_drag_begin, trigger_set_drag_height, trigger_use_drag_height },{ nullptr } },//drag during shift press (up down)
    { F, T, T,      A, A, A, SCENERY_KEY_ACTION_DRAG_APPEND_HEIGHT,{ nullptr },{ cont_set_elevation } },

    { T, F, T,      T, A, F, SCENERY_KEY_ACTION_DRAG_KEEP_HEIGHT,{ trigger_use_keep_height },{ nullptr } },//drag after ctrl press
    { T, F, T,      F, A, T, SCENERY_KEY_ACTION_DRAG_KEEP_HEIGHT,{ trigger_use_drag_height },{ nullptr } },//ctrl during drag
    { T, F, T,      F, A, F, SCENERY_KEY_ACTION_DRAG_KEEP_HEIGHT,{ trigger_use_drag_height },{ nullptr } },//at the same time
    { T, F, T,      T, F, T, SCENERY_KEY_ACTION_DRAG_KEEP_HEIGHT,{ trigger_use_drag_height },{ nullptr } },//at the same time

    { T, T, T,      A, A, A, SCENERY_KEY_ACTION_NONE,                nullptr, nullptr },//tbd, raise lower drag at position selected by either ctrl or alt inital press
};

static void window_top_toolbar_scenery_process_keypad(sint16 x, sint16 y, uint16 selected_scenery, scenery_key_action * key_action) {
    //--------------------------------------------------------------
    // When we press keys in one state we want a trigger to happend 
    // and state machine to keep that state. Doing this with if else
    // with 2 buttons is ok. With 3 buttons is doable but hard af for
    // debugging. Instead use a special array for this task. Woo!
    //--------------------------------------------------------------

    rct_window* w = window_find_by_class(WC_SCENERY);

    if (w == nullptr) {
        if (key_action) {
            *key_action = SCENERY_KEY_ACTION_NONE;
        }
        return;
    }
    if (!key_action) {
        return;
    }

    window_top_toolbar_state alt_pressed=F;
    window_top_toolbar_state alt_was_pressed=F;

    window_top_toolbar_state ctrl_pressed=F;
    window_top_toolbar_state ctrl_was_pressed=F;

    window_top_toolbar_state shift_pressed = F;
    window_top_toolbar_state shift_was_pressed = F;

    uint8 scenery_type = selected_scenery >> 8;

    if (input_test_place_object_modifier(PLACE_OBJECT_MODIFIER_COPY_Z)) ctrl_pressed = T;
    if (input_test_place_object_modifier(PLACE_OBJECT_MODIFIER_SHIFT_Z)) shift_pressed = T;
    if (input_test_place_object_modifier(PLACE_OBJECT_MODIFIER_DRAG)) alt_pressed = T;

    if (gSceneryCtrl.pressed) ctrl_was_pressed = T;
    if (gSceneryShift.pressed) shift_was_pressed = T;
    if (gSceneryDrag.pressed) alt_was_pressed = T;

    bool can_raise_item = false;
    if (scenery_type == SCENERY_TYPE_SMALL) {
        rct_scenery_entry* scenery_entry = get_small_scenery_entry(selected_scenery);

        if (scenery_entry->small_scenery.flags & SMALL_SCENERY_FLAG_STACKABLE) {
            can_raise_item = true;
        }
    }
    else if (scenery_type == SCENERY_TYPE_WALL || scenery_type == SCENERY_TYPE_LARGE) {
        can_raise_item = true;
    }

    can_raise_item = can_raise_item || gCheatsDisableSupportLimits;
    if (!can_raise_item)
    {
        ctrl_pressed = F;
        shift_pressed = F;
    }

    //detect state changed (different keys are pressed now)
    if (alt_pressed != alt_was_pressed || ctrl_pressed != ctrl_was_pressed || shift_pressed != shift_was_pressed) {
        uint8 index = 0;
        //state was changed, detect actions on change now
        for (uint8 i = 0; i <= sizeof(skey_tab) / sizeof(skey_tab[0]); i++) {
            if (skey_tab[i].ctrl_pressed == ctrl_pressed &&
                skey_tab[i].alt_pressed == alt_pressed &&
                skey_tab[i].shift_pressed == shift_pressed &&
                (skey_tab[i].past_ctr == A || skey_tab[i].past_ctr == ctrl_was_pressed) &&
                (skey_tab[i].past_alt == A || skey_tab[i].past_alt == alt_was_pressed) &&
                (skey_tab[i].past_shift == A || skey_tab[i].past_shift == shift_was_pressed))
            {
                index = i+1;
                break;
            }
        }
        if (!index) {
            *key_action = SCENERY_KEY_ACTION_NONE;
            return;
        }
        else {
            index = index - 1;
            *key_action = skey_tab[index].result_state;
            if (skey_tab[index].trigger) {
                bool check = true;
                for (uint8 i = 0; i < SPECIAL_KEY_FUNC_ARR_SIZE; i++) {
                    if (skey_tab[index].trigger[i]) {
                        skey_tab[index].trigger[i](x, y, selected_scenery);
                        if (!check) {
                            *key_action = SCENERY_KEY_ACTION_NONE;
                            return;
                        }
                    }
                }
                if (ctrl_pressed == T) {
                    gSceneryCtrl.pressed = true;
                }
                else {
                    gSceneryCtrl.pressed = false;
                }
                if (shift_pressed == T) {
                    gSceneryShift.pressed = true;
                }
                else {
                    gSceneryShift.pressed = false;
                }
                if (alt_pressed == T) {
                    gSceneryDrag.pressed = true;
                }
                else {
                    gSceneryDrag.pressed = false;
                }
            }
        }
    }
    else {
        //maintain the state
        uint8 index = 0;
        for (uint8 i = 0; i <= sizeof(skey_tab) / sizeof(skey_tab[0]); i++) {
            if (skey_tab[i].ctrl_pressed == ctrl_pressed &&
                skey_tab[i].alt_pressed == alt_pressed &&
                skey_tab[i].shift_pressed == shift_pressed &&
                (skey_tab[i].past_ctr == A || skey_tab[i].past_ctr == ctrl_was_pressed) &&
                (skey_tab[i].past_alt == A || skey_tab[i].past_alt == alt_was_pressed) &&
                (skey_tab[i].past_shift == A || skey_tab[i].past_shift == shift_was_pressed))
            {
                index = i + 1;
                break;
            }
        }
        if (!index) {
            *key_action = SCENERY_KEY_ACTION_NONE;
            return;
        }
        else {
            index = index - 1;
            *key_action = skey_tab[index].result_state;
            if (skey_tab[index].support) {
                bool check = true;
                for (uint8 i = 0; i < SPECIAL_KEY_FUNC_ARR_SIZE; i++) {
                    if (skey_tab[index].support[i]) {
                        skey_tab[index].support[i](x, y, selected_scenery);
                        if (!check) {
                            *key_action = SCENERY_KEY_ACTION_NONE;
                            return;
                        }
                    }
                }
            }
        }
    }
    return;
}


static void window_top_toolbar_scenery_get_scenery_rotation(rct_scenery_entry* scenery, uint8 * rotation)
{
    *rotation = gWindowSceneryRotation;
    if (!(scenery->small_scenery.flags & SMALL_SCENERY_FLAG_ROTATABLE)) {
        *rotation = util_rand() & 0xFF;
    }
    *rotation -= get_current_rotation();
    *rotation &= 0x3;
    return;
}

static void window_top_toolbar_scenery_get_tile_params (
    sint16 x,
    sint16 y,
    uint16 selected_scenery,
    sint16* grid_x,
    sint16* grid_y, 
    scenery_key_action key_action,
    uint32* parameter_1,
    uint32* parameter_2,
    uint32* parameter_3
    ) {
    rct_window* w = window_find_by_class(WC_SCENERY);
    if (w == nullptr) {
        *grid_x = MAP_LOCATION_NULL;
        return;
    }

    uint8 scenery_type = selected_scenery >> 8;
        
    switch (scenery_type) {
    case SCENERY_TYPE_SMALL:
    {
        rct_scenery_entry* scenery = get_small_scenery_entry(selected_scenery);
        uint8 rotation = 0;
        if (!(scenery->small_scenery.flags & SMALL_SCENERY_FLAG_FULL_TILE)) {
            uint8 cl = 0;
            sint16 z = 0;
            switch (key_action) {
                case SCENERY_KEY_ACTION_NONE:
                case SCENERY_KEY_ACTION_DRAG:
                    screen_get_map_xy_quadrant(x, y, grid_x, grid_y, &cl);
                    if (*grid_x == MAP_LOCATION_NULL)
                        return;
                    gSceneryPlaceZ = 0;
                    break;                
                case SCENERY_KEY_ACTION_DRAG_KEEP_HEIGHT:
                case SCENERY_KEY_ACTION_KEEP_HEIGHT:
                    z = gScenerySetHeight;
                    screen_get_map_xy_quadrant_with_z(x, y, z, grid_x, grid_y, &cl);
                    z = Math::Max<sint16>(z, 16);
                    gSceneryPlaceZ = z;
                    break;
                case SCENERY_KEY_ACTION_RAISE_HEIGHT:
                case SCENERY_KEY_ACTION_DRAG_APPEND_HEIGHT:
                {
                    x = gSceneryShift.x;
                    y = gSceneryShift.y;
                    screen_get_map_xy_quadrant(x, y, grid_x, grid_y, &cl);
                    if (*grid_x == MAP_LOCATION_NULL)
                        return;
                    rct_map_element* map_element = map_get_surface_element_at(*grid_x / 32, *grid_y / 32);
                    if (map_element == nullptr) {
                        *grid_x = MAP_LOCATION_NULL;
                        return;
                    }
                    z = (map_element->base_height * 8) & 0xFFF0;
                    z += gSceneryShift.offset;
                    z = Math::Max<sint16>(z, 16);
                    gSceneryPlaceZ = z;
                    break;
                }
                case SCENERY_KEY_ACTION_RAISE_AT_SELECTED:
                    x = gSceneryShift.x;
                    y = gSceneryShift.y;
                    z = gScenerySetHeight;
                    screen_get_map_xy_quadrant_with_z(x, y, z, grid_x, grid_y, &cl);
                    z += gSceneryShift.offset;
                    z = Math::Max<sint16>(z, 16);
                    gSceneryPlaceZ = z;
                    break;
                default: //not supported
                        return;
            }
            if (*grid_x == MAP_LOCATION_NULL)
                return;
            window_top_toolbar_scenery_get_scenery_rotation(scenery, &rotation);
            // Also places it in lower but think thats for clobbering
            *parameter_1 = (selected_scenery & 0xFF) << 8;
            *parameter_2 = (cl ^ (1 << 1)) | (gWindowSceneryPrimaryColour << 8);
            *parameter_3 = rotation | (gWindowScenerySecondaryColour << 16);
            return;
        }
        else {
            uint16 flags =
                VIEWPORT_INTERACTION_MASK_TERRAIN &
                VIEWPORT_INTERACTION_MASK_WATER;
            sint32 interaction_type = 0;
            rct_map_element* map_element;
            uint16 water_height = 0;
            sint16 z = 0;
            switch (key_action) {
                case SCENERY_KEY_ACTION_NONE:
                case SCENERY_KEY_ACTION_DRAG:
                    get_map_coordinates_from_pos(x, y, flags, grid_x, grid_y, &interaction_type, &map_element, nullptr);
                    if (interaction_type == VIEWPORT_INTERACTION_ITEM_NONE) {
                        *grid_x = MAP_LOCATION_NULL;
                        return;
                    }
                    gSceneryPlaceZ = 0;
                    water_height = map_element->properties.surface.terrain & MAP_ELEMENT_WATER_HEIGHT_MASK;
                    if (water_height != 0) {
                        gSceneryPlaceZ = water_height * 16;
                    }
                    break;
                case SCENERY_KEY_ACTION_KEEP_HEIGHT:
                case SCENERY_KEY_ACTION_DRAG_KEEP_HEIGHT:
                    z = gScenerySetHeight; 
                    screen_get_map_xy_with_z(x, y, z, grid_x, grid_y);
                    z = Math::Max<sint16>(z, 16);
                    gSceneryPlaceZ = z;
                    break;
                case SCENERY_KEY_ACTION_RAISE_HEIGHT:
                case SCENERY_KEY_ACTION_DRAG_APPEND_HEIGHT:
                    x = gSceneryShift.x;
                    y = gSceneryShift.y;
                    get_map_coordinates_from_pos(x, y, flags, grid_x, grid_y, &interaction_type, &map_element, nullptr);

                    if (interaction_type == VIEWPORT_INTERACTION_ITEM_NONE) {
                        *grid_x = MAP_LOCATION_NULL;
                        return;
                    }
                    gSceneryPlaceZ = 0;
                    water_height = map_element->properties.surface.terrain & MAP_ELEMENT_WATER_HEIGHT_MASK;
                    if (water_height != 0) {
                        gSceneryPlaceZ = water_height * 16;
                    }
                    map_element = map_get_surface_element_at(*grid_x / 32, *grid_y / 32);
                    if (map_element == nullptr) {
                        *grid_x = MAP_LOCATION_NULL;
                        return;
                    }
                    z = (map_element->base_height * 8) & 0xFFF0;
                    z += gSceneryShift.offset;
                    z = Math::Max<sint16>(z, 16);
                    gSceneryPlaceZ = z;
                    break;
                case SCENERY_KEY_ACTION_RAISE_AT_SELECTED:
                    x = gSceneryShift.x;
                    y = gSceneryShift.y;
                    z = gScenerySetHeight;
                    screen_get_map_xy_with_z(x, y, z, grid_x, grid_y);
                    z += gSceneryShift.offset;
                    z = Math::Max<sint16>(z, 16);
                    gSceneryPlaceZ = z;
                    break;
                default:
                    return;
            }
            if (*grid_x == MAP_LOCATION_NULL)
                return;

            *grid_x &= 0xFFE0;
            *grid_y &= 0xFFE0;
            
            window_top_toolbar_scenery_get_scenery_rotation(scenery, &rotation);

            // Also places it in lower but think thats for clobbering
            *parameter_1 = (selected_scenery & 0xFF) << 8;
            *parameter_2 = 0 | (gWindowSceneryPrimaryColour << 8);
            *parameter_3 = rotation | (gWindowScenerySecondaryColour << 16);
            return;
        }
    }
    case SCENERY_TYPE_PATH_ITEM:
    {
        uint16 flags =
            VIEWPORT_INTERACTION_MASK_FOOTPATH &
            VIEWPORT_INTERACTION_MASK_FOOTPATH_ITEM;
        sint32 interaction_type = 0;
        rct_map_element* map_element;

        get_map_coordinates_from_pos(x, y, flags, grid_x, grid_y, &interaction_type, &map_element, nullptr);
        if (interaction_type == VIEWPORT_INTERACTION_ITEM_NONE) {
            *grid_x = MAP_LOCATION_NULL;
            return;
        }
        *parameter_1 = 0 | ((map_element->properties.path.type & 0x7) << 8);
        *parameter_2 = map_element->base_height | ((map_element->properties.path.type >> 4) << 8);
        if (map_element->type & 1) {
            *parameter_2 |= MAP_LOCATION_NULL;
        }
        *parameter_3 = (selected_scenery & 0xFF) + 1;
        return;
    }
    case SCENERY_TYPE_WALL:
    {
        uint8 cl=0;
        sint16 z = 0;
        switch (key_action) {
            case SCENERY_KEY_ACTION_NONE:
            case SCENERY_KEY_ACTION_DRAG:
                screen_get_map_xy_side(x, y, grid_x, grid_y, &cl);
                if (*grid_x == MAP_LOCATION_NULL)
                    return;
                gSceneryPlaceZ = 0;
                break;
            case SCENERY_KEY_ACTION_KEEP_HEIGHT:
            case SCENERY_KEY_ACTION_DRAG_KEEP_HEIGHT:
                z = gScenerySetHeight;
                screen_get_map_xy_side_with_z(x, y, z, grid_x, grid_y, &cl);
                z = Math::Max<sint16>(z, 16);
                gSceneryPlaceZ = z;
                break;
            case SCENERY_KEY_ACTION_RAISE_HEIGHT:
            case SCENERY_KEY_ACTION_DRAG_APPEND_HEIGHT:
            {
                x = gSceneryShift.x;
                y = gSceneryShift.y;
                screen_get_map_xy_side(x, y, grid_x, grid_y, &cl);
                if (*grid_x == MAP_LOCATION_NULL)
                    return;
                rct_map_element* map_element = map_get_surface_element_at(*grid_x / 32, *grid_y / 32);
                if (map_element == nullptr) {
                    *grid_x = MAP_LOCATION_NULL;
                    return;
                }
                z = (map_element->base_height * 8) & 0xFFF0;
                z += gSceneryShift.offset;
                z = Math::Max<sint16>(z, 16);
                gSceneryPlaceZ = z;
                break;
            }
            case SCENERY_KEY_ACTION_RAISE_AT_SELECTED:
                x = gSceneryShift.x;
                y = gSceneryShift.y;
                z = gScenerySetHeight;
                screen_get_map_xy_side_with_z(x, y, z, grid_x, grid_y, &cl);
                z += gSceneryShift.offset;
                z = Math::Max<sint16>(z, 16);
                gSceneryPlaceZ = z;
                break;
            default:
                break;
        }
        if (*grid_x == MAP_LOCATION_NULL)
            return;

        _unkF64F15 = gWindowScenerySecondaryColour | (gWindowSceneryTertiaryColour << 8);
        // Also places it in lower but think thats for clobbering
        *parameter_1 = (selected_scenery & 0xFF) << 8;
        *parameter_2 = cl | (gWindowSceneryPrimaryColour << 8);
        *parameter_3 = 0;
        
        break;
    }
    case SCENERY_TYPE_LARGE:
    {
        sint16 z = 0;
        switch (key_action) {
            case SCENERY_KEY_ACTION_NONE:
            case SCENERY_KEY_ACTION_DRAG:
                sub_68A15E(x, y, grid_x, grid_y, nullptr, nullptr);
                if (*grid_x == MAP_LOCATION_NULL)
                    return;
                gSceneryPlaceZ = 0;
                break;
            case SCENERY_KEY_ACTION_KEEP_HEIGHT:
            case SCENERY_KEY_ACTION_DRAG_KEEP_HEIGHT:
                z = gScenerySetHeight;
                screen_get_map_xy_with_z(x, y, z, grid_x, grid_y);
                z = Math::Max<sint16>(z, 16);
                gSceneryPlaceZ = z;
                break;            
            case SCENERY_KEY_ACTION_RAISE_HEIGHT:
            case SCENERY_KEY_ACTION_DRAG_APPEND_HEIGHT:
            {
                x = gSceneryShift.x;
                y = gSceneryShift.y;
                sub_68A15E(x, y, grid_x, grid_y, nullptr, nullptr);
                if (*grid_x == MAP_LOCATION_NULL)
                    return;
                rct_map_element* map_element = map_get_surface_element_at(*grid_x / 32, *grid_y / 32);
                if (map_element == nullptr) {
                    *grid_x = MAP_LOCATION_NULL;
                    return;
                }
                z = (map_element->base_height * 8) & 0xFFF0;
                z += gSceneryShift.offset;
                z = Math::Max<sint16>(z, 16);
                gSceneryPlaceZ = z;
                break;
            }
            case SCENERY_KEY_ACTION_RAISE_AT_SELECTED:
                x = gSceneryShift.y;
                y = gSceneryShift.y;
                z = gScenerySetHeight;
                screen_get_map_xy_with_z(x, y, z, grid_x, grid_y);
                z += gSceneryShift.offset;
                z = Math::Max<sint16>(z, 16);
                gSceneryPlaceZ = z;
                break;
            default:
                break;
        }
        if (*grid_x == MAP_LOCATION_NULL)
            return;

        *grid_x &= 0xFFE0;
        *grid_y &= 0xFFE0;

        uint8 rotation = gWindowSceneryRotation;
        rotation -= get_current_rotation();
        rotation &= 0x3;

        *parameter_1 = (rotation << 8);
        *parameter_2 = gWindowSceneryPrimaryColour | (gWindowScenerySecondaryColour << 8);
        *parameter_3 = selected_scenery & 0xFF;
        break;
    }
    case SCENERY_TYPE_BANNER:
    {
        uint16 flags =
            VIEWPORT_INTERACTION_MASK_FOOTPATH &
            VIEWPORT_INTERACTION_MASK_FOOTPATH_ITEM;
        sint32 interaction_type = 0;
        rct_map_element* map_element;

        get_map_coordinates_from_pos(x, y, flags, grid_x, grid_y, &interaction_type, &map_element, nullptr);
        if (interaction_type == VIEWPORT_INTERACTION_ITEM_NONE) {
            *grid_x = MAP_LOCATION_NULL;
            return;
        }
        uint8 rotation = gWindowSceneryRotation;
        rotation -= get_current_rotation();
        rotation &= 0x3;
        sint16 z = map_element->base_height;
        if (map_element->properties.path.type & (1 << 2)) {
            if (rotation != ((map_element->properties.path.type & 3) ^ 2)) {
                z += 2;
            }
        }
        z /= 2;
        // Also places it in lower but think thats for clobbering
        *parameter_1 = (selected_scenery & 0xFF) << 8;
        *parameter_2 = z | (rotation << 8);
        *parameter_3 = gWindowSceneryPrimaryColour;
        return;
    }
  }
  return;
}

void game_command_callback_place_banner(sint32 eax, sint32 ebx, sint32 ecx, sint32 edx, sint32 esi, sint32 edi, sint32 ebp)
{
    if (ebx != MONEY32_UNDEFINED) {
        sint32 bannerId = edi;

        audio_play_sound_at_location(SOUND_PLACE_ITEM, gCommandPosition.x, gCommandPosition.y, gCommandPosition.z);
        window_banner_open(bannerId);
    }
}
/**
 *
 *  rct2: 0x006E2CC6
 */
static void window_top_toolbar_scenery_tool_down(sint16 x, sint16 y, rct_window *w, rct_widgetindex widgetIndex)
{
    //remove ghosts only
    scenery_remove_ghost_tool_placement(true);
    if (gWindowSceneryPaintEnabled & 1) {
        repaint_scenery_tool_down(x, y, widgetIndex);
        scenery_remove_ghost_tool_placement(false);
        return;
    } else if (gWindowSceneryEyedropperEnabled) {
        scenery_eyedropper_tool_down(x, y, widgetIndex);
        scenery_remove_ghost_tool_placement(false);
        return;
    }

    sint32 selectedTab = gWindowSceneryTabSelections[gWindowSceneryActiveTabIndex];
    uint8 sceneryType = (selectedTab & 0xFF00) >> 8;
    scenery_key_action key_action = SCENERY_KEY_ACTION_NONE;

    if (selectedTab == -1) return;

    sint16 gridX, gridY;
    uint32 parameter_1, parameter_2, parameter_3;

    window_top_toolbar_scenery_process_keypad(x, y, selectedTab, &key_action);
    window_top_toolbar_scenery_get_tile_params(x, y, selectedTab, &gridX, &gridY, key_action, &parameter_1, &parameter_2, &parameter_3);
    //if(key_action < SCENERY_KEY_ACTION_DRAG)
    //    scenery_remove_ghost_tool_placement(false);

    if (gridX == MAP_LOCATION_NULL) return;

    switch (sceneryType){
    case SCENERY_TYPE_SMALL:
    {
        sint32 quantity = 1;
        sint32 total_cost = 0;
        sint32 cost = 0;
        sint32 flags = (parameter_1 & 0xFF00) | GAME_COMMAND_FLAG_APPLY;
        gDisableErrorWindowSound = true;
        gGameCommandErrorTitle = STR_CANT_POSITION_THIS_HERE;
        bool isCluster = gWindowSceneryClusterEnabled && (network_get_mode() != NETWORK_MODE_CLIENT || network_can_perform_command(network_get_current_player_group_index(), -2));
        if (isCluster) {
            quantity = 35;
        }
        //sint32 successfulPlacements = 0;
        
        if (isCluster) {
            for (sint32 q = 0; q < quantity; q++) {
                sint32 zCoordinate = gSceneryPlaceZ;
                rct_scenery_entry* scenery = get_small_scenery_entry((parameter_1 >> 8) & 0xFF);
                sint16 cur_grid_x = gridX;
                sint16 cur_grid_y = gridY;
                uint8 place_rotation;
                //bool success = false;
                //calculate random position for cluster
                if (!(scenery->small_scenery.flags & SMALL_SCENERY_FLAG_FULL_TILE)) {
                    parameter_2 &= 0xFF00;
                    parameter_2 |= util_rand() & 3;
                }
                cur_grid_x += ((util_rand() % 16) - 8) * 32;
                cur_grid_y += ((util_rand() % 16) - 8) * 32;
                if (!(scenery->small_scenery.flags & SMALL_SCENERY_FLAG_ROTATABLE)) {
                    place_rotation = (place_rotation + 1) & 3;
                }
                //old approach: try to paste in a range - up/down
                uint8 zAttemptRange = 1;
                if (
                    gSceneryPlaceZ != 0 &&
                    gSceneryShift.pressed
                    ) {
                    zAttemptRange = 20;
                }
                for (; zAttemptRange != 0; zAttemptRange--) {
                     cost = game_do_command(
                        cur_grid_x,
                        flags,
                        cur_grid_y,
                        parameter_2,
                        GAME_COMMAND_PLACE_SCENERY,
                        place_rotation | (parameter_3 & 0xFFFF0000),
                        gSceneryPlaceZ
                    );
                    gDisableErrorWindowSound = false;

                    if (cost != MONEY32_UNDEFINED) {
                        //window_close_by_class(WC_ERROR);
                        //audio_play_sound_at_location(SOUND_PLACE_ITEM, gCommandPosition.x, gCommandPosition.y, gCommandPosition.z);
                        //success = true;
                        break;
                    }
                    if (
                        gGameCommandErrorText == STR_NOT_ENOUGH_CASH_REQUIRES ||
                        gGameCommandErrorText == STR_CAN_ONLY_BUILD_THIS_ON_WATER
                        ) {
                        break;
                    }
                    gSceneryPlaceZ += 8;
                }
                if (cost == MONEY32_UNDEFINED && gGameCommandErrorText == STR_NOT_ENOUGH_CASH_REQUIRES) {
                    break;
                }
                if (cost != MONEY32_UNDEFINED) {
                    total_cost += cost;
                }
                gSceneryPlaceZ = zCoordinate;
            }
        }
        else {
            //not a cluster; so may be a result of dragging
            for (sint16 i = 0; i<SCENERY_GHOST_LIST_SIZE; i++) {
                if (!gSceneryGhost[i].placable)
                    break;
                cost = game_do_command(
                    gSceneryGhost[i].position.x,
                    flags,
                    gSceneryGhost[i].position.y,
                    parameter_2,
                    GAME_COMMAND_PLACE_SCENERY,
                    gSceneryGhost[i].rotation | (parameter_3 & 0xFFFF0000),
                    gSceneryGhost[i].placing_height
                );
                if (cost != MONEY32_UNDEFINED)
                    total_cost += cost;
                if (
                    gGameCommandErrorText == STR_NOT_ENOUGH_CASH_REQUIRES ||
                    gGameCommandErrorText == STR_CAN_ONLY_BUILD_THIS_ON_WATER
                    ) {
                    break;
                }
            }
        }
        gDisableErrorWindowSound = false;
        if (total_cost != MONEY32_UNDEFINED) {
            window_close_by_class(WC_ERROR);
            audio_play_sound_at_location(SOUND_PLACE_ITEM, gCommandPosition.x, gCommandPosition.y, gCommandPosition.z);
            scenery_remove_ghost_tool_placement(false);
            return;
        }        
        audio_play_sound_at_location(SOUND_ERROR, gCommandPosition.x, gCommandPosition.y, gCommandPosition.z);

        //if (successfulPlacements > 0) {
        //    window_close_by_class(WC_ERROR);
        //} else {
        //    audio_play_sound_at_location(SOUND_ERROR, gCommandPosition.x, gCommandPosition.y, gCommandPosition.z);
       // }
        break;
    }
    case SCENERY_TYPE_PATH_ITEM:
    {
        sint32 flags = GAME_COMMAND_FLAG_APPLY | GAME_COMMAND_FLAG_PATH_SCENERY | (parameter_1 & 0xFF00);

        gGameCommandErrorTitle = STR_CANT_POSITION_THIS_HERE;
        sint32 cost = game_do_command(gridX, flags, gridY, parameter_2, GAME_COMMAND_PLACE_PATH, parameter_3, 0);
        if (cost != MONEY32_UNDEFINED) {
            audio_play_sound_at_location(SOUND_PLACE_ITEM, gCommandPosition.x, gCommandPosition.y, gCommandPosition.z);
        }
        break;
    }
    case SCENERY_TYPE_WALL:
    {
        sint32 total_cost = 0;        
        sint32 flags = (parameter_1 & 0xFF00) | GAME_COMMAND_FLAG_APPLY;
        sint32 cost = 0;
        gDisableErrorWindowSound = true;
        gGameCommandErrorTitle = STR_CANT_BUILD_PARK_ENTRANCE_HERE;

        for (sint16 i = 0; i<SCENERY_GHOST_LIST_SIZE; i++) {
            if (!gSceneryGhost[i].placable)
                break;
            cost = game_do_command(
                gSceneryGhost[i].position.x,
                flags,
                gSceneryGhost[i].position.y,
                parameter_2 & 0xFFFFFF00 | gSceneryGhost[i].rotation,
                GAME_COMMAND_PLACE_WALL,
                gSceneryGhost[i].placing_height,
                _unkF64F15);
            if (cost != MONEY32_UNDEFINED)
                total_cost += cost;
            if (
                gGameCommandErrorText == STR_NOT_ENOUGH_CASH_REQUIRES ||
                gGameCommandErrorText == STR_CAN_ONLY_BUILD_THIS_ON_WATER
                ) {
                break;
            }
        }
        gDisableErrorWindowSound = false;
        if (total_cost != MONEY32_UNDEFINED) {
            window_close_by_class(WC_ERROR);
            audio_play_sound_at_location(SOUND_PLACE_ITEM, gCommandPosition.x, gCommandPosition.y, gCommandPosition.z);
            scenery_remove_ghost_tool_placement(false);
            return;
        }
        audio_play_sound_at_location(SOUND_ERROR, gCommandPosition.x, gCommandPosition.y, gCommandPosition.z);
        break;
    }
    case SCENERY_TYPE_LARGE:
    {
        uint8 zAttemptRange = 1;
        if (
            gSceneryPlaceZ != 0 &&
            gSceneryShift.pressed
        ) {
            zAttemptRange = 20;
        }

        for (; zAttemptRange != 0; zAttemptRange--) {
            sint32 flags = (parameter_1 & 0xFF00) | GAME_COMMAND_FLAG_APPLY;

            gDisableErrorWindowSound = true;
            gGameCommandErrorTitle = STR_CANT_POSITION_THIS_HERE;
            sint32 cost = game_do_command(gridX, flags, gridY, parameter_2, GAME_COMMAND_PLACE_LARGE_SCENERY, parameter_3, gSceneryPlaceZ);
            gDisableErrorWindowSound = false;

            if (cost != MONEY32_UNDEFINED){
                window_close_by_class(WC_ERROR);
                audio_play_sound_at_location(SOUND_PLACE_ITEM, gCommandPosition.x, gCommandPosition.y, gCommandPosition.z);
                scenery_remove_ghost_tool_placement(false);
                return;
            }

            if (
                gGameCommandErrorText == STR_NOT_ENOUGH_CASH_REQUIRES ||
                gGameCommandErrorText == STR_CAN_ONLY_BUILD_THIS_ON_WATER
            ) {
                break;
            }

            gSceneryPlaceZ += 8;
        }

        audio_play_sound_at_location(SOUND_ERROR, gCommandPosition.x, gCommandPosition.y, gCommandPosition.z);
        break;
    }
    case SCENERY_TYPE_BANNER:
    {
        sint32 flags = (parameter_1 & 0xFF00) | GAME_COMMAND_FLAG_APPLY;

        gGameCommandErrorTitle = STR_CANT_POSITION_THIS_HERE;
        game_command_callback = game_command_callback_place_banner;
        game_do_command(gridX, flags, gridY, parameter_2, GAME_COMMAND_PLACE_BANNER, parameter_3, gWindowSceneryPrimaryColour);
        break;
    }
    }
    scenery_remove_ghost_tool_placement(false);
}

/**
*
*  rct2: 0x0068E213
*/
static void top_toolbar_tool_update_scenery_clear(sint16 x, sint16 y){
    map_invalidate_selection_rect();
    gMapSelectFlags &= ~MAP_SELECT_FLAG_ENABLE;

    rct_xy16 mapTile = { 0 };
    screen_get_map_xy(x, y, &mapTile.x, &mapTile.y, nullptr);

    if (mapTile.x == MAP_LOCATION_NULL) {
        if (gClearSceneryCost != MONEY32_UNDEFINED) {
            gClearSceneryCost = MONEY32_UNDEFINED;
            window_invalidate_by_class(WC_CLEAR_SCENERY);
        }
        return;
    }

    uint8 state_changed = 0;

    if (!(gMapSelectFlags & MAP_SELECT_FLAG_ENABLE)) {
        gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE;
        state_changed++;
    }

    if (gMapSelectType != MAP_SELECT_TYPE_FULL) {
        gMapSelectType = MAP_SELECT_TYPE_FULL;
        state_changed++;
    }

    sint16 tool_size = Math::Max<uint16>(1, gLandToolSize);
    sint16 tool_length = (tool_size - 1) * 32;

    // Move to tool bottom left
    mapTile.x -= (tool_size - 1) * 16;
    mapTile.y -= (tool_size - 1) * 16;
    mapTile.x &= 0xFFE0;
    mapTile.y &= 0xFFE0;

    if (gMapSelectPositionA.x != mapTile.x){
        gMapSelectPositionA.x = mapTile.x;
        state_changed++;
    }

    if (gMapSelectPositionA.y != mapTile.y){
        gMapSelectPositionA.y = mapTile.y;
        state_changed++;
    }

    mapTile.x += tool_length;
    mapTile.y += tool_length;

    if (gMapSelectPositionB.x != mapTile.x){
        gMapSelectPositionB.x = mapTile.x;
        state_changed++;
    }

    if (gMapSelectPositionB.y != mapTile.y){
        gMapSelectPositionB.y = mapTile.y;
        state_changed++;
    }

    map_invalidate_selection_rect();
    if (!state_changed)
        return;

    sint32 eax = gMapSelectPositionA.x;
    sint32 ecx = gMapSelectPositionA.y;
    sint32 edi = gMapSelectPositionB.x;
    sint32 ebp = gMapSelectPositionB.y;
    sint32 clear = (gClearSmallScenery << 0) | (gClearLargeScenery << 1) | (gClearFootpath << 2);
    money32 cost = game_do_command(eax, 0, ecx, clear, GAME_COMMAND_CLEAR_SCENERY, edi, ebp);

    if (gClearSceneryCost != cost) {
        gClearSceneryCost = cost;
        window_invalidate_by_class(WC_CLEAR_SCENERY);
        return;
    }
}

static void top_toolbar_tool_update_land_paint(sint16 x, sint16 y){
    map_invalidate_selection_rect();
    gMapSelectFlags &= ~MAP_SELECT_FLAG_ENABLE;

    rct_xy16 mapTile = { 0 };
    screen_get_map_xy(x, y, &mapTile.x, &mapTile.y, nullptr);

    if (mapTile.x == MAP_LOCATION_NULL) {
        if (gClearSceneryCost != MONEY32_UNDEFINED) {
            gClearSceneryCost = MONEY32_UNDEFINED;
            window_invalidate_by_class(WC_CLEAR_SCENERY);
        }
        return;
    }

    uint8 state_changed = 0;

    if (!(gMapSelectFlags & MAP_SELECT_FLAG_ENABLE)) {
        gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE;
        state_changed++;
    }

    if (gMapSelectType != MAP_SELECT_TYPE_FULL) {
        gMapSelectType = MAP_SELECT_TYPE_FULL;
        state_changed++;
    }

    sint16 tool_size = Math::Max<uint16>(1, gLandToolSize);
    sint16 tool_length = (tool_size - 1) * 32;

    // Move to tool bottom left
    mapTile.x -= (tool_size - 1) * 16;
    mapTile.y -= (tool_size - 1) * 16;
    mapTile.x &= 0xFFE0;
    mapTile.y &= 0xFFE0;

    if (gMapSelectPositionA.x != mapTile.x){
        gMapSelectPositionA.x = mapTile.x;
        state_changed++;
    }

    if (gMapSelectPositionA.y != mapTile.y){
        gMapSelectPositionA.y = mapTile.y;
        state_changed++;
    }

    mapTile.x += tool_length;
    mapTile.y += tool_length;

    if (gMapSelectPositionB.x != mapTile.x){
        gMapSelectPositionB.x = mapTile.x;
        state_changed++;
    }

    if (gMapSelectPositionB.y != mapTile.y){
        gMapSelectPositionB.y = mapTile.y;
        state_changed++;
    }

    map_invalidate_selection_rect();
    if (!state_changed)
        return;
}

/**
*
*  rct2: 0x00664280
*/
static void top_toolbar_tool_update_land(sint16 x, sint16 y){

    uint8 side;

    if (!gMapCtrlPressed) {
        if (input_test_place_object_modifier(PLACE_OBJECT_MODIFIER_COPY_Z)) {
            gMapCtrlPressed = true;
        }
    }
    else {
        if (!input_test_place_object_modifier(PLACE_OBJECT_MODIFIER_COPY_Z)) {
            gMapCtrlPressed = false;
        }
    }

    map_invalidate_selection_rect();

    if (gCurrentToolId == TOOL_UP_DOWN_ARROW){
        if (!(gMapSelectFlags & MAP_SELECT_FLAG_ENABLE))
            return;

        money32 lower_cost = selection_lower_land(0);
        money32 raise_cost = selection_raise_land(0);

        if (gLandToolRaiseCost != raise_cost ||
            gLandToolLowerCost != lower_cost){
            gLandToolRaiseCost = raise_cost;
            gLandToolLowerCost = lower_cost;
            window_invalidate_by_class(WC_LAND);
        }
        return;
    }
	
    sint16 tool_size = gLandToolSize;
    rct_xy16 mapTile = { x, y };

    gMapSelectFlags &= ~MAP_SELECT_FLAG_ENABLE;
    if (tool_size == 1){
        sint32 direction;

        screen_get_map_xy_side(x, y, &mapTile.x, &mapTile.y, &side); //reuse old variables, we do not need dummies here
        mapTile.x = x;
        mapTile.y = y;
        screen_pos_to_map_pos(&mapTile.x, &mapTile.y, &direction); //this is the correct set

        if (mapTile.x == MAP_LOCATION_NULL) {
            money32 lower_cost = MONEY32_UNDEFINED;
            money32 raise_cost = MONEY32_UNDEFINED;

            if (gLandToolRaiseCost != raise_cost ||
                gLandToolLowerCost != lower_cost){
                gLandToolRaiseCost = raise_cost;
                gLandToolLowerCost = lower_cost;
                window_invalidate_by_class(WC_LAND);
            }
            return;
        }			

        uint8 state_changed = 0;

        if (!(gMapSelectFlags & MAP_SELECT_FLAG_ENABLE)) {
            gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE;
            state_changed++;
        }

        if (gMapSelectType != direction) {
            gMapSelectType = direction;
            state_changed++;
        }

        if ((gMapSelectType != MAP_SELECT_TYPE_EDGE_0 + (side & 0xFF)) && gMapCtrlPressed) {
            gMapSelectType = MAP_SELECT_TYPE_EDGE_0 + (side & 0xFF);
            state_changed++;
        }

        if (gMapSelectPositionA.x != mapTile.x){
            gMapSelectPositionA.x = mapTile.x;
            state_changed++;
        }

        if (gMapSelectPositionA.y != mapTile.y){
            gMapSelectPositionA.y = mapTile.y;
            state_changed++;
        }

        if (gMapSelectPositionB.x != mapTile.x){
            gMapSelectPositionB.x = mapTile.x;
            state_changed++;
        }

        if (gMapSelectPositionB.y != mapTile.y){
            gMapSelectPositionB.y = mapTile.y;
            state_changed++;
        }

        map_invalidate_selection_rect();
        if (!state_changed)
            return;

        money32 lower_cost = selection_lower_land(0);
        money32 raise_cost = selection_raise_land(0);
  

        if (gLandToolRaiseCost != raise_cost ||
            gLandToolLowerCost != lower_cost){
            gLandToolRaiseCost = raise_cost;
            gLandToolLowerCost = lower_cost;
            window_invalidate_by_class(WC_LAND);
        }
        return;
    }

    screen_get_map_xy_side(x, y, &mapTile.x, &mapTile.y, &side); //reuse old variables, we do not need dummies here

    //screen_get_map_xy(x, y, &mapTile.x, &mapTile.y, nullptr);

    if (mapTile.x == MAP_LOCATION_NULL) {
        money32 lower_cost = MONEY32_UNDEFINED;
        money32 raise_cost = MONEY32_UNDEFINED;

        if (gLandToolRaiseCost != raise_cost ||
            gLandToolLowerCost != lower_cost){
            gLandToolRaiseCost = raise_cost;
            gLandToolLowerCost = lower_cost;
            window_invalidate_by_class(WC_LAND);
        }
        return;
    }

    uint8 state_changed = 0;

    if (!(gMapSelectFlags & MAP_SELECT_FLAG_ENABLE)) {
        gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE;
        state_changed++;
    }

    if (gMapSelectType != MAP_SELECT_TYPE_FULL) {
        gMapSelectType = MAP_SELECT_TYPE_FULL;
        state_changed++;
    }

    if ((gMapSelectType != MAP_SELECT_TYPE_EDGE_0 + (side & 0xFF)) && gMapCtrlPressed) {
        gMapSelectType = MAP_SELECT_TYPE_EDGE_0 + (side & 0xFF);
        state_changed++;
    }


    if (tool_size == 0)
        tool_size = 1;

    sint16 tool_length = (tool_size - 1) * 32;

    // Decide on shape of the brush for bigger selection size
    switch (gMapSelectType)
    {
        case MAP_SELECT_TYPE_EDGE_0:
        case MAP_SELECT_TYPE_EDGE_2:
            // Line
            mapTile.y -= (tool_size - 1) * 16;
            mapTile.y &= 0xFFE0;
            break;
        case MAP_SELECT_TYPE_EDGE_1:
        case MAP_SELECT_TYPE_EDGE_3:
            // Line
            mapTile.x -= (tool_size - 1) * 16;
            mapTile.x &= 0xFFE0;
            break;
        default:
            // Move to tool bottom left
            mapTile.x -= (tool_size - 1) * 16;
            mapTile.y -= (tool_size - 1) * 16;
            mapTile.x &= 0xFFE0;
            mapTile.y &= 0xFFE0;
            break;
    }

    if (gMapSelectPositionA.x != mapTile.x){
        gMapSelectPositionA.x = mapTile.x;
        state_changed++;
    }

    if (gMapSelectPositionA.y != mapTile.y){
        gMapSelectPositionA.y = mapTile.y;
        state_changed++;
    }

    // Go to other side
    switch (gMapSelectType)
    {
    case MAP_SELECT_TYPE_EDGE_0:
    case MAP_SELECT_TYPE_EDGE_2:
        // Line
        mapTile.y += tool_length;
        gMapSelectType = MAP_SELECT_TYPE_FULL;
        break;
    case MAP_SELECT_TYPE_EDGE_1:
    case MAP_SELECT_TYPE_EDGE_3:
        // Line
        mapTile.x += tool_length;
        gMapSelectType = MAP_SELECT_TYPE_FULL;
        break;
    default:
        mapTile.x += tool_length;
        mapTile.y += tool_length;
        break;
    }
    

    if (gMapSelectPositionB.x != mapTile.x){
        gMapSelectPositionB.x = mapTile.x;
        state_changed++;
    }

    if (gMapSelectPositionB.y != mapTile.y){
        gMapSelectPositionB.y = mapTile.y;
        state_changed++;
    }

    map_invalidate_selection_rect();
    if (!state_changed)
        return;

    money32 lower_cost = selection_lower_land(0);
    money32 raise_cost = selection_raise_land(0);

    if (gLandToolRaiseCost != raise_cost ||
        gLandToolLowerCost != lower_cost){
        gLandToolRaiseCost = raise_cost;
        gLandToolLowerCost = lower_cost;
        window_invalidate_by_class(WC_LAND);
    }
}

/**
*
*  rct2: 0x006E6BDC
*/
static void top_toolbar_tool_update_water(sint16 x, sint16 y){
    map_invalidate_selection_rect();

    if (gCurrentToolId == TOOL_UP_DOWN_ARROW){
        if (!(gMapSelectFlags & MAP_SELECT_FLAG_ENABLE))
            return;

        money32 lower_cost = lower_water(
            gMapSelectPositionA.x,
            gMapSelectPositionA.y,
            gMapSelectPositionB.x,
            gMapSelectPositionB.y,
            0);

        money32 raise_cost = raise_water(
            gMapSelectPositionA.x,
            gMapSelectPositionA.y,
            gMapSelectPositionB.x,
            gMapSelectPositionB.y,
            0);

        if (gWaterToolRaiseCost != raise_cost || gWaterToolLowerCost != lower_cost) {
            gWaterToolRaiseCost = raise_cost;
            gWaterToolLowerCost = lower_cost;
            window_invalidate_by_class(WC_WATER);
        }
        return;
    }

    gMapSelectFlags &= ~MAP_SELECT_FLAG_ENABLE;

    rct_xy16 mapTile = { 0 };
    sint32 interaction_type = 0;
    get_map_coordinates_from_pos(
        x,
        y,
        VIEWPORT_INTERACTION_MASK_TERRAIN & VIEWPORT_INTERACTION_MASK_WATER,
        &mapTile.x,
        &mapTile.y,
        &interaction_type,
        nullptr,
        nullptr);

    if (interaction_type == VIEWPORT_INTERACTION_ITEM_NONE){
        if (gWaterToolRaiseCost != MONEY32_UNDEFINED || gWaterToolLowerCost != MONEY32_UNDEFINED) {
            gWaterToolRaiseCost = MONEY32_UNDEFINED;
            gWaterToolLowerCost = MONEY32_UNDEFINED;
            window_invalidate_by_class(WC_WATER);
        }
        return;
    }

    mapTile.x += 16;
    mapTile.y += 16;

    uint8 state_changed = 0;

    if (!(gMapSelectFlags & MAP_SELECT_FLAG_ENABLE)) {
        gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE;
        state_changed++;
    }

    if (gMapSelectType != MAP_SELECT_TYPE_FULL_WATER) {
        gMapSelectType = MAP_SELECT_TYPE_FULL_WATER;
        state_changed++;
    }

    sint16 tool_size = Math::Max<uint16>(1, gLandToolSize);
    sint16 tool_length = (tool_size - 1) * 32;

    // Move to tool bottom left
    mapTile.x -= (tool_size - 1) * 16;
    mapTile.y -= (tool_size - 1) * 16;
    mapTile.x &= 0xFFE0;
    mapTile.y &= 0xFFE0;

    if (gMapSelectPositionA.x != mapTile.x){
        gMapSelectPositionA.x = mapTile.x;
        state_changed++;
    }

    if (gMapSelectPositionA.y != mapTile.y){
        gMapSelectPositionA.y = mapTile.y;
        state_changed++;
    }

    mapTile.x += tool_length;
    mapTile.y += tool_length;

    if (gMapSelectPositionB.x != mapTile.x){
        gMapSelectPositionB.x = mapTile.x;
        state_changed++;
    }

    if (gMapSelectPositionB.y != mapTile.y){
        gMapSelectPositionB.y = mapTile.y;
        state_changed++;
    }

    map_invalidate_selection_rect();
    if (!state_changed)
        return;

    money32 lower_cost = lower_water(
        gMapSelectPositionA.x,
        gMapSelectPositionA.y,
        gMapSelectPositionB.x,
        gMapSelectPositionB.y,
        0);

    money32 raise_cost = raise_water(
        gMapSelectPositionA.x,
        gMapSelectPositionA.y,
        gMapSelectPositionB.x,
        gMapSelectPositionB.y,
        0);

    if (gWaterToolRaiseCost != raise_cost || gWaterToolLowerCost != lower_cost) {
        gWaterToolRaiseCost = raise_cost;
        gWaterToolLowerCost = lower_cost;
        window_invalidate_by_class(WC_WATER);
    }
}

/**
 *
 *  rct2: 0x006E24F6
 * On failure returns MONEY32_UNDEFINED
 * On success places ghost scenery and returns cost to place proper
 */
static money32 try_place_ghost_scenery(rct_xy16 map_tile, rct_xy16 map_tile2, rct_xy16 current_tile, uint32 parameter_1, uint32 parameter_2, uint32 parameter_3, uint16 selected_tab){
    //scenery_remove_ghost_tool_placement();

    uint8 scenery_type = selected_tab >> 8;
    money32 cost = 0;
    money32 total_cost = 0;
    rct_map_element* mapElement;
    uint16 ghost_index = 0;
    rct_xyzd16 pos = { 0,0,0,0 };
    uint8 underground = 0x00;
    rct_scenery_entry* scenery = nullptr;
    bool side1 = false;
    bool side2 = false;
    bool side3 = false;
    bool side4 = false;

    if (scenery_type == SCENERY_TYPE_SMALL)
    {
        scenery = get_small_scenery_entry(selected_tab);
    }
    uint16 index = 0;
    rct_xyzd16 position_list[SCENERY_GHOST_LIST_SIZE] = { 0 };

    //for shift click we need to correct height a bit using CURRENT tile limits
    //wall is problematic due to the fact that it can be hollow (some tiles do not match current tile
    //direction). Current tile, luckily, is always the border one so we can check it.
    if (gSceneryPlaceZ != 0 && gSceneryShift.pressed)
    {
        pos.x = current_tile.x;
        pos.y = current_tile.y;

        if (gSceneryShape == SCENERY_SHAPE_HOLLOW && scenery_type==SCENERY_TYPE_WALL) {
            //check any of the coords match any of corner points
            side1 = abs(pos.x - map_tile.x) < 32;
            side2 = abs(pos.x - map_tile2.x) < 32;
            side3 = abs(pos.y - map_tile.y) < 32;
            side4 = abs(pos.y - map_tile2.y) < 32;
            //check for edges
            if (side1 || side2 || side3 || side4)
            {
                //check for direction
                if (side1) pos.direction = 0;
                else if (side2) pos.direction = 2;
                else if (side3) pos.direction = 3;
                else if (side4) pos.direction = 1;
            }
        }
        else if (scenery_type == SCENERY_TYPE_WALL) {
            pos.direction = parameter_2 & 0xFF;
        }
        else if (scenery_type == SCENERY_TYPE_SMALL) {
            pos.direction = parameter_1 & 0xFF;
        }
        else if (scenery_type == SCENERY_TYPE_LARGE) {
            pos.direction = (parameter_1 >> 8) & 0xFF;
        } 
        else {
            pos.direction = 0;
        }
        //for small objects there is a corner issue - similar to hollow
        if (gSceneryShape == SCENERY_SHAPE_HOLLOW && scenery_type == SCENERY_TYPE_SMALL) {

        }
        
        uint16 bl = 0;
        bl = 20;
        cost = 0;
        for (; bl != 0; bl--) {
            switch (scenery_type) {
                case SCENERY_TYPE_SMALL:
                    cost = game_do_command(
                        pos.x,
                        parameter_1 | 0x69,
                        pos.y,
                        parameter_2,
                        GAME_COMMAND_PLACE_SCENERY,
                        parameter_3,
                        gSceneryPlaceZ);
                    mapElement = gSceneryMapElement;
                    if (cost != MONEY32_UNDEFINED)
                    {
                        game_do_command(
                            pos.x,
                            105 | ((mapElement->type) << 8),
                            pos.y,
                            mapElement->base_height | (selected_tab << 8),
                            //mapElement->base_height | pos.direction,
                            GAME_COMMAND_REMOVE_SCENERY,
                            0,
                            0);
                    }
                    break;
                case SCENERY_TYPE_WALL:
                    cost = game_do_command(
                        pos.x,
                        parameter_1 | 0x69,
                        pos.y,
                        (parameter_2 & 0xFFFFFF00 | pos.direction),
                        GAME_COMMAND_PLACE_WALL,
                        gSceneryPlaceZ,
                        _unkF64F15);
                    mapElement = gSceneryMapElement;
                    if (cost != MONEY32_UNDEFINED)
                    {
                        game_do_command(
                            pos.x,
                            105 | (scenery_type << 8),
                            pos.y,
                            pos.direction | (mapElement->base_height << 8),
                            GAME_COMMAND_REMOVE_WALL,
                            0,
                            0);
                    }
                    break;
                case SCENERY_TYPE_LARGE:
                    cost = game_do_command(
                        pos.x,
                        parameter_1 | 0x69,
                        pos.y,
                        parameter_2,
                        GAME_COMMAND_PLACE_LARGE_SCENERY,
                        parameter_3,
                        gSceneryPlaceZ);
                    mapElement = gSceneryMapElement;
                    if (cost != MONEY32_UNDEFINED)
                    {
                        game_do_command(
                            pos.x,
                            105 | (pos.direction << 8),
                            pos.y,
                            mapElement->base_height,
                            GAME_COMMAND_REMOVE_LARGE_SCENERY,
                            0,
                            0);
                    }
                    break;
                default:
                    cost = 0;
                    break;
            }
            if (cost != MONEY32_UNDEFINED)
                break;
            gSceneryPlaceZ += 8;
        }
        if (cost == MONEY32_UNDEFINED) {
            return cost;
        }
    }
    cost = 0;
    mapElement = nullptr;
    //specific handling required for each shape
    if (gSceneryShape == SCENERY_SHAPE_HOLLOW )
    {
        index = 0;
        for (pos.x = map_tile.x; pos.x <= map_tile2.x; pos.x += 32) {
            for (pos.y = map_tile.y; pos.y <= map_tile2.y; pos.y += 32) {
                if (index >= SCENERY_GHOST_LIST_SIZE) {
                    gSceneryCannotDisplay = true;
                    return MONEY32_UNDEFINED;
                }
                side1 = abs(pos.x - map_tile.x) < 32;
                side2 = abs(pos.x - map_tile2.x) < 32;
                side3 = abs(pos.y - map_tile.y) < 32;
                side4 = abs(pos.y - map_tile2.y) < 32;
                //check for edges
                if (side1|| side2|| side3 || side4)
                {
                    position_list[index].x = pos.x;
                    position_list[index].y = pos.y;

                    //check for direction
                    if (side1) position_list[index].direction = 0;
                    else if (side2) position_list[index].direction = 2;
                    else if (side3) position_list[index].direction = 3;
                    else if (side4) position_list[index].direction = 1;

                    if (index + 1 < SCENERY_GHOST_LIST_SIZE) {                        
                        if (side1 && side3) {
                            index++;
                            position_list[index].x = pos.x;
                            position_list[index].y = pos.y;
                            position_list[index].direction = 3;
                        } 
                        else if (side1 && side4) {
                            index++;
                            position_list[index].x = pos.x;
                            position_list[index].y = pos.y;
                            position_list[index].direction = 1;
                        } 
                        else if (side2 && side3) {
                            index++;
                            position_list[index].x = pos.x;
                            position_list[index].y = pos.y;
                            position_list[index].direction = 3;
                        }
                        else if (side2 && side4) {
                            index++;
                            position_list[index].x = pos.x;
                            position_list[index].y = pos.y;
                            position_list[index].direction = 1;
                        }
                    }
                    index++;
                }
            }
        }
    }
    else {
        index = 0;
        for (pos.x = map_tile.x; pos.x <= map_tile2.x; pos.x += 32) {
            for (pos.y = map_tile.y; pos.y <= map_tile2.y; pos.y += 32) {
                if (index >= SCENERY_GHOST_LIST_SIZE) {
                    gSceneryCannotDisplay = true;
                    return MONEY32_UNDEFINED;
                }
                position_list[index].x = pos.x;
                position_list[index].y = pos.y;
                position_list[index].direction = parameter_2 & 0xFF;
                index++;
            }
        }
    }

    //save last attempt
    gSceneryLastGhost.posA.x = map_tile.x;
    gSceneryLastGhost.posA.y = map_tile.y;
    gSceneryLastGhost.posA.z = gSceneryPlaceZ;
    gSceneryLastGhost.posB.x = map_tile2.x;
    gSceneryLastGhost.posB.y = map_tile2.y;
    gSceneryLastGhost.parameter_1 = parameter_1;
    gSceneryLastGhost.parameter_2 = parameter_2;
    gSceneryLastGhost.parameter_3 = parameter_3;
    gSceneryLastGhost.selected_tab = selected_tab;
    //gSceneryLastGhost.rotation = parameter_2 & 0xFF;
    //gSceneryLastGhost.type = scenery_type;
    //gSceneryLastGhost.place_object = parameter_3 & 0xFFFF;

    //attempt to place all from temp list
    for (; index!= 0; index--) {
        pos = position_list[index-1];
        switch (scenery_type) {
            case SCENERY_TYPE_SMALL:
            {
                gSceneryMapElement = nullptr;
                uint8 rotation = 0;
                if (scenery) {
                    window_top_toolbar_scenery_get_scenery_rotation(scenery, &rotation);
                    parameter_3 = parameter_3 & 0xFFFFFF00 | rotation;
                }
                cost = game_do_command(
                    pos.x,
                    parameter_1 | 0x69,
                    pos.y,
                    parameter_2,
                    GAME_COMMAND_PLACE_SCENERY,
                    parameter_3,
                    gSceneryPlaceZ);
                mapElement = gSceneryMapElement;
                if (!mapElement) {
                    cost = MONEY32_UNDEFINED;
                    break;
                }
                if (cost != MONEY32_UNDEFINED) {
                    gSceneryLastIndex++;
                    gSceneryGhost[ghost_index].type |= (1 << 0);
                    gSceneryGhost[ghost_index].position.x = pos.x;
                    gSceneryGhost[ghost_index].position.y = pos.y;
                    //gSceneryPlaceRotation = (uint8)(parameter_3 & 0xFF);
                    gSceneryGhost[ghost_index].placing_height = gSceneryPlaceZ;
                    gSceneryGhost[ghost_index].position.z = mapElement->base_height;
                    gSceneryGhost[ghost_index].element_type = mapElement->type;
                    gSceneryGhost[ghost_index].element = mapElement;
                    gSceneryGhost[ghost_index].orientation = (parameter_2 & 0xFF);
                    gSceneryGhost[ghost_index].rotation = parameter_3 & 0xFF;
                    //gSceneryPlaceObject = selected_tab;
                    underground |= gSceneryGroundFlags;
                }
            }
            break;
            case SCENERY_TYPE_PATH_ITEM:
                // Path Bits
                //6e265b
                cost = game_do_command(
                    pos.x,
                    (parameter_1 & 0xFF00) | (
                        GAME_COMMAND_FLAG_APPLY |
                        GAME_COMMAND_FLAG_ALLOW_DURING_PAUSED |
                        GAME_COMMAND_FLAG_5 |
                        GAME_COMMAND_FLAG_GHOST |
                        GAME_COMMAND_FLAG_PATH_SCENERY),
                    pos.y,
                    parameter_2,
                    GAME_COMMAND_PLACE_PATH,
                    parameter_3,
                    0);
                if (cost != MONEY32_UNDEFINED) {
                    gSceneryLastIndex++;
                    gSceneryGhost[ghost_index].type |= (1 << 1);
                    gSceneryGhost[ghost_index].position.x = pos.x;
                    gSceneryGhost[ghost_index].position.y = pos.y;
                    gSceneryGhost[ghost_index].position.z = (parameter_2 & 0xFF);
                    gSceneryPlacePathSlope = ((parameter_1 >> 8) & 0xFF);
                    gSceneryPlacePathType = ((parameter_2 >> 8) & 0xFF);
                    gSceneryGhost[ghost_index].path_type = parameter_3;
                }
            break;
            case SCENERY_TYPE_WALL:
                // Walls
                //6e26b0
                gSceneryMapElement = nullptr;
                cost = game_do_command(
                    pos.x,
                    parameter_1 | 0x69,
                    pos.y,
                    (parameter_2 & 0xFFFFFF00 | pos.direction),
                    GAME_COMMAND_PLACE_WALL,
                    gSceneryPlaceZ,
                    _unkF64F15);
                mapElement = gSceneryMapElement;
                if (!mapElement) {
                    cost = MONEY32_UNDEFINED;
                    break;
                }
                if (cost != MONEY32_UNDEFINED) {
                    gSceneryLastIndex++;
                    gSceneryGhost[ghost_index].type |= (1 << 2);
                    gSceneryGhost[ghost_index].position.x = pos.x;
                    gSceneryGhost[ghost_index].position.y = pos.y;
                    gSceneryGhost[ghost_index].rotation = pos.direction;
                    gSceneryGhost[ghost_index].element = mapElement;
                    gSceneryGhost[ghost_index].placing_height = gSceneryPlaceZ;
                    gSceneryGhost[ghost_index].element_type = mapElement->type;
                    gSceneryGhost[ghost_index].position.z = mapElement->base_height;
                }
                
            break;
            case SCENERY_TYPE_LARGE:
                // Large Scenery
                //6e25a7
                gSceneryMapElement = nullptr;
                cost = game_do_command(
                    pos.x,
                    parameter_1 | 0x69,
                    pos.y,
                    parameter_2,
                    GAME_COMMAND_PLACE_LARGE_SCENERY,
                    parameter_3,
                    gSceneryPlaceZ);
                mapElement = gSceneryMapElement;
                if (!mapElement) {
                    cost = MONEY32_UNDEFINED;
                    break;
                }
                if (cost != MONEY32_UNDEFINED) {
                    gSceneryLastIndex++;
                    gSceneryGhost[ghost_index].element = mapElement;
                    gSceneryGhost[ghost_index].type |= (1 << 3);
                    gSceneryGhost[ghost_index].position.x = pos.x;
                    gSceneryGhost[ghost_index].position.y = pos.y;
                    gSceneryGhost[ghost_index].placing_height = gSceneryPlaceZ;
                    //gSceneryPlaceRotation = ((parameter_1 >> 8) & 0xFF);
                    gSceneryGhost[ghost_index].orientation = ((parameter_1 >> 8) & 0xFF);
                    gSceneryGhost[ghost_index].position.z = mapElement->base_height;
                    underground |= gSceneryGroundFlags;
                }
            break;
            case SCENERY_TYPE_BANNER:
                // Banners
                //6e2612
                cost = game_do_command(
                    pos.x,
                    parameter_1 | 0x69,
                    pos.y,
                    parameter_2,
                    GAME_COMMAND_PLACE_BANNER,
                    parameter_3,
                    0);
                if (cost != MONEY32_UNDEFINED) {
                    gSceneryLastIndex++;
                    gSceneryGhost[ghost_index].type |= (1 << 4);
                    gSceneryGhost[ghost_index].position.x = pos.x;
                    gSceneryGhost[ghost_index].position.y = pos.y;
                    gSceneryGhost[ghost_index].position.z = (parameter_2 & 0xFF) * 2 + 2;
                    gSceneryGhost[ghost_index].orientation = ((parameter_2 >> 8) & 0xFF);
                    //gSceneryPlaceRotation = ((parameter_2 >> 8) & 0xFF);
                }
            break;
        }
        if (cost != MONEY32_UNDEFINED)
        {
            total_cost += cost;
            ghost_index++;
        }
    }
    if (total_cost == 0)
        total_cost = cost;

    if (underground & ELEMENT_IS_UNDERGROUND) {
        //Set underground on
        viewport_set_visibility(4);
    }
    else {
        //Set underground off
        viewport_set_visibility(5);
    }

   /* bool enc = false;
    for (sint16 i = 0; i < SCENERY_GHOST_LIST_SIZE; i++)
    {
        if (!gSceneryGhost[i].type) enc = true;
        if (gSceneryGhost[i].type && enc)
        {
            enc = true;
        }
    }*/

    return total_cost;
}

/**
*
*  rct2: 0x006E287B
*/
//----------------------------------------------------------------------------
// General refactor concept allowing multiple ghost scenery elements:
// 1. Firstly calculate switches basing on user input so we know how to
// format the area and how big it should be.
// 2. Proceed the shape to obtain the list of points with x and y coords we 
// need to check
// 3. Try to insert the ghost according to height rules in choosen points
// 5. Update only if general settings or last point (mouseover) have changed
// 6. If clicked attempt to insert all elements based on list of ghosts
// 7. DO not change the rules made during ghost placement, as it might
// lead to desync in mp game
//-----------------------------------------------------------------------------
static void top_toolbar_tool_update_scenery(sint16 x, sint16 y){

    //invalidate all old markings, we are proceeding from scratch
    map_invalidate_selection_rect();
    map_invalidate_map_selection_tiles();

    gMapSelectFlags &= ~MAP_SELECT_FLAG_ENABLE;
    gMapSelectFlags &= ~MAP_SELECT_FLAG_ENABLE_CONSTRUCT;

    if (gWindowSceneryPaintEnabled)
        return;
    if (gWindowSceneryEyedropperEnabled)
        return;

    sint16 selected_tab = gWindowSceneryTabSelections[gWindowSceneryActiveTabIndex];

    if (selected_tab == -1){
        scenery_remove_ghost_tool_placement(false);
        return;
    }
    if (selected_tab == -1) {
        scenery_remove_ghost_tool_placement(false);
        gSceneryCannotDisplay = false;
    }

    uint8 scenery_type = (selected_tab & 0xFF00) >> 8;
    uint8 selected_scenery = selected_tab & 0xFF;
    rct_xy16 mapTile = { 0 };
    uint32 parameter1, parameter2, parameter3;
    scenery_key_action key_action = SCENERY_KEY_ACTION_NONE;
    scenery_key_shape key_shape = SCENERY_SHAPE_POINT;
    uint16 rotation_type = 0;

    window_top_toolbar_scenery_process_keypad(x, y, selected_tab, &key_action);
    window_top_toolbar_scenery_get_tile_params(x, y, selected_tab, &mapTile.x, &mapTile.y, key_action, &parameter1, &parameter2, &parameter3);

    if (mapTile.x == MAP_LOCATION_NULL) {
        scenery_remove_ghost_tool_placement(false);
        return;
    }
    if (!gSceneryLastIndex) {
        gSceneryLastIndex = 1;
    }

    rct_scenery_entry* scenery;
    money32 cost = 0;
    sint16 swap = 0;
    rct_xy16 PositionA = { 0, 0 };
    rct_xy16 PositionB = { 0, 0 };
    
    //place the scenery
    switch (scenery_type){
        case SCENERY_TYPE_SMALL:
            //check selection type
            gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE;
            gMapSelectType = MAP_SELECT_TYPE_FULL;
            scenery = get_small_scenery_entry(selected_scenery);
            if (!(scenery->small_scenery.flags & SMALL_SCENERY_FLAG_FULL_TILE) && !gWindowSceneryClusterEnabled) {
                gMapSelectType = MAP_SELECT_TYPE_QUARTER_0 + ((parameter2 & 0xFF) ^ 2);
            }
            //check shape and border points
            if (key_action >= SCENERY_KEY_ACTION_DRAG)
            {
                key_shape = SCENERY_SHAPE_RECT;
                PositionA.x = gSceneryDrag.x;
                PositionA.y = gSceneryDrag.y;
            }
            else
            {
                key_shape = SCENERY_SHAPE_POINT;
                PositionA.x = mapTile.x;
                PositionA.y = mapTile.y;
            }               
            PositionB.x = mapTile.x;
            PositionB.y = mapTile.y;

            //correct border rotation
            if (PositionB.x < PositionA.x) {
                swap = PositionB.x;
                PositionB.x = PositionA.x;
                PositionA.x = swap;
            //correct border rotation
            }
            if (PositionB.y < PositionA.y) {
                swap = PositionB.y;
                PositionB.y = PositionA.y;
                PositionA.y = swap;
            }

            //cluster breaks selection, use temp values
            gMapSelectPositionA = PositionA;
            gMapSelectPositionB = PositionB;

            if (gWindowSceneryClusterEnabled && scenery_type == SCENERY_TYPE_SMALL) {
                gMapSelectPositionA.x = mapTile.x - (8 << 5);
                gMapSelectPositionA.y = mapTile.y - (8 << 5);
                gMapSelectPositionB.x = mapTile.x + (7 << 5);
                gMapSelectPositionB.y = mapTile.y + (7 << 5);
                gMapSelectType = MAP_SELECT_TYPE_FULL;
            }

            map_invalidate_selection_rect();

            // If no change in ghost placement
            if (gSceneryLastGhost.posA.x == PositionA.x &&
                gSceneryLastGhost.posB.x == PositionB.x &&
                gSceneryLastGhost.posA.y == PositionA.y &&
                gSceneryLastGhost.posB.y == PositionB.y &&
                gSceneryLastGhost.selected_tab == selected_tab &&
                gSceneryLastGhost.parameter_1 == parameter1 &&
                gSceneryLastGhost.parameter_2 == parameter2 &&
                gSceneryLastGhost.parameter_3 == parameter3 &&
                gSceneryLastGhost.posA.z == gSceneryPlaceZ )
                //gSceneryLastGhost.rotation == parameter2 & 0xFF &&
            { 
                return;
            }
            gSceneryShape = key_shape;
            scenery_remove_ghost_tool_placement(false);
            cost = try_place_ghost_scenery(
                PositionA,
                PositionB,
                mapTile,
                parameter1,
                parameter2,
                parameter3,
                selected_tab);

            gSceneryPlaceCost = cost;
        break;
        case SCENERY_TYPE_PATH_ITEM:
            //check selection type
            gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE;
            key_shape = SCENERY_SHAPE_POINT;
            gMapSelectType = MAP_SELECT_TYPE_FULL;
            //check shape and border points
            PositionA.x = mapTile.x;
            PositionA.y = mapTile.y;
            PositionB.x = mapTile.x;
            PositionB.y = mapTile.y;

            gMapSelectPositionA = PositionA;
            gMapSelectPositionB = PositionB;

            map_invalidate_selection_rect();
            // If no change in ghost placement
            if (gSceneryLastGhost.posA.x == PositionA.x &&
                gSceneryLastGhost.posB.x == PositionB.x &&
                gSceneryLastGhost.posA.y == PositionA.y &&
                gSceneryLastGhost.posB.y == PositionB.y &&
                gSceneryLastGhost.posA.z == gSceneryPlaceZ &&
                gSceneryLastGhost.selected_tab == selected_tab &&
                gSceneryLastGhost.parameter_1 == parameter1 &&
                gSceneryLastGhost.parameter_2 == parameter2 &&
                gSceneryLastGhost.parameter_3 == parameter3)
                //gSceneryLastGhost.place_object == selected_tab &&
                //gSceneryLastGhost.rotation == parameter2 & 0xFF &&
            {
                return;
            }
            gSceneryShape = key_shape;
            scenery_remove_ghost_tool_placement(false);
            cost = try_place_ghost_scenery(
                mapTile,
                mapTile,
                mapTile,
                parameter1,
                parameter2,
                parameter3,
                selected_tab);
            gSceneryPlaceCost = cost;
        break;
        case SCENERY_TYPE_WALL:            
            gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE;
            gMapSelectType = MAP_SELECT_TYPE_EDGE_0 + (parameter2 & 0xFF);
            PositionA.x = gSceneryDrag.x;
            PositionA.y = gSceneryDrag.y;
            PositionB.x = mapTile.x;
            PositionB.y = mapTile.y;

            if (key_action >= SCENERY_KEY_ACTION_DRAG) {
                gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE;
                key_shape = SCENERY_SHAPE_HOLLOW;
                gMapSelectType = MAP_SELECT_TYPE_FULL;
                parameter2 = parameter2 & 0xFFFFFF00 | gSceneryDrag.rotation;

                //check if this is a line
                if ((PositionA.x == PositionB.x) ^
                    (PositionA.y == PositionB.y))
                {
                    key_shape = SCENERY_SHAPE_LINE;
                    sint16 draglenX = 0;
                    sint16 draglenY = 0;

                    PositionA.x = gSceneryDrag.x;
                    PositionA.y = gSceneryDrag.y;

                    rotation_type = MAP_SELECT_TYPE_EDGE_0 + gSceneryDrag.rotation;
                    switch (rotation_type)
                    {
                        case MAP_SELECT_TYPE_EDGE_0:
                        case MAP_SELECT_TYPE_EDGE_2:
                            draglenY = (mapTile.y - gSceneryDrag.y) / 32;
                            //if (draglenY > 0) draglenY++; //correction
                            PositionB.y = gSceneryDrag.y + draglenY * 32;
                            PositionB.x = gSceneryDrag.x;
                            draglenX = 0;
                            break;
                        case MAP_SELECT_TYPE_EDGE_1:
                        case MAP_SELECT_TYPE_EDGE_3:
                            draglenX = (mapTile.x - gSceneryDrag.x) / 32;
                            //if (draglenX > 0) draglenX++; //correction
                            PositionB.x = gSceneryDrag.x + draglenX * 32;
                            PositionB.y = gSceneryDrag.y;
                            draglenY = 0;
                            break;
                        default:
                            return;
                    }
                }
            }
            else
            {
                key_shape = SCENERY_SHAPE_POINT;
                PositionA.x = mapTile.x;
                PositionA.y = mapTile.y;
                PositionB.x = mapTile.x;
                PositionB.y = mapTile.y;
            }

            //correct border rotation
            if (PositionB.x < PositionA.x) {
                swap = PositionB.x;
                PositionB.x = PositionA.x;
                PositionA.x = swap;
            //correct border rotation
            }
            if (PositionB.y < PositionA.y) {
                swap = PositionB.y;
                PositionB.y = PositionA.y;
                PositionA.y = swap;
            }

            gMapSelectPositionA = PositionA;
            gMapSelectPositionB = PositionB;
            map_invalidate_selection_rect();
            // If no change in ghost placement
            if (gSceneryLastGhost.posA.x == PositionA.x &&
                gSceneryLastGhost.posB.x == PositionB.x &&
                gSceneryLastGhost.posA.y == PositionA.y &&
                gSceneryLastGhost.posB.y == PositionB.y &&
                gSceneryLastGhost.selected_tab == selected_tab &&
                gSceneryLastGhost.parameter_1 == parameter1 &&
                gSceneryLastGhost.parameter_2 == parameter2 &&
                gSceneryLastGhost.parameter_3 == parameter3 &&
                gSceneryLastGhost.posA.z == gSceneryPlaceZ)
            {
                return;
            }
            gSceneryShape = key_shape;
            scenery_remove_ghost_tool_placement(false);
            cost = 0;
            cost = try_place_ghost_scenery(
                PositionA,
                PositionB,
                mapTile,
                parameter1,
                parameter2,
                parameter3,
                selected_tab);
        
            gSceneryPlaceCost = cost;
        break;
        case SCENERY_TYPE_LARGE:
        {
            //check selection type
            key_shape = SCENERY_SHAPE_POINT;
            gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE_CONSTRUCT;
            gMapSelectType = MAP_SELECT_TYPE_FULL;
            //check shape and border points
            PositionA.x = mapTile.x;
            PositionA.y = mapTile.y;
            PositionB.x = mapTile.x;
            PositionB.y = mapTile.y;
            //selection
           
            scenery = get_large_scenery_entry(selected_scenery);
            rct_xy16* selectedTile = gMapSelectionTiles;

            for (rct_large_scenery_tile* tile = scenery->large_scenery.tiles; tile->x_offset != (sint16)(uint16)0xFFFF; tile++) {
                rct_xy16 tileLocation = { tile->x_offset, tile->y_offset };

                rotate_map_coordinates(&tileLocation.x, &tileLocation.y, (parameter1 >> 8) & 0xFF);

                tileLocation.x += mapTile.x;
                tileLocation.y += mapTile.y;

                selectedTile->x = tileLocation.x;
                selectedTile->y = tileLocation.y;
                selectedTile++;
            }

            selectedTile->x = -1;

            map_invalidate_map_selection_tiles();
            // If no change in ghost placement
            if (gSceneryLastGhost.posA.x == PositionA.x &&
                gSceneryLastGhost.posB.x == PositionB.x &&
                gSceneryLastGhost.posA.y == PositionA.y &&
                gSceneryLastGhost.posB.y == PositionB.y &&
                gSceneryLastGhost.selected_tab == selected_tab &&
                gSceneryLastGhost.parameter_1 == parameter1 &&
                gSceneryLastGhost.parameter_2 == parameter2 &&
                gSceneryLastGhost.parameter_3 == parameter3 &&
                gSceneryLastGhost.posA.z == gSceneryPlaceZ)
            {
                return;
            }
            gSceneryShape = key_shape;
            scenery_remove_ghost_tool_placement(false);
            cost = 0;
            cost = try_place_ghost_scenery(
                mapTile,
                mapTile,
                mapTile,
                parameter1,
                parameter2,
                parameter3,
                selected_tab);
            gSceneryPlaceCost = cost;
        }
        break;
        
        case SCENERY_TYPE_BANNER:
            gMapSelectFlags |= MAP_SELECT_FLAG_ENABLE;
            key_shape = SCENERY_SHAPE_POINT;
            gMapSelectType = MAP_SELECT_TYPE_FULL;

            PositionA.x = mapTile.x;
            PositionA.y = mapTile.y;
            PositionB.x = mapTile.x;
            PositionB.y = mapTile.y;

            gMapSelectPositionA = PositionA;
            gMapSelectPositionB = PositionB;

            map_invalidate_selection_rect();
            // If no change in ghost placement
            if (gSceneryLastGhost.posA.x == PositionA.x &&
                gSceneryLastGhost.posB.x == PositionB.x &&
                gSceneryLastGhost.posA.y == PositionA.y &&
                gSceneryLastGhost.posB.y == PositionB.y &&
                gSceneryLastGhost.parameter_1 == parameter1 &&
                gSceneryLastGhost.parameter_2 == parameter2 &&
                gSceneryLastGhost.parameter_3 == parameter3 &&
                gSceneryLastGhost.selected_tab == selected_tab)
            {
                return;
            }
            gSceneryShape = key_shape;
            scenery_remove_ghost_tool_placement(false);
            cost = try_place_ghost_scenery(
                mapTile,
                mapTile,
                mapTile,
                parameter1,
                parameter2,
                parameter3,
                selected_tab);

            gSceneryPlaceCost = cost;
        break;
    }
}

/**
 *
 *  rct2: 0x0066CB25
 */
static void window_top_toolbar_tool_update(rct_window* w, rct_widgetindex widgetIndex, sint32 x, sint32 y)
{
    switch (widgetIndex) {
    case WIDX_CLEAR_SCENERY:
        top_toolbar_tool_update_scenery_clear(x, y);
        break;
    case WIDX_LAND:
        if (gLandPaintMode)
            top_toolbar_tool_update_land_paint(x, y);
        else
            top_toolbar_tool_update_land(x, y);
        break;
    case WIDX_WATER:
        top_toolbar_tool_update_water(x, y);
        break;
    case WIDX_SCENERY:
        top_toolbar_tool_update_scenery(x, y);
        break;
    }
}

/**
 *
 *  rct2: 0x0066CB73
 */
static void window_top_toolbar_tool_down(rct_window* w, rct_widgetindex widgetIndex, sint32 x, sint32 y)
{
    switch (widgetIndex){
    case WIDX_CLEAR_SCENERY:
        if (!(gMapSelectFlags & MAP_SELECT_FLAG_ENABLE))
            break;

        gGameCommandErrorTitle = STR_UNABLE_TO_REMOVE_ALL_SCENERY_FROM_HERE;

        game_do_command(
            gMapSelectPositionA.x,
            1,
            gMapSelectPositionA.y,
            ((gClearSmallScenery ? 1 : 0) | (gClearLargeScenery ? 1 : 0) << 1 | (gClearFootpath ? 1 : 0) << 2),
            GAME_COMMAND_CLEAR_SCENERY,
            gMapSelectPositionB.x,
            gMapSelectPositionB.y
            );
        gCurrentToolId = TOOL_CROSSHAIR;
        break;
    case WIDX_LAND:
        if (gMapSelectFlags & MAP_SELECT_FLAG_ENABLE) {
            gGameCommandErrorTitle = STR_CANT_CHANGE_LAND_TYPE;
            game_do_command(
                gMapSelectPositionA.x,
                1,
                gMapSelectPositionA.y,
                gLandToolTerrainSurface | (gLandToolTerrainEdge << 8),
                GAME_COMMAND_CHANGE_SURFACE_STYLE,
                gMapSelectPositionB.x,
                gMapSelectPositionB.y
                );
            gCurrentToolId = TOOL_UP_DOWN_ARROW;
        }
        break;
    case WIDX_WATER:
        if (gMapSelectFlags & MAP_SELECT_FLAG_ENABLE) {
            gCurrentToolId = TOOL_UP_DOWN_ARROW;
        }
        break;
    case WIDX_SCENERY:
        window_top_toolbar_scenery_tool_down(x, y, w, widgetIndex);
        break;
    }
}

/**
*
*  rct2: 0x006644DD
*/
money32 selection_raise_land(uint8 flags)
{
    sint32 centreX = (gMapSelectPositionA.x + gMapSelectPositionB.x) / 2;
    sint32 centreY = (gMapSelectPositionA.y + gMapSelectPositionB.y) / 2;
    centreX += 16;
    centreY += 16;

    uint32 xBounds = (gMapSelectPositionA.x & 0xFFFF) | (gMapSelectPositionB.x << 16);
    uint32 yBounds = (gMapSelectPositionA.y & 0xFFFF) | (gMapSelectPositionB.y << 16);

    gGameCommandErrorTitle = STR_CANT_RAISE_LAND_HERE;
    if (gLandMountainMode) {
        return game_do_command(centreX, flags, centreY, xBounds, GAME_COMMAND_EDIT_LAND_SMOOTH, gMapSelectType, yBounds);
    } else {
        return game_do_command(centreX, flags, centreY, xBounds, GAME_COMMAND_RAISE_LAND, gMapSelectType, yBounds);
    }
}

/**
*
*  rct2: 0x006645B3
*/
money32 selection_lower_land(uint8 flags)
{
    sint32 centreX = (gMapSelectPositionA.x + gMapSelectPositionB.x) / 2;
    sint32 centreY = (gMapSelectPositionA.y + gMapSelectPositionB.y) / 2;
    centreX += 16;
    centreY += 16;

    uint32 xBounds = (gMapSelectPositionA.x & 0xFFFF) | (gMapSelectPositionB.x << 16);
    uint32 yBounds = (gMapSelectPositionA.y & 0xFFFF) | (gMapSelectPositionB.y << 16);

    gGameCommandErrorTitle = STR_CANT_LOWER_LAND_HERE;
    if (gLandMountainMode) {
        return game_do_command(centreX, flags, centreY, xBounds, GAME_COMMAND_EDIT_LAND_SMOOTH, 0x8000+gMapSelectType, yBounds);
    } else {
        return game_do_command(centreX, flags, centreY, xBounds, GAME_COMMAND_LOWER_LAND, gMapSelectType, yBounds);
    }
}

/**
*  part of window_top_toolbar_tool_drag(0x0066CB4E)
*  rct2: 0x00664454
*/
static void window_top_toolbar_land_tool_drag(sint16 x, sint16 y)
{
    rct_window *window = window_find_from_point(x, y);
    if (!window)
        return;
    rct_widgetindex widget_index = window_find_widget_from_point(window, x, y);
    if (widget_index == -1)
        return;
    rct_widget *widget = &window->widgets[widget_index];
    if (widget->type != WWT_VIEWPORT)
        return;
    rct_viewport *viewport = window->viewport;
    if (!viewport)
        return;

    sint16 tile_height = -16 / (1 << viewport->zoom);

    sint32 y_diff = y - gInputDragLastY;

    if (y_diff <= tile_height) {
        gInputDragLastY += tile_height;

        selection_raise_land(GAME_COMMAND_FLAG_APPLY);

        gLandToolRaiseCost = MONEY32_UNDEFINED;
        gLandToolLowerCost = MONEY32_UNDEFINED;
    } else if (y_diff >= -tile_height) {
        gInputDragLastY -= tile_height;

        selection_lower_land(GAME_COMMAND_FLAG_APPLY);  

        gLandToolRaiseCost = MONEY32_UNDEFINED;
        gLandToolLowerCost = MONEY32_UNDEFINED;
    }
}

/**
*  part of window_top_toolbar_tool_drag(0x0066CB4E)
*  rct2: 0x006E6D4B
*/
static void window_top_toolbar_water_tool_drag(sint16 x, sint16 y)
{
    rct_window *window = window_find_from_point(x, y);
    if (!window)
        return;
    rct_widgetindex widget_index = window_find_widget_from_point(window, x, y);
    if (widget_index == -1)
        return;
    rct_widget *widget = &window->widgets[widget_index];
    if (widget->type != WWT_VIEWPORT)
        return;
    rct_viewport *viewport = window->viewport;
    if (!viewport)
        return;

    sint16 dx = -16;
    dx >>= viewport->zoom;

    y -= gInputDragLastY;

    if (y <= dx) {
        gInputDragLastY += dx;

        gGameCommandErrorTitle = STR_CANT_RAISE_WATER_LEVEL_HERE;

        game_do_command(
            gMapSelectPositionA.x,
            1,
            gMapSelectPositionA.y,
            dx,
            GAME_COMMAND_RAISE_WATER,
            gMapSelectPositionB.x,
            gMapSelectPositionB.y
            );
        gWaterToolRaiseCost = MONEY32_UNDEFINED;
        gWaterToolLowerCost = MONEY32_UNDEFINED;

        return;
    }

    dx = -dx;

    if (y >= dx) {
        gInputDragLastY += dx;

        gGameCommandErrorTitle = STR_CANT_LOWER_WATER_LEVEL_HERE;

        game_do_command(
            gMapSelectPositionA.x,
            1,
            gMapSelectPositionA.y,
            dx,
            GAME_COMMAND_LOWER_WATER,
            gMapSelectPositionB.x,
            gMapSelectPositionB.y
            );
        gWaterToolRaiseCost = MONEY32_UNDEFINED;
        gWaterToolLowerCost = MONEY32_UNDEFINED;

        return;
    }
}

/**
 *
 *  rct2: 0x0066CB4E
 */
static void window_top_toolbar_tool_drag(rct_window* w, rct_widgetindex widgetIndex, sint32 x, sint32 y)
{
    switch (widgetIndex){
    case WIDX_CLEAR_SCENERY:
        if (window_find_by_class(WC_ERROR) != nullptr)
            break;

        if (!(gMapSelectFlags & MAP_SELECT_FLAG_ENABLE))
            break;

        gGameCommandErrorTitle = STR_UNABLE_TO_REMOVE_ALL_SCENERY_FROM_HERE;

        game_do_command(
            gMapSelectPositionA.x,
            1,
            gMapSelectPositionA.y,
            ((gClearSmallScenery ? 1 : 0) | (gClearLargeScenery ? 1 : 0) << 1 | (gClearFootpath ? 1 : 0) << 2),
            GAME_COMMAND_CLEAR_SCENERY,
            gMapSelectPositionB.x,
            gMapSelectPositionB.y
        );
        gCurrentToolId = TOOL_CROSSHAIR;
        break;
    case WIDX_LAND:
        // Custom setting to only change land style instead of raising or lowering land
        if (gLandPaintMode) {
            if (gMapSelectFlags & MAP_SELECT_FLAG_ENABLE) {
                gGameCommandErrorTitle = STR_CANT_CHANGE_LAND_TYPE;
                game_do_command(
                    gMapSelectPositionA.x,
                    1,
                    gMapSelectPositionA.y,
                    gLandToolTerrainSurface | (gLandToolTerrainEdge << 8),
                    GAME_COMMAND_CHANGE_SURFACE_STYLE,
                    gMapSelectPositionB.x,
                    gMapSelectPositionB.y
                    );
                // The tool is set to 12 here instead of 3 so that the dragging cursor is not the elevation change cursor
                gCurrentToolId = TOOL_CROSSHAIR;
            }
        } else {
            window_top_toolbar_land_tool_drag(x, y);
        }
        break;
    case WIDX_WATER:
        window_top_toolbar_water_tool_drag(x, y);
        break;
    case WIDX_SCENERY:
        if (gWindowSceneryPaintEnabled & 1)
            window_top_toolbar_scenery_tool_down(x, y, w, widgetIndex);
        if (gWindowSceneryEyedropperEnabled)
            window_top_toolbar_scenery_tool_down(x, y, w, widgetIndex);
        break;
    }
}

/**
 *
 *  rct2: 0x0066CC5B
 */
static void window_top_toolbar_tool_up(rct_window* w, rct_widgetindex widgetIndex, sint32 x, sint32 y)
{
    switch (widgetIndex) {
    case WIDX_LAND:
        map_invalidate_selection_rect();
        gMapSelectFlags &= ~MAP_SELECT_FLAG_ENABLE;
        gCurrentToolId = TOOL_DIG_DOWN;
        break;
    case WIDX_WATER:
        map_invalidate_selection_rect();
        gMapSelectFlags &= ~MAP_SELECT_FLAG_ENABLE;
        gCurrentToolId = TOOL_WATER_DOWN;
        break;
    case WIDX_CLEAR_SCENERY:
        map_invalidate_selection_rect();
        gMapSelectFlags &= ~MAP_SELECT_FLAG_ENABLE;
        gCurrentToolId = TOOL_CROSSHAIR;
        break;
    }
}

/**
 *
 *  rct2: 0x0066CA58
 */
static void window_top_toolbar_tool_abort(rct_window *w, rct_widgetindex widgetIndex)
{
    switch (widgetIndex) {
    case WIDX_LAND:
    case WIDX_WATER:
    case WIDX_CLEAR_SCENERY:
        hide_gridlines();
        break;
    }
}

void top_toolbar_init_fastforward_menu(rct_window* w, rct_widget* widget) {
    sint32 num_items = 4;
    gDropdownItemsFormat[0] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[1] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[2] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[3] = STR_TOGGLE_OPTION;
    if (gConfigGeneral.debugging_tools) {
        gDropdownItemsFormat[4] = STR_EMPTY;
        gDropdownItemsFormat[5] = STR_TOGGLE_OPTION;
        gDropdownItemsArgs[5] = STR_SPEED_HYPER;
        num_items = 6;
    }

    gDropdownItemsArgs[0] = STR_SPEED_NORMAL;
    gDropdownItemsArgs[1] = STR_SPEED_QUICK;
    gDropdownItemsArgs[2] = STR_SPEED_FAST;
    gDropdownItemsArgs[3] = STR_SPEED_TURBO;


    window_dropdown_show_text(
        w->x + widget->left,
        w->y + widget->top,
        widget->bottom - widget->top + 1,
        w->colours[0] | 0x80,
        0,
        num_items
        );

    // Set checkmarks
    if (gGameSpeed <= 4) {
        dropdown_set_checked(gGameSpeed - 1, true);
    }
    if (gGameSpeed == 8) {
        dropdown_set_checked(5, true);
    }

    if (gConfigGeneral.debugging_tools) {
        gDropdownDefaultIndex = (gGameSpeed == 8 ? 0 : gGameSpeed);
    } else {
        gDropdownDefaultIndex = (gGameSpeed >= 4 ? 0 : gGameSpeed);
    }
    if (gDropdownDefaultIndex == 4) {
        gDropdownDefaultIndex = 5;
    }
}

void top_toolbar_fastforward_menu_dropdown(sint16 dropdownIndex)
{
    rct_window* w = window_get_main();
    if (w) {
        if (dropdownIndex >= 0 && dropdownIndex <= 5) {
            gGameSpeed = dropdownIndex + 1;
            if (gGameSpeed >= 5)
                gGameSpeed = 8;
            window_invalidate(w);
        }
    }
}

void top_toolbar_init_rotate_menu(rct_window* w, rct_widget* widget)
{
    gDropdownItemsFormat[0] = STR_ROTATE_CLOCKWISE;
    gDropdownItemsFormat[1] = STR_ROTATE_ANTI_CLOCKWISE;

    window_dropdown_show_text(
        w->x + widget->left,
        w->y + widget->top,
        widget->bottom - widget->top + 1,
        w->colours[1] | 0x80,
        0,
        2
    );

    gDropdownDefaultIndex = DDIDX_ROTATE_CLOCKWISE;
}

void top_toolbar_rotate_menu_dropdown(sint16 dropdownIndex)
{
    rct_window* w = window_get_main();
    if (w) {
        if (dropdownIndex == 0) {
            window_rotate_camera(w, 1);
            window_invalidate(w);
        }
        else if (dropdownIndex == 1){
            window_rotate_camera(w, -1);
            window_invalidate(w);
        }
    }
}

void top_toolbar_init_debug_menu(rct_window* w, rct_widget* widget)
{
    gDropdownItemsFormat[DDIDX_CONSOLE] = STR_TOGGLE_OPTION;
    gDropdownItemsArgs[DDIDX_CONSOLE] = STR_DEBUG_DROPDOWN_CONSOLE;
    gDropdownItemsFormat[DDIDX_TILE_INSPECTOR] = STR_TOGGLE_OPTION;
    gDropdownItemsArgs[DDIDX_TILE_INSPECTOR] = STR_DEBUG_DROPDOWN_TILE_INSPECTOR;
    gDropdownItemsFormat[DDIDX_OBJECT_SELECTION] = STR_TOGGLE_OPTION;
    gDropdownItemsArgs[DDIDX_OBJECT_SELECTION] = STR_DEBUG_DROPDOWN_OBJECT_SELECTION;
    gDropdownItemsFormat[DDIDX_INVENTIONS_LIST] = STR_TOGGLE_OPTION;
    gDropdownItemsArgs[DDIDX_INVENTIONS_LIST] = STR_DEBUG_DROPDOWN_INVENTIONS_LIST;
    gDropdownItemsFormat[DDIDX_SCENARIO_OPTIONS] = STR_TOGGLE_OPTION;
    gDropdownItemsArgs[DDIDX_SCENARIO_OPTIONS] = STR_DEBUG_DROPDOWN_SCENARIO_OPTIONS;
    gDropdownItemsFormat[DDIDX_DEBUG_PAINT] = STR_TOGGLE_OPTION;
    gDropdownItemsArgs[DDIDX_DEBUG_PAINT] = STR_DEBUG_DROPDOWN_DEBUG_PAINT;

    window_dropdown_show_text(
        w->x + widget->left,
        w->y + widget->top,
        widget->bottom - widget->top + 1,
        w->colours[0] | 0x80,
        DROPDOWN_FLAG_STAY_OPEN,
        TOP_TOOLBAR_DEBUG_COUNT
    );

    // Disable items that are not yet available in multiplayer
    if (network_get_mode() != NETWORK_MODE_NONE) {
        dropdown_set_disabled(DDIDX_OBJECT_SELECTION, true);
        dropdown_set_disabled(DDIDX_INVENTIONS_LIST, true);
    }

    dropdown_set_checked(DDIDX_DEBUG_PAINT, window_find_by_class(WC_DEBUG_PAINT) != nullptr);
    gDropdownDefaultIndex = DDIDX_CONSOLE;
}

void top_toolbar_init_network_menu(rct_window* w, rct_widget* widget)
{
    gDropdownItemsFormat[0] = STR_MULTIPLAYER;

    window_dropdown_show_text(
        w->x + widget->left,
        w->y + widget->top,
        widget->bottom - widget->top + 1,
        w->colours[0] | 0x80,
        0,
        1
    );

    gDropdownDefaultIndex = DDIDX_MULTIPLAYER;
}

void top_toolbar_debug_menu_dropdown(sint16 dropdownIndex)
{
    rct_window* w = window_get_main();
    if (w) {
        switch (dropdownIndex) {
        case DDIDX_CONSOLE:
            console_open();
            break;
        case DDIDX_TILE_INSPECTOR:
            window_tile_inspector_open();
            break;
        case DDIDX_OBJECT_SELECTION:
            window_close_all();
            window_editor_object_selection_open();
            break;
        case DDIDX_INVENTIONS_LIST:
            context_open_window(WC_EDITOR_INVENTION_LIST);
            break;
        case DDIDX_SCENARIO_OPTIONS:
            context_open_window(WC_EDITOR_SCENARIO_OPTIONS);
            break;
        case DDIDX_DEBUG_PAINT:
            if (window_find_by_class(WC_DEBUG_PAINT) == nullptr) {
                context_open_window(WC_DEBUG_PAINT);
            } else {
                window_close_by_class(WC_DEBUG_PAINT);
            }
            break;
        }
    }
}

void top_toolbar_network_menu_dropdown(sint16 dropdownIndex)
{
    rct_window* w = window_get_main();
    if (w) {
        switch (dropdownIndex) {
        case DDIDX_MULTIPLAYER:
            context_open_window(WC_MULTIPLAYER);
            break;
        }
    }
}

/**
 *
 *  rct2: 0x0066CDE4
 */
void top_toolbar_init_view_menu(rct_window* w, rct_widget* widget) {
    gDropdownItemsFormat[0] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[1] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[2] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[3] = STR_EMPTY;
    gDropdownItemsFormat[4] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[5] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[6] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[7] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[8] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[9] = STR_EMPTY;
    gDropdownItemsFormat[10] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[11] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[12] = STR_TOGGLE_OPTION;
    gDropdownItemsFormat[13] = DROPDOWN_SEPARATOR;
    gDropdownItemsFormat[DDIDX_VIEW_CLIPPING] = STR_TOGGLE_OPTION;

    gDropdownItemsArgs[0] = STR_UNDERGROUND_VIEW;
    gDropdownItemsArgs[1] = STR_REMOVE_BASE_LAND;
    gDropdownItemsArgs[2] = STR_REMOVE_VERTICAL_FACES;
    gDropdownItemsArgs[4] = STR_SEE_THROUGH_RIDES;
    gDropdownItemsArgs[5] = STR_SEE_THROUGH_SCENERY;
    gDropdownItemsArgs[6] = STR_SEE_THROUGH_PATHS;
    gDropdownItemsArgs[7] = STR_INVISIBLE_SUPPORTS;
    gDropdownItemsArgs[8] = STR_INVISIBLE_PEOPLE;
    gDropdownItemsArgs[10] = STR_HEIGHT_MARKS_ON_LAND;
    gDropdownItemsArgs[11] = STR_HEIGHT_MARKS_ON_RIDE_TRACKS;
    gDropdownItemsArgs[12] = STR_HEIGHT_MARKS_ON_PATHS;
    gDropdownItemsArgs[DDIDX_VIEW_CLIPPING] = STR_VIEW_CLIPPING_MENU;

    window_dropdown_show_text(
        w->x + widget->left,
        w->y + widget->top,
        widget->bottom - widget->top + 1,
        w->colours[1] | 0x80,
        0,
        TOP_TOOLBAR_VIEW_MENU_COUNT
    );

    // Set checkmarks
    rct_viewport* mainViewport = window_get_main()->viewport;
    if (mainViewport->flags & VIEWPORT_FLAG_UNDERGROUND_INSIDE)
        dropdown_set_checked(0, true);
    if (mainViewport->flags & VIEWPORT_FLAG_HIDE_BASE)
        dropdown_set_checked(1, true);
    if (mainViewport->flags & VIEWPORT_FLAG_HIDE_VERTICAL)
        dropdown_set_checked(2, true);
    if (mainViewport->flags & VIEWPORT_FLAG_SEETHROUGH_RIDES)
        dropdown_set_checked(4, true);
    if (mainViewport->flags & VIEWPORT_FLAG_SEETHROUGH_SCENERY)
        dropdown_set_checked(5, true);
    if (mainViewport->flags & VIEWPORT_FLAG_SEETHROUGH_PATHS)
        dropdown_set_checked(6, true);
    if (mainViewport->flags & VIEWPORT_FLAG_INVISIBLE_SUPPORTS)
        dropdown_set_checked(7, true);
    if (mainViewport->flags & VIEWPORT_FLAG_INVISIBLE_PEEPS)
        dropdown_set_checked(8, true);
    if (mainViewport->flags & VIEWPORT_FLAG_LAND_HEIGHTS)
        dropdown_set_checked(10, true);
    if (mainViewport->flags & VIEWPORT_FLAG_TRACK_HEIGHTS)
        dropdown_set_checked(11, true);
    if (mainViewport->flags & VIEWPORT_FLAG_PATH_HEIGHTS)
        dropdown_set_checked(12, true);
    if (mainViewport->flags & VIEWPORT_FLAG_PAINT_CLIP_TO_HEIGHT)
        dropdown_set_checked(DDIDX_VIEW_CLIPPING, true);

    gDropdownDefaultIndex = DDIDX_UNDERGROUND_INSIDE;
}

/**
 *
 *  rct2: 0x0066CF8A
 */
void top_toolbar_view_menu_dropdown(sint16 dropdownIndex)
{
    rct_window* w = window_get_main();
    if (w) {
        switch (dropdownIndex) {
        case DDIDX_UNDERGROUND_INSIDE:
            w->viewport->flags ^= VIEWPORT_FLAG_UNDERGROUND_INSIDE;
            break;
        case DDIDX_HIDE_BASE:
            w->viewport->flags ^= VIEWPORT_FLAG_HIDE_BASE;
            break;
        case DDIDX_HIDE_VERTICAL:
            w->viewport->flags ^= VIEWPORT_FLAG_HIDE_VERTICAL;
            break;
        case DDIDX_SEETHROUGH_RIDES:
            w->viewport->flags ^= VIEWPORT_FLAG_SEETHROUGH_RIDES;
            break;
        case DDIDX_SEETHROUGH_SCENARY:
            w->viewport->flags ^= VIEWPORT_FLAG_SEETHROUGH_SCENERY;
            break;
        case DDIDX_SEETHROUGH_PATHS:
            w->viewport->flags ^= VIEWPORT_FLAG_SEETHROUGH_PATHS;
            break;
        case DDIDX_INVISIBLE_SUPPORTS:
            w->viewport->flags ^= VIEWPORT_FLAG_INVISIBLE_SUPPORTS;
            break;
        case DDIDX_INVISIBLE_PEEPS:
            w->viewport->flags ^= VIEWPORT_FLAG_INVISIBLE_PEEPS;
            break;
        case DDIDX_LAND_HEIGHTS:
            w->viewport->flags ^= VIEWPORT_FLAG_LAND_HEIGHTS;
            break;
        case DDIDX_TRACK_HEIGHTS:
            w->viewport->flags ^= VIEWPORT_FLAG_TRACK_HEIGHTS;
            break;
        case DDIDX_PATH_HEIGHTS:
            w->viewport->flags ^= VIEWPORT_FLAG_PATH_HEIGHTS;
            break;
        case DDIDX_VIEW_CLIPPING:
            if (window_find_by_class(WC_VIEW_CLIPPING) == nullptr) {
                context_open_window(WC_VIEW_CLIPPING);
            } else {
                // If window is already open, toggle the view clipping on/off
                w->viewport->flags ^= VIEWPORT_FLAG_PAINT_CLIP_TO_HEIGHT;
            }
            break;
        default:
            return;
        }
        window_invalidate(w);
    }
}

/**
 *
 *  rct2: 0x0066CCE7
 */
void toggle_footpath_window()
{
    if (window_find_by_class(WC_FOOTPATH) == nullptr) {
        context_open_window(WC_FOOTPATH);
    } else {
        tool_cancel();
        window_close_by_class(WC_FOOTPATH);
    }
}

/**
 *
 *  rct2: 0x0066CD54
 */
void toggle_land_window(rct_window *topToolbar, rct_widgetindex widgetIndex)
{
    if ((input_test_flag(INPUT_FLAG_TOOL_ACTIVE)) && gCurrentToolWidget.window_classification == WC_TOP_TOOLBAR && gCurrentToolWidget.widget_index == WIDX_LAND) {
        tool_cancel();
    } else {
        show_gridlines();
        gMapCtrlPressed = false;
        tool_set(topToolbar, widgetIndex, TOOL_DIG_DOWN);
        input_set_flag(INPUT_FLAG_6, true);
        context_open_window(WC_LAND);
    }
}

/**
 *
 *  rct2: 0x0066CD0C
 */
void toggle_clear_scenery_window(rct_window *topToolbar, rct_widgetindex widgetIndex)
{
    if ((input_test_flag(INPUT_FLAG_TOOL_ACTIVE) && gCurrentToolWidget.window_classification == WC_TOP_TOOLBAR && gCurrentToolWidget.widget_index == WIDX_CLEAR_SCENERY)) {
        tool_cancel();
    } else {
        show_gridlines();
        tool_set(topToolbar, widgetIndex, TOOL_CROSSHAIR);
        input_set_flag(INPUT_FLAG_6, true);
        context_open_window(WC_CLEAR_SCENERY);
    }
}

/**
 *
 *  rct2: 0x0066CD9C
 */
void toggle_water_window(rct_window *topToolbar, rct_widgetindex widgetIndex)
{
    if ((input_test_flag(INPUT_FLAG_TOOL_ACTIVE)) && gCurrentToolWidget.window_classification == WC_TOP_TOOLBAR && gCurrentToolWidget.widget_index == WIDX_WATER) {
        tool_cancel();
    } else {
        show_gridlines();
        tool_set(topToolbar, widgetIndex, TOOL_WATER_DOWN);
        input_set_flag(INPUT_FLAG_6, true);
        context_open_window(WC_WATER);
    }
}

/**
 *
 *  rct2: 0x0066D104
 */
bool land_tool_is_active()
{
    if (!(input_test_flag(INPUT_FLAG_TOOL_ACTIVE)))
        return false;
    if (gCurrentToolWidget.window_classification != WC_TOP_TOOLBAR)
        return false;
    if (gCurrentToolWidget.widget_index != WIDX_LAND)
        return false;
    return true;
}

/**
 *
 *  rct2: 0x0066D125
 */
bool clear_scenery_tool_is_active()
{
    if (!(input_test_flag(INPUT_FLAG_TOOL_ACTIVE)))
        return false;
    if (gCurrentToolWidget.window_classification != WC_TOP_TOOLBAR)
        return false;
    if (gCurrentToolWidget.widget_index != WIDX_CLEAR_SCENERY)
        return false;
    return true;
}

/**
 *
 *  rct2: 0x0066DB3D
 */
bool scenery_tool_is_active()
{
    sint32 toolWindowClassification = gCurrentToolWidget.window_classification;
    rct_widgetindex toolWidgetIndex = gCurrentToolWidget.widget_index;
    if (input_test_flag(INPUT_FLAG_TOOL_ACTIVE))
        if (toolWindowClassification == WC_TOP_TOOLBAR && toolWidgetIndex == WIDX_SCENERY)
            return true;

    return false;
}

/**
 *
 *  rct2: 0x0066D125
 */
bool water_tool_is_active()
{
    if (!(input_test_flag(INPUT_FLAG_TOOL_ACTIVE)))
        return false;
    if (gCurrentToolWidget.window_classification != WC_TOP_TOOLBAR)
        return false;
    if (gCurrentToolWidget.widget_index != WIDX_WATER)
        return false;
    return true;
}
