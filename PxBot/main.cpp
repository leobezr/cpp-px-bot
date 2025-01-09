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

	const bool IS_DEVELOPMENT_MODE = true;
	const bool CAVEBOT_ENABLED = !IS_DEVELOPMENT_MODE;
	const bool HEALING_ENABLED = !IS_DEVELOPMENT_MODE;

	Profile profile = ProfileLoader::prompt_profile_name("swamp-troll-ph");
	
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

	return 0;
}