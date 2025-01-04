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
};

