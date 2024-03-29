#pragma once
#include <string>
#include <list>
#include <filesystem>
#include <map>

#include "utils.h"



struct AUTO_SUGGESTIONS {
	std::string auto_sug;
	std::list<std::string> auto_sugs; 
	std::list<std::string>::iterator auto_sugs_pos;

	AUTO_SUGGESTIONS() {
		auto_sug = "";
		auto_sugs = std::list<std::string>();
		auto_sugs_pos = auto_sugs.begin();
	}
};

void read_cmd_structure(const std::filesystem::path filepath, CMD_STRUCTURE& cmds);
void read_cmd_names(std::filesystem::path filepath, CMD_NAMES& cmd_names);
void read_cmd_help(std::filesystem::path filepath, CMD_NAMES& cmd_names);
void find_cmd_suggestion(const COMMAND_INPUT& auto_input, AUTOCOMPLETE& auto_comp, AUTO_SUGGESTIONS& auto_suggestions);

#ifdef _WIN32
void cicle_suggestions(Log& logger, COMMAND_INPUT& auto_input, AUTOCOMPLETE& auto_comp, AUTO_SUGGESTIONS& auto_suggestions, std::string& last_input, size_t& length_last_suggestion, bool up);
void get_suggestion(Log& logger, COMMAND_INPUT& auto_input, AUTOCOMPLETE& auto_comp, AUTO_SUGGESTIONS& auto_suggestions, std::string& last_input, size_t& length_last_suggestion);
#else

extern COMMAND_INPUT auto_input;
extern std::map<std::string, int> tag_count;
extern std::unordered_map<int, std::string> mode_names;

char* completion_generator(const char* text, int state);
char** completer(const char* text, int start, int end);
#endif