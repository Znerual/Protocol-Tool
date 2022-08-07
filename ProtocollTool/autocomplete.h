#pragma once
#include <string>
#include <list>
#include <filesystem>

#include "utils.h"

struct AUTO_INPUT {
	std::string input;
	CMD_STRUCTURE cmd_structure;
	CMD_NAMES cmd_names;
};

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
void parse_cmd(const AUTO_INPUT& auto_input, AUTOCOMPLETE& auto_comp, AUTO_SUGGESTIONS& auto_suggestions);

void cicle_suggestions(Log& logger, AUTO_INPUT& auto_input, AUTOCOMPLETE& auto_comp, AUTO_SUGGESTIONS& auto_suggestions, std::string& last_input, size_t& length_last_suggestion, bool up);
void get_suggestion(Log& logger, AUTO_INPUT& auto_input, AUTOCOMPLETE& auto_comp, AUTO_SUGGESTIONS& auto_suggestions, std::string& last_input, size_t& length_last_suggestion);