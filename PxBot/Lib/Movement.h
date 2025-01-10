#pragma once
#include <windows.h>
#include <iostream>
#include "./Camera.h"
#include "../Constants/Header.h"

using namespace std;

class Movement
{
public:
	enum Direction
	{
		north,
		east,
		south,
		west,
		north_west,
		south_west,
		north_east,
		south_east,
	};

	struct Position 
	{
		int x;
		int y;
	};

	void move(Direction direction)
	{
		switch (direction)
		{
			case Direction::north:
				this->__key_press(0x57);
				break;

			case Direction::south:
				this->__key_press(0x53);
				break;

			case Direction::west:
				this->__key_press(0x41);
				break;

			case Direction::east:
				this->__key_press(0x44);
				break;

			case Direction::north_west:
				this->__key_press(0x51);
				break;

			case Direction::south_west:
				this->__key_press(0x5A);
				break;

			case Direction::north_east:
				this->__key_press(0x45);
				break;

			case Direction::south_east:
				this->__key_press(0x43);
				break;
		}
	}

	void press(WORD virtual_key_code)
	{
		this->__key_press(virtual_key_code);
	}

	void mouse_over(Rect position)
	{
		this->__focus_window(this->__get_window());
		SetCursorPos(position.x, position.y);
	}

	void scroll(int direction, int amount)
	{
		INPUT input = { 0 };
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_WHEEL;
		input.mi.mouseData = direction * amount;

		int min_delay = 41;
		int humanizer = rand() % 100;
		int delay = humanizer >= min_delay ? humanizer : min_delay;

		SendInput(1, &input, sizeof(INPUT));
		Sleep(delay);
	}

	void click_mouse(Rect position)
	{
		this->__focus_window(this->__get_window());

		int random_delay = rand() % 80;
		int use_delay = random_delay >= 45 ? random_delay : 31;
		Sleep(use_delay);

		INPUT inputs[2] = {};

		inputs[0].type = INPUT_MOUSE;
		inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

		inputs[1].type = INPUT_MOUSE;
		inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

		SetCursorPos(position.x, position.y);
		SendInput(2, inputs, sizeof(INPUT));
		Sleep(50);
		SetCursorPos(position.x - 200, position.y);

		int delay_to_update_scene = 180;
		Sleep(delay_to_update_scene);
	}

	void use_item(WORD virtual_key_code, string direction)
	{
		press(virtual_key_code);
		Position click_pos = __get_direction_from_character(direction);
		click_mouse({ click_pos.x, click_pos.y, 0, 0 });
	}

private:
	HWND __cached_hwnd;

	HWND __get_window()
	{
		if (this->__cached_hwnd)
		{
			return this->__cached_hwnd;
		}
		this->__cached_hwnd = Camera::get_game_window();
		return this->__cached_hwnd;
	}

	Position __get_direction_from_character(string direction)
	{
		const int sqm_size = constants::MOVEMENT_SQM_SIZE;
		
		const int interface_corner_size = constants::INTERFACE_CORNER_SIZE;
		const int interface_tile_to_center_x = constants::INTERFACE_TILE_TO_CENTER_X;
		
		const int interface_window_top_bar = constants::INTERFACE_WINDOW_TOP_BAR;
		const int interface_tile_to_center_y = constants::INTERFACE_TILE_TO_CENTER_Y;

		const Position center_position = { 
			interface_corner_size + interface_tile_to_center_x * sqm_size,
			interface_window_top_bar + interface_tile_to_center_y * sqm_size
		};

		if (direction == "n")
			return { center_position.x, center_position.y - sqm_size };
		else if (direction == "s")
			return { center_position.x, center_position.y + sqm_size };
		else if (direction == "w")
			return { center_position.x - sqm_size, center_position.y };
		else if (direction == "e")
			return { center_position.x + sqm_size, center_position.y };
		else if (direction == "nw")
			return { center_position.x - sqm_size, center_position.y - sqm_size };
		else if (direction == "sw")
			return { center_position.x - sqm_size, center_position.y + sqm_size };
		else if (direction == "ne")
			return { center_position.x + sqm_size, center_position.y - sqm_size };
		else if (direction == "se")
			return { center_position.x + sqm_size, center_position.y + sqm_size };
		else
			return center_position;
	}

	void __focus_window(HWND hwnd)
	{
		if (IsIconic(hwnd))
		{
			ShowWindow(hwnd, SW_RESTORE);
		}
		SetForegroundWindow(hwnd);
	}

	void __key_press(WORD virtual_key_code)
	{
		HWND hwnd = this->__get_window();

		if (hwnd)
		{
			this->__focus_window(hwnd);
			int min_delay = 31;
			int random_simulated_delay = rand() % 72;
			int use_delay = random_simulated_delay >= min_delay ? random_simulated_delay : min_delay;

			SendMessage(hwnd, WM_KEYDOWN, virtual_key_code, 0);
			SendMessage(hwnd, WM_KEYUP, virtual_key_code, use_delay);
		}
		else
		{
			cout << "<!--		ERROR!, could not find client		-->" << endl;
		}
	}

	void __key_release(WORD virtual_key_code)
	{
		INPUT input = { 0 };
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = virtual_key_code;
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(INPUT));
	}
};