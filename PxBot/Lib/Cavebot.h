#pragma once
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <windows.h>
#include <iostream>
#include "./Camera.h";
#include "./Movement.h"
#include "../Types/Profile.h"
#include "../Helpers/ProfileLoader.h"

using namespace cv;
using CreatureDirectory = unordered_map<string, Mat>;
using namespace std;

class Cavebot
{
public:
	struct CavebotConfig
	{
		bool cavebot_enabled;
		bool targeting_enabled;
	};

	Cavebot(CavebotConfig cavebot_setup) : __camera(Camera())
	{
		this->__movement = Movement();
		this->__scene = Mat();
		this->__cached_battle_window_ROI = Rect();
		
		this->__is_cavebot_enabled = cavebot_setup.cavebot_enabled;
		this->__is_targeting_enabled = cavebot_setup.targeting_enabled;
	}

	void register_waypoint_print()
	{
		Mat map_scene = this->__get_map_scene();
		// TODO: Check to see if using this Rect is easier to find the center of the map
		Rect map_area = Rect(20, 4, 107, 110);
		Mat map = map_scene(map_area);

		Rect map_center = Rect(map.cols / 2, map.rows / 2, 0, 0);
		int map_cut_width = this->__map_thumb_size / 2, map_cut_height = this->__map_thumb_size / 2;

		Rect map_cut = Rect(
			map_center.x - map_cut_width,
			map_center.y - map_cut_height,
			this->__map_thumb_size,
			this->__map_thumb_size
		);

		string profile_name = this->__hunting_profile.name;
		string directory = "Resources/Waypoints/" + profile_name + "/";
		string filename = __cavebot_category + "_" + to_string(this->__waypoint_position++);

		this->__camera.save_image(map(map_cut), directory, filename);
		this->__flicker_map(map_center);
	}

	void start_threads()
	{
		__cavebot_thread = thread(&Cavebot::__run_cavebot, this);
	}

	void pause()
	{
		this->__is_cavebot_paused = TRUE;
	}

	void jump_next_waypoint_category()
	{
		if (this->__cavebot_category == "start")
		{
			this->__cavebot_category = "hunt";
		}
		else if (this->__cavebot_category == "hunt")
		{
			this->__cavebot_category = "refil";
		}
		else if (this->__cavebot_category == "refil")
		{
			this->__cavebot_category = "start";
		}

		this->__waypoint_position = 0;
		cout << "Cavebot category: " << this->__cavebot_category << endl;
	}

	void register_creature_being_followed()
	{
		Mat scene = this->__scene.clone();
		cvtColor(scene, scene, COLOR_RGB2HSV);

		int win_width = 4, win_height = this->__battle_window_height - 15;
		Rect ROI_area = Rect(10, 15, win_width, win_height);
		
		Mat mask;
		Scalar lower(50, 108, 205), upper(113, 212, 255);
		inRange(scene, lower, upper, mask);

		int non_zero_count = countNonZero(mask(ROI_area));

		if (non_zero_count && !scene.empty())
		{
			Rect battle_window_position = Rect(this->__cached_battle_window_ROI.x, this->__cached_battle_window_ROI.y, this->__battle_window_width, this->__battle_window_height);
			Rect bounding_box = boundingRect(mask);

			this->__movement.press(VK_ESCAPE);
			Sleep(120);

			Mat new_scene = this->__camera.capture_scene();

			Rect creature_position = Rect(
				battle_window_position.x + bounding_box.x + 5,
				battle_window_position.y + bounding_box.y,
				40, 15
			);

			string directory = "Resources/Screenshots/";
            this->__camera.save_image(new_scene(creature_position), directory);
			cout << "Image saved" << endl;
		}
	}

	void prompt_profile_name(string profile_name = "")
	{
		if (!profile_name.empty())
		{
			this->use_profile(profile_name);
			return;
		}

		cout << "Enter profile name: ";
		getline(cin, profile_name);

		if (profile_name.empty())
		{
			cerr << "Profile name is empty." << endl;
			this->prompt_profile_name();
		}
		else
		{
			try
			{
				this->use_profile(profile_name);
			}
			catch(string& error)
			{
				cout << error << endl;
				this->prompt_profile_name();
			}
		}
	}

	void use_profile(string profile_name)
	{
		try {
			Profile profile = ProfileLoader::load_profile(profile_name);
			cout << "Loaded profile: " << profile.name << endl;

			this->__hunting_profile = profile;
			this->__cache_creature_images();
			this->start_threads();
		}
		catch (string& error)
		{
			throw error;
		}
	}

	void stop_threads()
	{
		this->__stop_threads = TRUE;
		this->__scene_cv.notify_all();
		this->__cavebot_thread.join();
	}

	void const update_scene(Mat scene)
	{
		{
			unique_lock<mutex> lock(this->__scene_mutex);

			Mat scene_clone;
			cvtColor(scene, scene_clone, cv::COLOR_BGRA2BGR);

			this->__entire_scene = scene_clone;
			this->__find_and_cache_map(scene_clone);

			Mat battle_window = __cut_battle_window(scene_clone);
			this->__scene = battle_window;
			this->__has_attack_square(battle_window);

			this_thread::sleep_for(chrono::milliseconds(500));
			this->__scene_ready = TRUE;
		}
		this->__scene_cv.notify_all();
	}

private:
	Camera __camera;
	Movement __movement;
	Profile __hunting_profile;
	CreatureDirectory __cached_creature_images;
	Mat __scene;
	Mat __entire_scene;
	Mat __previous_map_scene;

	thread __cavebot_thread;
	mutex __scene_mutex;
	condition_variable __scene_cv;
	bool __scene_ready = FALSE, __stop_threads = FALSE;

	int const __battle_window_width = 175;
	int const __battle_window_height = 170;
	Rect __cached_battle_window_ROI;
	Rect __cached_map_position_ROI;

	bool __is_cavebot_enabled = FALSE;
	bool __is_targeting_enabled = FALSE;
	bool __is_attacking = FALSE;
	bool __is_cavebot_paused = FALSE;

	int __waypoint_position = 0;
	int __map_thumb_size = 18 * 2;
	
	string __cavebot_category = "start";

	Mat const __cut_battle_window(Mat scene)
	{
		if (this->__cached_battle_window_ROI.area() != 0)
		{
			return scene(this->__cached_battle_window_ROI);
		}

		Mat needle = imread("Resources/battle-list.png", IMREAD_UNCHANGED);
		bool const breaks_if_not_found = TRUE;
		int const win_width = this->__battle_window_width, win_height = this->__battle_window_height;

		double threshold = .49;
		Point battle_window = this->__camera.find_needle(scene, needle, threshold, breaks_if_not_found);

		int pos_x = battle_window.x - 10,
			pos_y = battle_window.y;

		Rect ROI_area = Rect(pos_x, pos_y, win_width, win_height);

		if (ROI_area.x == 0 || ROI_area.y == 0)
		{
			cout << "<!--		ERROR! Battle window not found		->" << endl;
			return scene;
		}

		this->__cache_in_battle_window_ROI(ROI_area);
		return scene(ROI_area);
	}

	void __run_cavebot()
	{
		cout << "<!--		Cavebot Started		-->" << endl;
		this->__waypoint_position = 0;
		cout << "== Waypoint: " << this->__waypoint_position << endl;

		while (this->__is_cavebot_enabled)
		{
			unique_lock<mutex> lock(this->__scene_mutex);
			this->__scene_cv.wait(lock, [this]() { return this->__scene_ready || this->__stop_threads || !this->__is_cavebot_enabled; });

			if (!this->__is_cavebot_enabled || this->__stop_threads) break;

			if (this->__is_cavebot_paused)
			{
				cout << "Cavebot paused." << endl;
				return;
			}

			if (!this->__is_attacking && this->__is_targeting_enabled)
			{
				this->__attack_possible_creatures();
			}

			if (this->__reached_waypoint_destination())
			{
				this->__next_waypoint();
			}

			Mat current_map_scene = this->__get_map_scene();
			Mat previous_map_scene = this->__previous_map_scene;
			double threshold = 20 * 1000;

			if (!this->__is_character_moving(previous_map_scene, current_map_scene, threshold))
			{
				this->__run_waypoints();
				this->__scene_ready = FALSE;
			}
			Sleep(400);
		}
	}

	vector<Waypoint> __get_waypoint_category() const
	{
		vector<Waypoint> waypoint_category;
		string category = this->__cavebot_category;

		if (category == "start")
		{
			waypoint_category = this->__hunting_profile.waypoint_category.start;
		}
		else if (category == "hunt")
		{
			waypoint_category = this->__hunting_profile.waypoint_category.hunt;
		}
		else if (category == "refil")
		{
			waypoint_category = this->__hunting_profile.waypoint_category.refil;
		}

		return waypoint_category;
	}

	void __run_waypoints()
	{
		Rect waypoint = this->__find_waypoint_map();

		if (waypoint.area() > 0)
		{
			{
				Mat map_scene = this->__get_map_scene();
				int map_offset_x = 0, map_offset_y = 21;

				Rect map_click_position = Rect(
					__cached_map_position_ROI.x + waypoint.x + (__map_thumb_size / 2) + map_offset_x,
					__cached_map_position_ROI.y + waypoint.y + (__map_thumb_size / 2) + map_offset_y,
					waypoint.width, waypoint.height
				);

				Mat cloned = __entire_scene.clone();
				int rect_size = 15;

				if (
					map_click_position.x > cloned.cols ||
					map_click_position.y > cloned.rows ||
					map_click_position.x < 0 ||
					map_click_position.y < 0
					)
				{
					cout << "<!--		Map rectangle out of bounds		-->" << endl;
					return;
				}

				this->__movement.click_mouse(map_click_position);
				this->__movement.mouse_over(Rect(500, 300, 0, 0));
			}
			this_thread::sleep_for(chrono::milliseconds(320));
		}
	}

	Mat __get_map_scene()
	{
		Mat map_scene = this->__entire_scene(this->__cached_map_position_ROI);

		if (this->__previous_map_scene.empty())
		{
			this->__previous_map_scene = map_scene.clone();
		}

		return map_scene;
	}

	void const __find_and_cache_map(Mat& scene)
	{
		Rect cached_map_ROI = this->__cached_map_position_ROI;

		if (cached_map_ROI.empty())
		{
			Mat needle = imread("Resources/map-buttons.png", IMREAD_UNCHANGED);
			double threshold = .6;
			bool breaks_if_not_found = TRUE;
			Point map_position = this->__camera.find_needle(scene, needle, threshold, breaks_if_not_found);

			int pos_x = map_position.x - 10,
				pos_y = map_position.y;

			int map_width = 176, map_height = 116;
			int map_btn_width = 59, map_btn_height = 70;
			int offset_x = map_width - map_btn_width;
			int offset_y = map_height - map_btn_height;
			
			Rect ROI_area = Rect(
				pos_x - offset_x, pos_y - offset_y, 
				map_width, map_height
			);

			if (ROI_area.x == 0 || ROI_area.y == 0)
			{
				cout << "<!--		ERROR! Map not found		->" << endl;
				return;
			}

			this->__cached_map_position_ROI = ROI_area;
		}
	}

	bool __is_character_moving(Mat& previous_scene, Mat& current_scene, double threshold)
	{
		bool is_walking = FALSE;

		{
			Mat diff;
			absdiff(previous_scene, current_scene, diff);
			double sum_of_all_px_different = sum(diff)[0];
			is_walking = sum_of_all_px_different >= threshold;
			this->__previous_map_scene = current_scene.clone();

			if (is_walking)
			{
				cout << "<!--		Character is walking		-->" << endl;
			}
			else {
				cout << "<!--		Character is standing		-->" << endl;
			}

			imshow("previous", previous_scene);
			imshow("current", current_scene);
			waitKey(1);
		}
		this_thread::sleep_for(chrono::milliseconds(240));

		return is_walking;
	}

	Rect __find_waypoint_map()
	{
		string category = this->__cavebot_category;
		int position = this->__waypoint_position;

		vector<Waypoint> waypoint_category = this->__get_waypoint_category();
		Waypoint waypoint;

		if (position < 0 || waypoint_category.size() == 0)
		{
			cout << "<!--		No waypoints found		-->" << endl;
			cout << "== Waypoint: " << this->__waypoint_position << endl;

			return Rect();
		}
		else if (position > waypoint_category.size())
		{
			this->__next_waypoint();
			return this->__find_waypoint_map();
		}

		waypoint = waypoint_category[position];

		string map_node_directory = "Resources/Waypoints/" + this->__hunting_profile.name + "/";
		string map_node_filename = category + "_" + to_string(position) + ".jpg";
		Mat map_node = imread(map_node_directory + map_node_filename, IMREAD_UNCHANGED);
		Mat map_scene = this->__get_map_scene();

		double threshold = 0.85;
		bool breaks_if_not_found = FALSE;
		Mat needle = map_node.clone(), haystack = map_scene.clone();

		Point map_click_position = this->__camera.find_needle(
			haystack, needle, threshold, 
			breaks_if_not_found
		);
			
		if (map_click_position.x <= 0 || map_click_position.y <= 0)
		{
			cout << "<!--		Waypoint not found		-->" << endl;
			this->__next_waypoint();
			
			return Rect();
		}

		return Rect(map_click_position.x, map_click_position.y, this->__map_thumb_size, this->__map_thumb_size);
	}

	void __flicker_map(Rect map_center)
	{		
		Rect map_position = this->__cached_map_position_ROI;
		Rect mouse_over_position = Rect(map_position.x + map_center.x, map_position.y + map_center.y, 0, 0);

		this->__movement.mouse_over(mouse_over_position);
		this->__movement.scroll(-1, 2);
		this->__movement.scroll(1, 2);

		Rect out_of_the_way_position = Rect(500, 200, 0, 0);
		this->__movement.mouse_over(out_of_the_way_position);
	}

	void __attack_possible_creatures()
	{
		if (this->__hunting_profile.target.empty())
		{
			cout << "No targets to attack." << endl;
			return;
		}
		else
		{
			this->__find_creature_in_battle();
		}
	}

	bool __reached_waypoint_destination() {
		Mat map_scene = this->__get_map_scene();
		string profile_name = this->__hunting_profile.name;
		string directory = "Resources/Waypoints/" + profile_name + "/";
		string filename = __cavebot_category + "_" + to_string(this->__waypoint_position) + ".jpg";
		string path = directory + filename;

		Mat needle = imread(path, IMREAD_UNCHANGED);
		if (needle.empty()) {
			cerr << "Error: Unable to load needle image at path: " << path << endl;
			return FALSE;
		}

		Mat haystack = map_scene;

		double threshold = 0.85;
		bool breaks_if_not_found = FALSE;
		const Point point = this->__camera.find_needle(haystack, needle, threshold, breaks_if_not_found);

		int expected_pos_x = 55, expected_pos_y = 41;
		int tolerance = this->__get_waypoint_category()[this->__waypoint_position].node_size;

		if ((point.x >= expected_pos_x - tolerance && point.x <= expected_pos_x + tolerance) &&
			(point.y >= expected_pos_y - tolerance && point.y <= expected_pos_y + tolerance)) {
			return TRUE;
		}

		return FALSE;
	}


	void __next_waypoint()
	{
		vector<Waypoint> waypoint_category = this->__get_waypoint_category();
		this->__waypoint_position = (static_cast<unsigned long long>(this->__waypoint_position) + 1) % waypoint_category.size();
		cout << "== Waypoint: " << this->__waypoint_position << endl;
	}

	void __find_creature_in_battle()
	{
		for (auto& target : this->__hunting_profile.target)
		{
			
			bool breaks_if_not_found = FALSE;
			double haystack_needle_threshold = 0.7;
			
			Point creature_position = this->__camera.find_needle(
				this->__scene, 
				this->__cached_creature_images[target.name], 
				haystack_needle_threshold,
				breaks_if_not_found,
				FALSE
			);

			bool creature_found = creature_position.x > 0 && creature_position.y > 0;

			if (creature_found)
			{
				Rect battle_window_position = Rect(this->__cached_battle_window_ROI.x, this->__cached_battle_window_ROI.y, this->__battle_window_width, this->__battle_window_height);

				Rect click_position = Rect(
					battle_window_position.x + creature_position.x + 10,
					battle_window_position.y + creature_position.y + 25,
					0, 0
				);

				this->__movement.click_mouse(click_position);
				break;
			}
		}
	}

	void __cache_creature_images()
	{
		for (auto& target : this->__hunting_profile.target)
		{
			bool is_cached = this->__cached_creature_images.find(target.name) != this->__cached_creature_images.end();

			if (is_cached)
			{
				continue;
			}
			else
			{
				this->__cached_creature_images[target.name] = imread("Resources/Screenshots/" + target.name + ".jpg", IMREAD_UNCHANGED);
			}
		}
	}

    void __cache_in_battle_window_ROI(Rect ROI_area)
    {
		this->__cached_battle_window_ROI = Rect(
			ROI_area.x, ROI_area.y, 
			ROI_area.width, ROI_area.height
		);
    }

	void __press_auto_loot()
	{
		for (int i = 0; i < 5; i++)
		{
			this->__movement.press(VK_OEM_3);
			Sleep(20);
		}
	}

	bool const __has_attack_square(Mat scene)
	{
		Mat scene_copy = scene.clone();

		cvtColor(scene_copy, scene_copy, COLOR_BGR2HSV);
		int win_width = 4, win_height = this->__battle_window_height - 15;
		Rect ROI_area = Rect(10, 15, win_width, win_height);

		Mat mask;
		Scalar lower(0, 125, 114), upper(0, 201, 255);
		inRange(scene_copy, lower, upper, mask);

		int non_zero_count = countNonZero(mask(ROI_area));
		bool is_attacking = non_zero_count != 0;
		
		if (is_attacking && this->__is_attacking == FALSE)
		{
			this->__is_attacking = is_attacking;
			cout << "<!--		ATTACKING = TRUE		-->" << endl;
		}
		else if (!is_attacking && this->__is_attacking == TRUE)
		{
			this->__is_attacking = FALSE;
			this->__press_auto_loot();
			cout << "<!--		ATTACKING = FALSE		-->" << endl;
		}

		return non_zero_count != 0;
	}
};

