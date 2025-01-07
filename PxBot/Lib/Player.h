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
		dev_tool({ setup.developer_mode_enabled, setup.profile.name })
    {
        __start_all_threads();
    }

private:
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
		}
	}
};

