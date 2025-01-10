#pragma once
#include <windows.h>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>

using namespace std;

struct HealingRule
{
	int min_health;
	int max_health;
	int min_mana;
	int max_mana;
};

struct Healing
{
	HealingRule low_health = { 0, 0, 0, 0 };
	HealingRule high_health = { 0, 0, 0, 0 };
	HealingRule mana = { 0, 0, 0, 0 };
};

struct Target
{
	string name;
	int priority;
};

struct Waypoint
{
	string method;
	int node_size = 1;
	string label;
};

struct WaypointCategory
{
	vector<Waypoint> start;
	vector<Waypoint> hunt;
	vector<Waypoint> refil;
};

struct Profile
{
	string name;
	Healing healing;
	vector<Target> target;
	WaypointCategory waypoint_category;
};

inline void from_json(const nlohmann::json& j, HealingRule& healing_rule)
{
	j.at("min_health").get_to(healing_rule.min_health);
	j.at("max_health").get_to(healing_rule.max_health);
	j.at("min_mana").get_to(healing_rule.min_mana);
	j.at("max_mana").get_to(healing_rule.max_mana);
}

inline void from_json(const nlohmann::json& j, Healing& healing)
{
	j.at("low_health").get_to(healing.low_health);
	j.at("high_health").get_to(healing.high_health);
	j.at("mana").get_to(healing.mana);
}

inline void from_json(const nlohmann::json& j, Target& target)
{
	j.at("name").get_to(target.name);
	j.at("priority").get_to(target.priority);
}

inline void from_json(const nlohmann::json& j, Waypoint& waypoint)
{
	j.at("method").get_to(waypoint.method);
	j.at("node_size").get_to(waypoint.node_size);
	j.at("label").get_to(waypoint.label);
}

inline void from_json(const nlohmann::json& j, WaypointCategory& waypoint_category)
{
	j.at("start").get_to(waypoint_category.start);
	j.at("hunt").get_to(waypoint_category.hunt);
	j.at("refil").get_to(waypoint_category.refil);
}

inline void from_json(const nlohmann::json& j, Profile& profile)
{
	j.at("name").get_to(profile.name);
	j.at("healing").get_to(profile.healing);
	j.at("targets").get_to(profile.target);
	j.at("waypoint_category").get_to(profile.waypoint_category);
}

inline void to_json(nlohmann::json& j, const HealingRule& healing_rule)
{
	j = nlohmann::json{
		{"min_health", healing_rule.min_health},
		{"max_health", healing_rule.max_health},
		{"min_mana", healing_rule.min_mana},
		{"max_mana", healing_rule.max_mana}
	};
}

inline void to_json(nlohmann::json& j, const Healing& healing)
{
	j = nlohmann::json{
		{"low_health", healing.low_health},
		{"high_health", healing.high_health},
		{"mana", healing.mana}
	};
}

inline void to_json(nlohmann::json& j, const Target& target)
{
	j = nlohmann::json{
		{"name", target.name},
		{"priority", target.priority}
	};
}

inline void to_json(nlohmann::json& j, const Waypoint& waypoint)
{
	j = nlohmann::json{
		{"method", waypoint.method},
		{"node_size", waypoint.node_size},
		{"label", waypoint.label}
	};
}

inline void to_json(nlohmann::json& j, const WaypointCategory& waypoint_category)
{
	j = nlohmann::json{
		{"start", waypoint_category.start},
		{"hunt", waypoint_category.hunt},
		{"refil", waypoint_category.refil}
	};
}

inline void to_json(nlohmann::json& j, const Profile& profile)
{
	j = nlohmann::json{
		{"name", profile.name},
		{"healing", profile.healing},
		{"targets", profile.target},
		{"waypoint_category", profile.waypoint_category}
	};
}