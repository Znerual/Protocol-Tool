#pragma once

#include "log.h"
#include "utils.h"

#include <map>
#include <string>
#include <thread>

class Command {
public:
	Command(Log* logger, PATHS* paths, Config* conf) : logger(logger), paths(paths), conf(conf) {};
	virtual void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs) = 0;

protected:
	Log* logger;
	PATHS* paths;
	Config* conf;

	
};

class Show_todos : public Command {
public:
	Show_todos(Log* logger, PATHS* paths, Config* conf, std::vector<std::jthread>* open_files, HANDLE* hExit) : Command(logger, paths, conf), open_files(open_files), hExit(hExit) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	std::vector<std::jthread>* open_files;
	HANDLE* hExit;
};

class Show_modes : public Command {
public:
	Show_modes(Log* logger, PATHS* paths, Config* conf, std::unordered_map<int, std::string>* mode_names, int* active_mode) : Command(logger, paths, conf), mode_names(mode_names), active_mode(active_mode) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	std::unordered_map<int, std::string>* mode_names;
	int* active_mode;
};

class Activate_mode : public Command {
public:
	Activate_mode(Log* logger, PATHS* paths, Config* conf, int* active_mode, std::vector<std::string>* mode_tags, std::vector<std::jthread>* file_watchers, std::string* file_ending, bool* update_files, HANDLE* hStopEvent, std::vector<std::jthread>* open_files, HANDLE* hExit) : Command(logger, paths, conf), active_mode(active_mode), mode_tags(mode_tags), file_watchers(file_watchers), file_ending(file_ending), update_files(update_files), hStopEvent(hStopEvent), open_files(open_files), hExit(hExit) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	int* active_mode;
	std::vector<std::string>* mode_tags;
	std::vector<std::jthread>* file_watchers;
	std::string* file_ending;
	bool* update_files;
	HANDLE* hStopEvent;
	std::vector<std::jthread>* open_files;
	HANDLE* hExit;
};


void update_tags(Log& logger, const PATHS& paths, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>>& tag_map, std::map<std::string, int>& tag_count, std::vector<std::string>& filter_selection, const bool add_new_to_filter_selection);
void filter_notes(Log& logger, std::istringstream& iss, const PATHS& paths, std::vector<std::string>& filter_selection, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>> tag_map, std::vector<std::string> mode_tags);
void find_notes(Log& logger, std::istringstream& iss, const PATHS& paths, std::vector<std::string>& filter_selection, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>> tag_map, std::vector<std::string> mode_tags);
void show_filtered_notes(Log& logger, std::istringstream& iss, Config& conf, int& active_mode, const PATHS& paths, const std::string& tmp_filename, std::vector<std::string>& filter_selection, const bool& has_pandoc, std::vector<std::jthread>& open_files, HANDLE& hExit);
void add_note(Log& logger, std::istringstream& iss, const PATHS& paths, const std::string& file_ending, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>>& tag_map, std::vector<std::string>& mode_tags, std::vector<std::string>& filter_selection, std::vector<std::jthread>& open_files, HANDLE& hExit);
void add_data(Log& logger, std::istringstream& iss, const PATHS& paths, std::vector<std::string> filter_selection);
void show_details(Log& logger, std::istringstream& iss, const PATHS& paths, Config& conf, int& active_mode, std::vector<std::string> filter_selection, std::map<std::string, time_t> file_map, std::map<std::string, std::vector<std::string>> tag_map);
void open_selection(Log& logger, const PATHS& paths, std::vector<std::string> filter_selection, std::vector<std::jthread>& open_files, HANDLE& hExit);
void create_mode(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, int& num_modes, std::unordered_map<int, std::string>& mode_names, std::vector<std::string>& mode_tags, int& active_mode, std::vector<std::jthread>& file_watchers, const std::string& file_ending, bool& update_files, HANDLE& hStopEvent, std::vector<std::jthread>& open_files, HANDLE& hExit);
void delete_mode(Log& logger, std::istringstream& iss, Config& conf, int& num_modes, std::unordered_map<int, std::string>& mode_names, std::vector<std::string>& mode_tags, int& active_mode, std::vector<std::jthread>& file_watchers, HANDLE& hStopEvent);
void edit_mode(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, std::unordered_map<int, std::string>& mode_names, std::vector<std::string>& mode_tags, int& active_mode, std::vector<std::jthread>& file_watchers, const std::string& file_ending, bool& update_files, HANDLE& hStopEvent, std::vector<std::jthread>& open_files, HANDLE& hExit);
void activate_mode_command(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, std::unordered_map<int, std::string>& mode_names, int& active_mode, std::vector<std::string>& mode_tags, std::vector<std::jthread>& file_watchers, const std::string& file_ending, bool& update_files, HANDLE& hStopEvent, std::vector<std::jthread>& open_files, HANDLE& hExit);

void deactivate_mode(Log& logger, Config& conf, int& active_mode, std::vector<std::string>& mode_tags, std::vector<std::jthread>& file_watchers, HANDLE& hStopEvent);

void activate_mode(Log& logger, Config& conf, const PATHS& paths, int& active_mode, std::vector<std::string>& mode_tags, std::vector<std::jthread>& file_watchers, const std::string& file_ending, bool& update_files, HANDLE& hStopEvent, std::vector<std::jthread>& open_files, HANDLE& hExit);
void show_modes(Log& logger, std::istringstream& iss, Config& conf, std::unordered_map<int, std::string>& mode_names, int& active_mode);
//void show_todos(Log& logger, const PATHS& paths, std::vector<std::jthread>& open_files, HANDLE& hExit);