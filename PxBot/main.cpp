#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <iostream>
#include <windows.h>
#include "lib/Camera.h"
#include "lib/Health.h"
#include <nlohmann/json.hpp>
#include "Lib/Cavebot.h"

using namespace std;

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

	bool IS_DEVELOPMENT_MODE = FALSE;

	Health::HealthConfig health_settings{ 80, 10, TRUE, TRUE };
	Cavebot::CavebotConfig cavebot_settings{ !IS_DEVELOPMENT_MODE, !IS_DEVELOPMENT_MODE };

	Camera camera;
	Health health(health_settings);
	Cavebot cavebot(cavebot_settings);

	cavebot.prompt_profile_name("bug-carlin");

	while (true)
	{
		cv::Mat scene = camera.capture_scene();
		
		//health.update_scene(scene);
		cavebot.update_scene(scene);

		if (GetAsyncKeyState(VK_PAUSE) & 0x8000)
		{
			cavebot.pause();
			break;
		}

		if (IS_DEVELOPMENT_MODE)
		{
			cavebot.register_creature_being_followed();

			if (GetAsyncKeyState(VK_ADD) & 0x8000)
			{
				cavebot.register_waypoint_print();
			}
		}

		if (GetAsyncKeyState(VK_DIVIDE) & 0x8000)
		{
			cavebot.jump_next_waypoint_category();
		}

		Sleep(45);
	}

	health.stop_threads();
	cavebot.stop_threads();

	return 0;
}