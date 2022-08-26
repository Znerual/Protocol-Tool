#pragma once

#include <string>
#include <map>
#include <filesystem>
#include <vector>
#include <list>
#include <thread>
#include <boost/bimap.hpp>
#include <stdlib.h>
#include <stdio.h>
#include <set>

#ifdef _WIN32
#include <tchar.h>
#include <Windows.h>
#endif

#include "Config.h"
#include "log.h"
#include "trietree.h"

enum ARGUMENT_MODE {NONE, ARG_DATE, CONTAINS_TAGS, CONTAINS_ALL_TAGS, NO_TAGS, REG_TEXT, REG_TEXT_CONTENT, VERSIONS};
enum OPEN_MODE { HTML, MARKDOWN, LATEX, PDF, DOCX };
enum FOLDER_WATCHER_MODE { READ_NONE, READ_FOLDER, READ_TAGS};
enum ALIGN {LEFT, MIDDLE, RIGHT};

enum class CMD { NEW, FIND, FILTER, SHOW, ADD_DATA, DETAILS, TAGS, QUIT, CREATE_MODE, DELETE_MODE, EDIT_MODE, ACTIVATE, DEACTIVATE, MODES, UPDATE, OPEN, HELP, TODOS };
enum class PA { DATE, TAGS, MODE_NAME, CMD };
enum class OA { DATA, DATE_R, CTAGS, CATAGS, NTAGS, REGT, REGC, VERS_R, STAGS, MDATA, TOC, NODATE, IMG, HTML, TEX, PDF, DOCX, MD, PATH, LPATH, LMOD, CONTENT, ADDOPT, REMOPT, ADDTAG, REMTAG, CHANAME, TAGS, DATES, REGTEXT, VERSIONS, NAME, CMD, STAGS_ARG, MDATA_ARG, TOC_ARG, NODATE_ARG, IMG_ARG, HTML_ARG, TEX_ARG, PDF_ARG, DOCX_ARG, MD_ARG, PATH_ARG, LPATH_ARG, LMOD_ARG, CONTENT_ARG, ADDWF, REMWF, PATHANDTAGS, PATHD };
extern std::set<OA> OA_DATA;


typedef boost::bimap<std::string, CMD> cmd_map;
typedef boost::bimap<std::string, PA> pa_map;
typedef boost::bimap<std::string, OA> oa_map;

typedef std::unordered_map<CMD, std::pair<std::list<PA>, std::unordered_map<OA, std::list<OA>>>> CMD_STRUCTURE;
typedef std::unordered_map<OA, bool> MODE_OPTIONS;

/*
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
*/
struct PATHS {
	std::filesystem::path base_path;
	std::filesystem::path file_path;
	std::filesystem::path data_path;
	std::filesystem::path tmp_path;

	std::filesystem::path md_exe;
	std::filesystem::path pdf_exe;
	std::filesystem::path html_exe;
	std::filesystem::path docx_exe;
	std::filesystem::path tex_exe;
};
struct CMD_NAMES {
	cmd_map cmd_names;
	pa_map pa_names;
	oa_map oa_names;
};

struct COMMAND_INPUT {
	std::string input;
	CMD_STRUCTURE cmd_structure;
	CMD_NAMES cmd_names;
};

class AUTOCOMPLETE {
public:
	TrieTree cmd_names;
	TrieTree tags;
	TrieTree mode_names;

	AUTOCOMPLETE(const CMD_NAMES& cmd_names, const std::list<std::string>& tags, const std::list<std::string>& mode_names);
	AUTOCOMPLETE(const CMD_NAMES& cmd_names, const std::map<std::string, int>& tag_count, const std::unordered_map<int, std::string>& mode_names);
};

const std::string WHITESPACE = " ,\n\r\t\f\v";
std::string ltrim(const std::string& s, const std::string& characters = WHITESPACE);
std::string rtrim(const std::string& s, const std::string& characters = WHITESPACE);
std::string trim(const std::string& s, const std::string& characters = WHITESPACE);

template<typename T>
void pad(std::basic_string<T>& s, typename std::basic_string<T>::size_type n, T c, const bool cap_right = true, const ALIGN align = LEFT);
std::string wrap(const std::string s, const int margin = 2, const ALIGN align = LEFT);

void read_mode_names(const Config& conf, std::list<std::string>& mode_names);

void set_console_font();
void set_console_background(const int& width, const int& height);

void get_console_size(int& rows, int& columns);
void load_config(const std::string init_path, Config& conf);
void check_base_path(Config& conf, PATHS& paths);
void check_standard_paths(PATHS& paths);

#ifdef _WIN32
void RunExternalProgram(Log& logger, std::filesystem::path executeable, std::filesystem::path file, HANDLE& hExit);
/**
* Similar to getline(stream, string, delimiter), but waits until a tag OR newline event occures
* and returns the WinUser.h keycode for it (VK_TAB 9, VK_RETURN 13)
* @param c string of awaited user input
**/
int getinput(std::string& c);
bool parse_folder_watcher(std::string& argument, FOLDER_WATCHER_MODE& mode, std::string& current_folder, std::unordered_map<std::string, std::vector<std::string>>& folder_watcher_tags);
 void get_default_applications(PATHS& paths);
void parse_create_mode(Log& logger, std::istringstream& iss, Config& conf, std::string& mode_name, std::vector<std::string>& mode_tags, std::unordered_map<std::string, std::vector<std::string>>& folder_watcher_tags, MODE_OPTIONS& mode_options);
#else
void parse_create_mode(Log& logger, std::istringstream& iss, Config& conf, std::string& mode_name, std::vector<std::string>& mode_tags,	MODE_OPTIONS& mode_options);
#endif


 void init_mode_options(MODE_OPTIONS& mode_options);
 void set_mode_options(Config& conf, MODE_OPTIONS& mode_options, const int& active_mode);
 void get_mode_options(Config& conf, MODE_OPTIONS& mode_options, const int& active_mode);

 void get_mode_tags(Config& conf, const int& mode_id, std::vector<std::string>& mode_tags);
 void set_mode_tags(Config& conf, const int& mode_id, std::vector<std::string>& mode_tags);

 void print_greetings(const int& width);

 void get_pandoc_installed(Log& logger, Config& conf,bool& ask_pandoc, bool& has_pandoc);
 void parse_cmd(Log& logger, const COMMAND_INPUT& command_input, CMD& cmd, std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);
 /*
 void parse_find_args(Log& logger, std::istringstream& iss, bool& data_only, std::vector<time_t>& date_args, std::vector<std::string>& ctags_args, std::vector<std::string>& catags_args, std::vector<std::string>& ntags_args, std::string& regex, std::string& regex_content, std::vector<char>& version_args);
 void parse_details_args(Log& logger, std::istringstream& iss, Config& conf, int& active_mode, DETAIL_OPTIONS& detail_options);
 void parse_show_args(Log& logger, std::istringstream& iss, Config& conf, int& active_mode, SHOW_OPTIONS& show_options, FORMAT_OPTIONS& format_options);
 void parse_add_note(Log& logger, std::istringstream& iss, const PATHS& paths, const std::string& file_ending, std::string& filename, time_t& date_t, std::vector<std::string>& tags, bool& add_data);
 bool parse_mode_option(std::string& argument, MODE_OPTIONS& mode_options);
 bool parse_format(Log& logger, std::string& argument, FORMAT_OPTIONS& format_options);

 void parse_cmd(Log& logger, const COMMAND_INPUT& command_input, CMD& cmd, std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs);

 
 */