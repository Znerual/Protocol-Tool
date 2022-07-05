#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>
#include <shlwapi.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <array>
#include "utils.h"
#include "conversions.h"
#include "Config.h"

using namespace std;

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

map<string, time_t> list_all_files(const string& base_path, const string& file_path)
{
	/*
	Takes the path to the directory containing the files and returns a map of all files in that directory with their
	path as key and corresponding last access times in the time_t format as value.
	*/
	map<string, time_t> file_map;

	for (const auto& entry : filesystem::directory_iterator(filesystem::path(base_path) / filesystem::path(file_path))) 
	{
		file_map[(filesystem::path(file_path) / entry.path().filename()).string()] = chrono::system_clock::to_time_t(clock_cast<chrono::system_clock>(entry.last_write_time()));
	}

	return file_map;
}

map<string, vector<string>> list_all_tags(const string& base_path, const string& file_path) {
	/*
	Reads all files in the given path and creates the tag map
	*/
	map<string, vector<string>> tag_map;

	for (const auto& entry : filesystem::directory_iterator(filesystem::path(base_path) / filesystem::path(file_path)))
	{
		tag_map[(filesystem::path(file_path) / entry.path().filename()).string()] = read_tags(entry.path().string());
	}
	return tag_map;
}

vector<string> read_tags(const string& path) {
	vector<string> tags = vector<string>();

	ifstream file(path);
	if (!file.is_open()) {
		cout << "File " << path << " could not be openend." << '\n';
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

void update_tags(const string& base_path, const string& file_path, map<string, time_t>& file_map, map<string, vector<string>>& tag_map, map<string, int>& tag_count, vector<string>& filter_selection)
{
	/*
	Takes an existing file_map (return of list_all_files) and uses it to only updates the tags of the modified files 
	*/
	map<string, time_t> new_file_map = list_all_files(base_path, file_path);
	map<string, vector<string>> new_tag_map;
	vector<string> new_filter_selection;
	new_filter_selection.reserve(filter_selection.size());
	for (auto const& [path, time] : new_file_map)
	{
		if (time > file_map[path]) // file was changed
		{
			new_tag_map[path] = read_tags(path);
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
	tag_count = get_tag_count(tag_map);
}


string get_filename(const string& base_path, const string& file_path, time_t date, vector<string> tags, const string& file_ending)
{
	
	filesystem::path dir = filesystem::path(base_path) / filesystem::path(file_path);
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

void write_file(const string& base_path, const string& file_path, const string& filename, time_t date, vector<string> tags, const string& file_ending, const string& data_path)
{
	filesystem::path file_dir = filesystem::path(base_path) / filesystem::path(file_path);
	filesystem::path data_dir = filesystem::path(base_path) / filesystem::path(data_path);

	// create data folder
	filesystem::create_directories(data_dir / filesystem::path(filename).stem());
	
	// create file
	ofstream file(file_dir / filesystem::path(filename));
	if (!file.is_open()) {
		cout << "Error opening file " << file_dir / filename << endl;
		return;
	}

	
	string date_str;
	date2str(date_str, date);
	cout << "Created file " << filename << " at the path " << file_path << " and its corresponding data folder at " << (data_path / filesystem::path(filename).stem()).string() << ".\n";
	file << "! FILENAME " << (filesystem::path(file_path) / filesystem::path(filename)).string() << '\n';
	file << "! DATAFOLDER " << (data_path / filesystem::path(filename).stem()).string() << '\n';
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

void write_file(const string& base_path, const string& file_path, const string& filename,  time_t date, vector<string> tags, const string& file_ending)
{
	filesystem::path dir = filesystem::path(base_path) / filesystem::path(file_path);
	ofstream file(dir / filesystem::path(filename));
	if (!file.is_open()) {
		cout << "Error opening file " << dir / filename << endl;
		return;
	}

	string date_str;
	date2str(date_str, date);
	cout << "Created file " << filename << " at the path " << file_path << ".\n";
	file << "! FILENAME " << (filesystem::path(file_path) / filesystem::path(filename)).string() << '\n';
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




int convert_document_to(const std::string& format, const std::string& ending, const filesystem::path& base_path, const filesystem::path& file_path, const std::string& filename, const std::string& output_filename)
{
	const string command_html = "pandoc -f markdown " + (base_path / file_path / filesystem::path(filename)).string() + " -t " + format + " -o " + (base_path / file_path / filesystem::path(output_filename + "." + ending)).string();
	FILE* pipe = _popen(command_html.c_str(), "r");
	int returnCode = _pclose(pipe);
	return returnCode;
}


void parse_show_args(std::istringstream& iss, OPEN_MODE default_open, bool& show_tags, bool& show_metadata, bool& show_table_of_content, bool& show_data, bool& open_image_data, bool& hide_date, bool& open_as_html, bool& open_as_tex, bool& open_as_pdf, bool& open_as_docx, bool& open_as_markdown)
{
	show_tags = false;
	show_metadata = false;
	show_table_of_content = false;
	show_data = false;
	open_image_data = false;
	hide_date = false;
	open_as_html = false;
	open_as_tex = false;
	open_as_pdf = false;
	open_as_docx = false;
	open_as_markdown = false;

	string argument;
	while (iss >> argument)
	{
		if (argument == "t" || argument == "-t" || argument == "-tags" || argument == "-show_tags")
		{
			show_tags = true;
		}
		else if (argument == "m" || argument == "-m" || argument == "-metadata" || argument == "-show_metadata")
		{
			show_metadata = true;
		}
		else if (argument == "c" || argument == "-c" || argument == "-table_of_content" || argument == "-show_table_of_content")
		{
			if (hide_date)
			{
				cout << "Can't create a table of content when hide_date is set." << endl;
				continue;
			}

			show_table_of_content = true;
		}
		else if (argument == "d" || argument == "-d" || argument == "-data" || argument == "-show_data")
		{
			show_data = true;
		}
		else if (argument == "hd" || argument == "-hd" || argument == "-date" || argument == "-hide_date")
		{
			hide_date = true;
			if (show_table_of_content)
			{
				cout << "Can't create a table of content when hide_date is set." << endl;
				show_table_of_content = false;
			}
		}
		else if (argument == "i" || argument == "-i" || argument == "-images" || argument == "-open_images")
		{
			show_data = true;
			open_image_data = true;
		}
		else if (argument == "html" || argument == "-html" ||  argument == "-open_as_html")
		{
			open_as_html = true;
		}
		else if (argument == "tex" || argument == "-tex" || argument == "-open_as_tex")
		{
			open_as_tex = true;
		}
		else if (argument == "pdf" || argument == "-pdf" || argument == "-open_as_pdf")
		{
			open_as_pdf = true;
		}
		else if (argument == "docx" || argument == "-docx"  || argument == "-open_as_docx")
		{
			open_as_docx = true;
		}
		else if (argument == "md" || argument == "-md" ||  argument == "-open_as_markdown")
		{
			open_as_markdown = true;
		}
		else {
			cout << "Input argument " << argument;
			cout << " could not be parsed. Expected one of the following inputs:\n-show_(t)ags, show_(m)etadata, -show_table_of_(c)ontent, ";
			cout << "-show_(d)ata, -(h)ide_(d)ate, -open_(i)mage_data, -open_as_(html), -open_as_(tex), -open_as(pdf) ";
			cout << "-open_as_(docx), -open_as_(m)ark(d)own" << endl;
			return;
		}
	}
	if (!open_as_docx && !open_as_html && !open_as_markdown && !open_as_pdf && !open_as_tex)
	{
		switch (default_open)
		{
		case HTML:
			open_as_html = true;
			break;
		case MARKDOWN:
			open_as_markdown = true;
			break;
		case LATEX:
			open_as_tex = true;
			break;
		case PDF:
			open_as_pdf = true;
			break;
		case DOCX:
			open_as_docx = true;
			break;
		default:
			cout << "Error: Unknown default open mode " << default_open << endl;
			break;
		}
	}
}

void parse_find_args(istringstream& iss, bool& data_only, vector<time_t>& date_args, vector<string>& ctags_args, vector<string>& catags_args, vector<string>& ntags_args, string& regex, vector<char>& version_args) {
	
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
				cout << "The argument " << argument << " was not recognized. It should be one of the following options: \n";
				cout << "-d, -date: finds nodes between start and end date. \n";
				cout << "-ct, -contains_tags: selects nodes containing at least one of the specified tags.\n";
				cout << "-cat, -contains_all_tags: selectes nodes containing all of the specified tags.\n";
				cout << "-nt, -no_tags: selects nodes that do not contain the specified tags.\n";
				cout << "-rt, -reg_text: uses regular expressions on the node content to select nodes that match the given pattern.\n";
				cout << "-do, -data_only: only select nodes that have data associated with them." << endl;
				return;
			}
			switch (mode)
			{
			case NONE:
				cout << "Could not parse the find command " << argument << ". It should be one of the following: \n";
				cout << "-d, -date: finds nodes between start and end date. \n";
				cout << "-ct, -contains_tags: selects nodes containing at least one of the specified tags.\n";
				cout << "-cat, -contains_all_tags: selectes nodes containing all of the specified tags.\n";
				cout << "-nt, -no_tags: selects nodes that do not contain the specified tags.\n";
				cout << "-rt, -reg_text: uses regular expressions on the node content to select nodes that match the given pattern.\n";
				cout << "-do, -data_only: only select nodes that have data associated with them." << endl;
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
			default:
				cout << "Something went wrong. Unexpected mode in find with argument " << argument << endl;
				return;
			}
		}
	}

	
	// parse date input
	time_t start_date, end_date;
	CONV_ERROR ret1, ret2;
	int delim_pos;
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
				cout << "The date " << _date_args.at(0) << " could not be parsed to start_date and end_date, because the separator (-) could not be found and the input is not a date." << endl;
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
			cout << "The start date " << start_date << " could not be parsed to a date." << endl;
			break;
		}
		else if (ret2 != CONV_SUCCESS) {
			cout << "The end date " << end_date << " could not be parsed to a date." << endl;
			break;
		}

		date_args.push_back(start_date);
		date_args.push_back(end_date);
		break;
	case 3:
		ret1 = str2date(start_date, _date_args.at(0).c_str());
		ret2 = str2date(end_date, _date_args.at(2).c_str());
		if (ret1 != CONV_SUCCESS) {
			cout << "The start date " << start_date << " could not be parsed to a date." << endl;
			break;
		}
		else if (ret2 != CONV_SUCCESS) {
			cout << "The end date " << end_date << " could not be parsed to a date." << endl;
			break;
		}

		date_args.push_back(start_date);
		date_args.push_back(end_date);
		break;
	default:
		cout << "Unexpected white spaces in the date parameter. Found " << _date_args.size() << " whitespaces and expects zero or two." << endl;
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

		cout << "Could not parse the version input " << vers_arg << ". It should either be a character, number or range in the form a-c or a:c." << endl;
		return;
	}
}

void parse_create_mode(std::istringstream& iss, Config& conf, string& mode_name, std::vector<std::string>& mode_tags, OPEN_MODE& open_mode)
{
	if (iss)
	{
		iss >> mode_name;
	}
	else {
		return;
	}
	

	bool read_open_mode = false;
	string argument;
	while (iss >> argument)
	{
		boost::to_lower(argument);
		if (argument == "-o" || argument == "-open_as") {
			read_open_mode = true;
		}
		else if (read_open_mode) {
			if (argument == "html")
			{
				open_mode = HTML;
			}
			else if (argument == "docx") {
				open_mode == DOCX;
			}
			else if (argument == "tex" || argument == "latex") {
				open_mode = LATEX;
			}
			else if (argument == "pdf") {
				open_mode = PDF;
			}
			else if (argument == "md" || argument == "markdown") {
				open_mode = MARKDOWN;
			}
			else {
				cout << "Open as format " << argument << " was not recognized. Available options are:\n html, docx, tex, pdf, markdown." << endl;
				int open_mode_int;
				conf.get("DEFAULT_OPEN_MODE", open_mode_int);
				open_mode = static_cast<OPEN_MODE>(open_mode_int);
			}
			read_open_mode = false;
		}
		else {
			mode_tags.push_back(argument);
		}
	}
}

void parse_details_args(istringstream& iss, bool& show_path, bool& show_long_path, bool& show_tags, bool& show_last_modified, bool& show_content)
{
	show_path = false;
	show_long_path = false;
	show_tags = false;
	show_last_modified = false;
	show_content = false;

	string argument;
	while (iss >> argument)
	{
		if (argument == "t" || argument == "-t" || argument == "-tags" || argument == "-show_tags")
		{
			show_tags = true;
		}
		else if (argument == "p" || argument == "-p" || argument == "-path" || argument == "-show_path")
		{
			show_path = true;
		}
		else if (argument == "lp" || argument == "-lp" || argument == "-long_path" || argument == "-show_long_path")
		{
			show_long_path = true;
		}
		else if (argument == "m" || argument == "-m" || argument == "-last_modified" || argument == "-show_last_modified")
		{
			show_last_modified = true;
		}
		else if (argument == "c" || argument == "-c" || argument == "-content" || argument == "-show_content")
		{
			show_content = true;
		}
		else {
			cout << "Input argument " << argument;
			cout << " could not be parsed. Expected one of the following inputs:\n-show_(t)ags, show_(p)ath, -show_(l)ong_(p)ath, ";
			cout << "-show_last_(m)odified, -show_(c)ontent" << endl;
			return;
		}
	}
}

void parse_add_note(std::istringstream& iss, const filesystem::path& base_path, const filesystem::path& file_path, const std::string& file_ending, std::string& filename, time_t& date_t, std::vector<std::string>& tags, bool& add_data)
{
	add_data = false;

	string date_str;
	CONV_ERROR ret;
	iss >> date_str;
	ret = str2date(date_t, date_str);
	if (ret != CONV_ERROR::CONV_SUCCESS) {
		cout << "First argument of the new command has to be the date: ((t)oday, (y)esterday, dd, dd.mm, dd.mm.yyyy), set date to today" << endl;
	}
	else {
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

	filename = get_filename(base_path.string(), file_path.string(), date_t, tags, file_ending);
}


void show_filtered_notes(std::istringstream& iss, const OPEN_MODE default_open, const filesystem::path& base_path, const filesystem::path& file_path, const filesystem::path& data_path, const filesystem::path& tmp_path, const string& tmp_filename, std::vector<std::string>& filter_selection, const bool& has_pandoc)
{
	// parse arguments
	bool show_tags;
	bool show_metadata;
	bool show_table_of_content;
	bool show_data;
	bool open_image_data;
	bool hide_date;
	bool open_as_html;
	bool open_as_tex;
	bool open_as_pdf;
	bool open_as_docx;
	bool open_as_markdown;
	parse_show_args(iss, default_open, show_tags, show_metadata, show_table_of_content, show_data, open_image_data, hide_date, open_as_html, open_as_tex, open_as_pdf, open_as_docx, open_as_markdown);

	// write content of selected files to output file
	ofstream show_file(base_path / tmp_path / filesystem::path(tmp_filename));
	string date_str;
	time_t date_t;
	CONV_ERROR ret;
	string line;
	if (show_table_of_content && !hide_date)
	{
		show_file << "# Table of Contents" << '\n';

		int i = 1;
		for (const string& path : filter_selection)
		{
			ret = str2date_short(date_t, filesystem::path(path).stem().string().substr(0, 8));
			if (ret != CONV_SUCCESS) {
				cout << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}
			ret = date2str(date_str, date_t);
			if (ret != CONV_SUCCESS) {
				cout << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}

			show_file << i++ << ". [Node from " << date_str << " Version " << filesystem::path(path).stem().string()[8] << "](#" << filesystem::path(path).stem().string() << ")\n";
			ifstream file(base_path / filesystem::path(path));
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
		if (!hide_date) {
			ret = str2date_short(date_t, filesystem::path(path).stem().string().substr(0, 8));
			if (ret != CONV_SUCCESS) {
				cout << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}
			ret = date2str(date_str, date_t);
			if (ret != CONV_SUCCESS) {
				cout << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}

			show_file << "# " << date_str << " Version " << filesystem::path(path).stem().string()[8] << "<a id=\"" << filesystem::path(path).stem().string() << "\"></a>\n";
		}

		ifstream file(base_path / file_path / filesystem::path(path).filename());
		if (!file.is_open()) {
			cout << "Error opening note file " << (base_path / file_path / filesystem::path(path).filename()).string() << endl;
		}
		vector<string> tags;
		bool reading_tags = false;
		int line_count = 0;
		while (getline(file, line))
		{
			// show everything
			if (show_metadata) {
				show_file << line;
				if (line[0] == '#')
				{
					show_file << "<a id=\"" << line_count << "\"></a>";
				}
				show_file << '\n';
			}

			// show tags
			else if (show_tags && !reading_tags)
			{
				if (line == "! TAGS START")
				{
					reading_tags = true;
				}
			}
			else if (show_tags && reading_tags)
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
	if (show_data)
	{
		for (const string& path : filter_selection)
		{
			const wstring datapath = (base_path / data_path / filesystem::path(path).stem()).wstring();
			ShellExecute(NULL, L"open", datapath.c_str(), NULL, NULL, SW_SHOWNORMAL);
			if (open_image_data)
			{
				// search for image data in the folder
				for (const auto& entry : filesystem::directory_iterator(base_path / data_path / filesystem::path(path).stem()))
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

		if (open_as_html)
		{
			if (int returnCode = convert_document_to("html", "html", base_path, tmp_path, tmp_filename, "show") != 0) {
				cout << "Error " << returnCode << " while converting markdown to html using pandoc" << endl;
			}
			else {
				ShellExecute(NULL, L"open", (base_path / tmp_path / filesystem::path("show.html")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		}

		if (open_as_markdown)
		{

			ShellExecute(NULL, L"open", (base_path / tmp_path / filesystem::path("show.md")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);

		}

		if (open_as_docx)
		{
			if (int returnCode = convert_document_to("docx", "docx", base_path, tmp_path, tmp_filename, "show") != 0) {
				cout << "Error " << returnCode << " while converting markdown to docx using pandoc" << endl;
			}
			else {
				ShellExecute(NULL, L"open", (base_path / tmp_path / filesystem::path("show.docx")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		}

		if (open_as_tex)
		{
			if (int returnCode = convert_document_to("latex", "tex", base_path, tmp_path, tmp_filename, "show") != 0) {
				cout << "Error " << returnCode << " while converting markdown to latex (tex) using pandoc" << endl;
			}
			else {
				ShellExecute(NULL, L"open", (base_path / tmp_path / filesystem::path("show.tex")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		}

		if (open_as_pdf)
		{
			if (int returnCode = convert_document_to("pdf", "pdf", base_path, tmp_path, tmp_filename, "show") != 0) {
				cout << "Error " << returnCode << " while converting markdown to pdf using pandoc" << endl;
			}
			else {
				ShellExecute(NULL, L"open", (base_path / tmp_path / filesystem::path("show.pdf")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		}

	}
	else {
		ShellExecute(NULL, L"open", (base_path / tmp_path / filesystem::path(tmp_filename)).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
	}





}

void create_mode(std::istringstream& iss, Config& conf, int& num_modes, unordered_map<int, string>& mode_names, vector<string>& mode_tags, int& active_mode, OPEN_MODE& open_mode)
{
	string mode_name;
	mode_tags = vector<string>();
	parse_create_mode(iss, conf, mode_name, mode_tags, open_mode);
	boost::algorithm::to_lower(mode_name);

	for (const auto& [id, name] : mode_names) // check if name already exists
	{
		if (mode_name == name)
		{
			cout << "Mode already exists! Delete the old mode with the command (del)ete before creating a new one" << endl;
			return;
		}
	}

	active_mode = mode_names.size(); // set id to next free number
	mode_names[active_mode] = mode_name;

	num_modes += 1;
	conf.set("NUM_MODES", num_modes);
	conf.set("MODE_" + to_string(active_mode) + "_NAME", mode_name);
	conf.set("MODE_" + to_string(active_mode) + "_OPEN_AS", static_cast<int>(open_mode));
	conf.set("MODE_" + to_string(active_mode) + "_NUM_TAGS", static_cast<int>(mode_tags.size()));

	for (auto i = 0; i < mode_tags.size(); i++)
	{
		conf.set("MODE_" + to_string(active_mode) + "_TAG_" + to_string(i), mode_tags.at(i));
	}

}

void delete_mode(std::istringstream& iss, Config& conf, int& num_modes, unordered_map<int, string>& mode_names, vector<string>& mode_tags, int& active_mode, OPEN_MODE& open_mode)
{
	string mode_name;
	mode_tags = vector<string>();
	active_mode = -1; // set id to next free number

	int open_mode_int;
	conf.get("DEFAULT_OPEN_MODE", open_mode_int);
	open_mode = static_cast<OPEN_MODE>(open_mode_int);

	if (!iss)
	{
		cout << "Specify name of the mode that should be deleted" << endl;
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
		cout << "Mode " << mode_name << " not found." << endl;
		return;
	}
	

	num_modes -= 1;
	conf.set("NUM_MODES", num_modes);

	int num_tags;
	conf.get("MODE_" + to_string(mode_id) + "_NUM_TAGS", num_tags);
	for (auto i = 0; i < num_tags; i++)
	{
		conf.remove("MODE_" + to_string(mode_id) + "_TAG_" + to_string(i));
	}

	conf.remove("MODE_" + to_string(mode_id) + "_NAME");

}

void filter_notes(istringstream& iss, const string& base_path, const string& file_path, const string& data_path, vector<string>& filter_selection, map<string, time_t>& file_map, map<string, vector<string>> tag_map, vector<string> mode_tags)
{

	vector<string> _filter_selection;

	bool data_only = false;
	vector<time_t> date_args;
	vector<string> ctags_args;
	vector<string> catags_args;
	vector<string> ntags_args;
	vector<char> vers_args;
	string regex{};

	parse_find_args(iss, data_only, date_args, ctags_args, catags_args, ntags_args, regex, vers_args);
	catags_args.insert(catags_args.begin(), mode_tags.begin(), mode_tags.end());

	for (const auto& path : filter_selection)
	{
		// filter for date
		if (date_args.size() > 0) 
		{
			time_t node_date;
			str2date_short(node_date, filesystem::path(path).stem().string());
			if (node_date < date_args.at(0) || node_date > date_args.at(1)) {
				continue;
			}
		}

		// filter for version
		if (vers_args.size() > 0)
		{
			char node_version = filesystem::path(path).stem().string()[8];
			if (find(vers_args.begin(), vers_args.end(), node_version) == vers_args.end())
			{
				continue;
			}

		}

		// filter for having a data folder
		if (data_only) 
		{
			if (!filesystem::exists(filesystem::path(base_path) / filesystem::path(data_path) / filesystem::path(path).filename())) {
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

		// filter for tags that can not be in the tag list of the node
		if (ntags_args.size() > 0)
		{
			bool has_tag = false;
			vector<string> tags = tag_map[path];
			for (auto& tag : ntags_args)
			{
				tag = trim(tag);
				boost::algorithm::to_lower(tag);
				if (find(tags.begin(), tags.end(), tag) != tags.end()) {
					has_tag = true;
					break;
				}
			}

			if (has_tag)
			{
				continue;
			}
		}

		// filtering for regex in the node text
		if (!regex.empty())
		{
			cout << "Regex in find needs to be implemented" << endl;
		}
		

		_filter_selection.push_back(path);
	}
	filter_selection = _filter_selection;
}

void find_notes(istringstream& iss, const string& base_path, const string& file_path, const string& data_path, vector<string>& filter_selection, map<string, time_t>& file_map, map<string, vector<string>> tag_map, vector<string> mode_tags)
{
	filter_selection = vector<string>();
	filter_selection.reserve(tag_map.size());

	for (const auto& [path, tag] : tag_map)
	{
		filter_selection.push_back(path);
	}

	filter_notes(iss, base_path, file_path, data_path, filter_selection, file_map, tag_map, mode_tags);
}

void add_note(std::istringstream& iss, const std::filesystem::path& base_path, const std::filesystem::path& file_path, const std::filesystem::path& data_path, const std::string& file_ending, std::map<std::string, time_t> file_map, std::map<std::string, std::vector<std::string>> tag_map, vector<string> mode_tags)
{
	string filename;
	vector<string> tags;
	bool add_data;
	time_t date;
	parse_add_note(iss, base_path, file_path, file_ending, filename, date, tags, add_data);
	tags.insert(tags.end(), mode_tags.begin(), mode_tags.end());

	// TODO: simplify to one function call write_file
	if (add_data) {
		write_file(base_path.string(), file_path.string(), filename, date, tags, file_ending, data_path.string());
	}
	else {
		write_file(base_path.string(), file_path.string(), filename, date, tags, file_ending);
	}

	// open new file
	const wstring exec_filename = (base_path / file_path / filesystem::path(filename)).wstring();
	ShellExecute(0, L"open", exec_filename.c_str(), 0, 0, SW_SHOW);

	// update file_map, tag_map, tag_count and filter selection
	string path = (file_path / filesystem::path(filename)).string();
	file_map[path] = time_t(NULL);
	tag_map[path] = tags;
	get_tag_count(tag_map);
}

void add_data(std::istringstream& iss, const std::filesystem::path& base_path, const std::filesystem::path& data_path, std::vector<std::string> filter_selection)
{
	filesystem::path data_dir = filesystem::path(base_path) / filesystem::path(data_path);
	if (filter_selection.size() > 1)
	{
		string choice;
		cout << filter_selection.size() << " found in the selection. Add data folders for all of them? (y/n)" << endl;
		cin >> choice;
		cin.clear();
		cin.ignore(10000, '\n');

		if (choice == "y" || choice == "Y") {

			for (const string& path : filter_selection)
			{
				filesystem::create_directories(data_dir / filesystem::path(path).stem());
				cout << "Added data directory for " << filesystem::path(path).stem() << endl;
			}
		}
	}
	else if (filter_selection.size() == 1)
	{
		filesystem::create_directories(data_dir / filesystem::path(filter_selection.at(0)).stem());
		cout << "Added data directory for " << filesystem::path(filter_selection.at(0)).stem() << endl;
	}
	else {
		cout << "No file found in the selection. Use the find command to specify which note should have a data folder added." << endl;
		return;
	}
}

void show_details(std::istringstream& iss, const std::filesystem::path& base_path, const std::filesystem::path& file_path, const std::filesystem::path& data_path, vector<string> filter_selection, std::map<std::string, time_t> file_map, std::map<std::string, std::vector<std::string>> tag_map)
{
	bool show_path;
	bool show_long_path;
	bool show_tags;
	bool show_last_modified;
	bool show_content;
	parse_details_args(iss, show_path, show_long_path, show_tags, show_last_modified, show_content);

	time_t node_date;
	string date_str, modified_date_str;
	for (const string& path : filter_selection)
	{
		string filename = filesystem::path(path).stem().string();
		str2date_short(node_date, filename.substr(0, 8));
		date2str(date_str, node_date);
		cout << date_str;
		if (filename[8] != 'a')
		{
			cout << " Version " << filename[8];
		}
		if (show_long_path)
		{
			cout << " at " << base_path / filesystem::path(path);
		}
		else if (show_path)
		{
			cout << " at " << path;
		}
		if (show_tags)
		{
			cout << " with tags: ";
			for (const auto& tag : tag_map[path])
			{
				cout << tag << " ";
			}
		}
		if (show_last_modified)
		{
			date2str_long(modified_date_str, file_map[path]);
			cout << " last modified at " << modified_date_str;
		}
		if (show_content)
		{
			ifstream file(base_path / filesystem::path(path));
			string str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
			file.close();

			cout << " with node text:\n" << str;
		}
		cout << endl;
	}
}

void open_selection(const std::filesystem::path& base_path, std::vector<std::string> filter_selection)
{
	if (filter_selection.size() == 1)
	{
		const wstring exec_filename = (base_path / filesystem::path(filter_selection.at(0))).wstring();
		ShellExecute(0, L"open", exec_filename.c_str(), 0, 0, SW_SHOW);
	}
	else if (filter_selection.size() > 1)
	{
		string choice;
		cout << filter_selection.size() << " found in the selection. Open all of them? (y/n)" << endl;
		cin >> choice;
		cin.clear();
		cin.ignore(10000, '\n');

		if (choice == "y" || choice == "Y") {

			for (const string& path : filter_selection)
			{
				const wstring exec_filename = (base_path / filesystem::path(path)).wstring();
				ShellExecute(0, L"open", exec_filename.c_str(), 0, 0, SW_SHOW);
			}
		}
	}
	else {
		cout << "Found no file to open in the current selection. Use (f)ind to set selection." << endl;
	}
}

void activate_mode(std::istringstream& iss, Config& conf, std::unordered_map<int, std::string>& mode_names, int& active_mode, vector<string>& mode_tags, OPEN_MODE& open_mode)
{
	mode_tags = vector<string>();
	active_mode = -1;

	string mode_name;
	if (!iss)
	{
		cout << "Specify the name of the mode you want to activate" << endl;
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
		int open_mode_int;
		conf.get("MODE_" + to_string(active_mode) + "_OPEN_AS", open_mode_int);
		open_mode = static_cast<OPEN_MODE>(open_mode_int);

		int num_tags;
		conf.get("MODE_" + to_string(active_mode) + "_NUM_TAGS", num_tags);
		string tag;
		for (auto i = 0; i < num_tags; i++)
		{
			conf.get("MODE_" + to_string(active_mode) + "_TAG_" + to_string(i), tag);
			mode_tags.push_back(tag);
		}
	}
	else {
		cout << "Mode " << mode_name << " not found." << endl;
	}

}

map<string, int> get_tag_count(map<string, vector<string>>& tag_map)
{
	map<string, int> tag_count;

	for (const auto& [path, tags] : tag_map)
	{
		for (const auto& tag : tags)
		{
			tag_count[tag] += 1;
		}
	}
	return tag_count;
}
