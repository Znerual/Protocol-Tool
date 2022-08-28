#pragma once

#include "log.h"
#include "utils.h"

#include <map>
#include <string>
#include <thread>

class Command {
public:
	Command(Log* logger, PATHS* paths, Config* conf) : logger(logger), paths(paths), conf(conf) {};
	virtual ~Command() {};
	virtual void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs) = 0;

protected:
	Log* logger;
	PATHS* paths;
	Config* conf;

	
};

class Help : public Command {
public:
	Help(Log* logger, PATHS* paths, Config* conf) : Command(logger, paths, conf) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
};

class Quit : public Command {
public:
	Quit(Log* logger, PATHS* paths, Config* conf, HANDLE* hExit, bool* running) : Command(logger, paths, conf), hExit(hExit), running(running) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	HANDLE* hExit;
	bool* running;
};

class Show_todos : public Command {
public:
	Show_todos(Log* logger, PATHS* paths, Config* conf, std::vector<std::jthread>* open_files, HANDLE* hExit) : Command(logger, paths, conf), open_files(open_files), hExit(hExit) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	std::vector<std::jthread>* open_files;
	HANDLE* hExit;
};

class Show_tags : public Command {
public:
	Show_tags(Log* logger, PATHS* paths, Config* conf, int* active_mode, std::vector<std::string>* mode_tags, std::vector<std::string>* filter_selection, std::map<std::string, std::vector<std::string>>* tag_map) : Command(logger, paths, conf), active_mode(active_mode), mode_tags(mode_tags), filter_selection(filter_selection), tag_map(tag_map) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	int* active_mode;
	std::vector<std::string>* mode_tags;
	std::vector<std::string>* filter_selection;
	std::map<std::string, std::vector<std::string>>* tag_map;
};

class Show_modes : public Command {
public:
	Show_modes(Log* logger, PATHS* paths, Config* conf, std::unordered_map<int, std::string>* mode_names, int* active_mode, CMD_NAMES* cmd_names) : Command(logger, paths, conf), mode_names(mode_names), active_mode(active_mode), cmd_names(cmd_names) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	std::unordered_map<int, std::string>* mode_names;
	int* active_mode;
	CMD_NAMES* cmd_names;
};

class Activate_mode_command : public Command {
public:
	Activate_mode_command(Log* logger, PATHS* paths, Config* conf, std::unordered_map<int, std::string>* mode_names, int* active_mode, std::vector<std::string>* mode_tags, std::vector<std::jthread>* file_watchers, const std::string* file_ending, bool* update_files, HANDLE* hStopEvent, std::vector<std::jthread>* open_files, HANDLE* hExit) : Command(logger, paths, conf), mode_names(mode_names), active_mode(active_mode), mode_tags(mode_tags), file_watchers(file_watchers), file_ending(file_ending), update_files(update_files), hStopEvent(hStopEvent), open_files(open_files), hExit(hExit) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	std::unordered_map<int, std::string>* mode_names;
	int* active_mode;
	std::vector<std::string>* mode_tags;
	std::vector<std::jthread>* file_watchers;
	const std::string* file_ending;
	bool* update_files;
	HANDLE* hStopEvent;
	std::vector<std::jthread>* open_files;
	HANDLE* hExit;
};

class Deactivate_mode : public Command {
public:
	Deactivate_mode(Log* logger, PATHS* paths, Config* conf, int* active_mode, std::vector<std::string>* mode_tags, std::vector<std::jthread>* file_watchers, HANDLE* hStopEvent) : Command(logger, paths, conf), active_mode(active_mode), mode_tags(mode_tags), file_watchers(file_watchers),hStopEvent(hStopEvent) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	int* active_mode;
	std::vector<std::string>* mode_tags;
	std::vector<std::jthread>* file_watchers;
	HANDLE* hStopEvent;
};

class Edit_mode : public Command {
public:
	Edit_mode(Log* logger, PATHS* paths, Config* conf, std::unordered_map<int, std::string>* mode_names, std::vector<std::string>* mode_tags, int* active_mode, std::vector<std::jthread>* file_watchers, const std::string* file_ending, bool* update_files, HANDLE* hStopEvent, std::vector<std::jthread>* open_files, HANDLE* hExit) : Command(logger, paths, conf), mode_names(mode_names), active_mode(active_mode), mode_tags(mode_tags), file_watchers(file_watchers), file_ending(file_ending), update_files(update_files), hStopEvent(hStopEvent), open_files(open_files), hExit(hExit) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	std::unordered_map<int, std::string>* mode_names;
	int* active_mode;
	std::vector<std::string>* mode_tags;
	std::vector<std::jthread>* file_watchers;
	const std::string* file_ending;
	bool* update_files;
	HANDLE* hStopEvent;
	std::vector<std::jthread>* open_files;
	HANDLE* hExit;
};

class Delete_mode : public Command {
public:
	Delete_mode(Log* logger, PATHS* paths, Config* conf, int* num_modes, std::unordered_map<int, std::string>* mode_names, std::vector<std::string>* mode_tags, int* active_mode, std::vector<std::jthread>* file_watchers, HANDLE* hStopEvent) : Command(logger, paths, conf), num_modes(num_modes), mode_names(mode_names), active_mode(active_mode), mode_tags(mode_tags), file_watchers(file_watchers), hStopEvent(hStopEvent) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	int* num_modes;
	std::unordered_map<int, std::string>* mode_names;
	int* active_mode;
	std::vector<std::string>* mode_tags;
	std::vector<std::jthread>* file_watchers;
	HANDLE* hStopEvent;
};

class Create_mode : public Command {
public:
	Create_mode(Log* logger, PATHS* paths, Config* conf, int* num_modes, std::unordered_map<int, std::string>* mode_names, std::vector<std::string>* mode_tags, int* active_mode, std::vector<std::jthread>* file_watchers, const std::string* file_ending, bool* update_files, HANDLE* hStopEvent, std::vector<std::jthread>* open_files, HANDLE* hExit) : Command(logger, paths, conf), num_modes(num_modes), mode_names(mode_names), active_mode(active_mode), mode_tags(mode_tags), file_watchers(file_watchers), file_ending(file_ending), update_files(update_files), hStopEvent(hStopEvent), open_files(open_files), hExit(hExit) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	int* num_modes;
	std::unordered_map<int, std::string>* mode_names;
	int* active_mode;
	std::vector<std::string>* mode_tags;
	std::vector<std::jthread>* file_watchers;
	const std::string* file_ending;
	bool* update_files;
	HANDLE* hStopEvent;
	std::vector<std::jthread>* open_files;
	HANDLE* hExit;
};

class Open_selection : public Command {
public:
	Open_selection(Log* logger, PATHS* paths, Config* conf, std::vector<std::string>* filter_selection, std::vector<std::jthread>* open_files, HANDLE* hExit) : Command(logger, paths, conf), filter_selection(filter_selection), open_files(open_files),  hStopEvent(hStopEvent) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	std::vector<std::string>* filter_selection;
	std::vector<std::jthread>* open_files;
	HANDLE* hStopEvent;
};

class Show_details : public Command {
public:
	Show_details(Log* logger, PATHS* paths, Config* conf, int* active_mode, std::vector<std::string>* filter_selection, std::map<std::string, time_t>* file_map, std::map<std::string, std::vector<std::string>>* tag_map) : Command(logger, paths, conf), active_mode(active_mode), filter_selection(filter_selection), file_map(file_map), tag_map(tag_map) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	int* active_mode;
	std::vector<std::string>* filter_selection;
	std::map<std::string, time_t>* file_map;
	std::map<std::string, std::vector<std::string>>* tag_map;
};

class Add_data : public Command {
public:
	Add_data(Log* logger, PATHS* paths, Config* conf, std::vector<std::string>* filter_selection) : Command(logger, paths, conf), filter_selection(filter_selection) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	std::vector<std::string>* filter_selection;
};

class Add_note : public Command {
public:
	Add_note(Log* logger, PATHS* paths, Config* conf, int* active_mode, std::string* file_ending, std::map<std::string, time_t>* file_map, std::map<std::string, std::vector<std::string>>* tag_map, std::vector<std::string>* mode_tags, std::vector<std::string>* filter_selection, std::vector<std::jthread>* open_files, HANDLE* hExit) : Command(logger, paths, conf), active_mode(active_mode), file_ending(file_ending),  file_map(file_map), tag_map(tag_map), mode_tags(mode_tags), filter_selection(filter_selection), open_files(open_files), hExit(hExit) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	int* active_mode;
	std::string* file_ending;
	std::map<std::string, time_t>* file_map;
	std::map<std::string, std::vector<std::string>>* tag_map;
	std::vector<std::string>* mode_tags;
	std::vector<std::string>* filter_selection;
	std::vector<std::jthread>* open_files;
	HANDLE* hExit;
};

class Show_filtered_notes : public Command {
public:
	Show_filtered_notes(Log* logger, PATHS* paths, Config* conf, int* active_mode, std::string* tmp_filename, std::vector<std::string>* filter_selection, bool* has_pandoc, std::vector<std::jthread>* open_files, HANDLE* hExit) : Command(logger, paths, conf), active_mode(active_mode), tmp_filename(tmp_filename), has_pandoc(has_pandoc), filter_selection(filter_selection), open_files(open_files), hExit(hExit) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	int* active_mode;
	std::string* tmp_filename;
	bool* has_pandoc;
	std::vector<std::string>* filter_selection;
	std::vector<std::jthread>* open_files;
	HANDLE* hExit;
};

class Find_notes : public Command {
public:
	Find_notes(Log* logger, PATHS* paths, Config* conf, int* active_mode, std::vector<std::string>* filter_selection, std::map<std::string, time_t>* file_map, std::map<std::string, std::vector<std::string>>* tag_map, std::vector<std::string>* mode_tags, std::unordered_map<int, std::string>* mode_names) : Command(logger, paths, conf), active_mode(active_mode), filter_selection(filter_selection), file_map(file_map), tag_map(tag_map), mode_tags(mode_tags), mode_names(mode_names) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	int* active_mode;
	std::vector<std::string>* filter_selection;
	std::map<std::string, time_t>* file_map;
	std::map<std::string, std::vector<std::string>>* tag_map;
	std::vector<std::string>* mode_tags;
	std::unordered_map<int, std::string>* mode_names;

};

class Filter_notes : public Command {
public:
	Filter_notes(Log* logger, PATHS* paths, Config* conf, int* active_mode, std::vector<std::string>* filter_selection, std::map<std::string, time_t>* file_map, std::map<std::string, std::vector<std::string>>* tag_map, std::vector<std::string>* mode_tags, std::unordered_map<int, std::string>* mode_names) : Command(logger, paths, conf), active_mode(active_mode), filter_selection(filter_selection), file_map(file_map), tag_map(tag_map), mode_tags(mode_tags), mode_names(mode_names) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	int* active_mode;
	std::vector<std::string>* filter_selection;
	std::map<std::string, time_t>* file_map;
	std::map<std::string, std::vector<std::string>>* tag_map;
	std::vector<std::string>* mode_tags;
	std::unordered_map<int, std::string>* mode_names;
};

class Update_tags : public Command {
public:
	Update_tags(Log* logger, PATHS* paths, Config* conf, std::map<std::string, time_t>* file_map, std::map<std::string, std::vector<std::string>>* tag_map, std::map<std::string, int>* tag_count, std::vector<std::string>* filter_selection, bool add_new_to_filter_selection) : Command(logger, paths, conf), file_map(file_map), tag_map(tag_map), tag_count(tag_count), filter_selection(filter_selection), add_new_to_filter_selection(add_new_to_filter_selection) {};
	void run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
private:
	std::map<std::string, time_t>* file_map;
	std::map<std::string, std::vector<std::string>>* tag_map;
	std::map<std::string, int>* tag_count;
	std::vector<std::string>* filter_selection;
	bool add_new_to_filter_selection;
};

void filter_notes(Log* logger, PATHS* paths, Config* conf, int* active_mode, std::vector<std::string>* mode_tags, std::map<std::string, std::vector<std::string>>* tag_map, std::vector<std::string>* filter_selection, std::vector<OA>& oaflags, std::map<OA, std::vector<std::string>>& oastrargs);

void activate_mode(Log& logger, Config& conf, const PATHS& paths, int& active_mode, std::vector<std::string>& mode_tags, std::vector<std::jthread>& file_watchers, const std::string& file_ending, bool& update_files, HANDLE& hStopEvent, std::vector<std::jthread>& open_files, HANDLE& hExit);
void deactivate_mode(Log& logger, Config& conf, int& active_mode, std::vector<std::string>& mode_tags, std::vector<std::jthread>& file_watchers, HANDLE& hStopEvent);
/*

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

void show_modes(Log& logger, std::istringstream& iss, Config& conf, std::unordered_map<int, std::string>& mode_names, int& active_mode);
//void show_todos(Log& logger, const PATHS& paths, std::vector<std::jthread>& open_files, HANDLE& hExit);
*/