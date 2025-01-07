#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/utils/logger.hpp>
#include <iostream>
#include <windows.h>
#include "lib/Camera.h"
#include "lib/Health.h"
#include <nlohmann/json.hpp>
#include "Lib/Player.h"
#include "Helpers/ProfileLoader.h"

using namespace std;

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);

	const bool IS_DEVELOPMENT_MODE = false;
	const bool CAVEBOT_ENABLED = true;
	const bool HEALING_ENABLED = true;

	Profile profile = ProfileLoader::prompt_profile_name("wasp-thais");
	
	/* 
	* Orchestrator behind the bot.
	* Is responsible for holding the main thread and updating 
	* the scene and notifying observers. 
	* Holds instances of the bot's functionalities.
	*/
	Player player({ 
		profile, 
		CAVEBOT_ENABLED, 
		HEALING_ENABLED, 
		IS_DEVELOPMENT_MODE 
	});

	//	if (GetAsyncKeyState(VK_DIVIDE) & 0x8000)
	//	{
	//		TODO: ADD THIS TO CAVEBOT
	//		cavebot.jump_next_waypoint_category();
	//	}

	return 0;
}