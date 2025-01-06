#pragma once
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include "./Camera.h"
#include "../Types/Profile.h"
#include "./Movement.h"

using namespace std;
using namespace chrono;
using namespace cv;
using CreatureDirectory = unordered_map<string, Mat>;

class Targeting
{
public:
	struct TargetingConfig
	{
		bool targeting_enabled;
		vector<Target> targets;
	};

	Targeting(TargetingConfig targeting_config)
	{
		__enabled = targeting_config.targeting_enabled;
		__targets = targeting_config.targets;

		this->__sort_targets_by_priority();
		this->__hash_creature_images();

		this->start_threads();
	}

	/* 
	* Thread management 
	*/
	void start_threads()
	{
		__targeting_thread = thread(&Targeting::__update_targeting, this);
	}
	void stop_threads()
	{
		__targeting_thread.join();
		__stop_threads = TRUE;
	}
	void self_update_scene()
	{
		if (!__scene.empty())
		{
			__scene.release();
		}
		__scene = __camera.capture_scene();
	}

private:
	/* Utils */
	Camera __camera = Camera();
	Movement __movement = Movement();

	/* Scene related + ROI's */
	Mat __scene;
	Rect __cached_battle_window_ROI;
	const Rect __battle_window_size = Rect(0, 0, 175, 170);
	CreatureDirectory __hashed_creature_images;
	bool __is_attacking = FALSE;

	/* Profile */
	vector<Target> __targets;
	bool __enabled = FALSE;

	/* Thread management */
	thread __targeting_thread;
	
	bool __stop_threads = FALSE,
		__scene_ready = FALSE;

	/*
	* Thread call to target the elements.
	* Uses mutex lock receiving new scene.
	*/

	void __update_targeting()
	{
		if (__enabled)
			__log_profile_targets();
		else
			cout << "Targeting is: DISABLED" << endl;

		while (!__stop_threads && __enabled)
		{
			self_update_scene();

			if (__stop_threads) break;
			
			if (!__scene.empty() && !__get_is_attacking())
			{
				__target_elements();
			}
		}
	}

	/*
	* Deals with targeting the elements.
	*/

	void __target_elements()
	{
		if (__get_has_targeting_profile())
		{
			Rect creature_position = __get_creature_position_in_battle();

			if (creature_position.x > 0 && creature_position.y > 0)
			{
				Rect click_position = Rect(
					__cached_battle_window_ROI.x + creature_position.x + 10,
					__cached_battle_window_ROI.y + creature_position.y + 25,
					0, 0
				);

				__movement.click_mouse(click_position);
				__movement.mouse_over(Rect(click_position.x - 300, click_position.y, 0, 0));
			}
		}
	}

	void __press_auto_loot()
	{
		for (int i = 0; i < 3; i++)
		{
			__movement.press(VK_OEM_3);
		}
	}

	/*
	* Getters related to targeting and scene
	*/

	bool __get_has_targeting_profile()
	{
		if (__targets.empty())
		{
			cout << "<!--	Profile doesn't contain targets -->." << endl;
			return FALSE;
		}

		return TRUE;
	}

	bool __get_is_attacking()
	{
		int win_width = 4, win_height = __battle_window_size.height - 15;
		Rect ROI_area = Rect(10, 15, win_width, win_height);

		Mat mask, battle_window;
		Scalar lower(0, 125, 114), upper(0, 201, 255);
		cvtColor(__get_battle_window(), battle_window, cv::COLOR_BGR2HSV);
		inRange(battle_window, lower, upper, mask);

		bool is_attacking = countNonZero(mask(ROI_area)) != 0;

		if (is_attacking && __is_attacking == FALSE)
		{
			__is_attacking = TRUE;
			cout << "<!--	Attacking: TRUE	-->" << endl;
			this->__press_auto_loot();
		}
		else if (!is_attacking && __is_attacking == TRUE)
		{
			__is_attacking = FALSE;
			cout << "<!--	Attacking: FALSE	-->" << endl;
			this->__press_auto_loot();
		}

		battle_window.release();
		mask.release();

		return is_attacking;
	}

	Mat __get_battle_window()
	{
		if (__cached_battle_window_ROI.empty())
		{
			__cache_battle_window_ROI();
		}
		return __scene(__cached_battle_window_ROI);
	}

	Rect __get_creature_position_in_battle()
	{
		Rect creature_position = Rect();

		for (auto& target : __targets)
		{
			bool breaks_if_not_found = FALSE;
			double threshold = 0.85;

			Mat haystack = __get_battle_window();
			Mat needle = __hashed_creature_images[target.name];

			Point creature_position_point = __camera.find_needle(
				haystack, needle,
				threshold, breaks_if_not_found
			);

			creature_position = Rect(
				creature_position_point.x, creature_position_point.y,
				needle.cols, needle.rows
			);

			haystack.release();
			needle.release();

			if (creature_position.x > 0 && creature_position.y > 0)
			{
				break;
			}
		}
		return creature_position;
	}

	/*
	* Scene cutter and cache
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

	void __hash_creature_images()
	{
		for (auto& target : __targets)
		{
			bool is_cached = __hashed_creature_images.find(target.name) != __hashed_creature_images.end();

			if (is_cached)
				continue;
			else
				__hashed_creature_images[target.name] = imread("Resources/Screenshots/" + target.name + ".jpg", IMREAD_UNCHANGED);
		}
	}

	/*
	* Logger and helper methods
	*/

	void __log_profile_targets()
	{
		if (__get_has_targeting_profile())
		{
			cout << "<!-	Targeting ENABLED, Targets:	->" << endl;

			for (auto& target : __targets)
			{
				cout << "• Name: " << target.name << endl;
				cout << "• Priority: " << target.priority << endl;
				cout << "----------------------" << endl;
			}
		}
	}

	void __sort_targets_by_priority()
	{
		sort(__targets.begin(), __targets.end(), [this](const Target& a, const Target& b) {
			return a.priority > b.priority;
		});
	}
};

