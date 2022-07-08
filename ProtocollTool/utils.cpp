#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>
#include <shlwapi.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <array>
#include <set>
#include <regex>
#include "utils.h"
#include "conversions.h"
#include "Config.h"

using namespace std;

template<typename T>
void pad(basic_string<T>& s, typename basic_string<T>::size_type n, T c, const bool right) {
	if (n > s.length()) {
		s.append(n - s.length(), c);
	}
	else {
		if (right) {
			s = s.substr(0, n - 2) + "..";
		}
		else {
			s = ".." + s.substr(s.length() - n - 1);
		}
	}
}

std::string ltrim(const std::string& s)
{
	size_t start = s.find_first_not_of(WHITESPACE);
	return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string& s)
{
	size_t end = s.find_last_not_of(WHITESPACE);
	return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string& s) {
	return rtrim(ltrim(s));
}

int get_console_columns()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	int columns, rows;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

	return columns;
}

map<string, time_t> list_all_files(const PATHS& paths)
{
	/*
	Takes the path to the directory containing the files and returns a map of all files in that directory with their
	path as key and corresponding last access times in the time_t format as value.
	*/
	map<string, time_t> file_map;

	for (const auto& entry : filesystem::directory_iterator(paths.base_path / paths.file_path)) 
	{
		file_map[(paths.file_path / entry.path().filename()).string()] = chrono::system_clock::to_time_t(clock_cast<chrono::system_clock>(entry.last_write_time()));
	}

	return file_map;
}

map<string, vector<string>> list_all_tags(Log& logger, const PATHS& paths) {
	/*
	Reads all files in the given path and creates the tag map
	*/
	map<string, vector<string>> tag_map;

	for (const auto& entry : filesystem::directory_iterator(paths.base_path / paths.file_path))
	{
		tag_map[(paths.file_path / entry.path().filename()).string()] = read_tags(logger, entry.path().string());
	}
	return tag_map;
}

vector<string> read_tags(Log& logger, const string& path) {
	vector<string> tags = vector<string>();

	ifstream file(path);
	if (!file.is_open()) {
		logger << "File " << path << " could not be openend." << '\n';
		return tags;
	}

	bool reading_tags = false;
	string line, tag;
	while (getline(file, line))
	{
		if (line == "! TAGS START") {
			reading_tags = true;
		}
		else if (line == "! TAGS END") {
			reading_tags = false;
			break;
		}
		else if (reading_tags) {
			line.erase(0, 2); // removed the leading # part
			istringstream ss(line);
			while (ss >> tag) // split at delimiter
			{
				tag = trim(tag);
				boost::algorithm::to_lower(tag);
				tags.push_back(tag);
			}
		}

	}
	file.close();

	return tags;
}

void update_tags(Log& logger, const PATHS& paths, map<string, time_t>& file_map, map<string, vector<string>>& tag_map, map<string, int>& tag_count, vector<string>& filter_selection, const bool add_new_to_filter_selection)
{
	/*
	Takes an existing file_map (return of list_all_files) and uses it to only updates the tags of the modified files 
	*/
	map<string, time_t> new_file_map = list_all_files(paths);
	map<string, vector<string>> new_tag_map;
	vector<string> new_filter_selection;
	new_filter_selection.reserve(filter_selection.size());
	for (auto const& [path, time] : new_file_map)
	{
		if (time > file_map[path]) // file was changed
		{
			new_tag_map[path] = read_tags(logger, (paths.base_path / filesystem::path(path)).string());
			if (add_new_to_filter_selection)
				new_filter_selection.push_back(path);
		}
		else {
			new_tag_map[path] = tag_map[path];
		}
	}

	// remove deleted files from the selection
	for (const string& path : filter_selection)
	{
		if (new_file_map.find(path) != new_file_map.end()) {
			new_filter_selection.push_back(path);
		}
	}

	file_map = new_file_map;
	tag_map = new_tag_map;
	filter_selection = new_filter_selection;
	tag_count = get_tag_count(tag_map, filter_selection);
}


string get_filename(const PATHS& paths, time_t date, const string& file_ending)
{
	
	filesystem::path dir = paths.base_path / paths.file_path;
	string filename_base, filename;
	date2str_short(filename_base, date);
	char version = 'a';
	filename = filename_base;
	filename.push_back(version);
	filename += file_ending;
	while (filesystem::exists(dir / filesystem::path(filename))) {
		version += 1;
		filename = filename_base;
		filename.push_back(version);
		filename += file_ending;
	}

	return filename;
}

void write_file(Log& logger, const PATHS& paths, const string& filename, time_t date, vector<string> tags, const string& file_ending, const bool create_data)
{
	filesystem::path file_dir = paths.base_path / paths.file_path;
	filesystem::path data_dir = paths.base_path / paths.data_path;


	// create data folder
	if (create_data)
		filesystem::create_directories(data_dir / filesystem::path(filename).stem());
	
	// create file
	ofstream file(file_dir / filesystem::path(filename));
	if (!file.is_open()) {
		logger << "Error opening file " << file_dir / filename << endl;
		return;
	}

	
	string date_str;
	date2str(date_str, date);
	logger << "Created file " << filename << " at the path " << paths.file_path.string() << " and its corresponding data folder at " << (paths.data_path / filesystem::path(filename).stem()).string() << ".\n";
	file << "! FILENAME " << (paths.file_path / filesystem::path(filename)).string() << '\n';
	if (create_data)
		file << "! DATAFOLDER " << (paths.data_path / filesystem::path(filename).stem()).string() << '\n';
	file << "! DATE " << date_str << '\n';
	file << "! TAGS START" << '\n' << "! ";

	for (int i = 1; i <= tags.size(); i++) {
		if (i % 4 == 0 || i == tags.size()) { // new line for every 4th and last tag
			file << tags.at(i-1) << '\n' << "! ";
		}
		else {
			file << tags.at(i-1) << ", ";
		}
	}
	file << "TAGS END" << endl;


	file.close();
}

void add(FORMAT_OPTIONS& to, FORMAT_OPTIONS& from) {
	if (from.docx)
		to.docx = true;
	if (from.pdf)
		to.pdf = true;
	if (from.markdown)
		to.markdown = true;
	if (from.tex)
		to.tex = true;
	if (from.html)
		to.html = true;
	
}

void add(SHOW_OPTIONS& to, SHOW_OPTIONS& from) {
	if (from.show_data)
		to.show_data = true;
	if (from.show_metadata)
		to.show_metadata = true;
	if (from.show_tags)
		to.show_tags = true;
	if (from.show_table_of_content)
		to.show_table_of_content = true;
	if (from.hide_date)
		to.hide_date = true;
	if (from.open_image_data)
		to.open_image_data = true;
	

}

void add(DETAIL_OPTIONS& to, DETAIL_OPTIONS& from) {
	if (from.detail_content)
		to.detail_content = true;
	if (from.detail_path)
		to.detail_path = true;
	if (from.detail_long_path)
		to.detail_long_path = true;
	if (from.detail_last_modified)
		to.detail_last_modified = true;
	if (from.detail_tags)
		to.detail_tags = true;

}

void remove(FORMAT_OPTIONS& to, FORMAT_OPTIONS& from) {
	if (from.docx)
		to.docx = false;
	if (from.pdf)
		to.pdf = false;
	if (from.markdown)
		to.markdown = false;
	if (from.tex)
		to.tex = false;
	if (from.html)
		to.html = false;

}

void remove(SHOW_OPTIONS& to, SHOW_OPTIONS& from) {
	if (from.show_data)
		to.show_data = false;
	if (from.show_metadata)
		to.show_metadata = false;
	if (from.show_tags)
		to.show_tags = false;
	if (from.show_table_of_content)
		to.show_table_of_content = false;
	if (from.hide_date)
		to.hide_date = false;
	if (from.open_image_data)
		to.open_image_data = false;


}

void remove(DETAIL_OPTIONS& to, DETAIL_OPTIONS& from) {
	if (from.detail_content)
		to.detail_content = false;
	if (from.detail_path)
		to.detail_path = false;
	if (from.detail_long_path)
		to.detail_long_path = false;
	if (from.detail_last_modified)
		to.detail_last_modified = false;
	if (from.detail_tags)
		to.detail_tags = false;

}

void add(MODE_OPTIONS& to, MODE_OPTIONS& from) {
	add(to.format_options, from.format_options);
	add(to.detail_options, from.detail_options);
	add(to.show_options, from.show_options);
}

void remove(MODE_OPTIONS& to, MODE_OPTIONS& from) {
	remove(to.format_options, from.format_options);
	remove(to.detail_options, from.detail_options);
	remove(to.show_options, from.show_options);
}

void set_mode_options(Config& conf, MODE_OPTIONS& mode_options, const int& active_mode)
{
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_TAGS", mode_options.show_options.show_tags);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_METADATA", mode_options.show_options.show_metadata);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_TABLE_OF_CONTENT", mode_options.show_options.show_table_of_content);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_DATA", mode_options.show_options.show_data);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_HIDE_DATE", mode_options.show_options.hide_date);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_IMAGES", mode_options.show_options.open_image_data);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_HTML", mode_options.format_options.html);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_TEX", mode_options.format_options.tex);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_PDF", mode_options.format_options.pdf);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_DOCX", mode_options.format_options.docx);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_MD", mode_options.format_options.markdown);

	conf.set("MODE_" + to_string(active_mode) + "_DETAIL_TAGS", mode_options.detail_options.detail_tags);
	conf.set("MODE_" + to_string(active_mode) + "_DETAIL_PATH", mode_options.detail_options.detail_path);
	conf.set("MODE_" + to_string(active_mode) + "_DETAIL_LONG_PATH", mode_options.detail_options.detail_long_path);
	conf.set("MODE_" + to_string(active_mode) + "_DETAIL_LAST_MODIFIED", mode_options.detail_options.detail_last_modified);
	conf.set("MODE_" + to_string(active_mode) + "_DETAIL_SHOW_CONTENT", mode_options.detail_options.detail_content);
}

void get_mode_options(Config& conf, MODE_OPTIONS& mode_options, const int& active_mode)
{
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_TAGS", mode_options.show_options.show_tags);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_METADATA", mode_options.show_options.show_metadata);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_TABLE_OF_CONTENT", mode_options.show_options.show_table_of_content);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_DATA", mode_options.show_options.show_data);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_HIDE_DATE", mode_options.show_options.hide_date);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_IMAGES", mode_options.show_options.open_image_data);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_HTML", mode_options.format_options.html);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_TEX", mode_options.format_options.tex);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_PDF", mode_options.format_options.pdf);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_DOCX", mode_options.format_options.docx);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_MD", mode_options.format_options.markdown);
		 
	conf.get("MODE_" + to_string(active_mode) + "_DETAIL_TAGS", mode_options.detail_options.detail_tags);
	conf.get("MODE_" + to_string(active_mode) + "_DETAIL_PATH", mode_options.detail_options.detail_path);
	conf.get("MODE_" + to_string(active_mode) + "_DETAIL_LONG_PATH", mode_options.detail_options.detail_long_path);
	conf.get("MODE_" + to_string(active_mode) + "_DETAIL_LAST_MODIFIED", mode_options.detail_options.detail_last_modified);
	conf.get("MODE_" + to_string(active_mode) + "_DETAIL_SHOW_CONTENT", mode_options.detail_options.detail_content);
}


int convert_document_to(const std::string& format, const std::string& ending, const PATHS& paths, const std::string& filename, const std::string& output_filename)
{
	const string command_html = "pandoc -f markdown " + (paths.base_path / paths.tmp_path / filesystem::path(filename)).string() + " -t " + format + " -o " + (paths.base_path / paths.tmp_path / filesystem::path(output_filename + "." + ending)).string();
	FILE* pipe = _popen(command_html.c_str(), "r");
	int returnCode = _pclose(pipe);
	return returnCode;
}


void parse_show_args(Log& logger, std::istringstream& iss, Config& conf, int& active_mode, SHOW_OPTIONS& show_options, FORMAT_OPTIONS& format_options)
{
	

	string argument;
	while (iss >> argument)
	{
		if (argument == "t" || argument == "-t" || argument == "-tags" || argument == "-show_tags")
		{
			show_options.show_tags = true;
		}
		else if (argument == "m" || argument == "-m" || argument == "-metadata" || argument == "-show_metadata")
		{
			show_options.show_metadata = true;
		}
		else if (argument == "c" || argument == "-c" || argument == "-table_of_content" || argument == "-show_table_of_content")
		{
			if (show_options.hide_date)
			{
				logger << "Can't create a table of content when hide_date is set." << endl;
				continue;
			}

			show_options.show_table_of_content = true;
		}
		else if (argument == "d" || argument == "-d" || argument == "-data" || argument == "-show_data")
		{
			show_options.show_data = true;
		}
		else if (argument == "hd" || argument == "-hd" || argument == "-date" || argument == "-hide_date")
		{
			show_options.hide_date = true;
			if (show_options.show_table_of_content)
			{
				cout << "Can't create a table of content when hide_date is set." << endl;
				show_options.show_table_of_content = false;
			}
		}
		else if (argument == "i" || argument == "-i" || argument == "-images" || argument == "-open_images")
		{
			show_options.show_data = true;
			show_options.open_image_data = true;
		}
		else if (parse_format(logger, argument, format_options)) {
			continue;
		}
		else {
			logger << "Input argument " << argument;
			logger << " could not be parsed. Expected one of the following inputs:\n-show_(t)ags, show_(m)etadata, -show_table_of_(c)ontent, ";
			logger << "-show_(d)ata, -(h)ide_(d)ate, -open_(i)mage_data, -(html), -la(tex), -(pdf) ";
			logger << "-(docx), -(m)ark(d)own" << endl;
			return;
		}
	}

	conf.get("MODE_" + to_string(active_mode) + "_SHOW_TAGS", show_options.show_tags);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_METADATA", show_options.show_metadata);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_TABLE_OF_CONTENT", show_options.show_table_of_content);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_DATA", show_options.show_data);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_HIDE_DATE", show_options.hide_date);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_IMAGES", show_options.open_image_data);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_HTML", format_options.html);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_TEX", format_options.tex);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_PDF", format_options.pdf);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_DOCX", format_options.docx);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_MD", format_options.markdown);


	if (!format_options.docx && !format_options.html && !format_options.markdown && !format_options.pdf && !format_options.tex)
	{
		int default_open_int;
		conf.get("DEFAULT_OPEN_MODE", default_open_int);
		OPEN_MODE default_open = static_cast<OPEN_MODE>(default_open_int);
		switch (default_open)
		{
		case HTML:
			format_options.html = true;
			break;
		case MARKDOWN:
			format_options.markdown = true;
			break;
		case LATEX:
			format_options.tex = true;
			break;
		case PDF:
			format_options.pdf = true;
			break;
		case DOCX:
			format_options.docx = true;
			break;
		default:
			cout << "Error: Unknown default open mode " << default_open << endl;
			break;
		}
	}
}

bool parse_mode_option(string& argument, MODE_OPTIONS& mode_options)
{
	if (argument == "-show_tags" || argument == "-st")
	{
		mode_options.show_options.show_tags = true;
		return true;
	}
	else if (argument == "-show_metadata" || argument == "-sm")
	{
		mode_options.show_options.show_metadata = true;
		return true;
	}
	else if (argument == "-show_table_of_content" || argument == "-sc")
	{
		mode_options.show_options.show_table_of_content = true;
		return true;
	}
	else if (argument == "-show_data" || argument == "-sd")
	{
		mode_options.show_options.show_data = true;
		return true;
	}
	else if (argument == "-show_hide_date" || argument == "-shd")
	{
		mode_options.show_options.hide_date = true;
		return true;
	}
	else if (argument == "-show_open_images" || argument == "-show_images" || argument == "-si")
	{
		mode_options.show_options.open_image_data = true;
		return true;
	}
	else if (argument == "-show_open_as_html" || argument == "-shtml")
	{
		mode_options.format_options.html = true;
		return true;
	}
	else if (argument == "-show_open_as_pdf" || argument == "-spdf")
	{
		mode_options.format_options.pdf = true;
		return true;
	}
	else if (argument == "-show_open_as_markdown" || argument == "-smd")
	{
		mode_options.format_options.markdown = true;
		return true;
	}
	else if (argument == "-show_open_as_docx" || argument == "-sdocx")
	{
		mode_options.format_options.docx = true;
		return true;
	}
	else if (argument == "-show_open_as_tex" || argument == "-show_open_as_latex" || argument == "-stex")
	{
		mode_options.format_options.tex = true;
		return true;
	}
	else if (argument == "-detail_tags" || argument == "-dt")
	{
		mode_options.detail_options.detail_tags = true;
		return true;
	}
	else if (argument == "-detail_path" || argument == "-dp")
	{
		mode_options.detail_options.detail_path = true;
		return true;
	}
	else if (argument == "-detail_long_path" || argument == "-dlt")
	{
		mode_options.detail_options.detail_long_path = true;
		return true;
	}
	else if (argument == "-detail_last_modified" || argument == "-dlm")
	{
		mode_options.detail_options.detail_last_modified = true;
		return true;
	}
	else if (argument == "-detail_content" || argument == "-dc")
	{
		mode_options.detail_options.detail_content = true;
		return true;
	}
	else {
		return false;
	}

}

bool parse_folder_watcher(string& argument, FOLDER_WATCHER_MODE& mode, string& current_folder, unordered_map<string, vector<string>>& folder_watcher_tags)
{
	if (argument == "-watch_folder" || argument == "-watch" || argument == "-folder" || argument == "-wf") {
		mode = READ_FOLDER;
		current_folder = "";
		return true;
	}
	else if (mode == READ_FOLDER) {
		if (!filesystem::exists(argument)) {
			mode = READ_NONE;
			return false;
		}
		folder_watcher_tags[argument] = vector<string>();
		current_folder = argument;
		mode = READ_TAGS;
		return true;
	}
	else if (mode == READ_TAGS) {
		folder_watcher_tags[current_folder].push_back(argument);
		return true;
	}
	return false;
}

bool parse_format(Log& logger, string& argument, FORMAT_OPTIONS& format_options) {
	if (argument == "html" || argument == "-html")
	{
		format_options.html = true;
		return true;
	}
	else if (argument == "docx" || argument == "-docx") {
		format_options.docx = true;
		return true;
	}
	else if (argument == "tex" || argument == "latex" || argument == "-tex" || argument == "-latex") {
		format_options.tex = true;
		return true;
	}
	else if (argument == "pdf" || argument == "-pdf") {
		format_options.pdf = true;
		return true;
	}
	else if (argument == "md" || argument == "markdown" || argument == "-md" || argument == "-markdown") {
		format_options.markdown = true;
		return true;
	}
	else {
		logger << "Format " << argument << " was not recognized. Available options are:\n html, docx, tex, pdf, markdown." << endl;
		return false;
	}
}

void parse_find_args(Log& logger, istringstream& iss, bool& data_only, vector<time_t>& date_args, vector<string>& ctags_args, vector<string>& catags_args, vector<string>& ntags_args, string& regex, string& regex_content, vector<char>& version_args) {
	
	data_only = false;
	vector<string> _date_args;
	vector<string> _vers_args;

	string argument;
	ARGUMENT_MODE mode = NONE;
	// parse arguments
	while (iss >> argument)
	{
		if (argument == "-d" || argument == "-date")
		{
			mode = ARGUMENT_MODE::ARG_DATE;
		}
		else if (argument == "-ct" || argument == "-contains_tags")
		{
			mode = CONTAINS_TAGS;
		}
		else if (argument == "-cat" || argument == "-contains_all_tags")
		{
			mode = CONTAINS_ALL_TAGS;
		}
		else if (argument == "-nt" || argument == "-no_tags")
		{
			mode = NO_TAGS;
		}
		else if (argument == "-rt" || argument == "-reg_text")
		{
			mode = REG_TEXT;
		}
		else if (argument == "-rtc" || argument == "-reg_text_content")
		{
			mode = REG_TEXT_CONTENT;
		}
		else if (argument == "-do" || argument == "-data_only")
		{
			data_only = true;
		}
		else if (argument == "-v" || argument == "-versions")
		{
			mode = VERSIONS;
		}
		else
		{
			if (argument.size() > 1 && argument[0] == '-') {
				logger << "The argument " << argument << " was not recognized. It should be one of the following options: \n";
				logger << "-d, -date: finds notes between start and end date. \n";
				logger << "-v, -versions: selectes all notes in the given versions range. \n";
				logger << "-ct, -contains_tags: selects notes containing at least one of the specified tags.\n";
				logger << "-cat, -contains_all_tags: selectes notes containing all of the specified tags.\n";
				logger << "-nt, -no_tags: selects notes that do not contain the specified tags.\n";
				logger << "-rt, -reg_text: uses regular expressions on the note tags to select notes that match the given pattern.\n";
				logger << "-rtc, -reg_text_content: uses regular expressions on the note content to select notes that match the given pattern.\n";
				logger << "-do, -data_only: only select notes that have data associated with them." << endl;
				return;
			}
			switch (mode)
			{
			case NONE:
				logger << "Could not parse the find command " << argument << ". It should be one of the following: \n";
				logger << "-d, -date: finds notes between start and end date. \n";
				logger << "-v, -versions: selectes all notes in the given versions range. \n";
				logger << "-ct, -contains_tags: selects notes containing at least one of the specified tags.\n";
				logger << "-cat, -contains_all_tags: selectes notes containing all of the specified tags.\n";
				logger << "-nt, -no_tags: selects notes that do not contain the specified tags.\n";
				logger << "-rt, -reg_text: uses regular expressions on the note content to select notes that match the given pattern.\n";
				logger << "-do, -data_only: only select notes that have data associated with them." << endl;
				return;
			case ARGUMENT_MODE::ARG_DATE:
				_date_args.push_back(argument);
				break;
			case CONTAINS_TAGS:
				ctags_args.push_back(argument);
				break;
			case VERSIONS:
				_vers_args.push_back(argument);
				break;
			case CONTAINS_ALL_TAGS:
				catags_args.push_back(argument);
				break;
			case NO_TAGS:
				ntags_args.push_back(argument);
				break;
			case REG_TEXT:
				regex = argument;
				break;
			case REG_TEXT_CONTENT:
				regex_content = argument;
				break;
			default:
				logger << "Something went wrong. Unexpected mode in find with argument " << argument << endl;
				return;
			}
		}
	}

	
	// parse date input
	time_t start_date, end_date;
	CONV_ERROR ret1, ret2;
	size_t delim_pos;
	switch (_date_args.size())
	{
	case 0:
		// cout << "The -(d)ate argument expects input in the format of start_date-end_date or start_date to end_date, where start_data and end_date are in the form of dd, dd.mm, dd.mm.yy or special dates (t)oday, (y)esterday. \n";
		break;
	case 1:
		
		delim_pos = _date_args.at(0).find('-');

		// one date given as start and end date
		if (delim_pos == string::npos) {
			ret1 = str2date(start_date, _date_args.at(0).c_str());

			if (ret1 != CONV_SUCCESS)
			{
				logger << "The date " << _date_args.at(0) << " could not be parsed to start_date and end_date, because the separator (-) could not be found and the input is not a date." << endl;
				break;
			}
			date_args.push_back(start_date);
			date_args.push_back(start_date);
			break;
		}
		
		// dates given separated by a -
		ret1 = str2date(start_date, _date_args.at(0).substr(0, delim_pos).c_str());
		ret2 = str2date(end_date, _date_args.at(0).substr(delim_pos+1).c_str());

		if (ret1 != CONV_SUCCESS) {
			logger << "The start date " << start_date << " could not be parsed to a date." << endl;
			break;
		}
		else if (ret2 != CONV_SUCCESS) {
			logger << "The end date " << end_date << " could not be parsed to a date." << endl;
			break;
		}

		date_args.push_back(start_date);
		date_args.push_back(end_date);
		break;
	case 3:
		ret1 = str2date(start_date, _date_args.at(0).c_str());
		ret2 = str2date(end_date, _date_args.at(2).c_str());
		if (ret1 != CONV_SUCCESS) {
			logger << "The start date " << start_date << " could not be parsed to a date." << endl;
			break;
		}
		else if (ret2 != CONV_SUCCESS) {
			logger << "The end date " << end_date << " could not be parsed to a date." << endl;
			break;
		}

		date_args.push_back(start_date);
		date_args.push_back(end_date);
		break;
	default:
		logger << "Unexpected white spaces in the date parameter. Found " << _date_args.size() << " whitespaces and expects zero or two." << endl;
		return;
		
	}
	
	// parse version input
	for (const string& vers_arg : _vers_args)
	{
		if (vers_arg.size() == 1) {
			version_args.push_back(vers_arg[0]);
			continue;
		}

		if (vers_arg.size() == 3 && (vers_arg[1] == '-' || vers_arg[1] == ':'))
		{
			for (char i = vers_arg[0]; i <= vers_arg[2]; i++)
			{
				version_args.push_back(i);
			}
			continue;
		}

		logger << "Could not parse the version input " << vers_arg << ". It should either be a character, number or range in the form a-c or a:c." << endl;
		return;
	}
}

void parse_create_mode(Log& logger, std::istringstream& iss, Config& conf, string& mode_name, std::vector<std::string>& mode_tags, unordered_map<string, vector<string>>& folder_watcher_tags, 
	MODE_OPTIONS& mode_options)
{
	// set default values
	mode_name = "default";
	mode_tags = vector<string>();

	if (iss)
	{
		iss >> mode_name;
		mode_name = trim(mode_name);
	}
	else {
		return;
	}

	string argument;
	string current_folder { "" };
	FOLDER_WATCHER_MODE mode {READ_NONE};
	while (iss >> argument)
	{
		boost::to_lower(argument);
		if (argument[0] == '-' || mode == READ_TAGS || mode == READ_FOLDER)
		{
			if (!parse_mode_option(argument, mode_options) && !parse_folder_watcher(argument,  mode, current_folder, folder_watcher_tags)) {
				logger << "Did not recognize parameter " << argument << ".\n It should be one of the following options controlling the (s)how command:\n";
				logger << "[-(s)how_(t)ags] [-(s)how_(m)etadata] [-(s)how_table_of_(c)ontent] ";
				logger << "[-(s)how_(d)ata] [-(s)how_(h)ide_(d)ate] [-(s)how_open_(i)mages] [-(s)how_open_as_(html)] ";
				logger << "[-(s)how_open_as_(pdf)] [-(s)how_open_as_(docx)] [-(s)how_open_as_(m)ark(d)own] [-(s)how_open_as_la(tex)]\n";
				logger << "And one of the following for controlling the (d)etails command:\n";
				logger << "[-(d)etail_tags] [-(d)etail_(p)ath] [-(d)etail_(l)ong_(p)ath] [-(d)etail_(l)ast_(m)odified] [-(d)etail_(c)ontent]\n";
				logger << "Or a path to a folder that should be observed such that when a file is created, the newly created file is added to the ";
				logger << "data folder and a new note is created. Additionally, tags that will be added when this occures can be entered: \n";
				logger << "[-(w)atch_(f)older path_to_folder tag1 tag2 ...]\n";
				logger << "Note that multiple folders can be specified by repeating the [-(w)atch_(f)older path_to_folder tag1 tag2 ...] command ";
				logger << "With different paths (and tags).\n\n";
			}
		}
		else if (argument[0] != '-') {
			mode_tags.push_back(argument);
		}
		else {
			logger << "Error in parse_create_mode, this condition should not have been reached. Input " << argument << " with current mode: " << mode << endl;
		}
		
	}
}

void parse_details_args(Log& logger, istringstream& iss, Config& conf, int& active_mode, DETAIL_OPTIONS& detail_options)
{

	string argument;
	while (iss >> argument)
	{
		if (argument == "t" || argument == "-t" || argument == "-tags" || argument == "-show_tags")
		{
			detail_options.detail_tags = true;
		}
		else if (argument == "p" || argument == "-p" || argument == "-path" || argument == "-show_path")
		{
			detail_options.detail_path = true;
		}
		else if (argument == "lp" || argument == "-lp" || argument == "-long_path" || argument == "-show_long_path")
		{
			detail_options.detail_long_path = true;
		}
		else if (argument == "m" || argument == "-m" || argument == "-last_modified" || argument == "-show_last_modified")
		{
			detail_options.detail_last_modified = true;
		}
		else if (argument == "c" || argument == "-c" || argument == "-content" || argument == "-show_content")
		{
			detail_options.detail_content = true;
		}
		else {
			logger << "Input argument " << argument;
			logger << " could not be parsed. Expected one of the following inputs:\n-show_(t)ags, show_(p)ath, -show_(l)ong_(p)ath, ";
			logger << "-show_last_(m)odified, -show_(c)ontent" << endl;
			return;
		}
	}

	if (active_mode != -1)
	{
		conf.get("MODE_" + to_string(active_mode) + "_DETAIL_TAGS", detail_options.detail_tags);
		conf.get("MODE_" + to_string(active_mode) + "_DETAIL_PATH", detail_options.detail_path);
		conf.get("MODE_" + to_string(active_mode) + "_DETAIL_LONG_PATH", detail_options.detail_long_path);
		conf.get("MODE_" + to_string(active_mode) + "_DETAIL_LAST_MODIFIED", detail_options.detail_last_modified);
		conf.get("MODE_" + to_string(active_mode) + "_DETAIL_SHOW_CONTENT", detail_options.detail_content);
	}
}

void parse_add_note(Log& logger, std::istringstream& iss, const PATHS& paths, const std::string& file_ending, std::string& filename, time_t& date_t, std::vector<std::string>& tags, bool& add_data)
{
	add_data = false;

	string date_str;
	CONV_ERROR ret;
	iss >> date_str;
	ret = str2date(date_t, date_str);
	if (ret != CONV_ERROR::CONV_SUCCESS) {
		logger << "First argument of the new command has to be the date: ((t)oday, (y)esterday, dd, dd.mm, dd.mm.yyyy), set date to today" << endl;
		date_t = time(NULL);
	}

	// read tags and optional add_data argument
	string argument;
	while (iss >> argument) {
		if (argument == "-d" || argument == "-add_data") {
			add_data = true;
			continue;
		}
		tags.push_back(argument);
	}

	filename = get_filename(paths, date_t, file_ending);
}


void show_filtered_notes(Log& logger, std::istringstream& iss, Config& conf, int& active_mode, const PATHS& paths, const string& tmp_filename, std::vector<std::string>& filter_selection, const bool& has_pandoc)
{
	// parse arguments
	SHOW_OPTIONS show_options;
	FORMAT_OPTIONS format_options;
	parse_show_args(logger, iss, conf, active_mode, show_options, format_options);

	// write content of selected files to output file
	ofstream show_file(paths.base_path / paths.tmp_path / filesystem::path(tmp_filename));
	string date_str;
	time_t date_t;
	CONV_ERROR ret;
	string line;
	if (show_options.show_table_of_content && !show_options.hide_date)
	{
		show_file << "# Table of Contents" << '\n';

		int i = 1;
		for (const string& path : filter_selection)
		{
			ret = str2date_short(date_t, filesystem::path(path).stem().string().substr(0, 8));
			if (ret != CONV_SUCCESS) {
				logger << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}
			ret = date2str(date_str, date_t);
			if (ret != CONV_SUCCESS) {
				logger << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}

			show_file << i++ << ". [note from " << date_str << " Version " << filesystem::path(path).stem().string()[8] << "](#" << filesystem::path(path).stem().string() << ")\n";
			ifstream file(paths.base_path / filesystem::path(path));
			int line_count = 0;
			while (getline(file, line))
			{
				if (line[0] == '#')
				{
					size_t start = line.find_first_not_of("# ");
					if (start == string::npos)
					{
						continue;
					}
					string tab;
					for (size_t j = 0; j < start - 1; j++)
					{
						tab += "\t";
					}
					show_file << tab << "-" << "[" << line.substr(start) << "](#" << line_count << ")\n";
				}
				line_count++;
			}
			file.close();
		}
		show_file << endl;
	}


	for (const string& path : filter_selection)
	{

		// add date to the output
		if (!show_options.hide_date) {
			ret = str2date_short(date_t, filesystem::path(path).stem().string().substr(0, 8));
			if (ret != CONV_SUCCESS) {
				logger << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}
			ret = date2str(date_str, date_t);
			if (ret != CONV_SUCCESS) {
				logger << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}

			show_file << "# " << date_str << " Version " << filesystem::path(path).stem().string()[8] << "<a id=\"" << filesystem::path(path).stem().string() << "\"></a>\n";
		}

		ifstream file(paths.base_path / paths.file_path / filesystem::path(path).filename());
		if (!file.is_open()) {
			logger << "Error opening note file " << (paths.base_path / paths.file_path / filesystem::path(path).filename()).string() << endl;
		}
		vector<string> tags;
		bool reading_tags = false;
		int line_count = 0;
		while (getline(file, line))
		{
			// show everything
			if (show_options.show_metadata) {
				show_file << line;
				if (line[0] == '#')
				{
					show_file << "<a id=\"" << line_count << "\"></a>";
				}
				show_file << '\n';
			}

			// show tags
			else if (show_options.show_tags && !reading_tags)
			{
				if (line == "! TAGS START")
				{
					reading_tags = true;
				}
			}
			else if (show_options.show_tags && reading_tags)
			{

				if (line != "! TAGS END")
				{
					line.erase(0, 2);
					istringstream ss(line);
					string tag;
					while (ss >> tag)
					{
						tag = trim(tag);
						tags.push_back(tag);
					}
				}
				else
				{
					show_file << "Tags: ";
					reading_tags = false;
					for (const string& tag : tags)
					{
						show_file << tag << " ";
					}
					show_file << '\n';
				}
			}
			// show only file content
			else
			{
				if (line.substr(0, 2) != "! ")
				{
					show_file << line;
					if (line[0] == '#')
					{
						show_file << "<a id=\"" << line_count << "\"></a>";
					}
					show_file << '\n';
				}
			}
			line_count++;

		}


		file.close();
		show_file << '\n';
	}

	show_file.close();

	// open showfile and data folders
	if (show_options.show_data)
	{
		for (const string& path : filter_selection)
		{
			const wstring datapath = (paths.base_path / paths.data_path / filesystem::path(path).stem()).wstring();
			ShellExecute(NULL, L"open", datapath.c_str(), NULL, NULL, SW_SHOWNORMAL);
			if (show_options.open_image_data)
			{
				// search for image data in the folder
				for (const auto& entry : filesystem::directory_iterator(paths.base_path / paths.data_path / filesystem::path(path).stem()))
				{
					const string ext = entry.path().extension().string();
					if (ext == "jpg" || ext == "png" || ext == "jpeg" || ext == "gif")
					{
						ShellExecute(NULL, L"open", entry.path().wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
					}
				}
			}
		}
	}

	// convert by pandoc by
	// pandoc -f markdown 28062022a.md -t latex -o test.tex
	if (has_pandoc)
	{

		if (format_options.html)
		{
			if (int returnCode = convert_document_to("html", "html", paths, tmp_filename, "show") != 0) {
				logger << "Error " << returnCode << " while converting markdown to html using pandoc" << endl;
			}
			else {
				ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.html")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		}

		if (format_options.markdown)
		{

			ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.md")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);

		}

		if (format_options.docx)
		{
			if (int returnCode = convert_document_to("docx", "docx", paths, tmp_filename, "show") != 0) {
				logger << "Error " << returnCode << " while converting markdown to docx using pandoc" << endl;
			}
			else {
				ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.docx")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		}

		if (format_options.tex)
		{
			if (int returnCode = convert_document_to("latex", "tex", paths, tmp_filename, "show") != 0) {
				logger << "Error " << returnCode << " while converting markdown to latex (tex) using pandoc" << endl;
			}
			else {
				ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.tex")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		}

		if (format_options.pdf)
		{
			if (int returnCode = convert_document_to("pdf", "pdf", paths, tmp_filename, "show") != 0) {
				logger << "Error " << returnCode << " while converting markdown to pdf using pandoc" << endl;
			}
			else {
				ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.pdf")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		}

	}
	else {
		ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path(tmp_filename)).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
	}





}

void create_mode(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, int& num_modes, unordered_map<int, string>& mode_names, vector<string>& mode_tags, int& active_mode, vector<jthread>& file_watchers, const string& file_ending, bool& update_files, HANDLE& hStopEvent)
{
	deactivate_mode(logger, conf, active_mode, mode_tags, file_watchers, hStopEvent);

	string mode_name;
	mode_tags = vector<string>();
	unordered_map<string, vector<string>> folder_watcher_tags;
	MODE_OPTIONS mode_options;
	parse_create_mode(logger, iss,conf, mode_name, mode_tags, folder_watcher_tags, mode_options);

	boost::algorithm::to_lower(mode_name);

	for (const auto& [id, name] : mode_names) // check if name already exists
	{
		if (mode_name == name)
		{
			logger << "Mode already exists! Delete the old mode with the command (del)ete before creating a new one" << endl;
			return;
		}
	}

	active_mode = static_cast<int>(mode_names.size()); // set id to next free number
	mode_names[active_mode] = mode_name;

	num_modes += 1;
	conf.set("NUM_MODES", num_modes);
	conf.set("MODE_" + to_string(active_mode) + "_NAME", mode_name);
	conf.set("MODE_" + to_string(active_mode) + "_NUM_TAGS", static_cast<int>(mode_tags.size()));

	set_mode_options(conf, mode_options, active_mode);

	for (auto i = 0; i < mode_tags.size(); i++)
	{
		conf.set("MODE_" + to_string(active_mode) + "_TAG_" + to_string(i), mode_tags.at(i));
	}

	
	conf.set("MODE_" + to_string(active_mode) + "_NUM_WATCH_FOLDERS", static_cast<int>(folder_watcher_tags.size()));
	int folder_counter = 0; // not ideal, order in map can change but does not matter because all entries are treated equal
	for (const auto& [folder_path, tags] : folder_watcher_tags) { 
		conf.set("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(folder_counter) + "_NUM_TAGS", static_cast<int>(tags.size()));
		conf.set("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(folder_counter) + "_PATH", folder_path);

		for (auto j = 0; j < tags.size(); j++)
		{
			conf.set("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(folder_counter) + "_TAG_" + to_string(j), tags.at(j));

		}
		folder_counter++;
	}
	

	activate_mode(logger, conf, paths, active_mode, mode_tags, file_watchers, file_ending, update_files, hStopEvent);

}

void delete_mode(Log& logger, std::istringstream& iss, Config& conf, int& num_modes, unordered_map<int, string>& mode_names, vector<string>& mode_tags, int& active_mode, vector<jthread>& file_watchers, HANDLE& hStopEvent)
{
	string mode_name;
	deactivate_mode(logger, conf, active_mode, mode_tags, file_watchers, hStopEvent);

	if (!iss)
	{
		logger << "Specify name of the mode that should be deleted" << endl;
		return;
	}

	iss >> mode_name;
	boost::algorithm::to_lower(mode_name);

	int mode_id = -1;
	for (const auto& [id, name] : mode_names) // check if name already exists
	{
		if (mode_name == name)
		{
			mode_id = id;
		}
	}

	if (mode_id != -1)
	{
		mode_names.erase(mode_id);
	}
	else {
		logger << "Mode " << mode_name << " not found." << endl;
		return;
	}
	

	num_modes -= 1;
	conf.set("NUM_MODES", num_modes);

	int num_tags;
	conf.get("MODE_" + to_string(mode_id) + "_NUM_TAGS", num_tags);
	conf.remove("MODE_" + to_string(mode_id) + "_NUM_TAGS");
	for (auto i = 0; i < num_tags; i++)
	{
		conf.remove("MODE_" + to_string(mode_id) + "_TAG_" + to_string(i));
	}

	conf.remove("MODE_" + to_string(mode_id) + "_NAME");

	int num_watch_folder, num_folder_tags;
	conf.get("MODE_" + to_string(active_mode) + "_NUM_WATCH_FOLDERS", num_watch_folder);
	conf.remove("MODE_" + to_string(active_mode) + "_NUM_WATCH_FOLDERS");
	for (auto i = 0; i < num_watch_folder; i++) {
		conf.get("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_NUM_TAGS", num_folder_tags);
		conf.remove("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_NUM_TAGS");
		conf.remove("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_PATH");

		for (auto j = 0; j < num_folder_tags; j++)
		{
			conf.remove("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_TAG_" + to_string(j));

		}
	}
}

void get_mode_tags(Config& conf, const int& mode_id, vector<string>& mode_tags) {
	int num_tags;
	conf.get("MODE_" + to_string(mode_id) + "_NUM_TAGS", num_tags);

	string mode_tag;
	for (auto i = 0; i < num_tags; i++)
	{
		conf.get("MODE_" + to_string(mode_id) + "_TAG_" + to_string(i), mode_tag);
		mode_tag = trim(mode_tag);
		boost::to_lower(mode_tag);
		mode_tags.push_back(mode_tag);
	}
	
}

void set_mode_tags(Config& conf, const int& mode_id, vector<string>& mode_tags) {
	conf.set("MODE_" + to_string(mode_id) + "_NUM_TAGS", static_cast<int>(mode_tags.size()));

	for (auto i = 0; i < mode_tags.size(); i++)
	{
		conf.set("MODE_" + to_string(mode_id) + "_TAG_" + to_string(i), mode_tags.at(i));
	}

}

void edit_mode(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, unordered_map<int, string>& mode_names, vector<string>& mode_tags, int& active_mode, vector<jthread>& file_watchers, const string& file_ending, bool& update_files, HANDLE& hStopEvent)
{
	deactivate_mode(logger, conf, active_mode, mode_tags, file_watchers, hStopEvent);

	string mode_name;
	if (!iss)
	{
		logger << "Specify name of the mode that should be edited" << endl;
		return;
	}

	iss >> mode_name;
	boost::algorithm::to_lower(mode_name);

	int mode_id = -1;
	for (const auto& [id, name] : mode_names) // check if name already exists
	{
		if (mode_name == name)
		{
			mode_id = id;
		}
	}

	if (mode_id == -1)
	{
		logger << "Mode " << mode_name << " could not be found! See available modes with the command: modes." << endl;
		return;
	}


	get_mode_tags(conf, mode_id, mode_tags);
	
	MODE_OPTIONS mode_options;
	get_mode_options(conf, mode_options, mode_id);
	unordered_map<string, vector<string>> folder_watcher_tags;
	get_folder_watcher(conf, mode_id, folder_watcher_tags);

	enum EDIT_MODE {NONE, ADD_OPTIONS, REMOVE_OPTIONS, ADD_TAGS, REMOVE_TAGS, CHANGE_MODE_NAME, ADD_FOLDER_WATCHER, REMOVE_FOLDER_WATCHER};
	EDIT_MODE mode = NONE;
	FOLDER_WATCHER_MODE folder_mode = READ_NONE;
	MODE_OPTIONS add_opt, remove_opt;
	unordered_map<string, vector<string>> add_folder_watcher_tags, remove_folder_watcher_tags;
	string current_folder;
	string new_mode_name;
	string argument;
	while (iss >> argument)
	{
		if (argument == "-add_opt" || argument == "-add_option" || argument == "-add_options")
		{
			mode = ADD_OPTIONS;
		}
		else if (argument == "-remove_opt" || argument == "-remove_option" || argument == "-remove_options")
		{
			mode = REMOVE_OPTIONS;
		}
		else if (argument == "-add_t" || argument == "-add_tags" || argument == "-add_tag") {
			mode = ADD_TAGS;
		}
		else if (argument == "-remove_t" || argument == "-remove_tags" || argument == "-remove_tag")
		{
			mode = REMOVE_TAGS;
		}
		else if (argument == "-change_name" || argument == "-change_mode_name") {
			mode = CHANGE_MODE_NAME;
		}
		else if (argument == "-add_watch_folder" || argument == "-add_folder" || argument == "-add_watch") {
			mode = ADD_FOLDER_WATCHER;
			current_folder = "";
			folder_mode = READ_NONE;
		}
		else if (argument == "-remove_watch_folder" || argument == "-remove_folder" || argument == "-remove_watch") {
			mode = REMOVE_FOLDER_WATCHER;
			current_folder = "";
			folder_mode = READ_NONE;
		}
		else if (mode == ADD_OPTIONS) {
			if (!parse_mode_option(argument, add_opt)) {
				logger << "Did not recognize parameter " << argument << ".\n It should be one of the following options controlling the (s)how command:\n";
				logger << "[-(s)how_(t)ags] [-(s)how_(m)etadata] [-(s)how_table_of_(c)ontent] ";
				logger << "[-(s)how_(d)ata] [-(s)how_(h)ide_(d)ate] [-(s)how_open_(i)mages] [-(s)how_open_as_(html)] ";
				logger << "[-(s)how_open_as_(pdf)] [-(s)how_open_as_(docx)] [-(s)how_open_as_(m)ark(d)own] [-(s)how_open_as_la(tex)]\n";
				logger << "And one of the following for controlling the (d)etails command:\n";
				logger << "[-(d)etail_tags] [-(d)etail_(p)ath] [-(d)etail_(l)ong_(p)ath] [-(d)etail_(l)ast_(m)odified] [-(d)etail_(c)ontent]\n\n";
			}

		}
		else if (mode == REMOVE_OPTIONS) {
			if (!parse_mode_option(argument, remove_opt)) {
				logger << "Did not recognize parameter " << argument << ".\n It should be one of the following options controlling the (s)how command:\n";
				logger << "[-(s)how_(t)ags] [-(s)how_(m)etadata] [-(s)how_table_of_(c)ontent] ";
				logger << "[-(s)how_(d)ata] [-(s)how_(h)ide_(d)ate] [-(s)how_open_(i)mages] [-(s)how_open_as_(html)] ";
				logger << "[-(s)how_open_as_(pdf)] [-(s)how_open_as_(docx)] [-(s)how_open_as_(m)ark(d)own] [-(s)how_open_as_la(tex)]\n";
				logger << "And one of the following for controlling the (d)etails command:\n";
				logger << "[-(d)etail_tags] [-(d)etail_(p)ath] [-(d)etail_(l)ong_(p)ath] [-(d)etail_(l)ast_(m)odified] [-(d)etail_(c)ontent]\n\n";
			}
		}
		else if (mode == ADD_TAGS) {
			argument = trim(argument);
			boost::to_lower(argument);
			if (find(mode_tags.begin(), mode_tags.end(), argument) == mode_tags.end()) {
				mode_tags.push_back(argument);
			}
			else {
				logger << "Tag " << argument << " already exists for mode " << mode_name << endl;
			}
		}
		else if (mode == REMOVE_TAGS) {
			argument = trim(argument);
			boost::to_lower(argument);
			if (find(mode_tags.begin(), mode_tags.end(), argument) != mode_tags.end()) {
				mode_tags.erase(remove(mode_tags.begin(), mode_tags.end(), argument));
			}
			else {
				logger << "Tag " << argument << " was not found in the existing tags list and can therefore not be removed." << endl;
			}
		}
		else if (mode == CHANGE_MODE_NAME) {
			new_mode_name = argument;
		}
		else if (mode == ADD_FOLDER_WATCHER) {
			// if folder is already watched, add tags to watcher else add watcher
			parse_folder_watcher(argument, folder_mode, current_folder, add_folder_watcher_tags);
			for (const auto& [new_folder, new_folder_tags] : add_folder_watcher_tags)
			{
				if (folder_watcher_tags.contains(new_folder)) // add tags
				{
					// TODO: convert vector<string> tags to set<string>
					for (const auto& new_tag : new_folder_tags) {
						if (find(folder_watcher_tags[new_folder].begin(), folder_watcher_tags[new_folder].end(), new_tag) == mode_tags.end()) {
							folder_watcher_tags[new_folder].push_back(new_tag);
						}
						else {
							logger << "Tag " << new_tag << " already exists for folder watcher " << new_folder << endl;
						}
					}
					
				}
				else { // add new watcher
					folder_watcher_tags[new_folder] = new_folder_tags;
				}
			}
		}
		else if (mode == REMOVE_FOLDER_WATCHER) {
			// if folder is already watched and tags are specified, remove tags from watcher else remove watcher
			parse_folder_watcher(argument, folder_mode, current_folder, remove_folder_watcher_tags);
		
			for (const auto& [remove_folder, remove_folder_tags] : remove_folder_watcher_tags)
			{
				if (remove_folder_tags.size() > 0  && folder_watcher_tags.contains(remove_folder)) // remove tags
				{
					// TODO: convert vector<string> tags to set<string>
					for (auto& old_tag : folder_watcher_tags[remove_folder]) {
						folder_watcher_tags[remove_folder].erase(remove(folder_watcher_tags[remove_folder].begin(), folder_watcher_tags[remove_folder].end(), old_tag));
					}
				}
				else { // add new watcher
					folder_watcher_tags.erase(remove_folder);
				}
			}
		}

	}

	if (!new_mode_name.empty()) {
		mode_names[mode_id] = new_mode_name;
		conf.set("MODE_" + to_string(mode_id) + "_NAME", new_mode_name);
	}

	add(mode_options, add_opt);
	remove(mode_options, remove_opt);
	set_mode_options(conf, mode_options, mode_id);
	set_mode_tags(conf, mode_id, mode_tags);

	conf.set("MODE_" + to_string(active_mode) + "_NUM_WATCH_FOLDERS", static_cast<int>(folder_watcher_tags.size()));
	int folder_counter = 0; // not ideal, order in map can change but does not matter because all entries are treated equal
	for (const auto& [folder_path, tags] : folder_watcher_tags) {
		conf.set("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(folder_counter) + "_NUM_TAGS", static_cast<int>(tags.size()));
		conf.set("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(folder_counter) + "_PATH", folder_path);

		for (auto j = 0; j < tags.size(); j++)
		{
			conf.set("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(folder_counter) + "_TAG_" + to_string(j), tags.at(j));

		}
		folder_counter++;
	}

	activate_mode(logger, conf, paths, active_mode, mode_tags, file_watchers, file_ending, update_files, hStopEvent);
}

void show_modes(Log& logger, std::istringstream& iss, Config& conf, std::unordered_map<int, std::string>& mode_names, int& active_mode)
{
	int con_col = get_console_columns();
	string mode_name{ "Mode name:" }, mode_tags{ "Mode tags: ..." }, mode_options{ "Options: ..." }, watch_folder{ "Watch folders: ..." }, folder_tags{ " with tags: ..." }, filler{};
	pad(mode_name, 26, ' ');
	pad(mode_tags, con_col - 26, ' '); //46

	pad(mode_options, con_col, ' ');
	pad(watch_folder, 30, ' ');
	pad(folder_tags, con_col - 30, ' ');
	pad(filler, con_col, '-');
	logger << filler << '\n';
	logger << mode_name << mode_tags << '\n' << mode_options << '\n' << watch_folder << folder_tags << '\n';
	logger << filler << "\n";
	for (const auto& [id, name] : mode_names)
	{
		// show name
		string display_name = name;
		pad(display_name, 25, ' ');
		logger << display_name << " ";

		// show tags
		int num_tags;
		conf.get("MODE_" + to_string(id) + "_NUM_TAGS", num_tags);
		ostringstream tags;
		int tag_width = 0;
		if (num_tags > 0) {
			tag_width = ((con_col - 26 - 1) / num_tags) - 1;
		}
		
			
		if (tag_width < 8) {
			tag_width = 7;
		}

		int extra_tag_width = 0;
		string tag;
		for (auto i = 0; i < num_tags; i++) {
			conf.get("MODE_" + to_string(id) + "_TAG_" + to_string(i), tag);

			if (tag.size() < tag_width) {
				extra_tag_width += tag_width - tag.size();
			}
			else if (extra_tag_width > 0) {
				int dif = tag.size() - tag_width;
				if (dif >= extra_tag_width) {
					pad(tag, tag_width + extra_tag_width, ' ');
					extra_tag_width = 0;
				}
				else {
					pad(tag, tag_width + dif, ' ');
					extra_tag_width -= dif;
				}
			}
			else {
				pad(tag, tag_width, ' ');
			}

			
			tags << tag << ' ';
		}
		string tags_str = tags.str();
		if (tags_str.size() > 0)
			tags_str.erase(tags_str.end() - 1);
		pad(tags_str, (con_col - 26 - 1), ' ');
		logger << tags_str << " ";


		// show options
		MODE_OPTIONS mode_options;
		get_mode_options(conf, mode_options, active_mode);

		ostringstream opt;
		if (mode_options.show_options.show_tags)
			opt << "-st ";
		if (mode_options.show_options.show_metadata)
			opt << "-sm ";
		if (mode_options.show_options.show_table_of_content)
			opt << "-sc ";
		if (mode_options.show_options.show_data)
			opt << "-sd ";
		if (mode_options.show_options.hide_date)
			opt << "-hd ";
		if (mode_options.show_options.open_image_data)
			opt << "-si ";
		if (mode_options.format_options.html)
			opt << "-shtml ";
		if (mode_options.format_options.tex)
			opt << "stex ";
		if (mode_options.format_options.pdf)
			opt << "spdf ";
		if (mode_options.format_options.docx)
			opt << "sdocx ";
		if (mode_options.format_options.markdown)
			opt << "smd ";
		if (mode_options.detail_options.detail_tags)
			opt << "dt ";
		if (mode_options.detail_options.detail_path)
			opt << "dp ";
		if (mode_options.detail_options.detail_long_path)
			opt << "dlp ";
		if (mode_options.detail_options.detail_last_modified)
			opt << "dlm ";
		if (mode_options.detail_options.detail_content)
			opt << "dc ";

		string options = opt.str();
		if (options.size() > 0)
			options.erase(options.end() - 1);

		pad(options, con_col, ' ');
		logger << options << '\n';

		// show watched folders and tags
		int num_folders;
		string folder_path_str;
		conf.get("MODE_" + to_string(id) + "_NUM_WATCH_FOLDERS", num_folders);
		for (auto i = 0; i < num_folders; i++)
		{
			int num_folder_tags;
			int folder_tag_width = 0;
			if (num_tags > 0) {
				folder_tag_width = ((con_col - 28 - 1) / num_tags) - 1;
			}
			if (folder_tag_width < 8) {
				folder_tag_width = 7;
			}

			string folder_tags_str;
			ostringstream folder_tags;
			conf.get("MODE_" + to_string(id) + "_WATCH_FOLDERS_" + to_string(i) + "_NUM_TAGS", num_folder_tags);
			conf.get("MODE_" + to_string(id) + "_WATCH_FOLDERS_" + to_string(i) + "_PATH", folder_path_str);

			string folder_tag;
			int extra_folder_tag_width = 0;
			for (auto j = 0; j < num_folder_tags; j++)
			{
				conf.get("MODE_" + to_string(id) + "_WATCH_FOLDERS_" + to_string(i) + "_TAG_" + to_string(j), folder_tag);
				// left bound padding of tags
				if (folder_tag.size() < folder_tag_width) {
					extra_folder_tag_width += folder_tag_width - folder_tag.size();
				}
				else if (extra_folder_tag_width > 0) {
					int dif = folder_tag.size() - folder_tag_width;
					if (dif >= extra_folder_tag_width) {
						pad(folder_tag, folder_tag_width + extra_folder_tag_width, ' ');
						extra_folder_tag_width = 0;
					}
					else {
						pad(folder_tag, folder_tag_width + dif, ' ');
						extra_folder_tag_width -= dif;
					}
				}
				else {
					pad(folder_tag, folder_tag_width, ' ');
				}
				
				folder_tags << folder_tag << " ";

			}
			folder_tags_str = folder_tags.str();
			folder_tags_str.erase(folder_tags_str.end() - 1);
			pad(folder_path_str, 29, ' ', false);
			pad(folder_tags_str, con_col - 30, ' ');
			logger << folder_path_str << " " << folder_tags_str << '\n';
		}

		logger << filler << '\n';
	}
}

void filter_notes(Log& logger, istringstream& iss, const PATHS& paths, vector<string>& filter_selection, map<string, time_t>& file_map, map<string, vector<string>> tag_map, vector<string> mode_tags)
{

	vector<string> _filter_selection;

	bool data_only = false;
	vector<time_t> date_args;
	vector<string> ctags_args;
	vector<string> catags_args;
	vector<string> ntags_args;
	vector<char> vers_args;
	string regex_pattern{}, regex_content_pattern{};

	parse_find_args(logger, iss, data_only, date_args, ctags_args, catags_args, ntags_args, regex_pattern, regex_content_pattern, vers_args);
	catags_args.insert(catags_args.begin(), mode_tags.begin(), mode_tags.end());

	for (const auto& path : filter_selection)
	{
		// filter for date
		if (date_args.size() > 0) 
		{
			time_t note_date;
			str2date_short(note_date, filesystem::path(path).stem().string());
			if (note_date < date_args.at(0) || note_date > date_args.at(1)) {
				continue;
			}
		}

		// filter for version
		if (vers_args.size() > 0)
		{
			char note_version = filesystem::path(path).stem().string()[8];
			if (find(vers_args.begin(), vers_args.end(), note_version) == vers_args.end())
			{
				continue;
			}

		}

		// filter for having a data folder
		if (data_only) 
		{
			if (!filesystem::exists(paths.base_path / paths.data_path / filesystem::path(path).stem())) {
				continue;
			}
		}

		// filter for tags where only one of the tags needs to match
		if (ctags_args.size() > 0)
		{
			bool has_tag = false;
			vector<string> tags = tag_map[path];
			for (auto& tag : ctags_args)
			{
				tag = trim(tag);
				boost::algorithm::to_lower(tag);
				if (find(tags.begin(), tags.end(), tag) != tags.end()) {
					has_tag = true;
					break;
				}
			}

			if (!has_tag)
			{
				continue;
			}
		}

		// filter for tags where all tags need to match
		if (catags_args.size() > 0)
		{
			bool all_match = true;
			vector<string> tags = tag_map[path];
			for (auto& tag : catags_args)
			{
				tag = trim(tag);
				boost::algorithm::to_lower(tag);
				if (find(tags.begin(), tags.end(), tag) == tags.end()) {
					all_match = false;
					break;
				}
			}
			if (!all_match)
			{
				continue;
			}

		}

		// filter for tags that can not be in the tag list of the note
		if (ntags_args.size() > 0)
		{
			bool has_tag = false;
			string tag_copy;
			for (const auto& tag : ntags_args)
			{
				tag_copy = trim(tag);
				boost::algorithm::to_lower(tag_copy);
				if (find(tag_map[path].begin(), tag_map[path].end(), tag_copy) != tag_map[path].end()) {
					has_tag = true;
					break;
				}
			}

			if (has_tag)
			{
				continue;
			}
		}

		// filtering for regex in the tags
		if (!regex_pattern.empty())
		{
			regex pat(regex_pattern);
			bool found_match = false;
			string tag_copy;
			for (const auto& tag : tag_map[path])
			{
				// found match
				tag_copy = trim(tag);
				boost::to_lower(tag_copy);
				if (regex_match(tag_copy, pat)) {
					found_match = true;
					break;
				}
			}

			if (!found_match) {
				continue;
			}
		}

		// filtering for regex in the file content
		if (!regex_content_pattern.empty())
		{
			regex pat(regex_content_pattern);
			ifstream file(paths.base_path / filesystem::path(path));
			const string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
			file.close();

			if (!regex_match(content, pat)) {
				continue;
			}
		}
		

		_filter_selection.push_back(path);
	}
	filter_selection = _filter_selection;
}

void find_notes(Log& logger, istringstream& iss, const PATHS& paths, vector<string>& filter_selection, map<string, time_t>& file_map, map<string, vector<string>> tag_map, vector<string> mode_tags)
{
	filter_selection = vector<string>();
	filter_selection.reserve(tag_map.size());

	for (const auto& [path, tag] : tag_map)
	{
		filter_selection.push_back(path);
	}

	filter_notes(logger, iss, paths, filter_selection, file_map, tag_map, mode_tags);
}

void add_note(Log& logger, std::istringstream& iss, const PATHS& paths, const std::string& file_ending, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>>& tag_map, vector<string>& mode_tags, vector<string>& filter_selection)
{
	string filename;
	vector<string> tags;
	bool add_data;
	time_t date;
	parse_add_note(logger, iss, paths, file_ending, filename, date, tags, add_data);
	tags.insert(tags.end(), mode_tags.begin(), mode_tags.end());

	write_file(logger, paths, filename, date, tags, file_ending, add_data);
	

	// open new file
	const wstring exec_filename = (paths.base_path / paths.file_path / filesystem::path(filename)).wstring();
	ShellExecute(0, L"open", exec_filename.c_str(), 0, 0, SW_SHOW);

	// update file_map, tag_map, tag_count and filter selection
	string path = (paths.file_path / filesystem::path(filename)).string();
	file_map[path] = time_t(NULL);
	tag_map[path] = tags;
	filter_selection.push_back(path);
	get_tag_count(tag_map, filter_selection);
}

void add_data(Log& logger, std::istringstream& iss, const PATHS& paths, std::vector<std::string> filter_selection)
{
	filesystem::path data_dir = paths.base_path / paths.data_path;
	if (filter_selection.size() > 1)
	{
		string choice;
		logger << filter_selection.size() << " found in the selection. Add data folders for all of them? (y/n)" << endl;
		cin >> choice;
		cin.clear();
		cin.ignore(10000, '\n');
		logger.input(choice);
		if (choice == "y" || choice == "Y") {

			for (const string& path : filter_selection)
			{
				filesystem::create_directories(data_dir / filesystem::path(path).stem());
				logger << "Added data directory for " << filesystem::path(path).stem() << endl;
			}
		}
	}
	else if (filter_selection.size() == 1)
	{
		filesystem::create_directories(data_dir / filesystem::path(filter_selection.at(0)).stem());
		logger << "Added data directory for " << filesystem::path(filter_selection.at(0)).stem() << endl;
	}
	else {
		logger << "No file found in the selection. Use the find command to specify which note should have a data folder added." << endl;
		return;
	}
}

void show_details(Log& logger, std::istringstream& iss, const PATHS& paths, Config& conf, int& active_mode, vector<string> filter_selection, std::map<std::string, time_t> file_map, std::map<std::string, std::vector<std::string>> tag_map)
{
	DETAIL_OPTIONS detail_options;
	parse_details_args(logger, iss, conf, active_mode, detail_options);

	time_t note_date;
	string date_str, modified_date_str;
	for (const string& path : filter_selection)
	{
		string filename = filesystem::path(path).stem().string();
		str2date_short(note_date, filename.substr(0, 8));
		date2str(date_str, note_date);
		logger << date_str;
		if (filename[8] != 'a')
		{
			logger << " Version " << filename[8];
		}
		if (detail_options.detail_long_path)
		{
			logger << " at " << paths.base_path / filesystem::path(path);
		}
		else if (detail_options.detail_path)
		{
			logger << " at " << path;
		}
		if (detail_options.detail_tags)
		{
			logger << " with tags: ";
			for (const auto& tag : tag_map[path])
			{
				logger << tag << " ";
			}
		}
		if (detail_options.detail_last_modified)
		{
			date2str_long(modified_date_str, file_map[path]);
			logger << " last modified at " << modified_date_str;
		}
		if (detail_options.detail_content)
		{
			ifstream file(paths.base_path / filesystem::path(path));
			string str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
			file.close();

			logger << " with note text:\n" << str;
		}
		logger << endl;
	}
}

void open_selection(Log& logger, const PATHS& paths, std::vector<std::string> filter_selection)
{
	if (filter_selection.size() == 1)
	{
		const wstring exec_filename = (paths.base_path / filesystem::path(filter_selection.at(0))).wstring();
		ShellExecute(0, L"open", exec_filename.c_str(), 0, 0, SW_SHOW);
	}
	else if (filter_selection.size() > 1)
	{
		string choice;
		logger << filter_selection.size() << " found in the selection. Open all of them? (y/n)" << endl;
		cin >> choice;
		cin.clear();
		cin.ignore(10000, '\n');
		logger.input(choice);
		if (choice == "y" || choice == "Y") {

			for (const string& path : filter_selection)
			{
				const wstring exec_filename = (paths.base_path / filesystem::path(path)).wstring();
				ShellExecute(0, L"open", exec_filename.c_str(), 0, 0, SW_SHOW);
			}
		}
	}
	else {
		logger << "Found no file to open in the current selection. Use (f)ind to set selection." << endl;
	}
}

void activate_mode(Log& logger, Config& conf, const PATHS& paths, int& active_mode, vector<string>& mode_tags, std::vector<std::jthread>& file_watchers, const string& file_ending, bool& update_files, HANDLE& hStopEvent)
{
	
	// read in mode tags
	int num_tags;
	conf.get("MODE_" + to_string(active_mode) + "_NUM_TAGS", num_tags);
	string tag;
	for (auto i = 0; i < num_tags; i++)
	{
		conf.get("MODE_" + to_string(active_mode) + "_TAG_" + to_string(i), tag);
		mode_tags.push_back(tag);
	}

	// read in folders to watch and their tags and start watching
	int num_folders = 0;
	string folder_path_str;
	conf.get("MODE_" + to_string(active_mode) + "_NUM_WATCH_FOLDERS", num_folders);
	for (auto i = 0; i < num_folders; i++)
	{
		int num_folder_tags;
		vector<string> folder_tags;
		conf.get("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_NUM_TAGS", num_folder_tags);
		conf.get("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_PATH", folder_path_str);
		for (auto j = 0; j < num_folder_tags; j++)
		{
			conf.get("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_TAG_" + to_string(j), tag);
			folder_tags.push_back(tag);
			
		}
		file_watchers.push_back(jthread(file_change_watcher, ref(logger), filesystem::path(folder_path_str), paths, file_ending, folder_tags, ref(update_files), ref(hStopEvent)));
	}
}

void deactivate_mode(Log& logger, Config& conf, int& active_mode, vector<string>& mode_tags, std::vector<std::jthread>& file_watchers, HANDLE& hStopEvent) {
	active_mode = -1;
	mode_tags = vector<string>();

	SetEvent(hStopEvent);
	for (jthread& watcher : file_watchers) {
		watcher.request_stop();
	}

	file_watchers = vector<jthread>();
}

void get_folder_watcher(Config& conf, int& active_mode, unordered_map<string, vector<string>>& folder_watcher_tags) {
	int num_folders = 0;
	string folder_path_str, tag;
	conf.get("MODE_" + to_string(active_mode) + "_NUM_WATCH_FOLDERS", num_folders);
	for (auto i = 0; i < num_folders; i++)
	{
		int num_folder_tags;
		vector<string> folder_tags;
		conf.get("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_NUM_TAGS", num_folder_tags);
		conf.get("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_PATH", folder_path_str);
		for (auto j = 0; j < num_folder_tags; j++)
		{
			conf.get("MODE_" + to_string(active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_TAG_" + to_string(j), tag);
			folder_tags.push_back(tag);

		}
		folder_watcher_tags[folder_path_str] = folder_tags;
	}
}

void activate_mode_command(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, std::unordered_map<int, std::string>& mode_names, int& active_mode, vector<string>& mode_tags, std::vector<std::jthread>& file_watchers, const string& file_ending, bool& update_files, HANDLE& hStopEvent)
{
	mode_tags = vector<string>();
	active_mode = -1;

	string mode_name;
	if (!iss)
	{
		logger << "Specify the name of the mode you want to activate" << endl;
		return;
	}

	iss >> mode_name;
	for (const auto& [id, name] : mode_names)
	{
		if (name == mode_name)
		{
			active_mode = id;
		}
	}

	if (active_mode != -1)
	{
		activate_mode(logger, conf, paths, active_mode, mode_tags, file_watchers, file_ending, update_files, hStopEvent);
	}
	else {
		logger << "Mode " << mode_name << " not found." << endl;
	}

}


map<string, int> get_tag_count(map<string, vector<string>>& tag_map, vector<string> filter_selection)
{
	map<string, int> tag_count;
	vector<string> tags;
	for (const auto& path : filter_selection)
	{
		tags = tag_map[path];
		for (const auto& tag : tags)
		{
			tag_count[tag] += 1;
		}
	}
	return tag_count;
}


void file_change_watcher(Log& logger, const filesystem::path watch_path, const PATHS paths, const std::string file_ending, vector<string> watch_path_tags, bool& update_files, HANDLE& hStopEvent)
{
	DWORD dwWaitStatus;
	HANDLE dwChangeHandles[3];
	TCHAR lpDrive[4];
	TCHAR lpFile[_MAX_FNAME];
	TCHAR lpExt[_MAX_EXT];

	_tsplitpath_s(watch_path.wstring().c_str(), lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

	lpDrive[2] = (TCHAR)'\\';
	lpDrive[3] = (TCHAR)'\0';


	// save state of directory to find changed files later on
	unordered_map<string, filesystem::file_time_type> files;
	for (const auto& entry : filesystem::directory_iterator(watch_path))
	{
		files[entry.path().string()] = entry.last_write_time();
	}

	// Watch the directory for file creation and deletion. 

	dwChangeHandles[0] = FindFirstChangeNotification(
		watch_path.wstring().c_str(),  // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 

	dwChangeHandles[1] = FindFirstChangeNotification(
		watch_path.wstring().c_str(),
		FALSE,
		FILE_NOTIFY_CHANGE_LAST_WRITE);

	dwChangeHandles[2] = hStopEvent;

	if (dwChangeHandles[0] == INVALID_HANDLE_VALUE || dwChangeHandles[1] == INVALID_HANDLE_VALUE)
	{
		logger << "ERROR: FindFirstChangeNotification function failed.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[0] == NULL || dwChangeHandles[1] == NULL)
	{
		logger << "ERROR: Unexpected NULL from FindFirstChangeNotification.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[2] == INVALID_HANDLE_VALUE)
	{
		logger << "ERROR: StopThread function failed.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[2] == NULL)
	{
		logger << "ERROR: Unexpected NULL from StopThread.\n";
		ExitProcess(GetLastError());
	}
	while (true)
	{
		// Wait for notification.
		dwWaitStatus = WaitForMultipleObjects(3, dwChangeHandles, FALSE, INFINITE);
		time_t date = time(NULL);
		string filename = get_filename(paths, date, file_ending);
		switch (dwWaitStatus)
		{
		case WAIT_OBJECT_0:

			// A file was created, renamed, or deleted in the directory.
			// Refresh this directory and restart the notification.
			//logger << "Monitored folder " << watch_path.string() << " had file changes." << endl;

			for (const auto& entry : filesystem::directory_iterator(watch_path))
			{
				if (!files.contains(entry.path().string())) {
					logger << "Add file: " << entry.path().filename().string() << " to the data folder " << filesystem::path(filename).stem().string() << endl;

					// add note and copy found file to the corresponding data folder
					write_file(logger, paths, filename, date, watch_path_tags, file_ending, true);
					ShellExecute(0, L"open", (paths.base_path / paths.file_path / filesystem::path(filename)).wstring().c_str(), 0, 0, SW_SHOW);
					filesystem::copy_file(entry.path(), paths.base_path / paths.data_path / filesystem::path(filename).stem() / entry.path().filename());
					
					files[entry.path().string()] = entry.last_write_time();
					
					
				}
			}
			
			// restart handle to listen for next change
			if (FindNextChangeNotification(dwChangeHandles[0]) == FALSE)
			{
				printf("\n ERROR: FindNextChangeNotification function failed.\n");
				ExitProcess(GetLastError());
			}
			break;
		case WAIT_OBJECT_0 + 1: // file was written to
			for (const auto& entry : filesystem::directory_iterator(watch_path))
			{
				if (files.contains(entry.path().string()) && files[entry.path().string()] < entry.last_write_time()) {
					logger << "File " << entry.path().filename().string() << " in the observed folder " << watch_path.string() << " was written to." << endl;
				}
			}
			
			// restart handle to listen for next change
			if (FindNextChangeNotification(dwChangeHandles[1]) == FALSE)
			{
				printf("\n ERROR: FindNextChangeNotification function failed.\n");
				ExitProcess(GetLastError());
			}
			break;
		case WAIT_OBJECT_0 + 2: // Stop event
			logger << "Stop event" << endl;
			return;
		case WAIT_TIMEOUT:

			// A timeout occurred, this would happen if some value other 
			// than INFINITE is used in the Wait call and no changes occur.
			// In a single-threaded environment you might not want an
			// INFINITE wait.

			printf("\nNo changes in the timeout period.\n");
			break;

		default:
			printf("\n ERROR: Unhandled dwWaitStatus.\n");
			ExitProcess(GetLastError());
			break;
		}
	}
}


void note_change_watcher(Log& logger, const PATHS paths, bool& update_files, HANDLE& hStopMonitorEvent)
{
	DWORD dwWaitStatus;
	HANDLE dwChangeHandles[3];
	TCHAR lpDrive[4];
	TCHAR lpFile[_MAX_FNAME];
	TCHAR lpExt[_MAX_EXT];

	_tsplitpath_s((paths.base_path / paths.file_path).wstring().c_str(), lpDrive, 4, NULL, 0, lpFile, _MAX_FNAME, lpExt, _MAX_EXT);

	lpDrive[2] = (TCHAR)'\\';
	lpDrive[3] = (TCHAR)'\0';


	dwChangeHandles[0] = FindFirstChangeNotification(
		(paths.base_path / paths.file_path).wstring().c_str(),  // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 

	dwChangeHandles[1] = FindFirstChangeNotification(
		(paths.base_path / paths.file_path).wstring().c_str(),
		FALSE,
		FILE_NOTIFY_CHANGE_LAST_WRITE);

	dwChangeHandles[2] = hStopMonitorEvent;

	if (dwChangeHandles[0] == INVALID_HANDLE_VALUE || dwChangeHandles[1] == INVALID_HANDLE_VALUE)
	{
		logger << "ERROR: FindFirstChangeNotification function failed.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[0] == NULL || dwChangeHandles[1] == NULL)
	{
		logger << "ERROR: Unexpected NULL from FindFirstChangeNotification.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[2] == INVALID_HANDLE_VALUE)
	{
		logger << "ERROR: StopThread function failed.\n";
		ExitProcess(GetLastError());
	}

	if (dwChangeHandles[2] == NULL)
	{
		logger << "ERROR: Unexpected NULL from StopThread.\n";
		ExitProcess(GetLastError());
	}
	while (true)
	{

		// Wait for notification.
		dwWaitStatus = WaitForMultipleObjects(3, dwChangeHandles, FALSE, INFINITE);
		switch (dwWaitStatus)
		{
		case WAIT_OBJECT_0: // A file was created, renamed, or deleted in the directory.
			update_files = true;
			logger << "File added to file dir" << endl;
				
			// restart handle to listen for next change
			if (FindNextChangeNotification(dwChangeHandles[0]) == FALSE)
			{
				printf("\n ERROR: FindNextChangeNotification function failed.\n");
				ExitProcess(GetLastError());
			}
			break;

		case WAIT_OBJECT_0 + 1: // file was written to
			update_files = true;
			logger << "File changed in file dir" << endl;
			// restart handle to listen for next change
			if (FindNextChangeNotification(dwChangeHandles[1]) == FALSE)
			{
				printf("\n ERROR: FindNextChangeNotification function failed.\n");
				ExitProcess(GetLastError());
			}
			break;

		case WAIT_OBJECT_0 + 2: // Stop event
			logger << "Stop monitoring files for changes" << endl;
			return;
		case WAIT_TIMEOUT:

			// A timeout occurred, this would happen if some value other 
			// than INFINITE is used in the Wait call and no changes occur.
			// In a single-threaded environment you might not want an
			// INFINITE wait.

			printf("\nNo changes in the timeout period.\n");
			break;

		default:
			printf("\n ERROR: Unhandled dwWaitStatus.\n");
			ExitProcess(GetLastError());
			break;
		}
	}
}
