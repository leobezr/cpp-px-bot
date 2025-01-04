#pragma once
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <condition_variable>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include "./Camera.h"
#include "./Player.h"
#include <ctime>

using namespace std;
using namespace chrono;

class Health
{
public:
	struct HealthConfig
	{
		int heal_on_health_percent;
		int heal_on_mana_percent;
		BOOL heal_hp_active;
		BOOL heal_mp_active;
	};

	Health(HealthConfig health_config) : __camera(Camera()), __player(Player())
	{
		this->__heal_on_health_percent = health_config.heal_on_health_percent;
		this->__heal_on_mana_percent = health_config.heal_on_mana_percent;
		this->__heal_hp_active = health_config.heal_hp_active;
		this->__heal_mp_active = health_config.heal_mp_active;

		this->__current_hp = 100;
		this->__current_mp = 100;

		this->start_threads();
	}

	void start_threads()
	{
		__update_health_thread = thread(&Health::__update_health_percentage, this);
		__heal_thread = thread(&Health::__heal, this);
	}

	void stop_threads()
	{
		this->__stop_threads = TRUE;
		__update_health_thread.join();
		__heal_thread.join();
	}

	void update_scene(cv::Mat scene)
	{
		{
			unique_lock<mutex> lock(scene_mutex);
			Rect screen_size = Camera::get_screen_size();
			Rect tibia_container = Rect(0, 0, 176, screen_size.height);
			Rect roi_area = Rect(screen_size.width - tibia_container.width, 0, tibia_container.width, tibia_container.height);
			Mat cropped_scene = scene(roi_area);

			__scene = cropped_scene;
			cvtColor(__scene, __scene, cv::COLOR_BGRA2BGR);
			cout << "<!--		HEALTH, Scene updated		-->" << endl;

			scene_ready = true;
		}
		scene_cv.notify_all();
	}

private:
	Camera __camera;
	Player __player;

	atomic_flag scene_updated = ATOMIC_FLAG_INIT;
	thread __update_health_thread, __heal_thread;
	mutex scene_mutex;
	Rect __cached_hp_bar_ROI, __cached_mp_bar_ROI;
	condition_variable scene_cv;
	bool __stop_threads = FALSE, scene_ready = FALSE;
	int __heal_on_health_percent, __heal_on_mana_percent;
	int __current_hp = 100, __current_mp = 100;
	bool __heal_hp_active = FALSE, __heal_mp_active = FALSE;

	cv::Mat __scene;

	void __update_health_percentage()
	{
		while (!__stop_threads)
		{
			unique_lock<mutex> lock(scene_mutex);
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

	void __heal()
	{
		while (!__stop_threads)
		{
			int current_hp, current_mp;
			{
				lock_guard<mutex> lock(scene_mutex);
				current_hp = __current_hp;
				current_mp = __current_mp;
			}

			if (__heal_hp_active && current_hp <= __heal_on_health_percent)
			{
				__player.cast(VK_F1);
				cout << "++ Healed HP, at percent: " << current_hp << "%" << endl;
			}

			if (__heal_mp_active && current_mp <= __heal_on_mana_percent)
			{
				__player.cast(VK_F3);
				cout << "++ Healed Mana, at percent: " << current_mp << "%" << endl;
			}

			this_thread::sleep_for(chrono::milliseconds(180));
		}
	}

	cv::Point __find_heart_icon()
	{
		BOOL mandatory = FALSE;
		string path_to_heart = "Resources/hp.png";
		cv::Mat heart = cv::imread(path_to_heart, cv::IMREAD_UNCHANGED);

		return this->__camera.find_needle(this->__scene, heart, 0.8, mandatory);
	}

	cv::Point __find_mana_icon()
	{
		BOOL mandatory = FALSE;
		string path_to_heart = "Resources/mana.png";
		cv::Mat heart = cv::imread(path_to_heart, cv::IMREAD_UNCHANGED);

		return this->__camera.find_needle(this->__scene, heart, 0.8, mandatory);
	}

	cv::Rect __get_hp_bar_ROI()
	{
		if (this->__cached_hp_bar_ROI.area() > 0)
		{
			return this->__cached_hp_bar_ROI;
		}
		else
		{
			cout << "Cached HP: " << this->__cached_hp_bar_ROI << endl;

			cv::Point heart = this->__find_heart_icon();
			int hp_bar_x = heart.x + 13, hp_bar_y = heart.y;
			int hp_bar_width = 94, hp_bar_height = 12;

			this->__cached_hp_bar_ROI = (Rect(
				hp_bar_x, hp_bar_y,
				hp_bar_width, hp_bar_height
			));

			return this->__cached_hp_bar_ROI;
		}
	}

	cv::Rect __get_mana_bar_ROI()
	{
		if (this->__cached_mp_bar_ROI.area() > 0)
		{
			return this->__cached_mp_bar_ROI;
		}
		else
		{
			cv::Point mana_icon = this->__find_mana_icon();

			int mp_bar_x = mana_icon.x + 13, mp_bar_y = mana_icon.y;
			int mp_bar_width = 95, mp_bar_height = 12;

			this->__cached_mp_bar_ROI = (Rect(
				mp_bar_x, mp_bar_y,
				mp_bar_width, mp_bar_height
			));

			return this->__cached_mp_bar_ROI;
		}
	}

	int __calculate_hp_bar_percentage(const cv::Mat& img) {
		int max_bar_size = 90;

		Mat mask;
		Scalar lower(80, 79, 216), upper(115, 115, 249);
		inRange(img, lower, upper, mask);
		
		int non_zero_count = cv::countNonZero(mask);
		if (non_zero_count == 0)
		{
			cout << "HEALTH BAR NOT FOUND IN THRESHOLD" << endl;
			return 100;
		}
		
		Rect bounding_box = cv::boundingRect(mask);
		int hp_bar_width = bounding_box.width;

		int hp_percentage = static_cast<double>(hp_bar_width) / max_bar_size * 100.0;
		return hp_percentage;
	}

	int __calculate_mp_bar_percentage(const cv::Mat& img) {
		int max_bar_size = 87;

		cv::Mat mask;
		Scalar lower(240, 113, 117), upper(250, 125, 129);
		cv::inRange(img, lower, upper, mask);

		int non_zero_count = cv::countNonZero(mask);
		if (non_zero_count == 0)
		{
			cout << "MANA BAR NOT FOUND IN THRESHOLD" << endl;
			return 100;
		}

		Rect bounding_box = cv::boundingRect(mask);

		int mp_bar_width = bounding_box.width;
		int mp_percentage = static_cast<double>(mp_bar_width) / max_bar_size * 100.0;
		return mp_percentage;
	}



	int __get_hp_percent()
	{
		if (this->__scene.empty() && this->__get_hp_bar_ROI().area() == 0)
		{
			cout << "<!--		Skipped Health percentage		-->" << endl;
			return 100;
		}
		else
		{
			Mat hp_bar = this->__scene(this->__get_hp_bar_ROI());
			int hp_percentage = this->__calculate_hp_bar_percentage(hp_bar.clone());

			cout << "= Health percent: " << hp_percentage << "%" << endl;

			return hp_percentage;
		}
	}

	int __get_mp_percent() {
		if (this->__scene.empty() && this->__get_mana_bar_ROI().area() == 0)
		{
			cout << "<!--		Skipped Mana percentage		-->" << endl;
			return 100;
		}
		else
		{
			cv::Mat mp_bar = this->__scene(this->__get_mana_bar_ROI()).clone();

			int mp_percentage = this->__calculate_mp_bar_percentage(mp_bar);
			cout << "= Mana percent: " << mp_percentage << "%" << endl;

			return mp_percentage;
		}
	}
};

