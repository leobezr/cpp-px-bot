#pragma once
#include <windows.h>
#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include "./Camera.h";
#include "./Health.h";
#include "./Cavebot.h";
#include "./Targeting.h";
#include "../Helpers/Observer.h"
#include <mutex>

using namespace std;
using namespace cv;

class Player
{
public:
	struct PlayerConfig
	{
		Profile profile;
		bool targeting_enabled;
		bool cavebot_enabled;
		bool healing_enabled;
		bool developer_mode_enabled;
	};

    Player(PlayerConfig player_setup) :__profile(player_setup.profile)
    {
        Health* health = new Health({ 
			player_setup.profile.healing, 
			player_setup.healing_enabled 
		});
        add_observer(health);

		__start_bot_main_thread();
    }

	/*
	* Handles the player's scene update.
	* Update all child classes from single scene.
	* 
	* Threads should be handled internally by the class.
	*/
	void const update_scene()
	{
		{
			lock_guard<mutex> lock(mtx);
			__scene = __camera.capture_scene();
		}
		notify_observers();
	}

	/*
	* Observer pattern.
	*/
	void add_observer(Observer* observer)
	{
		__observers.push_back(observer);
	}
	void remove_observer(Observer* observer)
	{
		__observers.erase(remove(__observers.begin(), __observers.end(), observer), __observers.end());
	}
	void notify_observers()
	{
		for (Observer* observer : __observers)
		{
			observer->update(__scene);
		}
	}

private:
	/* Reserved for class instances */
	Camera __camera;

	/* Cv elements */
	Mat __scene;

	/* Profile elements */
	Profile __profile;

	/* Core */
	mutex mtx;
	vector<Observer*> __observers;

	void __start_bot_main_thread()
	{
		while (true)
		{
			if (GetAsyncKeyState(VK_PAUSE) & 0x8000)
			{
				break;
			}

			update_scene();
		}
	}
};

