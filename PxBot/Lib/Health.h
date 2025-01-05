#pragma once
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <condition_variable>
#include <thread>
#include <mutex>
#include <chrono>
#include <ctime>
#include "./Camera.h"
#include "../Types/Profile.h"
#include "../Helpers/Observer.h"
#include "./Movement.h"

using namespace std;
using namespace chrono;
using namespace cv;

constexpr int hp_bar_ROI_offset_x = 13;
constexpr int hp_bar_width = 94;
constexpr int hp_bar_height = 12;

constexpr int mp_bar_ROI_offset_x = 13;
constexpr int mp_bar_width = 95;
constexpr int mp_bar_height = 12;

class Health : public Observer
{
public:
	struct HealthConfig
	{
		Healing healing;
		bool enabled;
	};

	Health(HealthConfig health_config) : __camera(Camera())
	{
		__healing_profile = health_config.healing;
		__enabled = health_config.enabled;

		this->start_threads();
	}

	/*
	* Oserver pattern.
	*/
	void update(const cv::Mat scene) override
	{
		{
			unique_lock<mutex> lock(mtx);
			Rect screen_size = Camera::get_screen_size();
			Rect tibia_container = Rect(0, 0, 176, screen_size.height);
			Rect roi_area = Rect(screen_size.width - tibia_container.width, 0, tibia_container.width, tibia_container.height);
			Mat cropped_scene = scene(roi_area);

			__scene = cropped_scene.clone();
			cropped_scene.release();
			cvtColor(__scene, __scene, cv::COLOR_BGRA2BGR);

			scene_ready = true;
		}
		scene_cv.notify_all();
	}

	/*
	* Thread management
	*/
	void start_threads()
	{
		if (__enabled)
		{
			__update_health_thread = thread(&Health::__update_health_percentage, this);
			__heal_thread = thread(&Health::__heal, this);
		}
	}
	void stop_threads()
	{
		this->__stop_threads = TRUE;
		__update_health_thread.join();
		__heal_thread.join();
	}

private:
	/* Profile */
	Healing __healing_profile;
	int __current_hp = 100, __current_mp = 100;
	bool __enabled = false;

	/* Scene related + ROI's */
	Mat __scene;
	Rect __cached_hp_bar_ROI, __cached_mp_bar_ROI;

	/* Utils */
	Camera __camera;
	Movement __movement;

	/* Thread management */
	thread __update_health_thread, __heal_thread;
	mutex mtx;
	condition_variable scene_cv;
	bool __stop_threads = false, 
		scene_ready = false;

	/* Prompt management */
	int __prompt_hp_counter = 0;
	int __prompt_mp_counter = 0;
	const int __counter_avoid_spam_limit = 30;
	
	/*
	* After scene is updated, 
	* this function will calculate the health and mana percentage.
	*/
	void __update_health_percentage()
	{
		while (!__stop_threads)
		{
			unique_lock<mutex> lock(mtx);
			scene_cv.wait(lock, [this]() { return scene_ready || __stop_threads; });

			if (__stop_threads) break;

			if (!__scene.empty())
			{
				__current_hp = __get_hp_percent();
				__current_mp = __get_mp_percent();
			}
			scene_ready = false;
		}
	}

	/*
	* Thread call to heal the player.
	* Uses mutex lock when updating health/mana percentages.
	*/
	void __heal()
	{
		if (__enabled) 
			cout << "<!-	Healing is: ENABLED		->" << endl;
		else
			cout << "<!-	Healing is: DISABLED	->" << endl;

		/* HP and Mana values,
		* Protected by mutex */
		int current_hp, current_mp;

		/* Healing Rules */
		HealingRule low_hp_rule = __healing_profile.low_health;
		HealingRule high_hp_rule = __healing_profile.high_health;
		HealingRule mana_rule = __healing_profile.mana;

		/* Hotkeys for key pressing */
		int key_for_low_hp = VK_F1;
		int key_for_high_hp = VK_F2;
		int key_for_mana = VK_F3;

		while (!__stop_threads && __enabled)
		{
			{
				lock_guard<mutex> lock(mtx);
				current_hp = __current_hp;
				current_mp = __current_mp;
			}

			/*
			 * Checking health conditions */
			if (current_hp >= low_hp_rule.min_health && current_hp < low_hp_rule.max_health)
			{
				__movement.press(key_for_low_hp);
			}
			else if (current_hp >= high_hp_rule.min_health && current_hp < high_hp_rule.max_health)
			{
				__movement.press(key_for_high_hp);
			}

			/*
			 * Checking mana conditions */
			if (current_mp >= mana_rule.min_mana && current_mp < mana_rule.max_mana)
			{
				__movement.press(key_for_mana);
			}

			/* Thread management */
			int exhaust_duration_avoid_puff = 180;
			this_thread::sleep_for(chrono::milliseconds(exhaust_duration_avoid_puff));
		}
	}

	/*
	* Space reserved to implement the following methods:
	* Methods to find ROI of health and mana bars.
	*
	* Looks for heart icon of hp */
	Point __find_heart_icon()
	{
		bool mandatory = FALSE;
		Mat needle = cv::imread("Resources/hp.png", cv::IMREAD_UNCHANGED);
		double threshold = 0.8;

		Point heart_position = this->__camera.find_needle(
			this->__scene, 
			needle, 
			threshold, 
			mandatory
		);

		needle.release();
		return heart_position;
	}
	Point __find_mana_icon()
	{
		BOOL mandatory = FALSE;
		Mat needle = cv::imread("Resources/mana.png", cv::IMREAD_UNCHANGED);
		double threshold = 0.8;

		Point mana_position = this->__camera.find_needle(
			this->__scene, 
			needle, 
			threshold, 
			mandatory
		);
		
		needle.release();
		return mana_position;
	}

	/*
	* Area to determine ROI
	*/

	Rect __get_hp_bar_ROI()
	{
		if (this->__cached_hp_bar_ROI.area() > 0)
		{
			return this->__cached_hp_bar_ROI;
		}
		else
		{
			cv::Point heart = this->__find_heart_icon();
			int hp_bar_x = heart.x + hp_bar_ROI_offset_x, hp_bar_y = heart.y;

			this->__cached_hp_bar_ROI = (Rect(
				hp_bar_x, hp_bar_y,
				hp_bar_width, hp_bar_height
			));

			return this->__cached_hp_bar_ROI;
		}
	}
	Rect __get_mana_bar_ROI()
	{
		if (this->__cached_mp_bar_ROI.area() > 0)
		{
			return this->__cached_mp_bar_ROI;
		}
		else
		{
			cv::Point mana_icon = this->__find_mana_icon();

			int mp_bar_x = mana_icon.x + mp_bar_ROI_offset_x, mp_bar_y = mana_icon.y;

			this->__cached_mp_bar_ROI = (Rect(
				mp_bar_x, mp_bar_y,
				mp_bar_width, mp_bar_height
			));

			return this->__cached_mp_bar_ROI;
		}
	}

	/*
	* Area to read scene and calculate health and mana percentages.
	*/

	int __calculate_hp_bar_percentage(const cv::Mat& img) {
		const int max_bar_size = 90;

		Mat mask;
		const Scalar lower(80, 79, 216), upper(115, 115, 249);
		inRange(img, lower, upper, mask);
		
		if (countNonZero(mask) == 0)
		{
			mask.release();
			return 0;
		}
		
		const Rect bounding_box = boundingRect(mask);
		const int hp_bar_width = bounding_box.width;
		const int hp_percentage = static_cast<double>(hp_bar_width) / max_bar_size * 100.0;

		mask.release();

		return hp_percentage;
	}

	int __calculate_mp_bar_percentage(const cv::Mat& img) {
		const int max_bar_size = 87;

		Mat mask;
		const Scalar lower(240, 113, 117), upper(250, 125, 129);
		inRange(img, lower, upper, mask);

		if (countNonZero(mask) == 0)
		{
			mask.release();
			return 0;
		}

		const Rect bounding_box = cv::boundingRect(mask);
		const int mp_bar_width = bounding_box.width;
		const int mp_percentage = static_cast<double>(mp_bar_width) / max_bar_size * 100.0;

		mask.release();

		return mp_percentage;
	}

	/*
	* Area for getters of 
	* health and mana percentages. 
	* 
	* Called by the thread to update health and mana.
	*/

	int __get_hp_percent()
	{
		if (this->__scene.empty() && this->__get_hp_bar_ROI().area() == 0)
		{
			cout << "<!--		Scene was empty, couldn't check health conditions		-->" << endl;
			return 0;
		}
		else
		{
			Mat hp_bar = this->__scene(this->__get_hp_bar_ROI());
			int hp_percentage = this->__calculate_hp_bar_percentage(hp_bar.clone());
			hp_bar.release();

			if (__prompt_hp_counter++ % __counter_avoid_spam_limit == 0)
			{
				cout << "= Health percent: " << hp_percentage << "%" << endl;
			}

			return hp_percentage;
		}
	}

	int __get_mp_percent() {
		if (this->__scene.empty() && this->__get_mana_bar_ROI().area() == 0)
		{
			cout << "<!--		Scene was empty, couldn't check mana conditions		-->" << endl;
			return 0;
		}
		else
		{
			Mat mp_bar = this->__scene(this->__get_mana_bar_ROI()).clone();
			int mp_percentage = this->__calculate_mp_bar_percentage(mp_bar);
			mp_bar.release();

			if (__prompt_mp_counter++ % __counter_avoid_spam_limit == 0)
			{
				cout << "= Mana percent: " << mp_percentage << "%" << endl;
			}

			return mp_percentage;
		}
	}
};

