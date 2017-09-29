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

#include "../game.h"
#include "../common.h"
#include "../localisation/localisation.h"
#include "../scenario/scenario.h"
#include "../cheats.h"
#include "../network/network.h"
#include "../object_list.h"
#include "Climate.h"
#include "Fountain.h"
#include "map.h"
#include "park.h"
#include "scenery.h"
#include "footpath.h"

uint8 gWindowSceneryActiveTabIndex;
uint16 gWindowSceneryTabSelections[20];
uint8 gWindowSceneryClusterEnabled;
uint8 gWindowSceneryPaintEnabled;
uint8 gWindowSceneryRotation;
colour_t gWindowSceneryPrimaryColour;
colour_t gWindowScenerySecondaryColour;
colour_t gWindowSceneryTertiaryColour;
bool gWindowSceneryEyedropperEnabled;

rct_map_element *gSceneryMapElement;
//uint8 gSceneryMapElementType;

//various
money32 gSceneryPlaceCost;
scenery_key_shape gSceneryShape;
//sint16 gSceneryPlaceObject;
sint16 gSceneryPlaceZ;
uint8 gSceneryPlacePathType;
uint8 gSceneryPlacePathSlope;
uint8 gSceneryPlaceRotation;
bool gSceneryCannotDisplay;

//ghosts related
scenery_ghosts_list gSceneryGhost[SCENERY_GHOST_LIST_SIZE];
scenery_ghosts_last gSceneryLastGhost;
uint16 gSceneryLastIndex;

//keypad related
scenery_key_shift gSceneryShift;
scenery_key_ctrl gSceneryCtrl;
scenery_key_drag gSceneryDrag;

sint16 gScenerySetHeight;
uint8 gSceneryGroundFlags;
money32 gClearSceneryCost;

// rct2: 0x009A3E74
const rct_xy8 ScenerySubTileOffsets[] = {
    {  7,  7 },
    {  7, 23 },
    { 23, 23 },
    { 23,  7 }
};


void scenery_increase_age(sint32 x, sint32 y, rct_map_element *mapElement);

void scenery_update_tile(sint32 x, sint32 y)
{
    rct_map_element *mapElement;

    mapElement = map_get_first_element_at(x >> 5, y >> 5);
    do {
        // Ghosts are purely this-client-side and should not cause any interaction,
        // as that may lead to a desync.
        if (network_get_mode() != NETWORK_MODE_NONE)
        {
            if (map_element_is_ghost(mapElement))
                continue;
        }

        if (map_element_get_type(mapElement) == MAP_ELEMENT_TYPE_SCENERY) {
            scenery_update_age(x, y, mapElement);
        } else if (map_element_get_type(mapElement) == MAP_ELEMENT_TYPE_PATH) {
            if (footpath_element_has_path_scenery(mapElement) && !footpath_element_path_scenery_is_ghost(mapElement)) {
                rct_scenery_entry *sceneryEntry = get_footpath_item_entry(footpath_element_get_path_scenery_index(mapElement));
                if (sceneryEntry != (void*)-1) {
                    if (sceneryEntry->path_bit.flags & PATH_BIT_FLAG_JUMPING_FOUNTAIN_WATER) {
                        jumping_fountain_begin(JUMPING_FOUNTAIN_TYPE_WATER, x, y, mapElement);
                    }
                    else if (sceneryEntry->path_bit.flags & PATH_BIT_FLAG_JUMPING_FOUNTAIN_SNOW) {
                        jumping_fountain_begin(JUMPING_FOUNTAIN_TYPE_SNOW, x, y, mapElement);
                    }
                }
            }
        }
    } while (!map_element_is_last_for_tile(mapElement++));
}

/**
 *
 *  rct2: 0x006E33D9
 */
void scenery_update_age(sint32 x, sint32 y, rct_map_element *mapElement)
{
    rct_map_element *mapElementAbove;
    rct_scenery_entry *sceneryEntry;

    sceneryEntry = get_small_scenery_entry(mapElement->properties.scenery.type);
    if (sceneryEntry == (rct_scenery_entry*)-1)
    {
        return;
    }

    if (gCheatsDisablePlantAging &&
        (sceneryEntry->small_scenery.flags & SMALL_SCENERY_FLAG_CAN_BE_WATERED)) {
        return;
    }

    if (
        !(sceneryEntry->small_scenery.flags & SMALL_SCENERY_FLAG_CAN_BE_WATERED) ||
        (gClimateCurrentWeather < WEATHER_RAIN) ||
        (mapElement->properties.scenery.age < 5)
    ) {
        scenery_increase_age(x, y, mapElement);
        return;
    }

    // Check map elements above, presumably to see if map element is blocked from rain
    mapElementAbove = mapElement;
    while (!(mapElementAbove->flags & 7)) {

        mapElementAbove++;

        // Ghosts are purely this-client-side and should not cause any interaction,
        // as that may lead to a desync.
        if (map_element_is_ghost(mapElementAbove))
            continue;

        switch (map_element_get_type(mapElementAbove)) {
        case MAP_ELEMENT_TYPE_SCENERY_MULTIPLE:
        case MAP_ELEMENT_TYPE_ENTRANCE:
        case MAP_ELEMENT_TYPE_PATH:
            map_invalidate_tile_zoom1(x, y, mapElementAbove->base_height * 8, mapElementAbove->clearance_height * 8);
            scenery_increase_age(x, y, mapElement);
            return;
        case MAP_ELEMENT_TYPE_SCENERY:
            sceneryEntry = get_small_scenery_entry(mapElementAbove->properties.scenery.type);
            if (sceneryEntry->small_scenery.flags & SMALL_SCENERY_FLAG_VOFFSET_CENTRE) {
                scenery_increase_age(x, y, mapElement);
                return;
            }
            break;
        }
    }

    // Reset age / water plant
    mapElement->properties.scenery.age = 0;
    map_invalidate_tile_zoom1(x, y, mapElement->base_height * 8, mapElement->clearance_height * 8);
}

void scenery_increase_age(sint32 x, sint32 y, rct_map_element *mapElement)
{
    if (mapElement->flags & SMALL_SCENERY_FLAG_ANIMATED)
        return;

    if (mapElement->properties.scenery.age < 255) {
        mapElement->properties.scenery.age++;
        map_invalidate_tile_zoom1(x, y, mapElement->base_height * 8, mapElement->clearance_height * 8);
    }
}

/**
 *
 *  rct2: 0x006E2712
 */
void scenery_remove_ghost_tool_placement(bool leaveplacable) {

    rct_xyz16 pos;
    gSceneryLastIndex = 0;

    /*bool enc = false;
    for (sint16 i = 0; i < SCENERY_GHOST_LIST_SIZE; i++)
    {
        if (!gSceneryGhost[i].type) enc = true;
        if (gSceneryGhost[i].type && enc)
        {
            enc = true;
        }
    }*/

    for (sint16 i = 0; i<SCENERY_GHOST_LIST_SIZE; i++) {
        if (!gSceneryGhost[i].type)
            break;

        pos = gSceneryGhost[i].position;

        if (gSceneryGhost[i].type & (1 << 0)) {
            game_do_command(
                pos.x,
                105 | (gSceneryGhost[i].element_type << 8),
                //105 | (gSceneryGhost[i].orientation << 8),
                pos.y,
                pos.z | (gSceneryLastGhost.selected_tab << 8),
                GAME_COMMAND_REMOVE_SCENERY,
                0,
                0);
        }

        if (gSceneryGhost[i].type & (1 << 1)) {
            rct_map_element* map_element = map_get_first_element_at(pos.x / 32, pos.y / 32);

            do {
                if (map_element_get_type(map_element) != MAP_ELEMENT_TYPE_PATH)
                    continue;

                if (map_element->base_height != pos.z)
                    continue;
                game_do_command(
                    pos.x,
                    233 | (gSceneryPlacePathSlope << 8),
                    pos.y,
                    pos.z | (gSceneryPlacePathType << 8),
                    GAME_COMMAND_PLACE_PATH,
                    gSceneryGhost[i].path_type & 0xFFFF0000,
                    0);
                break;
            } while (!map_element_is_last_for_tile(map_element++));
        }

        if (gSceneryGhost[i].type & (1 << 2)) {
            //rct_map_element* map_element = gSceneryGhost[i].element;
            game_do_command(
                pos.x,
                105 | (gSceneryGhost[i].element_type << 8),
                pos.y,
                gSceneryGhost[i].rotation | (gSceneryGhost[i].position.z << 8),
                GAME_COMMAND_REMOVE_WALL,
                0,
                0);
        }

        if (gSceneryGhost[i].type & (1 << 3)) {
            game_do_command(
                pos.x,
                105 | (gSceneryGhost[i].orientation << 8),
                pos.y,
                pos.z,
                GAME_COMMAND_REMOVE_LARGE_SCENERY,
                0,
                0);
        }

        if (gSceneryGhost[i].type & (1 << 4)) {
            game_do_command(
                pos.x,
                105,
                pos.y,
                pos.z | (gSceneryGhost[i].orientation << 8),
                GAME_COMMAND_REMOVE_BANNER,
                0,
                0);
        }
        if (gSceneryGhost[i].type & (1 << 0)) {
            gSceneryGhost[i].type &= ~(1 << 0);
        }
        if (gSceneryGhost[i].type & (1 << 1)) {
            gSceneryGhost[i].type &= ~(1 << 1);
        }
        if (gSceneryGhost[i].type & (1 << 2)) {
            gSceneryGhost[i].type &= ~(1 << 2);
        }
        if (gSceneryGhost[i].type & (1 << 3)) {
            gSceneryGhost[i].type &= ~(1 << 3);
        }
        if (gSceneryGhost[i].type & (1 << 4)) {
            gSceneryGhost[i].type &= ~(1 << 4);
        }
        if (leaveplacable) {
            gSceneryGhost[i].placable = true;
        }
    }
    //iterate in search for ghosts memory that was left as 'placable'
    for (sint16 i = 0; (i < SCENERY_GHOST_LIST_SIZE) && !leaveplacable; i++)
    {
        if (gSceneryGhost[i].placable)
            gSceneryGhost[i].placable = false;
        else
            break;
    }
}

rct_scenery_entry *get_small_scenery_entry(sint32 entryIndex)
{
    if (entryIndex >= object_entry_group_counts[OBJECT_TYPE_SMALL_SCENERY]) {
        return NULL;
    }
    return (rct_scenery_entry*)gSmallSceneryEntries[entryIndex];
}

rct_scenery_entry *get_large_scenery_entry(sint32 entryIndex)
{
    if (entryIndex >= object_entry_group_counts[OBJECT_TYPE_LARGE_SCENERY]) {
        return NULL;
    }
    return (rct_scenery_entry*)gLargeSceneryEntries[entryIndex];
}

rct_scenery_entry *get_wall_entry(sint32 entryIndex)
{
    if (entryIndex >= object_entry_group_counts[OBJECT_TYPE_WALLS]) {
        return NULL;
    }
    return (rct_scenery_entry*)gWallSceneryEntries[entryIndex];
}

rct_scenery_entry *get_banner_entry(sint32 entryIndex)
{
    if (entryIndex >= object_entry_group_counts[OBJECT_TYPE_BANNERS]) {
        return NULL;
    }
    return (rct_scenery_entry*)gBannerSceneryEntries[entryIndex];
}

rct_scenery_entry *get_footpath_item_entry(sint32 entryIndex)
{
    if (entryIndex >= object_entry_group_counts[OBJECT_TYPE_PATH_BITS]) {
        return NULL;
    }
    return (rct_scenery_entry*)gFootpathAdditionEntries[entryIndex];
}

rct_scenery_set_entry *get_scenery_group_entry(sint32 entryIndex)
{
    if (entryIndex >= object_entry_group_counts[OBJECT_TYPE_SCENERY_SETS]) {
        return NULL;
    }
    return (rct_scenery_set_entry*)gSceneryGroupEntries[entryIndex];
}

sint32 get_scenery_id_from_entry_index(uint8 objectType, sint32 entryIndex)
{
    switch (objectType) {
    case OBJECT_TYPE_SMALL_SCENERY: return entryIndex + SCENERY_SMALL_SCENERY_ID_MIN;
    case OBJECT_TYPE_PATH_BITS:     return entryIndex + SCENERY_PATH_SCENERY_ID_MIN;
    case OBJECT_TYPE_WALLS:         return entryIndex + SCENERY_WALLS_ID_MIN;
    case OBJECT_TYPE_LARGE_SCENERY: return entryIndex + SCENERY_LARGE_SCENERY_ID_MIN;
    case OBJECT_TYPE_BANNERS:       return entryIndex + SCENERY_BANNERS_ID_MIN;
    default:                        return -1;
    }
}

sint32 scenery_small_get_primary_colour(const rct_map_element *mapElement)
{
    return (mapElement->properties.scenery.colour_1 & 0x1F);
}

sint32 scenery_small_get_secondary_colour(const rct_map_element *mapElement)
{
    return (mapElement->properties.scenery.colour_2 & 0x1F);
}

void scenery_small_set_primary_colour(rct_map_element *mapElement, uint32 colour)
{
    assert(colour <= 31);
    mapElement->properties.scenery.colour_1 &= ~0x1F;
    mapElement->properties.scenery.colour_1 |= colour;

}

void scenery_small_set_secondary_colour(rct_map_element *mapElement, uint32 colour)
{
    assert(colour <= 31);
    mapElement->properties.scenery.colour_2 &= ~0x1F;
    mapElement->properties.scenery.colour_2 |= colour;
}

bool scenery_small_get_supports_needed(const rct_map_element *mapElement)
{
    return (bool)(mapElement->properties.scenery.colour_1 & MAP_ELEM_SMALL_SCENERY_COLOUR_FLAG_NEEDS_SUPPORTS);
}

void scenery_small_set_supports_needed(rct_map_element *mapElement)
{
    mapElement->properties.scenery.colour_1 |= MAP_ELEM_SMALL_SCENERY_COLOUR_FLAG_NEEDS_SUPPORTS;
}
