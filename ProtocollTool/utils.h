#pragma once

#include <string>
#include <map>
#include <filesystem>
#include <vector>
#include <thread>
#include "Config.h"
#include "log.h"
enum ARGUMENT_MODE {NONE, ARG_DATE, CONTAINS_TAGS, CONTAINS_ALL_TAGS, NO_TAGS, REG_TEXT, VERSIONS};
enum OPEN_MODE {HTML, MARKDOWN, LATEX, PDF, DOCX};
enum FOLDER_WATCHER_MODE { READ_NONE, READ_FOLDER, READ_TAGS};
struct SHOW_OPTIONS {
	bool show_tags = false;
	bool show_metadata = false;
	bool show_table_of_content = false;
	bool show_data = false;
	bool hide_date = false;
	bool open_image_data = false;
};

struct DETAIL_OPTIONS {
	bool detail_tags = false;
	bool detail_path = false;
	bool detail_long_path = false;
	bool detail_last_modified = false;
	bool detail_content = false;
};

struct FORMAT_OPTIONS {
	bool html = false;
	bool tex = false;
	bool pdf = false;
	bool docx = false;
	bool markdown = false;
};

struct MODE_OPTIONS {
	SHOW_OPTIONS show_options;
	FORMAT_OPTIONS format_options;
	DETAIL_OPTIONS detail_options;
};

struct PATHS {
	std::filesystem::path base_path;
	std::filesystem::path file_path;
	std::filesystem::path data_path;
	std::filesystem::path tmp_path;
};

const std::string WHITESPACE = " ,\n\r\t\f\v";
std::string ltrim(const std::string& s);
std::string rtrim(const std::string& s);
std::string trim(const std::string& s);

template<typename T>
void pad(std::basic_string<T>& s, typename std::basic_string<T>::size_type n, T c, const bool right = true);

void file_change_watcher(Log& logger, const std::filesystem::path watch_path, const PATHS paths, const std::string file_ending, std::vector<std::string> watch_path_tags, bool& update_files);
int get_console_columns();

 std::map<std::string, time_t> list_all_files(const PATHS& paths);
 std::map<std::string, std::vector<std::string>> list_all_tags(Log& logger, const PATHS& paths);
 std::map<std::string, int> get_tag_count(std::map<std::string, std::vector<std::string>>& tag_map, std::vector<std::string> filter_selection);
 std::vector<std::string> read_tags(Log& logger, const std::string& path);
 
 void update_tags(Log& logger, const PATHS& paths, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>>& tag_map, std::map<std::string, int>& tag_count, std::vector<std::string>& filter_selection);

 std::string get_filename(const PATHS& paths, time_t date, const std::string& file_ending);

 void add(FORMAT_OPTIONS& to, FORMAT_OPTIONS& from);
 void add(SHOW_OPTIONS& to, SHOW_OPTIONS& from);
 void add(DETAIL_OPTIONS& to, DETAIL_OPTIONS& from);
 void remove(FORMAT_OPTIONS& to, FORMAT_OPTIONS& from);
 void remove(SHOW_OPTIONS& to, SHOW_OPTIONS& from);
 void remove(DETAIL_OPTIONS& to, DETAIL_OPTIONS& from);
 void add(MODE_OPTIONS& to, MODE_OPTIONS& from);
 void remove(MODE_OPTIONS& to, MODE_OPTIONS& from);


 void set_mode_options(Config& conf, MODE_OPTIONS& mode_options, const int& active_mode);
 void get_mode_options(Config& conf, MODE_OPTIONS& mode_options, const int& active_mode);

 void get_mode_tags(Config& conf, const int& mode_id, std::vector<std::string>& mode_tags);
 void set_mode_tags(Config& conf, const int& mode_id, std::vector<std::string>& mode_tags);

 void write_file(Log& logger, const PATHS& paths, const std::string& filename, time_t date, std::vector<std::string> tags, const std::string& file_ending, const bool create_data);
 
 void filter_notes(Log& logger, std::istringstream& iss, const PATHS& paths, std::vector<std::string>& filter_selection, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>> tag_map, std::vector<std::string> mode_tags);
 void find_notes(Log& logger, std::istringstream& iss, const PATHS& paths, std::vector<std::string>& filter_selection, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>> tag_map, std::vector<std::string> mode_tags);
 void show_filtered_notes(Log& logger, std::istringstream& iss, const OPEN_MODE default_open, Config& conf, int& active_mode, const PATHS& paths, const std::string& tmp_filename, std::vector<std::string>& filter_selection, const bool& has_pandoc);
 void add_note(Log& logger, std::istringstream& iss, const PATHS& paths, const std::string& file_ending, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>>& tag_map, std::vector<std::string>& mode_tags, std::vector<std::string>& filter_selection);
 void add_data(Log& logger, std::istringstream& iss, const PATHS& paths, std::vector<std::string> filter_selection);
 void show_details(Log& logger, std::istringstream& iss, const PATHS& paths, Config& conf, int& active_mode, std::vector<std::string> filter_selection, std::map<std::string, time_t> file_map, std::map<std::string, std::vector<std::string>> tag_map);
 void open_selection(Log& logger, const PATHS& paths, std::vector<std::string> filter_selection);
 void create_mode(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, int& num_modes, std::unordered_map<int, std::string>& mode_names, std::vector<std::string>& mode_tags, int& active_mode, std::vector<std::jthread>& file_watchers, OPEN_MODE& open_mode, const std::string& file_ending, bool& update_files);
 void delete_mode(Log& logger, std::istringstream& iss, Config& conf, int& num_modes, std::unordered_map<int, std::string>& mode_names, std::vector<std::string>& mode_tags, int& active_mode, std::vector<std::jthread>& file_watchers, OPEN_MODE& open_mode);
 void edit_mode(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, std::unordered_map<int, std::string>& mode_names, std::vector<std::string>& mode_tags, int& active_mode, std::vector<std::jthread>& file_watchers, OPEN_MODE& open_mode, const std::string& file_ending, bool& update_files);
 void activate_mode_command(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, std::unordered_map<int, std::string>& mode_names, int& active_mode, std::vector<std::string>& mode_tags, std::vector<std::jthread>& file_watchers, OPEN_MODE& open_mode, const std::string& file_ending, bool& update_files);
 void deactivate_mode(Log& logger, Config& conf, int& active_mode, std::vector<std::string>& mode_tags, std::vector<std::jthread>& file_watchers, OPEN_MODE& open_mode);
 void activate_mode(Log& logger, Config& conf, const PATHS& paths, int& active_mode, std::vector<std::string>& mode_tags, std::vector<std::jthread>& file_watchers, OPEN_MODE& open_mode, const std::string& file_ending, bool& update_files);
 void show_modes(Log& logger, std::istringstream& iss, Config& conf, std::unordered_map<int, std::string>& mode_names, int& active_mode, OPEN_MODE& open_mode);

 void parse_find_args(Log& logger, std::string& input, bool& data_only, std::vector<time_t>& date_args, std::vector<std::string>& ctags_args, std::vector<std::string>& catags_args, std::vector<std::string>& ntags_args, std::string& regex, std::vector<char>& version_args);
 void parse_details_args(Log& logger, std::istringstream& iss, Config& conf, int& active_mode, DETAIL_OPTIONS& detail_options);
 void parse_show_args(Log& logger, std::istringstream& iss, OPEN_MODE default_open, Config& conf, int& active_mode, SHOW_OPTIONS& show_options, FORMAT_OPTIONS& format_options);
 void parse_add_note(Log& logger, std::istringstream& iss, const PATHS& paths, const std::string& file_ending, std::string& filename, time_t& date_t, std::vector<std::string>& tags, bool& add_data);
 bool parse_mode_option(std::string& argument, MODE_OPTIONS& mode_options);
 bool parse_folder_watcher(std::string& argument, FOLDER_WATCHER_MODE& mode, std::string& current_folder, std::unordered_map<std::string, std::vector<std::string>>& folder_watcher_tags);
 bool parse_format(Log& logger, std::string& argument, FORMAT_OPTIONS& format_options);
 void parse_create_mode(Log& logger, std::istringstream& iss, Config& conf, std::string& mode_name, std::vector<std::string>& mode_tags, std::unordered_map<std::string, std::vector<std::string>>& folder_watcher_tags, OPEN_MODE& open_mode,
	 MODE_OPTIONS& mode_options);

 void get_folder_watcher(Config& conf, int& active_mode, std::unordered_map<std::string, std::vector<std::string>>& folder_watcher_tags);

 void format2open_mode(Config& conf, FORMAT_OPTIONS& format_options, OPEN_MODE& open_mode);
 int convert_document_to(const std::string& format, const std::string& ending, const PATHS& paths, const std::string& filename, const std::string& output_filename = "show");