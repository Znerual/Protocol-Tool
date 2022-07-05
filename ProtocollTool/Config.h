#pragma once

#include <vector>
#include <string>
#include <tuple>

#include "exceptions.h"


enum CONFIG_ERROR { CONF_SUCCESS, CONF_NOT_FOUND };

class Config
{
private:
	std::vector<std::tuple<std::string, int>> para_int; // evt. rewrite to unordered_map
	std::vector<std::tuple<std::string, float>> para_float;
	std::vector<std::tuple<std::string, std::string>> para_string;
	CONFIG_ERROR set_file(const std::string& name, std::string& value);
	std::string filepath;
public:
	Config(const std::string& filepath);
	Config() {};
	CONFIG_ERROR get(const std::string& name, int& value);
	CONFIG_ERROR get(const std::string& name, bool& value);
	CONFIG_ERROR get(const std::string& name, float& value);
	CONFIG_ERROR get(const std::string& name, std::string& value);
	CONFIG_ERROR set(const std::string& name, int value);
	CONFIG_ERROR set(const std::string& name, bool value);
	CONFIG_ERROR set(const std::string& name, float value);
	CONFIG_ERROR set(const std::string& name, std::string value);
	CONFIG_ERROR remove(const std::string& name);
};

