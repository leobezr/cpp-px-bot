#pragma once
namespace constants
{
    constexpr int MAP_WIDTH = 176;
    constexpr int MAP_HEIGHT = 116;
    constexpr int MAP_BTN_WIDTH = 59;
    constexpr int MAP_BTN_HEIGHT = 70;
    constexpr int MAP_OFFSET_X = MAP_WIDTH - MAP_BTN_WIDTH;
    constexpr int MAP_OFFSET_Y = MAP_HEIGHT - MAP_BTN_HEIGHT;

    constexpr int MAP_THUMB_SIZE = 18 * 2;
    constexpr int MAP_CLICK_NUDGE_X = 0;
    constexpr int MAP_CLICK_NUDGE_Y = 21;

    constexpr int MAP_FIND_WAYPOINT_THRESHOLD = .76;
}