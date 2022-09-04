#include "file_manager.h"
#include "conversions.h"

#include <regex>
#include <boost/algorithm/string.hpp>

using namespace std;

#ifndef _WIN32
template <typename TP>
std::time_t to_time_t(TP tp)
{
	using namespace std::chrono;
	auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
		+ system_clock::now());
	return system_clock::to_time_t(sctp);
}
#endif
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

#ifdef _WIN32
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
#else
void open_file(Log& logger, const PATHS paths, const filesystem::path file) {

	system(("xdg-open " + file.string()).c_str());

}
#endif

int convert_document_to(const std::string& format, const std::string& ending, const PATHS& paths, const std::string& filename, const std::string& output_filename)
{
	const string command_html = "pandoc -f markdown " + (paths.base_path / paths.tmp_path / filesystem::path(filename)).string() + " -t " + format + " -o " + (paths.base_path / paths.tmp_path / filesystem::path(output_filename + "." + ending)).string();
#ifdef _WIN32
	FILE* pipe = _popen(command_html.c_str(), "r");
	int returnCode = _pclose(pipe);
#else
	FILE* pipe = popen(command_html.c_str(), "r");
	int returnCode = pclose(pipe);
#endif
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
#ifdef _WIN32
		file_map[(paths.file_path / entry.path().filename()).string()] = chrono::system_clock::to_time_t(clock_cast<chrono::system_clock>(entry.last_write_time()));
#else
		file_map[(paths.file_path / entry.path().filename()).string()] = to_time_t(entry.last_write_time());
#endif
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
	bool deprecated = false;
	string line, tag;
	while (getline(file, line))
	{
#ifndef _WIN32
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
#endif
		if (line == "tags:") {
			reading_tags = true;
		}
		else if (line == "! TAGS START") {
			reading_tags = true;
			deprecated = true;
		}
		else if (reading_tags && !line.starts_with("\t- #") && !line.starts_with("    - #")) {
			reading_tags = false;
			break;
		}
		else if (reading_tags && line == "! TAGS END") {
			reading_tags = false;
			break;
		}
		else if (reading_tags && deprecated) {
			line.erase(0, 2); // remove leading !  part
			istringstream ss(line);
			while (ss >> tag) // split at delimiter
			{
				tag = trim(tag);
				tag.erase(0, 1); // remove leading #
				boost::algorithm::to_lower(tag);
				tags.push_back(tag);
			}
		}
		else if (reading_tags) {
			line = trim(line); // remove whitespaces or tab
			line.erase(0, 3); // remove - #
			boost::algorithm::to_lower(line);
			tags.push_back(line);
		}

	}
	file.close();

	return tags;
}

void read_metadata_without_tags(Log& logger, const string& path, map<string, string>& metadata) {
	ifstream file(path);
	if (!file.is_open()) {
		logger << "File " << path << " could not be openend." << '\n';
		return;
	}

	bool reading_tags = false;
	bool deprecated = false, reading_metadata = false;
	string line, tag;
	while (getline(file, line)) {
#ifndef _WIN32
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
#endif
		if ((line == "---" && reading_metadata) || (deprecated && !line.starts_with("! "))) {
			return;
		}
		else if (line == "---" && !reading_metadata) {
			reading_metadata = true;
		}
		else if (line.starts_with("! ") && !reading_metadata) {
			reading_metadata = true;
			deprecated = true;
		}
		else if ((deprecated && line == "! TAGS START") || line == "tags:") {
			reading_tags = true;
		}
		else if (reading_tags && ((deprecated && line == "! TAGS END") || (!deprecated && (!line.starts_with("\t- #") || !line.starts_with("    - #"))))) {
			reading_tags = false;
		}
		else if (!reading_tags && reading_metadata) {
			if (size_t del_pos = line.find(":") == string::npos) {
				metadata[line] = "";
			}
			else {
				metadata[line.substr(0, del_pos)] = line.substr(del_pos);
			}
		}

	}
	file.close();
}

void read_metadata_tags_content(Log& logger, const filesystem::path& path, map<string, string>& metadata, vector<string>& tags, vector<string>& content, size_t& content_start) {
	
	content_start = 0;
	ifstream file(path);
	if (!file.is_open()) {
		logger << "File " << path << " could not be openend." << '\n';
		return;
	}

	bool reading_tags = false;
	bool deprecated = false, reading_metadata=false;
	string line, tag;
	while (getline(file, line)) {
#ifndef _WIN32
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
#endif
		
		if ((line == "---" && reading_metadata) || (deprecated && !line.starts_with("! "))) {
			reading_metadata = false;
			content_start++;
		}
		else if (line == "---" && !reading_metadata) {
			reading_metadata = true;
			content_start++;
		}
		else if (line.starts_with("! ") && !reading_metadata) {
			reading_metadata = true;
			deprecated = true;
			content_start++;
		} 
		else if ((deprecated && line == "! TAGS START") || line == "tags:") {
			reading_tags = true;
			content_start++;
		} 
		else if (reading_tags && ((deprecated && line == "! TAGS END") || (!deprecated && (!line.starts_with("\t- #") || !line.starts_with("    - #"))))) {
			reading_tags = false;
			content_start++;
		}
		else if (!reading_tags && reading_metadata) {
			if (size_t del_pos = line.find(":") == string::npos) {
				metadata[line] = "";
			}
			else {
				metadata[line.substr(0, del_pos)] = line.substr(del_pos);
			}
			content_start++;
		}
		else if (reading_tags && deprecated) {
			line.erase(0, 2); // remove leading !  part
			istringstream ss(line);
			while (ss >> tag) // split at delimiter
			{
				tag = trim(tag);
				tag.erase(0, 1); // remove leading #
				boost::algorithm::to_lower(tag);
				tags.push_back(tag);
			}
			content_start++;
		}
		else if (reading_tags) {
			line = trim(line); // remove whitespaces or tab
			line.erase(0, 3); // remove - #
			boost::algorithm::to_lower(line);
			tags.push_back(line);
			content_start++;
		}
		else {
			content.push_back(line);
		}

	}
	file.close();
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
	file << "---\n";
	file << "filename: " << (paths.file_path / filesystem::path(filename)).string() << '\n';
	if (create_data)
		file << "datafolder: " << (paths.data_path / filesystem::path(filename).stem()).string() << '\n';
	file << "date: " << date_str << '\n';
	file << "tags:" << '\n';

	for (int i = 0; i < tags.size(); i++) {
		file << "\t- #" << tags.at(i) << '\n';
	}
	file << "---" << endl;


	file.close();
}

/**
* Searches for TODOs, images, link and headers in a given file
*
* @param logger reference to logger instance
* @param paths reference to PATHS instance
* @param filename file to be parsed
**/
void parse_file(Log& logger, const PATHS& paths, const string& filename) {
	// eventually parse dates and create ics files https://github.com/jgonera/icalendarlib
	ifstream file(paths.base_path / paths.file_path / filesystem::path(filename));
	if (!file.is_open()) {
		logger << "Error parse_file: File " << paths.file_path / filesystem::path(filename) << " could not be openend." << '\n';
	}
	string ofile_name = "." + filesystem::path(filename).stem().string();
	ofstream ofile(paths.base_path / paths.tmp_path / filesystem::path(ofile_name));
	if (!ofile.is_open()) {
		logger << "Error writting in parse_file: File " << (paths.file_path / filesystem::path(ofile_name)).string() << " could not be openend." << '\n';
	}
	ofile << "! TODOS START" << '\n';

	vector<string> images = vector<string>();
	vector<string> links = vector<string>();
	vector<string> headers = vector<string>();

	// get note date
	time_t date_t;
	string date_str;
	str2date_short(date_t, filename);
	date2str(date_str, date_t);

	// matcher for links and images
	regex pat_link("(\\[\\w+\\])(\\([\\w\\.\\/]+\\))");
	regex pat_image("!(\\[\w+\\])(\\([\\w\\.\\/]+\\))");

	string line, last_section = "";
	bool metadata = false;
	bool deprecated = false;
	while (getline(file, line))
	{
#ifndef _WIN32
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
#endif
		// skip metadata
		
		// check for old metadata format
		if (line.starts_with("! ")) {
			deprecated = true;
			metadata = true;
			continue;
		}
		else if (deprecated && metadata) {
			metadata = false;
			continue;
		}
		// check for new metadata format
		else if (line == "---") {
			metadata = !metadata;
			continue;
		}

		// find most recent section for jumping to
		if (line.starts_with("#")) {
			string header_indent= "";
			for (size_t i = 0; i < line.size(); i++) {
				if (line[i] == '#')
					header_indent += '\t';
				else if (!isspace(line[i])) {
					break;
				}
			}

			last_section = trim(line, " ,\n\r\t\f\v#");
			headers.push_back(header_indent + last_section);
			for (auto i = 0; i < last_section.size(); i++) {
				if (isspace(last_section[i])) {
					last_section[i] = '-';
				}
			}
			last_section = "#" + last_section;
			continue;
		}

		// search for graphics/links
		smatch m_link, m_image;
		regex_search(line, m_link, pat_link);
		for (const auto& link_match : m_link) {
			links.push_back(link_match.str());
		}

		regex_search(line, m_image, pat_image);
		for (const auto& image_match : m_image) {
			images.push_back(image_match.str());
		}
		

		// search for TODO
		size_t pos = line.find("TODO");
		if (pos != string::npos) {

			// search for end of TODO
			size_t todo_end = line.find_first_of(")].}", pos);

			// convert to len for substr
			if (todo_end != string::npos) {
				todo_end -= pos;
			}

			ofile << "- " << line.substr(pos, todo_end) << " ([" << date_str << "](../";
			ofile << (paths.file_path / filesystem::path(filename)).string() << last_section << ")" << ")\n";
		}
	}
	file.close();

	ofile << "! TODOS END" << endl;

	ofile << "! LINKS START\n";
	for (const auto& link : links) {
		ofile << link << '\n';
	}
	ofile << "! LINKS END\n";

	ofile << "! HEADERS START\n";
	for (const auto& header : headers) {
		ofile << header << '\n';
	}
	ofile << "! HEADERS END\n";

	ofile << "! IMAGES START\n";
	for (const auto& image : images) {
		ofile << image << '\n';
	}
	ofile << "! IMAGES END" << endl;

	ofile.close();
}

void update_parsed_file(Log& logger, const PATHS& paths, list<string>& parsed_files) {
	map<string, time_t> files_map;

	for (const auto& entry : filesystem::directory_iterator(paths.base_path / paths.file_path))
	{
#ifdef _WIN32
		files_map[entry.path().stem().string()] = chrono::system_clock::to_time_t(clock_cast<chrono::system_clock>(entry.last_write_time()));
#else
		files_map[entry.path().stem().string()] = to_time_t(entry.last_write_time());
#endif
	}

	// remove files that have recent parsed files
	for (const auto& entry : filesystem::directory_iterator(paths.base_path / paths.tmp_path))
	{
		// check if path is path to parsed file by skipping leading .
		string parsed_fname = entry.path().filename().string().substr(1);
		if (files_map.contains(parsed_fname)) {

			// compare last modified values
#ifdef _WIN32
			if (files_map[parsed_fname] < chrono::system_clock::to_time_t(clock_cast<chrono::system_clock>(entry.last_write_time()))) {
#else
			if (files_map[parsed_fname] < to_time_t(entry.last_write_time())) {
#endif
				files_map.erase(parsed_fname);
				parsed_files.push_back(entry.path().filename().string());
			}
		}
	}

	// parse files when parsed files are old or missing
	for (const auto& [name_stem, mod_date] : files_map) {
		string filename = name_stem + ".md";
		parse_file(logger, paths, filename);
		parsed_files.push_back("." + name_stem);
	}

	parsed_files.sort();
	parsed_files.reverse();
}
/**
* Compares all files in the FILES directory to see whether they were modified after the last creation
* of their corresponding parsed file. Afterwards, the Todo information from the todo files is collected
* and written to the output file
*
* @param logger reference to logger instance
* @param paths reference to PATHS instance
**/
void update_todos(Log& logger, const PATHS& paths) {
	
	list<string> parsed_files;
	update_parsed_file(logger, paths, parsed_files);

	// combine results from parsed files to output file
	ofstream output(paths.base_path / paths.tmp_path / "todos.md");
	if (!output.is_open()) {
		logger << "Error update_todos: File " << paths.base_path / paths.tmp_path / "todos.md" << " could not be openend." << '\n';
	}
	for (const auto& pfile : parsed_files) {
		ifstream file(paths.base_path / paths.tmp_path /  filesystem::path(pfile));
		if (!file.is_open()) {
			logger << "Error update_todos: File " << paths.base_path / paths.tmp_path / filesystem::path(pfile) << " could not be openend." << '\n';
		}

		string line;
		bool read_todos = false;
		while (getline(file, line)) {
#ifndef _WIN32
			line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
#endif
			if (line == "! TODOS START") {
				read_todos = true;
				continue;
			}
			else if (line == "! TODOS END") {
				read_todos = false;
				continue;
			}
			else if (read_todos) {
				output << line << "\n";
			}
		}
		file.close();
	}
	output.close();
}

void get_headers_images_links(Log& logger, const PATHS& paths, const std::vector<std::string>& filter_selection, std::unordered_map<std::string, std::tuple<std::list<std::string>, std::list<std::string>, std::list<std::string>>>& headers_images_links) {
	list<string> parsed_files;
	update_parsed_file(logger, paths, parsed_files);

	for (const auto& pth : filter_selection) {
		string name = "." + filesystem::path(pth).stem().string();
		if (filesystem::exists(paths.base_path / paths.tmp_path / filesystem::path(name))) {
			ifstream file(paths.base_path / paths.tmp_path / filesystem::path(name));
			if (!file.is_open()) {
				logger.setColor(COLORS::RED);
				logger << "Error: Can't open parsed file " << (paths.base_path / paths.tmp_path / filesystem::path(name)).string() << endl;
				logger.setColor(COLORS::BLACK);
			}

			string line;
			enum class RM { HEADERS, IMAGES, LINKS, NONE };
			RM mode = RM::NONE;
			while (getline(file, line)) {
#ifndef _WIN32
				line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
#endif
				if (line == "! HEADERS START") {
					mode = RM::HEADERS;
				}
				else if (line == "! HEADERS END" || line == "! LINKS END" || line == "! IMAGES END") {
					mode = RM::NONE;
				}
				else if (line == "! IMAGES START") {
					mode = RM::IMAGES;
				}
				else if (line == "! LINKS START") {
					mode = RM::LINKS;
				}
				else if (mode == RM::HEADERS) {
					get<0>(headers_images_links[filesystem::path(pth).stem().string()]).push_back(line);
				}
				else if (mode == RM::IMAGES) {
					get<1>(headers_images_links[filesystem::path(pth).stem().string()]).push_back(line);
				}
				else if (mode == RM::LINKS) {
					get<2>(headers_images_links[filesystem::path(pth).stem().string()]).push_back(line);
				}
			}
		}
		else {
			logger.setColor(COLORS::RED);
			logger << "Error: Can't find parsed file " << (paths.base_path / paths.tmp_path / filesystem::path(name)).string() << endl;
			logger.setColor(COLORS::BLACK);
		}
	}
}