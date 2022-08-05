#include "commands_linux.h"
#include "file_manager_linux.h"


#include <sstream>
#include <fstream>
#include <filesystem>
#include <regex>

#include <boost/algorithm/string.hpp>
using namespace std;

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
			//const wstring datapath = (paths.base_path / paths.data_path / filesystem::path(path).stem()).wstring();
			//ShellExecute(NULL, L"open", datapath.c_str(), NULL, NULL, SW_SHOWNORMAL);
			open_file(logger, paths, paths.base_path / paths.data_path / filesystem::path(path).stem());
			if (show_options.open_image_data)
			{
				// search for image data in the folder
				for (const auto& entry : filesystem::directory_iterator(paths.base_path / paths.data_path / filesystem::path(path).stem()))
				{
					const string ext = entry.path().extension().string();
					if (ext == "jpg" || ext == "png" || ext == "jpeg" || ext == "gif")
					{
						open_file(logger, paths, entry.path());
						//ShellExecute(NULL, L"open", entry.path().wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
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
				open_file(logger, paths, paths.base_path / paths.tmp_path / filesystem::path("show.html"));
				//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.html")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			}
		}

		if (format_options.markdown)
		{

			//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.md")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
			open_file(logger, paths, paths.base_path / paths.tmp_path / filesystem::path("show.md"));
		}

		if (format_options.docx)
		{
			if (int returnCode = convert_document_to("docx", "docx", paths, tmp_filename, "show") != 0) {
				logger << "Error " << returnCode << " while converting markdown to docx using pandoc" << endl;
			}
			else {
				//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.docx")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
				open_file(logger, paths, paths.base_path / paths.tmp_path / filesystem::path("show.docx"));
			}
		}

		if (format_options.tex)
		{
			if (int returnCode = convert_document_to("latex", "tex", paths, tmp_filename, "show") != 0) {
				logger << "Error " << returnCode << " while converting markdown to latex (tex) using pandoc" << endl;
			}
			else {
				//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.tex")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
				open_file(logger, paths, paths.base_path / paths.tmp_path / filesystem::path("show.tex"));
			}
		}

		if (format_options.pdf)
		{
			if (int returnCode = convert_document_to("pdf", "pdf", paths, tmp_filename, "show") != 0) {
				logger << "Error " << returnCode << " while converting markdown to pdf using pandoc" << endl;
			}
			else {
				//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.pdf")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
				open_file(logger, paths, paths.base_path / paths.tmp_path / filesystem::path("show.pdf"));
			}
		}

	}
	else {
		//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path(tmp_filename)).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
		open_file(logger, paths, paths.base_path / paths.tmp_path / filesystem::path(tmp_filename));
	}





}

void create_mode(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, int& num_modes, unordered_map<int, string>& mode_names, vector<string>& mode_tags, int& active_mode, const string& file_ending)
{
	deactivate_mode(logger, conf, active_mode, mode_tags);

	string mode_name;
	mode_tags = vector<string>();
	
	MODE_OPTIONS mode_options;
	parse_create_mode(logger, iss, conf, mode_name, mode_tags, mode_options);

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



	activate_mode(logger, conf, paths, active_mode, mode_tags, file_ending);

}

void delete_mode(Log& logger, std::istringstream& iss, Config& conf, int& num_modes, unordered_map<int, string>& mode_names, vector<string>& mode_tags, int& active_mode)
{
	string mode_name;
	deactivate_mode(logger, conf, active_mode, mode_tags);

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

void edit_mode(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, unordered_map<int, string>& mode_names, vector<string>& mode_tags, int& active_mode, const string& file_ending)
{
	deactivate_mode(logger, conf, active_mode, mode_tags);

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
	
	enum EDIT_MODE { NONE, ADD_OPTIONS, REMOVE_OPTIONS, ADD_TAGS, REMOVE_TAGS, CHANGE_MODE_NAME, ADD_FOLDER_WATCHER, REMOVE_FOLDER_WATCHER };
	EDIT_MODE mode = NONE;
	MODE_OPTIONS add_opt, remove_opt;
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

	}

	if (!new_mode_name.empty()) {
		mode_names[mode_id] = new_mode_name;
		conf.set("MODE_" + to_string(mode_id) + "_NAME", new_mode_name);
	}

	add(mode_options, add_opt);
	remove(mode_options, remove_opt);
	set_mode_options(conf, mode_options, mode_id);
	set_mode_tags(conf, mode_id, mode_tags);

	

	activate_mode(logger, conf, paths, active_mode, mode_tags, file_ending);
}

void show_modes(Log& logger, std::istringstream& iss, Config& conf, std::unordered_map<int, std::string>& mode_names, int& active_mode)
{

	int con_col, con_row;
	get_console_size(con_row, con_col);

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
	open_file(logger, paths, (paths.base_path / paths.file_path / filesystem::path(filename)));
	//open_md_editor(logger, (paths.base_path / paths.file_path / filesystem::path(filename)));
	/*
	const wstring exec_filename = (paths.base_path / paths.file_path / filesystem::path(filename)).wstring();
	ShellExecute(0, L"open", exec_filename.c_str(), 0, 0, SW_SHOW);
	*/
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
		//const wstring exec_filename = (paths.base_path / filesystem::path(filter_selection.at(0))).wstring();
		//ShellExecute(0, L"open", exec_filename.c_str(), 0, 0, SW_SHOW);
		open_file(logger, paths, paths.base_path / filesystem::path(filter_selection.at(0)));
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
				//const wstring exec_filename = (paths.base_path / filesystem::path(path)).wstring();
				//ShellExecute(0, L"open", exec_filename.c_str(), 0, 0, SW_SHOW);
				open_file(logger, paths, paths.base_path / filesystem::path(path));
			}
		}
	}
	else {
		logger << "Found no file to open in the current selection. Use (f)ind to set selection." << endl;
	}
}

void activate_mode(Log& logger, Config& conf, const PATHS& paths, int& active_mode, vector<string>& mode_tags, const string& file_ending)
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

}

void deactivate_mode(Log& logger, Config& conf, int& active_mode, vector<string>& mode_tags) {
	active_mode = -1;
	mode_tags = vector<string>();

}

void activate_mode_command(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, std::unordered_map<int, std::string>& mode_names, int& active_mode, vector<string>& mode_tags,  const string& file_ending)
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
		activate_mode(logger, conf, paths, active_mode, mode_tags, file_ending);
	}
	else {
		logger << "Mode " << mode_name << " not found." << endl;
	}

}
