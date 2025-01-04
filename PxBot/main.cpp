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

	Health::HealthConfig health_settings{ 80, 10, TRUE, TRUE };
	Cavebot::CavebotConfig cavebot_settings{ TRUE, TRUE };

	Camera camera;
	Health health(health_settings);
	Cavebot cavebot(cavebot_settings);

	cavebot.prompt_profile_name("bug-carlin");

	while (true)
	{
		cv::Mat scene = camera.capture_scene();
		
		//health.update_scene(scene);
		cavebot.update_scene(scene);
		cavebot.register_creature_being_follow();

		if (GetAsyncKeyState(VK_PAUSE) & 0x8000)
		{
			break;
		}
		/*if (GetAsyncKeyState(VK_ADD) & 0x8000)
		{
			cavebot.register_creature_being_follow();
		}*/	
	}

	health.stop_threads();

	return 0;
}