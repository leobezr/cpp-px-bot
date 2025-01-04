#pragma once
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <unordered_map>
#include <windows.h>
#include <iostream>
#include "./Camera.h";
#include "./Movement.h"
#include "../Types/Profile.h"
#include "../Helpers/ProfileLoader.h"
#include "../Helpers/Toolbar.h"

using namespace cv;
using CreatureDirectory = unordered_map<string, Mat>;

class Cavebot
{
public:
	struct CavebotConfig
	{
		bool cavebot_enabled;
		bool targeting_enabled;
	};

	// REMOVE LATER
	//Toolbar toolbar;

	Cavebot(CavebotConfig cavebot_setup) : __camera(Camera())
	{
		this->__movement = Movement();
		this->__scene = Mat();
		this->__cached_battle_window_ROI = Rect();
		
		this->__is_cavebot_enabled = cavebot_setup.cavebot_enabled;
		this->__is_targeting_enabled = cavebot_setup.targeting_enabled;
		
		// REMOVE LATER
		//this->toolbar = Toolbar();
	}

	void register_creature_being_follow()
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

            this->__camera.save_image(new_scene(creature_position));
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
		}
		catch (string& error)
		{
			throw error;
		}
	}

	bool const update_scene(Mat scene)
	{
		Mat scene_clone;
		cvtColor(scene, scene_clone, cv::COLOR_BGRA2BGR);

		Mat battle_window = __cut_battle_window(scene_clone);
		this->__scene = battle_window;

		this->__has_attack_square(battle_window);

		if (!this->__is_attacking && this->__is_targeting_enabled)
		{
			this->__attack_possible_creatures();
		}

		return false;
	}

private:
	Camera __camera;
	Mat __scene;
	Movement __movement;
	Profile __hunting_profile;
	CreatureDirectory __cached_creature_images;

	int const __battle_window_width = 175;
	int const __battle_window_height = 170;
	Rect __cached_battle_window_ROI;

	bool __is_cavebot_enabled = FALSE;
	bool __is_targeting_enabled = FALSE;
	bool __is_attacking = FALSE;

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
		Point battle_window = this->__camera.find_needle(scene, needle, threshold, breaks_if_not_found, TRUE);

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
				TRUE
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
		for (int i = 0; i < 3; i++)
		{
			this->__movement.press(VK_OEM_3);
			Sleep(150);
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

