#pragma once
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <condition_variable>
#include <thread>
#include <mutex>
#include "../Helpers/Observer.h"
#include "./Camera.h"
#include "./Movement.h"

using namespace std;
using namespace chrono;
using namespace cv;

const Rect battle_window_size = Rect(0, 0, 175, 170);

class DeveloperTool : public Observer
{
public:
	struct DeveloperToolConfig
	{
		bool developer_mode_enabled;
		string profile_name;
	};

	DeveloperTool(DeveloperToolConfig dev_config)
	{
		__enabled = dev_config.developer_mode_enabled;
		__profile_name = dev_config.profile_name;
		
		start_threads();
	}

	/*
	* Observer pattern
	*/
	void update(const cv::Mat scene) override
	{
		if (__enabled)
		{
			{
				unique_lock<mutex> lock(mtx);
				__scene = scene.clone();
				__scene_ready = true;
			}
			__scene_cv.notify_all();
		}
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

private:
	/* Utils */
	Camera __camera = Camera();
	Movement __movement = Movement();

	/* Profile */
	bool __enabled;
	string __profile_name;

	/* Scene related and ROI's */
	Mat __scene;
	Rect __cached_battle_window_ROI;
	const Rect __battle_window_size = Rect(0, 0, 175, 170);

	/* Thread management */
	thread __developer_thread;
	mutex mtx;
	condition_variable __scene_cv;
	bool __stop_threads = false,
		__scene_ready = false;

	/* Waypoint Creator management */
	string __waypoint_category = "hunt";
	int __waypoint_position = 0;

	/*
	* Thread call to watch for developer actions.
	* Uses mutex lock when using scene.
	*/
	void __lookout_for_actions()
	{
		if (__enabled)
			cout << "<!-	Developer mode: ENABLED		->" << endl;

		while (!__stop_threads)
		{
			unique_lock<mutex> lock(mtx);
			__scene_cv.wait(lock, [this]() { return __scene_ready || __stop_threads; });

			if (__stop_threads) break;

			if (!__scene.empty())
			{
				__register_creature_being_followed();
			}
			__scene_ready = false;
		}
	}

	/*
	* Section register creatures to profile and take picture.
	*/
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

	/*
	* Scene cutters and cache
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
};

