#pragma once

#include <vector>
#include <string>
#include <tuple>
#include <unordered_map>

#include "exceptions.h"


enum CONFIG_ERROR { CONF_SUCCESS, CONF_NOT_FOUND };

class Config
{
private:
	std::unordered_map<std::string, int> para_int; // evt. rewrite to unordered_map
	std::unordered_map<std::string, float> para_float;
	std::unordered_map<std::string, std::string> para_string;
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

