#pragma once
#include <windows.h>
#include <iostream>
#include "./Health.h";
#include "./Hunter.h";
#include "./DeveloperTool.h"

using namespace std;
using namespace cv;

class Player
{
public:
	struct PlayerConfig
	{
		Profile profile;
		bool cavebot_enabled;
		bool healing_enabled;
		bool developer_mode_enabled;
	};

	DeveloperTool dev_tool;
	Health health;
	Hunter hunter;

    Player(PlayerConfig setup) : 
		health({ setup.profile.healing, setup.healing_enabled}),
		hunter({ setup.cavebot_enabled, setup.profile }),
		dev_tool({ setup.developer_mode_enabled, setup.profile })
    {
        __start_all_threads();
    }

private:
	const int __restart_thread_at = 200;
	int __thread_count_at = 0;
	int __restarted_count = 0;

	void __start_all_threads()
	{
		while (true)
		{
			if (GetAsyncKeyState(VK_PAUSE) & 0x8000)
			{
				health.stop_threads();
				hunter.stop_threads();
				dev_tool.stop_threads();

				break;
			}

			//__count_thread_before_restart();

			Sleep(40);
		}
	}

	void __count_thread_before_restart()
	{
		if (health.get_enabled() && hunter.get_enabled())
		{
			if (__thread_count_at++ >= __restart_thread_at)
			{
				cout << "Threads restarted" << endl;

				health.restart_threads();
				hunter.restart_threads();
				
				__thread_count_at = 0;
				__restarted_count++;
			}
		}
	}
};

