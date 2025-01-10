#pragma once
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <condition_variable>
#include <thread>
#include "./Camera.h"
#include "./Movement.h"
#include "../Constants/Header.h"
#include "../Helpers/ProfileLoader.h"

using namespace std;
using namespace chrono;
using namespace cv;

const Rect battle_window_size = Rect(0, 0, 175, 170);
const Rect map_area = Rect(20, 4, 107, 110);

const int map_width = 176, map_height = 116;
const int map_btn_width = 59, map_btn_height = 70;
const int offset_x = map_width - map_btn_width;
const int offset_y = map_height - map_btn_height;

class DeveloperTool
{
public:
	struct DeveloperToolConfig
	{
		bool developer_mode_enabled;
		Profile profile;
	};

	DeveloperTool(DeveloperToolConfig dev_config)
	{
		__enabled = dev_config.developer_mode_enabled;
		__profile = dev_config.profile;
		
		start_threads();
	}

	/*
	* Thread management
	*/
	void start_threads()
	{
		__developer_thread = thread(&DeveloperTool::__lookout_for_actions, this);
	}
	void stop_threads()
	{
		__stop_threads = true;
		__developer_thread.join();
	}
	void self_update_scene()
	{
		__scene = __camera.capture_scene();
	}

private:
	/* Utils */
	Camera __camera = Camera();
	Movement __movement = Movement();

	/* Profile */
	bool __enabled;
	Profile __profile;

	/* 
	* Scene related and ROI's 
	*/
	Mat __scene;

	/* Battle */
	Rect __cached_battle_window_ROI;
	const Rect __battle_window_size = Rect(0, 0, 175, 170);
	/* Map */
	Rect __cached_map_position_ROI;

	/* Thread management */
	thread __developer_thread;
	bool __stop_threads = false;

	/* Waypoint Creator management */
	string __waypoint_category = "start";
	int __waypoint_position = 0;
	string __waypoint_action = "move";
	int __node_size = 1;

	/*
	* Thread call to watch for developer actions.
	*/
	void __lookout_for_actions()
	{
		if (__enabled)
			cout << "Developer mode: ENABLED" << endl;
		else
			cout << "Developer mode: DISABLED" << endl;

		while (!__stop_threads && __enabled)
		{
			self_update_scene();

			if (__stop_threads) break;

			if (!__scene.empty())
			{
				__register_creature_being_followed();

					__capture_map_screenshot();
					__numpad_actions();
			}
		}
	}

	/*
	* Getters
	*/

	Mat __get_map()
	{
		if (__cached_map_position_ROI.empty())
		{
			__cache_map_ROI();
		}
		return __scene(__cached_map_position_ROI).clone();
	}

	void __register_creature_being_followed()
	{
		Mat scene, mask;
		Rect battle_area_with_offset = Rect(10, 15, 4, battle_window_size.height - 15);
		Rect ROI_area = battle_area_with_offset;
		Scalar lower(50, 108, 205), upper(113, 212, 255);

		cvtColor(__get_battle_window(), scene, COLOR_RGB2HSV);
		inRange(scene, lower, upper, mask);

		if (countNonZero(mask(ROI_area)) > 0 && !scene.empty())
		{
			Rect battle_window_position = Rect(
				this->__cached_battle_window_ROI.x, 
				this->__cached_battle_window_ROI.y, 
				battle_window_size.width,
				battle_window_size.height
			);

			Rect bounding_box = boundingRect(mask);
			
			mask.release();
			scene.release();

			this->__movement.press(VK_ESCAPE);
			Sleep(120);

			Mat scene_without_follow_square = this->__camera.capture_scene();
			const int thumb_width = 40, thumb_height = 15;

			Rect creature_position = Rect(
				battle_window_position.x + bounding_box.x + 5,
				battle_window_position.y + bounding_box.y,
				thumb_width, thumb_height
			);

			string directory = "Resources/Screenshots/";
			this->__camera.save_image(scene_without_follow_square(creature_position), directory);

			scene_without_follow_square.release();

			cout << "Image saved" << endl;
		}
	}

	/*
	* Getters
	*/
	Mat __get_battle_window()
	{
		if (__cached_battle_window_ROI.empty())
		{
			__cache_battle_window_ROI();
		}
		return __scene(__cached_battle_window_ROI);
	}

	vector<Waypoint>& __get_waypoint_category()
	{
		if (__waypoint_category == "start")
			return __profile.waypoint_category.start;
		else if (__waypoint_category == "hunt")
			return __profile.waypoint_category.hunt;
		else if (__waypoint_category == "refil")
			return __profile.waypoint_category.refil;
		else
			throw runtime_error("Invalid waypoint category.");
	}

	string __get_wpt_label()
	{
		return "wpt-" + to_string(__waypoint_position);
	}

	/*
	* Profile manipulator methods
	*/

    void __add_waypoint_to_profile(Waypoint wpt)
    {
		vector<Waypoint>& waypoints = __get_waypoint_category();
		waypoints.push_back(wpt);

		ProfileLoader::save_profile(__profile.name, __profile);

		cout << "Used action: " << __waypoint_action << endl;
		__waypoint_position++;

		Sleep(500);
    }

	/*
	* Actions with scene cutters and caching
	*/

	void __cache_battle_window_ROI()
	{
		Mat needle, scene = __scene.clone();

		cvtColor(imread("Resources/battle-list.png", IMREAD_UNCHANGED), needle, COLOR_BGRA2BGR);

		bool const breaks_if_not_found = TRUE;
		double threshold = .49;

		Point battle_window_coordinate = this->__camera.find_needle(
			scene, needle,
			threshold, breaks_if_not_found
		);

		int pos_x = battle_window_coordinate.x - 10,
			pos_y = battle_window_coordinate.y;

		Rect ROI_area = Rect(
			pos_x, pos_y,
			__battle_window_size.width, __battle_window_size.height
		);

		needle.release();
		scene.release();

		if (ROI_area.area() == 0)
		{
			cout << "<!--		ERROR! Battle window not found		->" << endl;
			return;
		}

		__cached_battle_window_ROI = ROI_area;
	}

	void __numpad_actions()
	{
		if (GetAsyncKeyState(VK_NUMPAD1) & 0x8000)
			__add_waypoint_to_profile(Waypoint({ "action:" + __waypoint_action + ":sw", 1, __get_wpt_label()}));
		else if (GetAsyncKeyState(VK_NUMPAD2) & 0x8000)
			__add_waypoint_to_profile(Waypoint({ "action:" + __waypoint_action + ":s", 1, __get_wpt_label() }));
		else if (GetAsyncKeyState(VK_NUMPAD3) & 0x8000)
			__add_waypoint_to_profile(Waypoint({ "action:" + __waypoint_action + ":se", 1, __get_wpt_label() }));
		else if (GetAsyncKeyState(VK_NUMPAD4) & 0x8000)
			__add_waypoint_to_profile(Waypoint({ "action:" + __waypoint_action + ":w", 1, __get_wpt_label() }));
		else if (GetAsyncKeyState(VK_NUMPAD5) & 0x8000)
			__add_waypoint_to_profile(Waypoint({ "action:" + __waypoint_action + ":c", 1, __get_wpt_label() }));
		else if (GetAsyncKeyState(VK_NUMPAD6) & 0x8000)
			__add_waypoint_to_profile(Waypoint({ "action:" + __waypoint_action + ":e", 1, __get_wpt_label() }));
		else if (GetAsyncKeyState(VK_NUMPAD7) & 0x8000)
			__add_waypoint_to_profile(Waypoint({ "action:" + __waypoint_action + ":nw", 1, __get_wpt_label() }));
		else if (GetAsyncKeyState(VK_NUMPAD8) & 0x8000)
			__add_waypoint_to_profile(Waypoint({ "action:" + __waypoint_action + ":n", 1, __get_wpt_label() }));
		else if (GetAsyncKeyState(VK_NUMPAD9) & 0x8000)
			__add_waypoint_to_profile(Waypoint({ "action:" + __waypoint_action + ":ne", 1, __get_wpt_label() }));
		else if (GetAsyncKeyState(VK_NUMPAD0) & 0x8000)
			__change_action_item();
		else if (GetAsyncKeyState(VK_DECIMAL) & 0x8000)
			__change_category();
		else if (GetAsyncKeyState(VK_ADD) & 0x8000)
			__increase_node();
		else if (GetAsyncKeyState(VK_PRIOR) & 0x8000)
			__increase_waypoint_position();
		else if (GetAsyncKeyState(VK_PRIOR) & 0x8000)
			__decrease_waypoint_position();
		else if (GetAsyncKeyState(VK_SUBTRACT) & 0x8000)
			__decrease_node();
	}

	void __decrease_waypoint_position()
	{
		__waypoint_position++;
		cout << "Waypoint position: " << __waypoint_position << endl;
	}

	void __increase_waypoint_position()
	{
		if (__waypoint_position > 0)
		{
			__waypoint_position--;
			cout << "Waypoint position: " << __waypoint_position << endl;
		}
	}

	void __increase_node()
	{
		if (__node_size < constants::WAYPOINT_NODE_MAX_VALUE)
		{
			__node_size++;
			cout << "Node size: " << __node_size << endl;
		}
		else
			cout << "Node at max size: " << __node_size << endl;
	}

	void __decrease_node()
	{
		if (__node_size > constants::WAYPOINT_NODE_MIN_VALUE)
		{
			__node_size--;
			cout << "Node size: " << __node_size << endl;
		}
		else
			cout << "Node at min size: " << __node_size << endl;
	}

	void __capture_map_screenshot()
	{
		if (GetAsyncKeyState(VK_DIVIDE) & 0x8000)
		{
			Mat map_scene = __get_map();
			Mat map = map_scene(map_area);

			Rect map_center = Rect(map.cols / 2, map.rows / 2, 0, 0);
			int map_half_thumb_size = constants::MAP_THUMB_SIZE / 2;

			Rect map_cut = Rect(
				map_center.x - map_half_thumb_size,
				map_center.y - map_half_thumb_size,
				constants::MAP_THUMB_SIZE,
				constants::MAP_THUMB_SIZE
			);

			string profile_name = __profile.name;
			string directory = "Resources/Waypoints/" + profile_name + "/";
			string filename = __waypoint_category + "_" + to_string(__waypoint_position);

			__camera.save_image(map(map_cut), directory, filename);
			__flicker_map(map_cut);
			__add_waypoint_to_profile(Waypoint({ "click", __node_size, __get_wpt_label() }));

		}
	}

	void __change_category()
	{
		if (__waypoint_category == "start")
			__waypoint_category = "hunt";
		else if (__waypoint_category == "hunt")
			__waypoint_category = "refil";
		else if (__waypoint_category == "refil")
			__waypoint_category = "start";
		cout << "Category changed to: " << __waypoint_category << endl;
	}

	void __change_action_item()
	{
		if (__waypoint_action == "move")
			__waypoint_action = "rope";
		else if (__waypoint_action == "rope")
			__waypoint_action = "shovel";
		else if (__waypoint_action == "shovel")
			__waypoint_action = "move";

		cout << "Action item changed to: " << __waypoint_action << endl;
	}

	void __flicker_map(Rect map_center)
	{
		Rect map_position = this->__cached_map_position_ROI;
		Rect mouse_over_position = Rect(map_position.x + map_center.x, map_position.y + map_center.y, 0, 0);

		this->__movement.mouse_over(mouse_over_position);
		this->__movement.scroll(-1, 1);
		this->__movement.scroll(1, 1);

		Rect out_of_the_way_position = Rect(500, 200, 0, 0);
		this->__movement.mouse_over(out_of_the_way_position);
	}

	void __cache_map_ROI()
	{
		if (__cached_map_position_ROI.empty())
		{
			Mat needle = imread("Resources/map-buttons.png", IMREAD_UNCHANGED);

			double threshold = .6;
			bool breaks_if_not_found = true;

			Point map_position = this->__camera.find_needle(
				__scene, needle,
				threshold, breaks_if_not_found
			);

			needle.release();

			const int pos_x = map_position.x - 10,
				pos_y = map_position.y;

			Rect ROI_area = Rect(
				pos_x - constants::MAP_OFFSET_X, pos_y - constants::MAP_OFFSET_Y,
				constants::MAP_WIDTH, constants::MAP_HEIGHT
			);

			if (ROI_area.x <= 0 || ROI_area.y <= 0)
			{
				cout << "<!--		ERROR! Map not found		->" << endl;
				return;
			}

			__cached_map_position_ROI = ROI_area;
		}
	}
};

