#include "autocomplete.h"

#ifndef _WIN32
#include <readline/readline.h>
#include <readline/history.h>
#else
#include <functional>
#endif
using namespace std;

void read_cmd_structure(const filesystem::path filepath, CMD_STRUCTURE& cmds)
{


	enum class PARSE_MODE { CMD_LINE, OA_LINE };
	ifstream file(filepath);

	PARSE_MODE mode = PARSE_MODE::CMD_LINE;

	CMD command;
	list<PA> poarg;
	unordered_map<OA, list<OA>> oparg;

	string line, arg;
	while (getline(file, line))
	{
#ifndef _WIN32
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
#endif // !_WIN32

		istringstream iss(line);
		switch (mode)
		{
		case PARSE_MODE::CMD_LINE:
			iss >> arg;
			command = static_cast<CMD>(stoi(arg));
			while (iss >> arg)
			{
				poarg.push_back(static_cast<PA>(stoi(arg)));
			}
			mode = PARSE_MODE::OA_LINE;
			break;
		case PARSE_MODE::OA_LINE:
			if (line == "END") {
				cmds[command] = make_pair(poarg, oparg);
				poarg = list<PA>();
				oparg = unordered_map<OA, list<OA>>();
				mode = PARSE_MODE::CMD_LINE;
			}
			else {
				OA oam;
				list<OA> oal{};
				iss >> arg;
				oam = static_cast<OA>(stoi(arg));
				while (iss >> arg)
				{
					oal.push_back(static_cast<OA>(stoi(arg)));
				}
				oparg[oam] = oal;
			}

			break;
		default:
			break;
		}
	}

	file.close();

}

void read_cmd_names(filesystem::path filepath, CMD_NAMES& cmd_names) {
	enum class PARSE_MODE { NONE, CMD_LINE, OA_LINE, PA_LINE };
	ifstream file(filepath);

	PARSE_MODE mode = PARSE_MODE::CMD_LINE;
	string line, arg1, arg2;
	while (getline(file, line)) {
#ifndef _WIN32
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
#endif // !_WIN32
		istringstream iss(line);
		if (line == "CMD") {
			mode = PARSE_MODE::CMD_LINE;
		}
		else if (line == "OA") {
			mode = PARSE_MODE::OA_LINE;
		}
		else if (line == "PA") {
			mode = PARSE_MODE::PA_LINE;
		}
		else if (mode == PARSE_MODE::CMD_LINE) {
			iss >> arg1 >> arg2;
			cmd_names.cmd_names.insert(cmd_map::value_type(arg2, static_cast<CMD>(stoi(arg1))));
		}
		else if (mode == PARSE_MODE::PA_LINE) {
			iss >> arg1 >> arg2;
			cmd_names.pa_names.insert(pa_map::value_type(arg2, static_cast<PA>(stoi(arg1))));
		}
		else if (mode == PARSE_MODE::OA_LINE) {
			iss >> arg1 >> arg2;
			cmd_names.oa_names.insert(oa_map::value_type(arg2, static_cast<OA>(stoi(arg1))));
		}
	}
}

void find_cmd_suggestion(const COMMAND_INPUT& auto_input, AUTOCOMPLETE& auto_comp, AUTO_SUGGESTIONS& auto_suggestions) {
	enum class CMD_STATE { CMD, PA, OA, OAOA };
	CMD_STATE state = CMD_STATE::CMD;

	// read whole input into vector
	list<string> input_words{};
	istringstream iss(auto_input.input);
	string word;
	while (iss >> word) {
		input_words.push_back(word);
	}

	// split into command parts, CMD state or PA state
	auto_suggestions.auto_sug = {};
	if (input_words.size() == 1) {// command state
		if (auto_input.cmd_names.cmd_names.left.count(input_words.front()) != 1) { // command not completely typed
			// find command suggestions
			auto_comp.cmd_names.findAutoSuggestion(input_words.front(), auto_suggestions.auto_sug);
			auto_comp.cmd_names.findAutoSuggestions(input_words.front(),auto_suggestions.auto_sugs);
			return;
		}

		if (auto_input.input.back() != ' ')
			return;


	}
	else if (input_words.size() == 0) {
		return;
	}
	else { // further states
		if (auto_input.cmd_names.cmd_names.left.count(input_words.front()) == 0) { // check whether command has typo
			return;
		}
	}

	const CMD command = auto_input.cmd_names.cmd_names.left.at(input_words.front());
	input_words.pop_front();

	list<PA> pa_args = auto_input.cmd_structure.at(command).first;
	if (pa_args.size() > 0) {

		// suggest first pa if first pa is tags or mode name (not if date)
		if (input_words.size() == 0) {
			if (pa_args.front() == PA::TAGS) {
				auto_comp.tags.findAutoSuggestion("", auto_suggestions.auto_sug);
				auto_comp.tags.findAutoSuggestions("", auto_suggestions.auto_sugs);
				return;
			}
			else if (pa_args.front() == PA::MODE_NAME) {
				auto_comp.mode_names.findAutoSuggestion("", auto_suggestions.auto_sug);
				auto_comp.mode_names.findAutoSuggestions("", auto_suggestions.auto_sugs);
				return;
			}
			else if (pa_args.front() == PA::DATE) {
				time_t now = time(nullptr);
				date2str_medium(auto_suggestions.auto_sug, now);

				string dates;
				for (auto i = 0; i < 33; i++) {
					now = time(nullptr) + (60 * 60 * 24) * i; // add next 32 days to list and then the last 32 days
					date2str_medium(dates, now);
					auto_suggestions.auto_sugs.push_back(dates);
				}
				for (auto i = -32; i < 0; i++) {
					now = time(nullptr) + (60 * 60 * 24) * i; // add next 32 days to list and then the last 32 days
					date2str_medium(dates, now);
					auto_suggestions.auto_sugs.push_back(dates);

				}
				return;
			}
			return;

		}
		else {
			bool skip_pa = false;
			int count = 0;
			for (const auto& pa_arg : pa_args) {
				// too many positional arguments specified
				if (count - 1 > (int)input_words.size()) { // -1 because tags as pa can be empty
					return; // to little input words
				}
				if (input_words.back().starts_with('-')) { //is a oa, skip pa
					skip_pa = true;
					break;
				}
				if (!auto_input.input.ends_with(" ")) { // test is pa is fully typed
					break;
				}
				if (pa_arg == PA::TAGS && (input_words.size()) >= count) { // if previous pa where set (1 arg per word), tags can be suggested
					if (auto_input.input.back() == ' ') {
						auto_comp.tags.findAutoSuggestion("", auto_suggestions.auto_sug);
						auto_comp.tags.findAutoSuggestions("", auto_suggestions.auto_sugs);
					}
					else {
						auto_comp.tags.findAutoSuggestion(input_words.back(), auto_suggestions.auto_sug);
						auto_comp.tags.findAutoSuggestions(input_words.back(), auto_suggestions.auto_sugs);
					}
					return;
				}
				else if (pa_arg == PA::MODE_NAME && (count == input_words.size() || count == input_words.size() - 1)) { // if pa of current input count (or next) is mode_name than suggest
					auto_comp.mode_names.findAutoSuggestion(input_words.back(), auto_suggestions.auto_sug);
					auto_comp.mode_names.findAutoSuggestions(input_words.back(), auto_suggestions.auto_sugs);
					return;
				}
				else if (pa_arg == PA::DATE && count == input_words.size()) { // suggest date
					time_t now = time(nullptr);
					date2str_medium(auto_suggestions.auto_sug, now);

					string dates;
					for (auto i = 0; i < 33; i++) {
						now = time(nullptr) + (60 * 60 * 24) * i; // add next 32 days to list and then the last 32 days
						date2str_medium(dates, now);
						auto_suggestions.auto_sugs.push_back(dates);
					}
					for (auto i = -32; i < 0; i++) {
						now = time(nullptr) + (60 * 60 * 24) * i; // add next 32 days to list and then the last 32 days
						date2str_medium(dates, now);
						auto_suggestions.auto_sugs.push_back(dates);

					}
					return;
				}

				count += 1;
			}
			if (!skip_pa)
				return; // cant suggest
		}

	}

	unordered_map<OA, list<OA>> oa_args = auto_input.cmd_structure.at(command).second;
	list<string> oa_strs;
	for (const auto& [oa_arg, _] : oa_args) {
		if (oa_arg == OA::CMD) {
			for (const auto& [name, id] : auto_input.cmd_names.cmd_names) {
				oa_strs.push_back(name);
			}
		}
		else {
			oa_strs.push_back(auto_input.cmd_names.oa_names.right.at(oa_arg));
		}
	}

	// nothing after command and positonal arguments written, suggest optional arguments
	if (input_words.empty()) {
		TrieTree oa_trietree = TrieTree(oa_strs);
		oa_trietree.findAutoSuggestion("", auto_suggestions.auto_sug);
		oa_trietree.findAutoSuggestions("", auto_suggestions.auto_sugs);
		return;
	}
	// find end of positional arguments by searching for first optional argument
	auto oa_start_pos = find_first_of(input_words.begin(), input_words.end(), oa_strs.begin(), oa_strs.end());

	// did not find a single complete oa, suggest oa
	if (oa_start_pos == input_words.end()) {
		TrieTree oa_trietree = TrieTree(oa_strs);

		// complete last input word
		oa_trietree.findAutoSuggestion(input_words.back(), auto_suggestions.auto_sug);
		oa_trietree.findAutoSuggestions(input_words.back(), auto_suggestions.auto_sugs);
		return;
	}
	else {

		input_words.erase(input_words.begin(), oa_start_pos); // erase all pa

		// erase set oa and find active oa (last oa, important if that oa expects tags)
		OA active_oa;
		list<string>::iterator active_oa_words_pos;
		for (auto it = input_words.begin(); it != input_words.end(); ++it) {
			auto oa_found = std::find(oa_strs.begin(), oa_strs.end(), *it);
			if (oa_found != oa_strs.end()) {
				active_oa = auto_input.cmd_names.oa_names.left.at(*it);
				active_oa_words_pos = it;
				oa_strs.erase(oa_found); // delete oa name from the list of available names
				//oa_args.erase(active_oa); // delete from the list of oa in the cmd structure
			}
		}

		// delete handled with oa
		input_words.erase(input_words.begin(), active_oa_words_pos);

		if (oa_args[active_oa].size() == 0) { // last complete oa has no follow up parameter, suggest next oa
			TrieTree oa_trietree = TrieTree(oa_strs);
			if (auto_input.input.back() == ' ') {
				oa_trietree.findAutoSuggestion("", auto_suggestions.auto_sug);
				oa_trietree.findAutoSuggestions("", auto_suggestions.auto_sugs);
			}
			else {
				oa_trietree.findAutoSuggestion(input_words.back(), auto_suggestions.auto_sug);
				oa_trietree.findAutoSuggestions(input_words.back(), auto_suggestions.auto_sugs);
			}
			return;
		}
		else if (oa_args[active_oa].size() == 1) { // only suggest one oaoa tag if that one is tags
			if (oa_args[active_oa].front() == OA::TAGS) {
				if (auto_input.input.back() == ' ') {
					auto_comp.tags.findAutoSuggestion("", auto_suggestions.auto_sug);
					auto_comp.tags.findAutoSuggestions("", auto_suggestions.auto_sugs);
				}
				else {
					auto_comp.tags.findAutoSuggestion(input_words.back(), auto_suggestions.auto_sug);
					auto_comp.tags.findAutoSuggestions(input_words.back(), auto_suggestions.auto_sugs);
				}
			}
			return;
		}
		else { // multiple options for oa parameter, show all
			list<string> oaoa_strs;
			for (const auto& oa_arg : oa_args[active_oa]) {
				oaoa_strs.push_back(auto_input.cmd_names.oa_names.right.at(oa_arg));
			}

			TrieTree oa_trietree = TrieTree(oaoa_strs);
			if (auto_input.input.back() == ' ') {
				oa_trietree.findAutoSuggestion("", auto_suggestions.auto_sug);
				oa_trietree.findAutoSuggestions("", auto_suggestions.auto_sugs);
			}
			else {
				oa_trietree.findAutoSuggestion(input_words.back(), auto_suggestions.auto_sug);
				oa_trietree.findAutoSuggestions(input_words.back(), auto_suggestions.auto_sugs);
			}
			return;
		}

	}

}

void get_suggestion(Log& logger, COMMAND_INPUT& auto_input, AUTOCOMPLETE& auto_comp, AUTO_SUGGESTIONS& auto_suggestions, std::string& last_input, size_t& length_last_suggestion)
{
	auto_suggestions = AUTO_SUGGESTIONS();
	find_cmd_suggestion(auto_input, auto_comp, auto_suggestions);
	logger.input('\t');

	// check if only one prediction exists. 
	if (!auto_suggestions.auto_sug.empty()) {

		// add prediction to console input
		logger << auto_suggestions.auto_sug;
		auto_input.input += auto_suggestions.auto_sug;
		length_last_suggestion = auto_suggestions.auto_sug.size();

		// delete old suggestions (because they are now written to screen)
		//auto_suggestions = AUTO_SUGGESTIONS();
	}
	last_input = auto_input.input;
	auto_suggestions.auto_sugs_pos = auto_suggestions.auto_sugs.begin(); // reset position for cycling through with arrow keys
}

void cicle_suggestions(Log& logger, COMMAND_INPUT& auto_input, AUTOCOMPLETE& auto_comp, AUTO_SUGGESTIONS& auto_suggestions, std::string& last_input, size_t& length_last_suggestion, bool up)
{
	// check if no suggestion is available or if the user changed the input
	if (auto_suggestions.auto_sugs.empty() || last_input != auto_input.input) {
		auto_suggestions = AUTO_SUGGESTIONS();
		find_cmd_suggestion(auto_input, auto_comp, auto_suggestions);
		auto_suggestions.auto_sugs_pos = auto_suggestions.auto_sugs.begin();
		length_last_suggestion = 0;
		last_input = auto_input.input;
	}

	// no alternate suggestions available
	if (auto_suggestions.auto_sugs.size() == 1) {
		auto_input.input += auto_suggestions.auto_sug;
		logger << auto_suggestions.auto_sug;
		auto_suggestions = AUTO_SUGGESTIONS();
		last_input = auto_input.input;
		length_last_suggestion = auto_suggestions.auto_sug.size();
		return;
	}
	else if (auto_suggestions.auto_sugs.empty()) {
		return;
	}
	else {
		if (up) {
			if (auto_suggestions.auto_sugs_pos == prev(auto_suggestions.auto_sugs.end())) {
				auto_suggestions.auto_sugs_pos = auto_suggestions.auto_sugs.begin();
			}
			else {
				++auto_suggestions.auto_sugs_pos;
			}
		}
		else {
			if (auto_suggestions.auto_sugs_pos == auto_suggestions.auto_sugs.begin()) {
				auto_suggestions.auto_sugs_pos = --auto_suggestions.auto_sugs.end();
			}
			else {
				--auto_suggestions.auto_sugs_pos;
			}
		}
		
	}

	size_t next_length = (*(auto_suggestions.auto_sugs_pos)).length();

	// delete previous suggestion from input
	auto_input.input.erase(auto_input.input.end() - length_last_suggestion, auto_input.input.end());

	// delte prev sug from console - set cursor position back
	for (auto i = 0; i < length_last_suggestion; i++) {
		logger << '\b';
	}

	auto_suggestions.auto_sug = *auto_suggestions.auto_sugs_pos;
	auto_input.input += auto_suggestions.auto_sug;
	last_input = auto_input.input;
	// overwrite output with new suggestion
	logger << auto_suggestions.auto_sug;

	// overwrite old data that was longer than the previous data
	if (length_last_suggestion > next_length) {
		for (auto i = 0; i < length_last_suggestion - next_length; i++) {
			logger << ' ';
		}
		for (auto i = 0; i < length_last_suggestion - next_length; i++) {
			logger << '\b';
		}
	}

	length_last_suggestion = auto_suggestions.auto_sug.size();
}
#ifndef _WIN32

char* completion_generator(const char* text, int state) {
	// This function is called with state=0 the first time; subsequent calls are
	// with a nonzero state. state=0 can be used to perform one-time
	// initialization for this completion session.
	static vector<string> matches;
	static size_t match_index = 0;

	if (state == 0) {
		// During initialization, compute the actual matches for 'text' and keep
		// them in a static vector.
		matches.clear();
		match_index = 0;
		AUTOCOMPLETE auto_comp(auto_input.cmd_names, tag_count, mode_names); // update trietrees when tags and/or modes are added/changed
		AUTO_SUGGESTIONS auto_suggestions = AUTO_SUGGESTIONS();
		auto_input.input = rl_line_buffer;
		// Collect a vector of matches: vocabulary words that begin with text.
		find_cmd_suggestion(auto_input, auto_comp, auto_suggestions);
		for (const auto& sug : auto_suggestions.auto_sugs) {
			matches.push_back(text + sug);
		}

	}

	if (match_index >= matches.size()) {
		// We return nullptr to notify the caller no more matches are available.
		return nullptr;
	}
	else {
		// Return a malloc'd char* for the match. The caller frees it.
		return strdup(matches[match_index++].c_str());
	}
}

char** completer(const char* text, int start, int end) {
	// Don't do filename completion even if our generator finds no matches.
	rl_attempted_completion_over = 1;
	// Note: returning nullptr here will make readline use the default filename
	// completer.
	
	return rl_completion_matches(text, completion_generator);
}

#endif