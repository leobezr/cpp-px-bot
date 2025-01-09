#pragma once
namespace constants
{
    constexpr int MAP_WIDTH = 176;
    constexpr int MAP_HEIGHT = 116;
    constexpr int MAP_BTN_WIDTH = 59;
    constexpr int MAP_BTN_HEIGHT = 70;
    constexpr int MAP_OFFSET_X = MAP_WIDTH - MAP_BTN_WIDTH;
    constexpr int MAP_OFFSET_Y = MAP_HEIGHT - MAP_BTN_HEIGHT;

    constexpr int MAP_THUMB_SIZE = 24 * 2;
    constexpr int MAP_CLICK_NUDGE_X = 0;
    constexpr int MAP_CLICK_NUDGE_Y = 21;

    constexpr double MAP_FIND_WAYPOINT_THRESHOLD = .76;
    
    constexpr double MAP_DESTINATION_CHECK_SIMULARITY_MODIFIER = 0.01;

    constexpr int DELAY_WAIT_AFTER_ARROW_MOVE = 90 * 2; // PING at 90ms

    constexpr int MOVEMENT_SQM_SIZE = 70;

    constexpr int INTERFACE_CORNER_SIZE = 363;
	constexpr int INTERFACE_WINDOW_TOP_BAR = 19;
	constexpr int INTERFACE_TILE_TO_CENTER_X = 8;
    constexpr int INTERFACE_TILE_TO_CENTER_Y = 6;

	constexpr int TIMER_REFRESH_RATE_HUNTER = 50;
    constexpr int TIMER_TARGETING_AFTER_PRESSING_ESC = 120;
}