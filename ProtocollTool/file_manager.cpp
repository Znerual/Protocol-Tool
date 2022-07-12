#include "file_manager.h"

#include <boost/algorithm/string.hpp>

using namespace std;

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


void open_file(Log& logger, const PATHS paths, const filesystem::path file, vector<jthread>& open_files, HANDLE& hExit) {

	if (file.extension().string() == ".md") {
		open_files.push_back(jthread(RunExternalProgram, ref(logger), paths.md_exe, file, ref(hExit)));
	}
	else if (file.extension().string() == ".pdf") {
		open_files.push_back(jthread(RunExternalProgram, ref(logger), paths.pdf_exe, file, ref(hExit)));
	}
	else if (file.extension().string() == ".tex") {
		open_files.push_back(jthread(RunExternalProgram, ref(logger), paths.tex_exe, file, ref(hExit)));
	}
	else if (file.extension().string() == ".html") {
		open_files.push_back(jthread(RunExternalProgram, ref(logger), paths.html_exe, file, ref(hExit)));
	}
	else if (file.extension().string() == ".docx") {
		open_files.push_back(jthread(RunExternalProgram, ref(logger), paths.docx_exe, file, ref(hExit)));
	}
	else {
		logger << "Can't open file because the extension is not known." << endl;
	}

}


int convert_document_to(const std::string& format, const std::string& ending, const PATHS& paths, const std::string& filename, const std::string& output_filename)
{
	const string command_html = "pandoc -f markdown " + (paths.base_path / paths.tmp_path / filesystem::path(filename)).string() + " -t " + format + " -o " + (paths.base_path / paths.tmp_path / filesystem::path(output_filename + "." + ending)).string();
	FILE* pipe = _popen(command_html.c_str(), "r");
	int returnCode = _pclose(pipe);
	return returnCode;
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
			file << tags.at(i - 1) << '\n' << "! ";
		}
		else {
			file << tags.at(i - 1) << ", ";
		}
	}
	file << "TAGS END" << endl;


	file.close();
}