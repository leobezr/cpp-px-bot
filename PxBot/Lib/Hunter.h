#pragma once
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <unordered_map>
#include <thread>
#include <windows.h>
#include <iostream>
#include "./Camera.h";
#include "./Movement.h"
#include "../Types/Profile.h"
#include "./Targeting.h"
#include "../Constants/Header.h"
#include "../Constants/Hotkeys.h"
#include "../Helpers/Helpers.h"

using namespace std;
using namespace chrono;
using namespace cv;

class Hunter
{
public:
	struct HunterConfig
	{
		bool enabled;
		Profile profile;
	};

	Targeting targeting;

	Hunter(HunterConfig setup) : __camera(Camera()), targeting({ setup.enabled, setup.profile.target })
	{
		__enabled = setup.enabled;
		__profile = setup.profile;

		this->start_threads();
	}

	/*
	* Thread management
	*/
	void start_threads()
	{
		__hunting_thread = thread(&Hunter::__hunt, this);
	}
	void stop_threads()
	{
		__stop_threads = true;
		__hunting_thread.join();
		targeting.stop_threads();
	}
	void restart_threads()
	{
		if (__hunting_thread.joinable())
		{
			stop_threads();
			start_threads();
		}
		else
		{
			start_threads();
		}
	}

	void self_update_scene()
	{
		bool scene_initially_empty = __scene.empty();

		Mat old_scene = __scene.clone();
		__scene = __camera.capture_scene();

		if (!scene_initially_empty && !__cached_map_position_ROI.empty())
		{
			if (old_scene.size() == __scene.size() && old_scene.type() == __scene.type())
			{
				Mat diff;
				absdiff(old_scene(__cached_map_position_ROI), __scene(__cached_map_position_ROI), diff);

				double sum_of_all_px_different = sum(diff)[0];
				__map_scene_diff = sum_of_all_px_different;

				old_scene.release();
				diff.release();
			}
		}

		this_thread::sleep_for(chrono::milliseconds(constants::TIMER_REFRESH_RATE_HUNTER));
	}

	bool get_enabled() const
	{
		return __enabled;
	}

private:
	/* Utils */
	Camera __camera = Camera();
	Movement __movement = Movement();

	/* Scene related + ROI's */
	Mat __scene;
	double __map_scene_diff = 0.0;
	Rect __cached_map_position_ROI;

	/* Waypoint controller */
	int __waypoint_position = 0;
	string __waypoint_category = "start";

	/* Profile */
	Profile __profile;
	bool __enabled = false;

	/* Thread management */
	thread __hunting_thread;
	bool __stop_threads = false;

	/*
	* Thread call to run waypoints.
	*/
	void __hunt()
	{
		if (__enabled)
		{
			__stop_threads = false;
			cout << "Hunting: ENABLED" << endl;
		}
		else
			cout << "Hunting: DISABLED" << endl;

		while (!__stop_threads && __enabled)
		{
			self_update_scene();

			if (__stop_threads) break;

			if (!__scene.empty())
			{
				bool can_move = !this->targeting.is_attacking(__scene);
				bool already_moving = __get_is_walking();

				if (can_move && !already_moving)
				{
					__go_to_waypoint();
				}
			}
		}
	}

	/*
	* Hunter actions
	*/

	void __go_to_waypoint()
	{
		string wpt_method = __get_waypoint_method();

		if (wpt_method == "click")
		{
			if (__get_has_reached_destination())
				__next_waypoint();
			else
				__click_on_map();
		}

		else if (wpt_method.find("move") != string::npos)
			__apply_wpt_method_move();

		else if (wpt_method.find("action") != string::npos)
			__apply_wpt_action();

		else
			cout << "Couldn't identify waypoint methods" << endl;
	}

	/*
	* Getters related to waypointer and scene
	*/

	Mat __get_map()
	{
		if (__cached_map_position_ROI.empty())
		{
			__cache_map_ROI();
		}
		return __scene(__cached_map_position_ROI).clone();
	}

	string __get_waypoint_method() const
	{
		Waypoint current_waypoint = __get_waypoint_category()[__waypoint_position];
		return current_waypoint.method;
	}

	bool __get_has_reached_destination()
	{
		string directory = "Resources/Waypoints/" + __profile.name + "/";
		string filename = __waypoint_category + "_" + to_string(this->__waypoint_position) + ".jpg";
		string path = directory + filename;

		Mat needle = imread(path, IMREAD_UNCHANGED);

		if (needle.empty()) {
			cerr << "Error: Unable to load needle image at path: " << path << endl;
			return false;
		}

		Mat haystack = __get_map().clone();
		bool breaks_if_not_found = false;

		const Point point = this->__camera.find_needle(
			haystack, needle, 
			constants::MAP_FIND_WAYPOINT_THRESHOLD, breaks_if_not_found
		);

		if (point.x <= 0 || point.y <= 0)
		{
			cout << "Didn't find waypoint when checking if reached destination" << endl;
			return false;
		}
		else if (constants::MAP_THUMB_SIZE > haystack.cols - point.x || constants::MAP_THUMB_SIZE > haystack.rows - point.y)
		{
			cout << "Checking destination reached, Haystack smaller than Point";
			return false;
		}
		else
		{
			cvtColor(haystack, haystack, COLOR_BGR2GRAY);
			cvtColor(needle, needle, COLOR_BGR2GRAY);

			GaussianBlur(haystack, haystack, Size(5, 5), 0);
			GaussianBlur(needle, needle, Size(5, 5), 0);

			Mat diff;

			absdiff(
				haystack(Rect(
					point.x, point.y, 
					constants::MAP_THUMB_SIZE, constants::MAP_THUMB_SIZE
				)), needle, diff
			);

			Mat threshold_diff;
			threshold(diff, threshold_diff, 25, 255, THRESH_BINARY);

			int pixel_in_diff = countNonZero(threshold_diff);
			double simularity = 1.0 - (double(pixel_in_diff) / (constants::MAP_THUMB_SIZE * constants::MAP_THUMB_SIZE));
			
			haystack.release();
			needle.release();
			threshold_diff.release();
			
			int wpt_tolerance = __get_waypoint_category()[__waypoint_position].node_size;
			double min_tolerance = abs(wpt_tolerance * constants::MAP_DESTINATION_CHECK_SIMULARITY_MODIFIER - 1);
			bool reached_destination = simularity >= min_tolerance;

			cout << "Expected at least: " << min_tolerance << ", Got: " << simularity << endl;
			cout << "Has reached: " << reached_destination << endl;

			return reached_destination;
		}
	}

	vector<Waypoint> __get_waypoint_category() const
	{
		vector<Waypoint> waypoint_category;
		string category = this->__waypoint_category;

		if (category == "start")
		{
			waypoint_category = this->__profile.waypoint_category.start;
		}
		else if (category == "hunt")
		{
			waypoint_category = this->__profile.waypoint_category.hunt;
		}
		else if (category == "refil")
		{
			waypoint_category = this->__profile.waypoint_category.refil;
		}

		return waypoint_category;
	}

	Rect __get_waypoint_click_poisition()
	{
		vector<Waypoint> waypoints = __get_waypoint_category();
		Waypoint waypoint = waypoints[__waypoint_position];

		if (__waypoint_position < 0 || waypoints.size() == 0)
		{
			cout << "<!--		No waypoints found		-->" << endl;
			cout << "== Waypoint: " << this->__waypoint_position << endl;

			return Rect();
		}

		string profile_waypoint_folder_path = "Resources/Waypoints/" + __profile.name + "/";
		string waypoint_file_name = __waypoint_category + "_" + to_string(__waypoint_position) + ".jpg";

		Mat needle = imread(profile_waypoint_folder_path + waypoint_file_name, IMREAD_UNCHANGED);
		Mat haystack = __get_map();

		double threshold = constants::MAP_FIND_WAYPOINT_THRESHOLD;
		bool breaks_if_not_found = false;

		Point map_click_position = __camera.find_needle(
			haystack, needle, 
			threshold, breaks_if_not_found,
			true
		);

		if (map_click_position.x <= 0 || map_click_position.y <= 0)
		{
			cout << "<!--		Waypoint not found		-->" << endl;
			__next_waypoint();

			return Rect();
		}

		return Rect(
			map_click_position.x, map_click_position.y, 
			0, 0
		);
	}

	bool __get_is_walking() const
	{
		double sum_of_all_px_different = __map_scene_diff;
		double threshold = 20 * 1000;

		bool is_walking = sum_of_all_px_different >= threshold;

		return is_walking;
	}

	/*
	* Scene related actions
	*/

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

	void __next_waypoint()
	{
		vector<Waypoint> waypoint_category = __get_waypoint_category();

		__waypoint_position = (static_cast<unsigned long long>(this->__waypoint_position) + 1) % waypoint_category.size();
		cout << "== Waypoint: " << __waypoint_position << endl;
	}

	void __apply_wpt_action()
	{
		string wpt_method = __get_waypoint_method();
		vector<string> result = Helpers::split(wpt_method, ':');

		string action_name = result[1];
		string action_param = result[2];

		if (action_name == "wait")
		{
			__handle_action_wait(action_param);
		}
		else
		{
			__handle_action_use_item_direction(action_name, action_param);
			cout << "Couldn't identify waypoint action" << endl;
		}
	}

	void __apply_wpt_method_move()
	{
		const string current_wpt = __get_waypoint_method();
		const vector<string> result = Helpers::split(current_wpt, ':');
		const Movement::Direction direction = static_cast<Movement::Direction>(result[1][0]);

		if (current_wpt.find("ne") != string::npos)
			__movement.move(Movement::north_east);
		else if (current_wpt.find("nw") != string::npos)
			__movement.move(Movement::north_west);
		else if (current_wpt.find("se") != string::npos)
			__movement.move(Movement::south_east);
		else if (current_wpt.find("sw") != string::npos)
			__movement.move(Movement::south_west);
		else if (current_wpt.find("n") != string::npos)
			__movement.move(Movement::north);
		else if (current_wpt.find("s") != string::npos)
			__movement.move(Movement::south);
		else if (current_wpt.find("w") != string::npos)
			__movement.move(Movement::west);
		else if (current_wpt.find("e") != string::npos)
			__movement.move(Movement::east);

		this_thread::sleep_for(milliseconds(constants::DELAY_WAIT_AFTER_ARROW_MOVE));
		__next_waypoint();
	}

	void __click_on_map()
	{
		Rect map_position = __get_waypoint_click_poisition();

		if (map_position.x > 0 && map_position.y > 0)
		{
			int map_center = constants::MAP_THUMB_SIZE / 2;

			Rect click_position = Rect(
				__cached_map_position_ROI.x + map_position.x + map_center + constants::MAP_CLICK_NUDGE_X,
				__cached_map_position_ROI.y + map_position.y + map_center + constants::MAP_CLICK_NUDGE_Y,
				constants::MAP_THUMB_SIZE, constants::MAP_THUMB_SIZE
			);

			Mat scene = __scene.clone();

			if (
				click_position.x > scene.cols ||
				click_position.y > scene.rows ||
				click_position.x < 0 ||
				click_position.y < 0
				)
			{
				cout << "<!--		Map rectangle out of bounds		-->" << endl;
				return;
			}

			__movement.press(VK_ESCAPE);
			this_thread::sleep_for(milliseconds(80));

			__movement.click_mouse(click_position);
			scene.release();
		}
	}

	void __handle_action_wait(string wait_param)
	{
		int wait_time = stoi(wait_param);
		this_thread::sleep_for(milliseconds(wait_time));
		__next_waypoint();
	}

	void __handle_action_use_item_direction(string item, string direction)
	{
		if (item == "rope")
			__movement.use_item(hotkeys::USE_ROPE, direction);
		else if (item == "shovel")
			__movement.use_item(hotkeys::USE_SHOVEL, direction);

		this_thread::sleep_for(milliseconds(constants::DELAY_WAIT_AFTER_ARROW_MOVE));
		__next_waypoint();
	}
};

