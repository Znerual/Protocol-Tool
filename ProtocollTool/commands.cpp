#include "file_manager.h"
#ifdef _WIN32
#include "commands.h"
#include "watcher_windows.h"
#else
#include "../ProtocollToolLinux/commands_linux.h"
#endif
//#include "autocomplete.h"


#include <sstream>
#include <fstream>
#include <filesystem>
#include <regex>

#include <tabulate/table.hpp>
#include <boost/algorithm/string.hpp>
using namespace std;

#ifdef _WIN32
void activate_mode(Log& logger, Config& conf, const PATHS& paths, int& active_mode, vector<string>& mode_tags, std::vector<std::jthread>& file_watchers, const string& file_ending, bool& update_files, HANDLE& hStopEvent, std::vector<std::jthread>& open_files, HANDLE& hExit)
#else
void activate_mode(Log& logger, Config& conf, const PATHS& paths, int& active_mode, vector<string>& mode_tags, const string& file_ending)
#endif
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
#ifdef _WIN32
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
		file_watchers.push_back(jthread(file_change_watcher, ref(logger), filesystem::path(folder_path_str), paths, file_ending, folder_tags, ref(update_files), ref(hStopEvent), ref(open_files), ref(hExit)));
	}
#endif
}

#ifdef _WIN32
void deactivate_mode(Log& logger, Config& conf, int& active_mode, vector<string>& mode_tags, std::vector<std::jthread>& file_watchers, HANDLE& hStopEvent) {
#else
void deactivate_mode(Log& logger, Config& conf, int& active_mode, vector<string>& mode_tags) {
#endif
	active_mode = -1;
	mode_tags = vector<string>();

#ifdef _WIN32
	SetEvent(hStopEvent);
	for (jthread& watcher : file_watchers) {
		watcher.request_stop();
	}

	file_watchers = vector<jthread>();
#endif
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

	for (size_t i = 0; i < mode_tags.size(); i++)
	{
		conf.set("MODE_" + to_string(mode_id) + "_TAG_" + to_string(i), mode_tags.at(i));
	}

}


void Show_todos::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	update_todos(*(this->logger), *(this->paths));
	// maybe do a convert to first
#ifdef _WIN32
	open_file(*logger, *paths, paths->base_path / paths->tmp_path / "todos.md", *open_files, *hExit);
#else
	open_file(*logger, *paths, paths->base_path / paths->tmp_path / "todos.md");
#endif
}

void Show_modes::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	//int con_col, con_row;
	//get_console_size(con_row, con_col);

	tabulate::Table modes_table;
	//modes_table.format().width(con_col-1);
	tabulate::Table name_tags_table;
	name_tags_table.add_row({ "Mode name", "Mode tags" });
	name_tags_table.format()
		.padding_top(0)
		.padding_bottom(0)
		.hide_border();
#ifdef _WIN32
	tabulate::Table wf_table;
	wf_table.add_row({ "Watch folders", "with tags" });
	wf_table.format()
		.padding_top(0)
		.padding_bottom(0)
		.hide_border();
#endif

	modes_table.add_row({ name_tags_table });
	modes_table.add_row({ "Options" });

#ifdef _WIN32
	modes_table.add_row({ wf_table });
	modes_table[2].format().padding_bottom(1);
#else
	modes_table[1].format().padding_bottom(1);
#endif

	modes_table.format()
		.font_style({ tabulate::FontStyle::italic })
		//.border_top(" ")
		//.border_bottom(" ")
		//.border_left(" ")
		//.border_right(" ")
		.corner(" ")
		.font_align({ tabulate::FontAlign::center });

	


	/*
	

	string mode_name{ "Mode name:" }, mode_tags{ "Mode tags: ..." }, mode_options{ "Options: ..." }, watch_folder{ "Watch folders: ..." }, folder_tags{ " with tags: ..." }, filler{};
	pad(mode_name, 26, ' ');
	pad(mode_tags, con_col - 26, ' '); //46

	pad(mode_options, con_col, ' ');
	pad(watch_folder, 30, ' ');
	pad(folder_tags, con_col - 30, ' ');
	pad(filler, con_col, '-');
	*logger << filler << '\n';
	*logger << mode_name << mode_tags << '\n' << mode_options << '\n' << watch_folder << folder_tags << '\n';
	*logger << filler << "\n";
	* */
	for (const auto& [id, name] : *mode_names)
	{
		// show name
		
		/*string display_name = name;
		pad(display_name, 25, ' ');
		*logger << display_name << " ";
		*/
		// show tags
		int num_tags;
		(*conf).get("MODE_" + to_string(id) + "_NUM_TAGS", num_tags);
		ostringstream tags;
		/*
		int tag_width = 0;
		if (num_tags > 0) {
			tag_width = ((con_col - 26 - 1) / num_tags) - 1;
		}


		if (tag_width < 8) {
			tag_width = 7;
		}
		int extra_tag_width = 0;
		*/
		string tag;
		for (auto i = 0; i < num_tags; i++) {
			(*conf).get("MODE_" + to_string(id) + "_TAG_" + to_string(i), tag);
			/*
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

			*/
			tags << tag << ' ';
		}
		string tags_str = tags.str();
		tags_str.erase(tags_str.end() - 1);
		/*
		if (tags_str.size() > 0)
		pad(tags_str, (con_col - 26 - 1), ' ');
		*logger << tags_str << " ";
		*/
		tabulate::Table m_table;
		m_table.add_row({ name, tags_str });
		m_table.format()
			.padding(0)
			.hide_border();

		modes_table.add_row({ m_table });

		// show options
		MODE_OPTIONS mode_options;
		get_mode_options((*conf), mode_options, *active_mode);

		ostringstream options;
		for (const auto& option : mode_options) {
			if (option.second) {
				options << this->cmd_names->oa_names.right.at(option.first)<< " ";
			}
		}
		string mode_options_str = options.str();
		mode_options_str.erase(mode_options_str.size() - 1);
		modes_table.add_row({ mode_options_str });

		/*
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
		*logger << options << '\n';
		*/
#ifdef _WIN32
		unordered_map<string, vector<string>> folder_watcher;
		get_folder_watcher(*conf, id, folder_watcher);
		for (const auto& [folder_path, fw_tags] : folder_watcher) {
			ostringstream fw_tags_ss;
			for (const auto& tagstr : fw_tags) {
				fw_tags_ss << tagstr << " ";
			}
			string fw_tags_str = fw_tags_ss.str();
			fw_tags_str.erase(fw_tags_str.size() - 1);
			tabulate::Table fw_table;

			fw_table.add_row({ folder_path, fw_tags_str });
			fw_table.format()
				.padding(0)
				//.border(" ")
				.hide_border();

			modes_table.add_row({ fw_table });
		}

#endif
		// show watched folders and tags
		/*
		int num_folders;
		string folder_path_str;
		(*conf).get("MODE_" + to_string(id) + "_NUM_WATCH_FOLDERS", num_folders);
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
			(*conf).get("MODE_" + to_string(id) + "_WATCH_FOLDERS_" + to_string(i) + "_NUM_TAGS", num_folder_tags);
			(*conf).get("MODE_" + to_string(id) + "_WATCH_FOLDERS_" + to_string(i) + "_PATH", folder_path_str);

			string folder_tag;
			int extra_folder_tag_width = 0;
			for (auto j = 0; j < num_folder_tags; j++)
			{
				(*conf).get("MODE_" + to_string(id) + "_WATCH_FOLDERS_" + to_string(i) + "_TAG_" + to_string(j), folder_tag);
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
			*logger << folder_path_str << " " << folder_tags_str << '\n';
		}

		*logger << filler << '\n';
		* 
		*/
	}
	*logger << modes_table << endl;
}

void Activate_mode_command::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	*mode_tags = vector<string>();
	*active_mode = -1;


	if (pargs.size() == 0)
	{
		*logger << "Specify the name of the mode you want to activate" << endl;
		return;
	}

	string mode_name = pargs.at(PA::MODE_NAME).at(0);
	
	for (const auto& [id, name] : *mode_names)
	{
		if (name == mode_name)
		{
			*active_mode = id;
		}
	}

	if (*active_mode != -1)
	{
#ifdef _WIN32
		activate_mode(*logger, *conf, *paths, *active_mode, *mode_tags, *file_watchers, *file_ending, *update_files, *hStopEvent, *open_files, *hExit);
#else
		activate_mode(*logger, *conf, *paths, *active_mode, *mode_tags, *file_ending);
#endif
	}
	else {
		*logger << "Mode " << mode_name << " not found." << endl;
	}
}

void Deactivate_mode::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	*active_mode = -1;
	*mode_tags = vector<string>();
#ifdef _WIN32
	SetEvent(*hStopEvent);
	for (jthread& watcher : *file_watchers) {
		watcher.request_stop();
	}

	*file_watchers = vector<jthread>();
#endif
}

void Edit_mode::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{

#ifdef _WIN32
	deactivate_mode(*logger, (*conf), *active_mode, *mode_tags, *file_watchers, *hStopEvent);
#else
	deactivate_mode(*logger, (*conf), *active_mode, *mode_tags);
#endif
	string mode_name;
	if (pargs.size() == 0)
	{
		*logger << "Specify name of the mode that should be edited" << endl;
		return;
	}

	mode_name = pargs.at(PA::MODE_NAME).at(0);
	boost::algorithm::to_lower(mode_name);

	int mode_id = -1;
	for (const auto& [id, name] : *mode_names) // check if name exists
	{
		if (mode_name == name)
		{
			mode_id = id;
		}
	}

	if (mode_id == -1)
	{
		*logger << "Mode " << mode_name << " could not be found! See available modes with the command: modes." << endl;
		return;
	}

	// Add or remove tags
	get_mode_tags((*conf), mode_id, *mode_tags);
	if (oastrargs.contains(OA::ADDTAG)) {
		for (const auto& tag : oastrargs.at(OA::ADDTAG)) {
			if (find(mode_tags->begin(), mode_tags->end(), tag) == mode_tags->end()) {
				mode_tags->push_back(tag);
			}
		}
		//(*mode_tags).insert(mode_tags->end(), oastrargs.at(OA::ADDTAG).begin(), oastrargs.at(OA::ADDTAG).end());
	}
	if (oastrargs.contains(OA::REMTAG)) {
		for (const auto& tag : oastrargs.at(OA::REMTAG)) {
			if (find(mode_tags->begin(), mode_tags->end(), tag) != mode_tags->end()) {
				mode_tags->erase(std::remove(mode_tags->begin(), mode_tags->end(), tag));
			}
		}
	}
	set_mode_tags((*conf), mode_id, *mode_tags);

	// Add or remove mode options
	MODE_OPTIONS mode_options;
	get_mode_options((*conf), mode_options, mode_id);
	if (oaoargs.contains(OA::ADDOPT)) {
		for (const auto& opt : oaoargs.at(OA::ADDOPT)) {
			mode_options.at(opt) = true;
		}
	}
	if (oaoargs.contains(OA::REMOPT)) {
		for (const auto& opt : oaoargs.at(OA::REMOPT)) {
			mode_options.at(opt) = false;
		}
	}
	set_mode_options((*conf), mode_options, mode_id);

#ifdef _WIN32
	// Add or remove file watchers
	unordered_map<string, vector<string>> folder_watcher_tags;
	get_folder_watcher((*conf), mode_id, folder_watcher_tags);
	if (oastrargs.contains(OA::ADDWF)) {
		// if path exists, set tags of folder to watch to new tags (excluding first argument, which is the path itself)
		if (filesystem::exists(oastrargs.at(OA::ADDWF).at(0))) {
			folder_watcher_tags[oastrargs.at(OA::ADDWF).at(0)] = vector<string>(oastrargs.at(OA::ADDWF).begin()+1, oastrargs.at(OA::ADDWF).end());
		}
		else {
			*logger << "Could not add watcher to folder " << oastrargs.at(OA::ADDWF).at(0) << " becaues the path does not exist." << endl;
		}
	}
	if (oastrargs.contains(OA::REMWF)) {
		if (folder_watcher_tags.contains(oastrargs.at(OA::REMWF).at(0))) {
			folder_watcher_tags.erase(oastrargs.at(OA::REMWF).at(0));
		}
		else {
			*logger << "Could not remove folder watcher from folder " << oastrargs.at(OA::REMWF).at(0) << " because no folder watcher is registered for this folder.";
			if (folder_watcher_tags.size() == 0) {
				*logger << " Currently, there are no folder watchers active." << endl;
			}
			else {
				*logger << " Registered folder are: ";
				for (const auto& [folder_path, _] : folder_watcher_tags) {
					*logger << folder_path << "; ";
				}
				*logger << endl;
			}
		}
	}
	set_folder_watcher((*conf), mode_id, folder_watcher_tags);
#endif
	// Change mode name
	if (oastrargs.contains(OA::CHANAME)) {
		string new_mode_name = oastrargs.at(OA::CHANAME).at(0);
		(*mode_names)[mode_id] = new_mode_name;
		(*conf).set("MODE_" + to_string(mode_id) + "_NAME", new_mode_name);
	}
#ifdef _WIN32
	activate_mode(*logger, *conf, *paths, *active_mode, *mode_tags, *file_watchers, *file_ending, *update_files, *hStopEvent, *open_files, *hExit);
#else
	activate_mode(*logger, *conf, *paths, *active_mode, *mode_tags,*file_ending);
#endif
}

void Delete_mode::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
#ifdef _WIN32
	deactivate_mode(*logger, (*conf), *active_mode, *mode_tags, *file_watchers, *hStopEvent);
#else
	deactivate_mode(*logger, (*conf), *active_mode, *mode_tags);
#endif
	if (pargs.size() == 0)
	{
		*logger << "Specify name of the mode that should be deleted" << endl;
		return;
	}

	string mode_name;
	mode_name = pargs.at(PA::MODE_NAME).at(0);
	boost::algorithm::to_lower(mode_name);

	int mode_id = -1;
	for (const auto& [id, name] : *mode_names) // check if name already exists
	{
		if (mode_name == name)
		{
			mode_id = id;
		}
	}

	if (mode_id != -1)
	{
		mode_names->erase(mode_id);
	}
	else {
		*logger << "Mode " << mode_name << " not found." << endl;
		return;
	}


	num_modes -= 1;
	(*conf).set("NUM_MODES", num_modes);

	int num_tags;
	(*conf).get("MODE_" + to_string(mode_id) + "_NUM_TAGS", num_tags);
	(*conf).remove("MODE_" + to_string(mode_id) + "_NUM_TAGS");
	for (auto i = 0; i < num_tags; i++)
	{
		(*conf).remove("MODE_" + to_string(mode_id) + "_TAG_" + to_string(i));
	}

	(*conf).remove("MODE_" + to_string(mode_id) + "_NAME");

#ifdef _WIN32
	int num_watch_folder, num_folder_tags;
	(*conf).get("MODE_" + to_string(*active_mode) + "_NUM_WATCH_FOLDERS", num_watch_folder);
	(*conf).remove("MODE_" + to_string(*active_mode) + "_NUM_WATCH_FOLDERS");
	for (auto i = 0; i < num_watch_folder; i++) {
		(*conf).get("MODE_" + to_string(*active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_NUM_TAGS", num_folder_tags);
		(*conf).remove("MODE_" + to_string(*active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_NUM_TAGS");
		(*conf).remove("MODE_" + to_string(*active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_PATH");

		for (auto j = 0; j < num_folder_tags; j++)
		{
			(*conf).remove("MODE_" + to_string(*active_mode) + "_WATCH_FOLDERS_" + to_string(i) + "_TAG_" + to_string(j));

		}
	}
#endif
}

void Create_mode::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
#ifdef _WIN32
	deactivate_mode(*logger, *conf, *active_mode, *mode_tags, *file_watchers, *hStopEvent);
#else
	deactivate_mode(*logger, *conf, *active_mode, *mode_tags);
#endif
	string mode_name;
	if (pargs.size() == 0) {
		*logger << "You need to specify a mode name" << endl;
	}
	mode_name = pargs.at(PA::MODE_NAME).at(0);
	boost::algorithm::to_lower(mode_name);

	for (const auto& [id, name] : *mode_names) // check if name already exists
	{
		if (mode_name == name)
		{
			*logger << "Mode already exists! Delete the old mode with the command (del)ete before creating a new one" << endl;
			return;
		}
	}

	*mode_tags = pargs.at(PA::TAGS);
#ifdef _WIN32
	unordered_map<string, vector<string>> folder_watcher_tags;
	if (oastrargs.contains(OA::ADDWF)) {
		folder_watcher_tags.at(oastrargs.at(OA::ADDWF).at(0)) = vector<string>(oastrargs.at(OA::ADDWF).begin() + 1, oastrargs.at(OA::ADDWF).end());
	}
#endif
	//MODE_OPTIONS mode_options;
	//parse_create_mode(logger, iss, conf, mode_name, mode_tags, folder_watcher_tags, mode_options);


	

	*active_mode = static_cast<int>(mode_names->size()); // set id to next free number
	(*mode_names)[*active_mode] = mode_name;

	num_modes += 1;
	(*conf).set("NUM_MODES", num_modes);
	(*conf).set("MODE_" + to_string(*active_mode) + "_NAME", mode_name);
	(*conf).set("MODE_" + to_string(*active_mode) + "_NUM_TAGS", static_cast<int>(mode_tags->size()));

	MODE_OPTIONS mode_options;
	init_mode_options(mode_options);
	for (const auto& flag : oaflags) {
		mode_options[flag] = true;
	}
	set_mode_options((*conf), mode_options, *active_mode);

	for (size_t i = 0; i < mode_tags->size(); i++)
	{
		(*conf).set("MODE_" + to_string(*active_mode) + "_TAG_" + to_string(i), mode_tags->at(i));
	}

#ifdef _WIN32
	(*conf).set("MODE_" + to_string(*active_mode) + "_NUM_WATCH_FOLDERS", static_cast<int>(folder_watcher_tags.size()));
	int folder_counter = 0; // not ideal, order in map can change but does not matter because all entries are treated equal
	for (const auto& [folder_path, tags] : folder_watcher_tags) {
		(*conf).set("MODE_" + to_string(*active_mode) + "_WATCH_FOLDERS_" + to_string(folder_counter) + "_NUM_TAGS", static_cast<int>(tags.size()));
		(*conf).set("MODE_" + to_string(*active_mode) + "_WATCH_FOLDERS_" + to_string(folder_counter) + "_PATH", folder_path);

		for (auto j = 0; j < tags.size(); j++)
		{
			(*conf).set("MODE_" + to_string(*active_mode) + "_WATCH_FOLDERS_" + to_string(folder_counter) + "_TAG_" + to_string(j), tags.at(j));

		}
		folder_counter++;
	}

	activate_mode(*logger, (*conf), *paths, *active_mode, *mode_tags, *file_watchers, *file_ending, *update_files, *hStopEvent, *open_files, *hExit);
#else
	activate_mode(*logger, (*conf), *paths, *active_mode, *mode_tags, *file_ending);
#endif
}

void Open_selection::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	if (filter_selection->size() == 1)
	{
		//const wstring exec_filename = (paths.base_path / filesystem::path(filter_selection.at(0))).wstring();
		//ShellExecute(0, L"open", exec_filename.c_str(), 0, 0, SW_SHOW);
#ifdef _WIN32
		open_file(*logger, *paths, paths->base_path / filesystem::path(filter_selection->at(0)), *open_files, *hStopEvent);
#else
		open_file(*logger, *paths, paths->base_path / filesystem::path(filter_selection->at(0)));
#endif
	}
	else if (filter_selection->size() > 1)
	{
		string choice;
		*logger << filter_selection->size() << " found in the selection. Open all of them? (y/n)" << endl;
		cin >> choice;
		cin.clear();
		cin.ignore(10000, '\n');
		logger->input(choice);
		if (choice == "y" || choice == "Y") {

			for (const string& path : *filter_selection)
			{
				//const wstring exec_filename = (paths.base_path / filesystem::path(path)).wstring();
				//ShellExecute(0, L"open", exec_filename.c_str(), 0, 0, SW_SHOW);
#ifdef _WIN32
				open_file(*logger, *paths, paths->base_path / filesystem::path(path), *open_files, *hStopEvent);
#else
				open_file(*logger, *paths, paths->base_path / filesystem::path(path));
#endif
			}
		}
	}
	else {
		*logger << "Found no file to open in the current selection. Use (f)ind to set selection." << endl;
	}
}

void Show_details::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	MODE_OPTIONS mode_options;
	get_mode_options(*conf, mode_options, *active_mode);
	for (const auto& flag : oaflags) {
		mode_options[flag] = true;
	}

	// check whether the header_images_links should be read in
	std::unordered_map<std::string, std::tuple<std::list<std::string>, std::list<std::string>, std::list<std::string>>> headers_images_links;
	if (mode_options.at(OA::SHEAD) || mode_options.at(OA::SIMG) || mode_options.at(OA::SLINK)) {
		get_headers_images_links(*logger, *paths, *filter_selection, headers_images_links);
	}

	time_t note_date;
	string date_str, modified_date_str;
	string output;
	for (const string& path : *filter_selection)
	{
		//*logger << path << endl;
		//continue;
		string filename = filesystem::path(path).stem().string();
		str2date_short(note_date, filename.substr(0, 8));
		date2str(date_str, note_date);
		output = date_str;
		//*logger << "  " << date_str;
		if (filename[8] != 'a')
		{
			output = output+ " Version " + filename[8];
		}
		if (mode_options.at(OA::LPATH))
		{
			output = output + " at " + (paths->base_path / filesystem::path(path)).string();
		}
		else if (mode_options.at(OA::PATH))
		{
			output = output + " at " + path;
		}
		if (mode_options.at(OA::LMOD))
		{
			date2str_long(modified_date_str, file_map->at(path));
			output = output + " last modified at " + modified_date_str;
		}
		if (mode_options.at(OA::STAGS))
		{
			output = output + " with tags: ";
			for (const auto& tag : tag_map->at(path))
			{
				output = output + tag + " ";
			}
		}
		if (mode_options.at(OA::SIMG))
		{
			output = output + ", with " + to_string(get<1>(headers_images_links[filename]).size()) + " images";
		}
		if (mode_options.at(OA::SLINK)) {
			output = output + ", with " + to_string(get<2>(headers_images_links[filename]).size()) + " links";
		}
		if (mode_options.at(OA::SHEAD)) {
			output = output + ", with " + to_string(get<0>(headers_images_links[filename]).size()) + " headers";
		}
		*logger << wrap(output);

		if (mode_options.at(OA::SIMG))
		{
			if (get<1>(headers_images_links[filename]).size() > 0) {
				logger->setColor(COLORS::GREEN);
				for (const auto& img : get<1>(headers_images_links[filename])) {
					*logger << "\t" << img << '\n';
				}
				logger->setColor(COLORS::BLACK);
			}
		}
		if (mode_options.at(OA::SLINK)) {
			if (std::get<2>(headers_images_links[filename]).size() > 0) {
				logger->setColor(COLORS::GREEN);
				for (const auto& link : get<2>(headers_images_links[filename])) {
					*logger << "\t" << link << '\n';
				}
				logger->setColor(COLORS::BLACK);
			}
		}
		if (mode_options.at(OA::SHEAD)) {
			if (std::get<0>(headers_images_links[filename]).size() > 0) {
				logger->setColor(COLORS::GREEN);
				for (const auto& link : get<0>(headers_images_links[filename])) {
					*logger << link << '\n';
				}
				logger->setColor(COLORS::BLACK);

			}
		}
		if (mode_options.at(OA::CONTENT))
		{
			ifstream file(paths->base_path / filesystem::path(path));
			string str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
			file.close();

			*logger << " with note text:\n" << str;
		}
		*logger << endl;
	}
}

void Add_data::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	filesystem::path data_dir = paths->base_path / paths->data_path;
	if (filter_selection->size() > 1)
	{
		string choice;
		*logger << filter_selection->size() << " found in the selection. Add data folders for all of them? (y/n)" << endl;
		cin >> choice;
		cin.clear();
		cin.ignore(10000, '\n');
		logger->input(choice);
		if (choice == "y" || choice == "Y") {

			for (const string& path : *filter_selection)
			{
				filesystem::create_directories(data_dir / filesystem::path(path).stem());
				*logger << "Added data directory for " << filesystem::path(path).stem() << endl;
			}
		}
	}
	else if (filter_selection->size() == 1)
	{
		filesystem::create_directories(data_dir / filesystem::path(filter_selection->at(0)).stem());
		*logger << "Added data directory for " << filesystem::path(filter_selection->at(0)).stem() << endl;
	}
	else {
		*logger << "No file found in the selection. Use the find command to specify which note should have a data folder added." << endl;
		return;
	}
}

void Add_note::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	MODE_OPTIONS mode_options;
	get_mode_options(*conf, mode_options, *active_mode);
	for (const auto& flag : oaflags) {
		mode_options[flag] = true;
	}

	if (pargs.size() == 0 || !pargs.contains(PA::DATE)) {
		*logger << "Need to input a date for the note" << endl;
	}

	time_t date;
	CONV_ERROR ret = str2date(date, pargs.at(PA::DATE).at(0));
	if (ret != CONV_ERROR::CONV_SUCCESS) {
		*logger << "First argument of the new command has to be the date: ((t)oday, (y)esterday, dd, dd.mm, dd.mm.yyyy), set date to today" << endl;
		date = time(NULL);
	}

	string filename = get_filename(*paths, date, *file_ending);
	vector<string> tags;
	if (pargs.contains(PA::TAGS)) {
		tags = pargs.at(PA::TAGS);
	}
	else {
		tags = vector<string>();
	}
	tags.insert(tags.end(), mode_tags->begin(), mode_tags->end());

	write_file(*logger, *paths, filename, date, tags, *file_ending, mode_options.at(OA::DATA));


	// open new file
#ifdef _WIN32
	open_file(*logger, *paths, (paths->base_path / paths->file_path / filesystem::path(filename)), *open_files, *hExit);
#else
	open_file(*logger, *paths, (paths->base_path / paths->file_path / filesystem::path(filename)));
#endif
	//open_md_editor(logger, (paths.base_path / paths.file_path / filesystem::path(filename)));

	// update file_map, tag_map, tag_count and filter selection
	string path = (paths->file_path / filesystem::path(filename)).string();
	(*file_map)[path] = time_t(NULL);
	(*tag_map)[path] = tags;
	filter_selection->push_back(path);
	get_tag_count(*tag_map, *filter_selection);
}

void Show_filtered_notes::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	// sort selection
	sort(filter_selection->begin(), filter_selection->end());

	MODE_OPTIONS mode_options;
	get_mode_options(*conf, mode_options, *active_mode);
	for (const auto& flag : oaflags) {
		mode_options[flag] = true;
	}
	// write content of selected files to output file
	ofstream show_file(paths->base_path / paths->tmp_path / filesystem::path(*tmp_filename));
	string date_str;
	time_t date_t;
	CONV_ERROR ret;
	string line;
	if (mode_options.at(OA::TOC) && !mode_options.at(OA::NODATE))
	{
		show_file << "# Table of Contents" << '\n';

		int i = 1;
		for (const string& path : *filter_selection)
		{
			ret = str2date_short(date_t, filesystem::path(path).stem().string().substr(0, 8));
			if (ret != CONV_SUCCESS) {
				*logger << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}
			ret = date2str(date_str, date_t);
			if (ret != CONV_SUCCESS) {
				*logger << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}

			show_file << i++ << ". [note from " << date_str << " Version " << filesystem::path(path).stem().string()[8] << "](#" << filesystem::path(path).stem().string() << ")\n";
			ifstream file(paths->base_path / filesystem::path(path));
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


	for (const string& path : *filter_selection)
	{

		// add date to the output
		if (!mode_options.at(OA::NODATE)) {
			ret = str2date_short(date_t, filesystem::path(path).stem().string().substr(0, 8));
			if (ret != CONV_SUCCESS) {
				*logger << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}
			ret = date2str(date_str, date_t);
			if (ret != CONV_SUCCESS) {
				*logger << "Error opening file " << path << ". Invalid filename." << endl;
				show_file.close();
				return;
			}

			show_file << "# " << date_str << " Version " << filesystem::path(path).stem().string()[8] << "<a id=\"" << filesystem::path(path).stem().string() << "\"></a>\n";
		}
		map<string, string> metadata;
		vector<string> tags;
		vector<string> content;
		size_t line_count;
		read_metadata_tags_content(*logger, paths->base_path / paths->file_path / filesystem::path(path).filename(), metadata, tags, content, line_count);
		// show metadata
		if (mode_options.at(OA::MDATA)) {
			for (const auto& [name, value] : metadata) {
				show_file << name << ": " << value << "; ";
			}
			show_file << endl;
		}
		// show tags
		if (mode_options.at(OA::STAGS)) 
		{
			for (const auto& tag : tags) {
				show_file << tag << ", ";
			}
			show_file << endl;
		}

		// write content
		for (const auto& line : content) {
			show_file << line;
			if (line[0] == '#') {
				show_file << "<a id=\"" << line_count << "\"></a>";
			}
			show_file << '\n';
			line_count++;
		}
		show_file << endl;
	}

	show_file.close();

	// open showfile and data folders
	if (mode_options.at(OA::DATA))
	{
		for (const string& path : *filter_selection)
		{
			//const wstring datapath = (paths.base_path / paths.data_path / filesystem::path(path).stem()).wstring();
			//ShellExecute(NULL, L"open", datapath.c_str(), NULL, NULL, SW_SHOWNORMAL);
#ifdef _WIN32
			open_file(*logger, *paths, paths->base_path / paths->data_path / filesystem::path(path).stem(), *open_files, *hExit);
#else
			open_file(*logger, *paths, paths->base_path / paths->data_path / filesystem::path(path).stem());
#endif
			if (mode_options.at(OA::IMG))
			{
				// search for image data in the folder
				for (const auto& entry : filesystem::directory_iterator(paths->base_path / paths->data_path / filesystem::path(path).stem()))
				{
					const string ext = entry.path().extension().string();
					if (ext == "jpg" || ext == "png" || ext == "jpeg" || ext == "gif")
					{
#ifdef _WIN32
						open_file(*logger, *paths, entry.path(), *open_files, *hExit);
#else
						open_file(*logger, *paths, entry.path());
#endif
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
		if (mode_options.at(OA::HTML))
		{
			if (int returnCode = convert_document_to("html", "html", *paths, *tmp_filename, "show") != 0) {
				*logger << "Error " << returnCode << " while converting markdown to html using pandoc" << endl;
			}
#ifdef _WIN32
			open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path("show.html"), *open_files, *hExit);
#else
			open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path("show.html"));
#endif
			//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.html")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);

		}

		if (mode_options.at(OA::MD))
		{

			//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.md")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
#ifdef _WIN32
			open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path("show.md"), *open_files, *hExit);
#else
			open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path("show.md"));
#endif
		}

		if (mode_options.at(OA::DOCX))
		{
			if (int returnCode = convert_document_to("docx", "docx", *paths, *tmp_filename, "show") != 0) {
				*logger << "Error " << returnCode << " while converting markdown to docx using pandoc" << endl;
			}

			//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.docx")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
#ifdef _WIN32
			open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path("show.docx"), *open_files, *hExit);
#else
			open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path("show.docx"));
#endif

		}

		if (mode_options.at(OA::TEX))
		{
			if (int returnCode = convert_document_to("latex", "tex", *paths, *tmp_filename, "show") != 0) {
				*logger << "Error " << returnCode << " while converting markdown to latex (tex) using pandoc" << endl;
			}

			//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.tex")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
#ifdef _WIN32
			open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path("show.tex"), *open_files, *hExit);
#else
			open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path("show.tex"));
#endif

		}

		if (mode_options.at(OA::PDF))
		{
			if (int returnCode = convert_document_to("pdf", "pdf", *paths, *tmp_filename, "show") != 0) {
				*logger << "Error " << returnCode << " while converting markdown to pdf using pandoc" << endl;
			}

			//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path("show.pdf")).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
#ifdef _WIN32
			open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path("show.pdf"), *open_files, *hExit);
#else
			open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path("show.pdf"));
#endif

		}

	}
	else {
		//ShellExecute(NULL, L"open", (paths.base_path / paths.tmp_path / filesystem::path(tmp_filename)).wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
#ifdef _WIN32
		open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path(*tmp_filename), *open_files, *hExit);
#else
		open_file(*logger, *paths, paths->base_path / paths->tmp_path / filesystem::path(*tmp_filename));
#endif
	}


}

void Find_notes::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	*filter_selection = vector<string>();
	filter_selection->reserve(tag_map->size());

	for (const auto& [path, tag] : *tag_map)
	{
		filter_selection->push_back(path);
	}

	filter_notes(logger, paths, conf, active_mode, mode_tags, tag_map, filter_selection, oaflags, oastrargs);

	logger->setColor(GREEN, WHITE);
	*logger << "  Found " << filter_selection->size() << " notes matching the filter.\n";
	if (*active_mode != -1) {
		*logger << "  Tags ";
		for (const auto& tag : *mode_tags) {
			*logger << tag << " ";
		}
		*logger << "from Mode " << mode_names->at(*active_mode) << " were added to the filter.\n";
	}
	logger->setColor(BLUE, WHITE);
	*logger << "  Run (s)how, (d)etails, (a)dd_data, (t)ags or (f)ilter for more.\n" << endl;
	logger->setColor(BLACK, WHITE);
}

void Filter_notes::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	filter_notes(logger, paths, conf, active_mode, mode_tags, tag_map, filter_selection, oaflags, oastrargs);
	logger->setColor(GREEN, WHITE);
	*logger << "  Found " << filter_selection->size() << " notes matching the filter.\n";
	if (*active_mode != -1) {
		*logger << "  Tags ";
		for (const auto& tag : *mode_tags) {
			*logger << tag << " ";
		}
		*logger << "from Mode " << mode_names->at(*active_mode) << " were added to the filter.\n";
	}
	logger->setColor(BLUE, WHITE);
	*logger << "  Run (s)how, (d)etails, (a)dd_data, (t)ags or (f)ilter for more.\n" << endl;
	logger->setColor(BLACK, WHITE);
}

void filter_notes(Log* logger, PATHS* paths, Config* conf, int *active_mode, std::vector<std::string>* mode_tags, std::map<std::string, std::vector<std::string>>* tag_map, std::vector<std::string>* filter_selection, std::vector<OA>& oaflags, std::map<OA, std::vector<string>>& oastrargs) {
	MODE_OPTIONS mode_options;
	get_mode_options(*conf, mode_options, *active_mode);
	for (const auto& flag : oaflags) {
		mode_options[flag] = true;
	}

	vector<string> _filter_selection;
	vector<string> catags_args;
	if (oastrargs.contains(OA::CATAGS)) {
		catags_args = oastrargs.at(OA::CATAGS);
	}
	catags_args.insert(catags_args.begin(), mode_tags->begin(), mode_tags->end());

	for (const auto& path : *filter_selection)
	{
		// filter for date
		if (oastrargs.contains(OA::DATE_R))
		{
			time_t note_date;
			str2date_short(note_date, filesystem::path(path).stem().string());
			if (oastrargs.at(OA::DATE_R).size() == 1)
			{
				size_t del_pos = oastrargs.at(OA::DATE_R).at(0).find_first_of("-:");
				if (del_pos == string::npos) {
					time_t filter_date;
					str2date(filter_date, oastrargs.at(OA::DATE_R).at(0));
					if (filter_date != note_date)
						continue;
				}
				else {
					time_t filter_begin, filter_end;
					str2date(filter_begin, oastrargs.at(OA::DATE_R).at(0).substr(0, del_pos));
					str2date(filter_end, oastrargs.at(OA::DATE_R).at(0).substr(del_pos+1));
					if (note_date < filter_begin || note_date > filter_end) {
						continue;
					}
				}
				
			}
			else if (oastrargs.at(OA::DATE_R).size() == 2) {
				time_t filter_begin, filter_end;
				str2date(filter_begin, oastrargs.at(OA::DATE_R).at(0));
				str2date(filter_end, oastrargs.at(OA::DATE_R).at(1));
				if (note_date < filter_begin || note_date > filter_end) {
					continue;
				}
			}
			else {
				*logger << "Invalid number of date arguments. Expects one or two date values." << endl;
			}

		}

		// filter for version
		if (oastrargs.contains(OA::VERS_R))
		{
			string note_version = to_string(filesystem::path(path).stem().string()[8]);
			if (find(oastrargs.at(OA::VERS_R).begin(), oastrargs.at(OA::VERS_R).end(), note_version) == oastrargs.at(OA::VERS_R).end())
			{
				continue;
			}

		}

		// filter for having a data folder
		if (mode_options.at(OA::DATA))
		{
			if (!filesystem::exists(paths->base_path / paths->data_path / filesystem::path(path).stem())) {
				continue;
			}
		}

		// filter for tags where only one of the tags needs to match
		if (oastrargs.contains(OA::CTAGS))
		{
			bool has_tag = false;
			vector<string> tags = tag_map->at(path);
			for (auto& tag : oastrargs.at(OA::CTAGS))
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
			vector<string> tags = tag_map->at(path);
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
		if (oastrargs.contains(OA::NTAGS))
		{
			bool has_tag = false;
			string tag_copy;
			for (const auto& tag : oastrargs.at(OA::CTAGS))
			{
				tag_copy = trim(tag);
				boost::algorithm::to_lower(tag_copy);
				if (find(tag_map->at(path).begin(), tag_map->at(path).end(), tag_copy) != tag_map->at(path).end()) {
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
		if (oastrargs.contains(OA::REGT))
		{
			regex pat(oastrargs.at(OA::REGT).at(0));
			bool found_match = false;
			string tag_copy;
			for (const auto& tag : tag_map->at(path))
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
		if (oastrargs.contains(OA::REGC))
		{
			regex pat(oastrargs.at(OA::REGC).at(0));
			ifstream file(paths->base_path / filesystem::path(path));
			const string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
			file.close();

			if (!regex_match(content, pat)) {
				continue;
			}
		}


		_filter_selection.push_back(path);
	}
	*filter_selection = _filter_selection;
}

void Update_tags::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	update_tags(*logger, *paths, *file_map, *tag_map, *tag_count, *filter_selection, true);

	// parse all files
	for (const auto& entry : filesystem::directory_iterator(paths->base_path / paths->file_path))
	{
		string filename = entry.path().stem().string() + ".md";
		parse_file(*logger, *paths, filename);
		
	}
}

void Show_tags::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	//TODO add option to sort by amount of tags and not alphabetic
	if (filter_selection->size() == 0) {
		filter_selection->reserve(tag_map->size());
		for (const auto& [path, tag] : *tag_map)
		{
			filter_selection->push_back(path);
		}
	}

	logger->setColor(GREEN, WHITE);
	*logger << "  Tags";
	if (*active_mode != -1) {
		*logger << " (Mode Tags:";
		string tag_copy;
		for (const auto& tag : *mode_tags) {
			tag_copy = trim(tag);
			boost::to_lower(tag_copy);
			*logger << " " << tag_copy;
		}
		*logger << ")";
	}
	*logger << "\n";

	auto tag_count = get_tag_count(*tag_map, *filter_selection);
	for (const auto& [tag, count] : tag_count)
	{
		*logger << "  " << tag << ": " << count << '\n';
	}
	logger->setColor(BLACK, WHITE);
}

void Quit::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
#ifdef _WIN32
	SetEvent(*hExit);
#endif
	*running = false;
}

void EditTask::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs) {
	*logger << "Editing Tasks not implemented" << endl;
}

void Help::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	//TODO: rework to use commands from the cmd.dat and cmd_names.dat, maybe add cmd_help.dat
	// create string for showing cmd info
	string help_string = "This program can be used by typing one of the following commands:\n";
	for (const auto& [cmd_name, cmd] : cmd_names->cmd_names.left) {
		help_string += cmd_name;
		if (cmd_names->cmd_abbreviations.right.count(cmd) == 1) {
			help_string += " (" + cmd_names->cmd_abbreviations.right.at(cmd) + ")";
		}
		help_string += ", ";
	}
	help_string.erase(help_string.size() - 2, 2);

	// check if command is specified, if not, show general cmd info
	bool not_found = false;
	
	if (!oastrargs.contains(OA::CMD)) {
		*logger << wrap(help_string);
		logger->setColor(COLORS::BLUE);
		*logger << wrap("Run help [CMD_NAME] for more information.\n");
		logger->setColor(COLORS::BLACK);
		return;
	}

	for (const auto& argument : oastrargs.at(OA::CMD)) {
		bool cmd_full_name = cmd_names->cmd_names.left.count(argument) == 1;
		bool cmd_abbreviation = cmd_names->cmd_abbreviations.left.count(argument) == 1;
		if (cmd_full_name || cmd_abbreviation) {
			CMD cmd = (cmd_full_name) ? cmd_names->cmd_names.left.at(argument) : cmd_names->cmd_abbreviations.left.at(argument);

			string cmd_structure_string1 = "The command " + cmd_names->cmd_names.right.at(cmd);
			if (cmd_names->cmd_abbreviations.right.count(cmd) == 1)
				cmd_structure_string1 += " (" + cmd_names->cmd_abbreviations.right.at(cmd) + ")";
			cmd_structure_string1 += " has the following structure:";

			string cmd_structure_string2 = cmd_names->cmd_names.right.at(cmd) + " ";
			string pa_help = "";
			for (const auto& pa : cmd_structure->at(cmd).first) {
				cmd_structure_string2 += cmd_names->pa_names.right.at(pa) + " ";
				pa_help += wrap(cmd_names->pa_names.right.at(pa) + ": " + cmd_names->pa_help.at(pa)) + "\n";
			}

			string oa_help = "";
			for (const auto& [oa, oa_para] : cmd_structure->at(cmd).second) {
				cmd_structure_string2 += cmd_names->oa_names.right.at(oa) + " ";
				oa_help += wrap(cmd_names->oa_names.right.at(oa) + ": " + cmd_names->oa_help.at(oa)) + "\n";
				for (const auto& oaoa : oa_para) {
					cmd_structure_string2 += cmd_names->oa_names.right.at(oaoa) + " ";
					oa_help += wrap(cmd_names->oa_names.right.at(oaoa) + ": " + cmd_names->oa_help.at(oaoa), 6) + "\n";
				}
			}

			*logger << wrap(cmd_structure_string1) << '\n';
			logger->setColor(COLORS::BLUE);
			*logger << wrap(cmd_structure_string2, 2, ALIGN::LEFT) << "\n\n";
			logger->setColor(COLORS::BLACK);
			*logger << wrap(cmd_names->cmd_help.at(cmd)) << "\n";
			*logger << pa_help << '\n';
			*logger << oa_help << endl;
		}
		else {
			not_found = true;
		}
	}
	
	
	if (not_found) {
		*logger << wrap(help_string);
		logger->setColor(COLORS::BLUE);
		*logger << wrap("Run help [CMD_NAME] for more information.\n");
		logger->setColor(COLORS::BLACK);
		return;
	}
	// check if cmd can be recognized, if not show general cmd info
	
	
	

	/*
	if (argument == "n" || argument == "new")
	{
		(*logger).setColor(BLUE, WHITE);
		(*logger) << "  (n)ew: date tag1 tag2 ... [-d]\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Creates a new note file and the optional parameter -d indicates that a data folder will be created. The given date can be in the format of dd or dd.mm or dd.mm.yyyy (where one digit numebers can be specified as one digit or two digits with leading zero) or the special format t for today and y for yesterday.", 3) << '\n' << '\n';

	}
	else if (argument == "f") {
		(*logger) << wrap("The abbreviation f will be interpreted as find if no note is currently selected and as filter, if multiple (or one) note(s) are selected.", 2) << "\n";
	}
	else if (argument == "find") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << wrap("(f)ind: [-(d)ate start_date-end_date] [-(c)ontains_(t)ags tag1 ...] [-(c)ontains_(a)ll_(t)ags tag1 ...] [-(n)o_(t)ags tag1 ...] [-(r)eg_(t)ext text] [-(v)ersions start_version-end_version] [-(d)ata_(o)nly]", 2) << "\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Looks for notes that fall within the specified ranges (start and end dates are included in the interval). -ct describes a list of tags where one of them needs to be found in the note, while -cat describes a list of tags which all need to be find in the note. -nt describes a list of tags that are not allowed to be in matching notes and -rt stands for regular expression text, which will be used to search the content of the note. -v gives an interval of versions (a-...) that are considered and -do stands for data_only, meaning only notes with a data folder will be selected.", 3) << "\n\n";
	}
	else if (argument == "filter") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << wrap("(f)ilter: [-(d)ate start_date-end_date] [-(c)ontains_(t)ags tag1 ...] [-(c)ontains_(a)ll_(t)ags tag1 ...] [-(n)o_(t)ags tag1 ...] [-(r)eg_(t)ext text] [-(v)ersions start_version-end_version] [-(d)ata_(o)nly]", 2) << "\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Reduces the current selection of notes with parameter similar to find. Also, intevals contain the start and end values. -ct describes a list of tags where one of them needs to be found in the note, while -cat describes a list of tags which all need to be find in the note. -nt describes a list of tags that are not allowed to be in matching notes and -rt stands for regular expression text, which will be used to search the content of the note. -v gives an interval of versions (a-...) that are considered and -do stands for data_only, meaning only notes with a data folder will be selected.", 3) << "\n\n";
	}
	else if (argument == "s" || argument == "show") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << wrap("(s)how: [(t)ags] [(m)etadata] [table_of_(c)ontent] [(d)ata] [(h)ide_(d)ate] [open_(i)mages] [open_as_(html)] [open_as_(tex)] [open_as_(pdf)] [open_as_(docx)] [open_as_(m)ark(d)own]", 2) << "\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Shows the current note selection (created by find and filter) in various formats (specified by the open_as_... parameter), where by default only the date and content of the note are shown. The parameters (t)ags, (m)etadata and table_of_(c)ontent add additional information to the output and (d)ata opens the data folders in the explorer. The command open_(i)mages additionally opens .jpg, .jpeg and .png files inside the data folders directly.", 3) << "\n\n";
	}
	else if (argument == "a" || argument == "add_data") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << "  (a)dd_data:\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Adds a data folder for the notes that are currently in the selection.The selection is created by the find commandand it can be refined by the filter command.", 3) << "\n";
	}
	else if (argument == "d" || argument == "details" || argument == "show_details") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << wrap("(d)etails: [-(t)ags] [-(p)ath] [-(l)ong_(p)ath] [-last_(m)odified] [-(c)ontent]", 2) << '\n';
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Shows details about the current selection of notes. The parameters controll what is shown.", 3) << "\n";
	}
	else if (argument == "t" || argument == "tags") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << "  (t)ags:\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Shows the statictics of tags of the current selection.If no selection was made, it shows the statistic of all tags of all notes.", 3) << "\n";
	}
	else if (argument == "q" || argument == "quit" || argument == "exit") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << "  (q)uit:\nEnds the programm.";
		(*logger).setColor(BLACK, WHITE);
	}
	else if (argument == "c" || argument == "create" || argument == "create_mode") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << wrap("(c)reate_mode: mode_name tag1 tag2 ... [-(s)how_(t)ags] [-(s)how_(m)etadata] [-(s)how_table_of_(c)ontent] [-(s)how_(d)ata] [-(s)how_(h)ide_(d)ate] [-(s)how_open_(i)mages] [-(s)how_open_as_(html)] [-(s)how_open_as_(m)ark(d)own] [-(s)how_open_as_(docx)] [-(s)how_open_as_(pdf)] [-(s)how_open_as_la(tex)] [-(d)etail_(t)ags] [-(d)etail_(p)ath] [-(d)etail_(l)ong_(p)ath] [-(d)etail_(l)ast_(m)odified] [-(d)etail_(c)ontent] [-(w)atch_(f)older path_to_folder1 tag1 tag2 ...] [-(w)atch_(f)older path_to_folder2 tag1 tag2 ...]", 2) << "\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("A mode allows you to set the standard behavior of the (s)how and (d)etail command, as well as influence the (n)ew, (f)ind and (f)ilter commands. Options starting with show in the name control the (s)how command and options starting with detail control the (d)etails command. All set options will always be added to the commands when the mode is active. The mode tags will be added to the tags that are manually specified in the (n)ew command and are also added to the (f)ind and (f)ilter command in the -(c)ontails_(a)ll_(t)ags parameter, meaning only notes contraining all mode tags are selected. In addition to setting default (s)how and (d)etail parameter, a path to a folder that should be observed can be added. This means that when a file inside the specified folder is created, the newly created file is automatically added to the data folder and a new note is created. Additionally, tags that will be added when this occures can be entered by the following structure of the command: [-(w)atch_(f)older path_to_folder tag1 tag2 ...]. Note that multiple folders can be specified by repeating the [-(w)atch_(f)older path_to_folder tag1 tag2 ...] command With different paths (and tags). Modes can be activated by the (act)ivate commandand and deactivated by the (deac)tivate command. Additionally, an existing mode can be edited by the (edit)_mode command and deleted by the (delete)_mode command. An overview over all modes is given by the (modes) command, which lists the name, open format, tags, all options and all observed folders with their tags of all modes.", 3) << "\n";
	}
	else if (argument == "del" || argument == "delete" || argument == "delete_mode") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << "  (del)ete_mode: mode_name\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Deletes the specified mode. More about modes can be found by running help create_mode.", 3) << "\n";
	}
	else if (argument == "act" || argument == "activate" || argument == "activate_mode") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << "  (act)ivate_mode: mode_name\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Activates the specified mode, such that all set options are now in effect. More about modes can be found by running help create_mode.", 3) << "\n";
	}
	else if (argument == "deac" || argument == "deactivate" || argument == "deactivate_mode") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << "  (deac)tivate_mode:\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Deactivate the current mode and uses standard options again. More about modes can be found by running help create_mode.", 3) << "\n";
	}

	else if (argument == "modes") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << "  (modes):\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Gives an overview over all existing modes and shows mode name, mode tags, default open format, mode options.", 3) << "\n";
	}
	else if (argument == "edit" || argument == "edit_mode") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << wrap("(edit)_mode: mode_name [-(add_opt)ion opt1 opt2 ...] [-(remove_opt)ion opt1 opt2 ...] [-(add_t)ags tag1 tag2 ...] [-(remove_t)ags tag1 tag2] [-change_name new_mode_name]", 2) << '\n';
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Where opt are the options from the (c)reate_mode command. More about modes can be found by running help create_mode", 3) << "\n";
	}
	else if (argument == "u" || argument == "update") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << "  (u)pdate:\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Updates the tag list, tag statistics and finds externally added files. Restarting the programm does the same trick.", 3) << "\n";
	}
	else if (argument == "o" || argument == "open") {
		(*logger).setColor(BLUE, WHITE);
		(*logger) << "  (o)pen:\n";
		(*logger).setColor(BLACK, WHITE);
		(*logger) << wrap("Opens the current selection of notes as markdown files, which can be edited by the editor of choice.", 3) << "\n";
	}
	else {
		(*logger) << "  command " << argument << " could not be found. It should be one of the following:\n";
		(*logger) << wrap("(n)ew, (f)ind, (f)ilter, (s)how, (a)dd_data, (d)etails, (t)ags, (q)uit, (c)reate_mode, (del)ete_mode, (act)ivate_mode, (deac)tivate_mode, (modes), (edit)_mode, (u)pdate and (o)pen.", 2) << '\n';
		(*logger) << wrap("Detailed information about a command can be found by running: help command.", 2) << "\n";
	}

	*/
	
}

/*
void Command::run(std::map<PA, std::vector<std::string>>& pargs, std::vector<OA>& oaflags, std::map<OA, std::vector<OA>>& oaoargs, std::map<OA, std::vector<std::string>>& oastrargs)
{
	throw std::invalid_argument("Do not instanciate the run of this base class! Overwrite for custom command");
}
*/