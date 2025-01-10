#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>
#include <fstream>
#include "../Types/Profile.h"

using namespace std;

class ProfileLoader
{
public:
    static Profile load_profile(const string& profile_name)
    {
        ifstream file("Resources/Profiles/" + profile_name + ".json");

        if (!file.is_open()) {
            throw string("Could not open the file.");
        }

        nlohmann::json j;
        file >> j;
        file.close();

        return j.get<Profile>();
    }

    static Profile prompt_profile_name(string profile_name = "")
    {
		if (!profile_name.empty())
		{
			return ProfileLoader::load_profile(profile_name);
		}

		cout << "Enter profile name: ";
		getline(cin, profile_name);

		if (profile_name.empty())
		{
			cerr << "Profile name is empty." << endl;
			return ProfileLoader::prompt_profile_name();
		}
		else
		{
			try
			{
				return ProfileLoader::load_profile(profile_name);
			}
			catch (string& error)
			{
				cout << error << endl;
				return ProfileLoader::prompt_profile_name();
			}
		}
    }

	static void save_profile(const string& profile_name, const Profile& profile)
	{
		ofstream file("Resources/Profiles/" + profile_name + ".json");

		if (!file.is_open()) {
			throw string("Could not open the file for saving.");
		}

		nlohmann::json j = profile;
		file << j.dump(4);
		file.close();
	}
};

