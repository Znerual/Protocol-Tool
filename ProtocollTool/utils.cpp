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
#include <WinUser.h>

#include "utils.h"
#include "conversions.h"
#include "Config.h"
#include "file_manager.h"
#include "trietree.h"

using namespace std;

template<typename T>
void pad(basic_string<T>& s, typename basic_string<T>::size_type n, T c, const bool cap_right, const ALIGN align) {
	if (n > s.length()) {
		switch (align)
		{
		case LEFT:
			s.append(n - s.length(), c);
			break;
		case RIGHT:
			s.insert(0, n - s.length(), c);
			break;
		case MIDDLE:
			s.insert(0, (n - s.length()) / 2, c);
			s.append((n - s.length()) - (n - s.length()) / 2, c);
			break;
		default:
			break;
		}
		
	}
	else {
		if (cap_right) {
			s = s.substr(0, n - 2) + "..";
		}
		else {
			s = ".." + s.substr(s.length() - n - 1);
		}
	}
}

std::string wrap(const std::string s, const int margin, const ALIGN align) {
	string res = s;
	int width, height;
	get_console_size(height, width);
	if (s.size() + 2 * margin > width) {
		// wrap
		istringstream iss(s);
		ostringstream oss;
		string word, margin_str(margin, ' ');
		int pos = margin;
		oss << margin_str;
		while (iss >> word) {
			if (pos + word.size() < width - margin)
			{
				oss << word << " ";
				pos += word.size() + 1;
			}
			else {
				oss << '\n' << margin_str << word << " ";
				pos = margin + word.size() + 1;
			}
		}
		res = oss.str();
	}
	else {
		// pad
		
		res.insert(0, margin, ' ');
		pad(res, width - 2 * margin, ' ', true, align);
		res.append(margin, ' ');
	}

	return res;
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

void get_console_size(int& rows, int& columns)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

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

void RunExternalProgram(Log& logger, std::filesystem::path executeable, std::filesystem::path file, HANDLE& hExit)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES saAttr;

	HANDLE m_hChildStd_OUT_Wr = NULL;
	HANDLE m_hChildStd_OUT_Rd = NULL;
	HANDLE dwChangeHandle[2];

	ZeroMemory(&saAttr, sizeof(saAttr));
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&m_hChildStd_OUT_Rd, &m_hChildStd_OUT_Wr, &saAttr, 0))
	{
		// log error
		logger << to_string(GetLastError()) << endl;
		return;
	}

	// Ensure the read handle to the pipe for STDOUT is not inherited.

	if (!SetHandleInformation(m_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
	{
		// log error
		logger << to_string(GetLastError()) << endl;
		return;
	}

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.hStdError = m_hChildStd_OUT_Wr;
	si.hStdOutput = m_hChildStd_OUT_Wr;
	si.dwFlags |= STARTF_USESTDHANDLES;

	ZeroMemory(&pi, sizeof(pi));

	wstring wexec{ executeable.wstring() }, wfile{ file.wstring() };
	vector<wchar_t> exec(wexec.begin(), wexec.end());
	exec.push_back(L'\0');

	vector<wchar_t> command_line(wfile.begin(), wfile.end());
	command_line.push_back(L'\0');

	// Start the child process. 
	if (!CreateProcess(
		&exec[0],           // No module name (use command line)
		&command_line[0],    // Command line
		NULL,                           // Process handle not inheritable
		NULL,                           // Thread handle not inheritable
		TRUE,                           // Set handle inheritance
		0,                              // No creation flags
		NULL,                           // Use parent's environment block
		NULL,                           // Use parent's starting directory 
		&si,                            // Pointer to STARTUPINFO structure
		&pi)                            // Pointer to PROCESS_INFORMATION structure
		)
	{
		logger << to_string(GetLastError()) << endl;
		return;
	}
	else
	{
		// could read data with, for this purpose, ignore childs cout
		//m_hreadDataFromExtProgram = CreateThread(0, 0, readDataFromExtProgram, NULL, 0, NULL);
	}

	dwChangeHandle[0] = pi.hProcess;
	dwChangeHandle[1] = hExit;

	WaitForMultipleObjects(2, dwChangeHandle, FALSE, INFINITE);

	CloseHandle(m_hChildStd_OUT_Rd);
	CloseHandle(m_hChildStd_OUT_Wr);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

}
/*
DWORD __stdcall readDataFromExtProgram(void* argh)
{
	DWORD dwRead;
	CHAR chBuf[1024];
	BOOL bSuccess = FALSE;

	for (;;)
	{
		bSuccess = ReadFile(m_hChildStd_OUT_Rd, chBuf, 1024, &dwRead, NULL);
		if (!bSuccess || dwRead == 0) continue;

		// Log chBuf

		if (!bSuccess) break;
	}
	return 0;
}
*/

int getinput(string& c)
{

	DWORD cNumRead,  i;
	INPUT_RECORD irInBuf[128];

	
	/*
	DWORD fdwSaveOldMode, fdwMode,;
	if (!GetConsoleMode(hStdin, &fdwSaveOldMode)) {
		cout << "Error get console mode" << endl;
		SetConsoleMode(hStdin, fdwSaveOldMode);
		ExitProcess(0);
	}
	 
	fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
	if (!SetConsoleMode(hStdin, fdwMode)) {
		cout << "Error set console mode" << endl;
		SetConsoleMode(hStdin, fdwSaveOldMode);
		ExitProcess(0);
	}
	*/	
	

	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	while (true)
	{
		// Wait for the events.

		ReadConsoleInput(
			hStdin,      // input buffer handle
			irInBuf,     // buffer to read into
			128,         // size of read buffer
			&cNumRead); // number of records read
		

		// Dispatch the events to the appropriate handler.
		for (i = 0; i < cNumRead; i++)
		{
			if (irInBuf[i].EventType == KEY_EVENT) {
				if (irInBuf[i].Event.KeyEvent.bKeyDown) {
					if (irInBuf[i].Event.KeyEvent.wVirtualKeyCode == VK_TAB) {
						return VK_TAB;
					}
					else if (irInBuf[i].Event.KeyEvent.wVirtualKeyCode == VK_RETURN) {
						return VK_RETURN;
					}
					else {
						cout << irInBuf[i].Event.KeyEvent.uChar.AsciiChar;
						if (irInBuf[i].Event.KeyEvent.uChar.AsciiChar == '\b') { // delete last character
							c.pop_back();
							cout << ' ' << '\b';
						}
						else {
							c += irInBuf[i].Event.KeyEvent.uChar.AsciiChar; // input new character
						}
						
					}
				}
				
			}
			//else if (irInBuf[i].EventType == MENU_EVENT) {
			//	cout << irInBuf[i].Event.MenuEvent.dwCommandId;
			//}
				
			/*
			case MOUSE_EVENT: // mouse input
				#ifndef MOUSE_HWHEELED
				#define MOUSE_HWHEELED 0x0008
				#endif
				printf("Mouse event: ");

				switch (irInBuf[i].Event.MouseEvent.dwEventFlags)
				{
				case 0:

					if (irInBuf[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
					{
						printf("left button press \n");
					}
					else if (irInBuf[i].Event.MouseEvent.dwButtonState == RIGHTMOST_BUTTON_PRESSED)
					{
						printf("right button press \n");
					}
					else
					{
						printf("button press\n");
					}
					break;
				case DOUBLE_CLICK:
					printf("double click\n");
					break;
				case MOUSE_HWHEELED:
					printf("horizontal mouse wheel\n");
					break;
				case MOUSE_MOVED:
					printf("mouse moved\n");
					break;
				case MOUSE_WHEELED:
					printf("vertical mouse wheel\n");
					break;
				default:
					printf("unknown\n");
					break;
				}
				break;

			case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
				printf("Resize event\n");
				printf("Console screen buffer is %d columns by %d rows.\n", irInBuf[i].Event.WindowBufferSizeEvent.dwSize.X, irInBuf[i].Event.WindowBufferSizeEvent.dwSize.Y);
				break;

			case FOCUS_EVENT:  // disregard focus events

			case MENU_EVENT:   // disregard menu events
				break;

			default:
				SetConsoleMode(hStdin, fdwSaveOldMode);
				ExitProcess(0);
				break;
			}
			*/
		}
	}

		
}




void read_cmd_structure(const filesystem::path filepath, CMD_STRUCTURE& cmds)
{
	
	
	enum class PARSE_MODE {CMD_LINE, OA_LINE};
	ifstream file(filepath);

	PARSE_MODE mode = PARSE_MODE::CMD_LINE;

	CMD command;
	list<PA> poarg;
	unordered_map<OA, list<OA>> oparg;

	string line, arg;
	while (getline(file, line))
	{
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

void parse_cmd(const string& input, const CMD_STRUCTURE& cmd_structure, const CMD_NAMES& cmd_names, AUTOCOMPLETE& auto_comp, string& auto_sug) {
	enum class CMD_STATE {CMD, PA, OA, OAOA};
	CMD_STATE state = CMD_STATE::CMD;

	// read whole input into vector
	list<string> input_words{};
	istringstream iss(input);
	string word;
	while (iss >> word) {
		input_words.push_back(word);
	}

	// split into command parts, CMD state or PA state
	auto_sug = {};
	if (input_words.size() == 1) {// command state
	
		if (!cmd_names.cmd_names.left.count(input_words.front()) == 1) { // command not completely typed
			// find command suggestions
			auto_comp.cmd_names.findAutoSuggestion(input_words.front(), auto_sug);
			return;
		}

	
	} else { // further states
		if (cmd_names.cmd_names.left.count(input_words.front()) == 0) { // check whether command has typo
			return;
		}
	}

	const CMD command = cmd_names.cmd_names.left.at(input_words.front());
	input_words.pop_front();
		
	list<PA> pa_args = cmd_structure.at(command).first;
	if (pa_args.size() > 0) {

		// suggest first pa if first pa is tags or mode name (not if date)
		if (input_words.size() == 0) {
			if (pa_args.front() == PA::TAGS) {
				auto_comp.tags.findAutoSuggestion("", auto_sug);
				return;
			}
			else if (pa_args.front() == PA::MODE_NAME) {
				auto_comp.mode_names.findAutoSuggestion("", auto_sug);
				return;
			}
			return;
			
		}
		else {
			bool skip_pa = false;
			int count = 0;
			for (const auto& pa_arg : pa_args) {
				if (count - 1 > (int)input_words.size()) { // -1 because tags as pa can be empty
					return; // to little input words
				}
				if (input_words.back().starts_with('-')) { //is a oa, skip pa
					skip_pa = true;
					break;
				}
				if (pa_arg == PA::TAGS && (input_words.size()) >= count) { // if previous pa where set (1 arg per word), tags can be suggested
					if (input.back() == ' ') {
						auto_comp.tags.findAutoSuggestion("", auto_sug);
					}
					else {
						auto_comp.tags.findAutoSuggestion(input_words.back(), auto_sug);
					}
					return;
				}
				else if (pa_arg == PA::MODE_NAME && (count == input_words.size() || count == input_words.size() - 1) ) { // if pa of current input count (or next) is mode_name than suggest
					auto_comp.mode_names.findAutoSuggestion(input_words.back(), auto_sug);
					return;
				}

				count += 1;
			}
			if (!skip_pa)
				return; // cant suggest
		}
		
	}
	
	unordered_map<OA, list<OA>> oa_args = cmd_structure.at(command).second;
	list<string> oa_strs;
	for (const auto& [oa_arg, _] : oa_args) {
		oa_strs.push_back(cmd_names.oa_names.right.at(oa_arg));
	}

	// find end of positional arguments by searching for first optional argument
	auto oa_start_pos = find_first_of(input_words.begin(), input_words.end(), oa_strs.begin(), oa_strs.end());

	// did not find a single oa, suggest oa
	if (oa_start_pos == input_words.end()) {
		TrieTree oa_trietree = TrieTree(oa_strs);

		// complete last input word
		oa_trietree.findAutoSuggestion(input_words.back(), auto_sug);

		if (auto_sug.empty()) { // no input word at all, present all optional options
			oa_trietree.findAutoSuggestion("", auto_sug);
		}
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
				active_oa = cmd_names.oa_names.left.at(*it);
				active_oa_words_pos = it;
				oa_strs.erase(oa_found); // delete oa name from the list of available names
				oa_args.erase(active_oa); // delete from the list of oa in the cmd structure
			}
		}

		// delete handled with oa
		input_words.erase(input_words.begin(), active_oa_words_pos);
		
		if (oa_args[active_oa].size() == 0) { // last complete oa has no follow up parameter, suggest next oa
			TrieTree oa_trietree = TrieTree(oa_strs);
			if (input.back() == ' ') {
				oa_trietree.findAutoSuggestion("", auto_sug);
			}
			else {
				oa_trietree.findAutoSuggestion(input_words.back(), auto_sug);
			}
			return;
		}
		else if (oa_args[active_oa].size() == 1) { // only suggest one oaoa tag if that one is tags
			if (oa_args[active_oa].front() == OA::TAGS) {
				if (input.back() == ' ') {
					auto_comp.tags.findAutoSuggestion("", auto_sug);
				}
				else {
					auto_comp.tags.findAutoSuggestion(input_words.back(), auto_sug);
				}
				return;
			}
		}
		else { // multiple options for oa parameter, show all
			list<string> oaoa_strs;
			for (const auto& oa_arg : oa_args[active_oa]) {
				oaoa_strs.push_back(cmd_names.oa_names.right.at(oa_arg));
			}

			TrieTree oa_trietree = TrieTree(oaoa_strs);
			if (input.back() == ' ') {
				oa_trietree.findAutoSuggestion("", auto_sug);
			}
			else {
				oa_trietree.findAutoSuggestion(input_words.back(), auto_sug);
			}
			return;
		}
		
	}
	
}

void read_mode_names(Config& conf, std::list<std::string>& mode_names)
{
	int num_modes = 0;
	conf.get("NUM_MODES", num_modes);

	string mode_name;
	for (auto i = 0; i < num_modes; i++) {
		conf.get("MODE_" + to_string(i) + "_NAME", mode_name);
		mode_names.push_back(mode_name);
	}
}


AUTOCOMPLETE::AUTOCOMPLETE(const CMD_NAMES& cmd_names, const std::list<std::string>& tags, const std::list<std::string>& mode_names)
{
	this->tags = TrieTree(tags);
	this->mode_names = TrieTree(mode_names);
	this->cmd_names = TrieTree();
	for (const auto& [name, cmd] : cmd_names.cmd_names) {
		this->cmd_names.insert(name);
	}
}

AUTOCOMPLETE::AUTOCOMPLETE(const CMD_NAMES& cmd_names, const std::map<std::string, int>& tag_count, const std::unordered_map<int, std::string>& mode_names)
{
	this->mode_names = TrieTree();
	for (const auto& [_, mode_name] : mode_names) {
		this->mode_names.insert(mode_name);
	}
	this->cmd_names = TrieTree();
	for (const auto& [name, cmd] : cmd_names.cmd_names) {
		this->cmd_names.insert(name);
	}
	this->tags = TrieTree();
	for (const auto& [tag, _] : tag_count) {
		this->tags.insert(tag);
	}
}
