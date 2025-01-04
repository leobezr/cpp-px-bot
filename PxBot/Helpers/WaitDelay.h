#include <iostream>
#include <unordered_map>
#include <chrono>
#include <functional>

using namespace std;

class WaitDelay {
private:
    unordered_map<string, chrono::steady_clock::time_point> callback_collection;

public:
    void callback(const string& callback_name, int delay, function<void()> callback_function) {
        auto now = chrono::steady_clock::now();
        auto next_allowed_time = callback_collection.find(callback_name);

        if (next_allowed_time == callback_collection.end() || now >= next_allowed_time->second) {
            callback_collection[callback_name] = now + chrono::milliseconds(delay);
            callback_function();
        }
    }
};
