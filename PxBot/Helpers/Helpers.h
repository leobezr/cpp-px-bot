#pragma once
#include <vector>
#include <string>
#include <sstream>

using namespace std;

class Helpers
{
public:
    static vector<string> split(const string& s, char delimiter)
    {
        vector<string> tokens;
        string token;
        istringstream tokenStream(s);
        while (getline(tokenStream, token, delimiter))
        {
            tokens.push_back(token);
        }
        return tokens;
    }
};
