#pragma once

#include "../ProtocollTool/log.h"
#include "../ProtocollTool/utils.h"

#include <map>
#include <string>
#include <thread>

void update_tags(Log& logger, const PATHS& paths, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>>& tag_map, std::map<std::string, int>& tag_count, std::vector<std::string>& filter_selection, const bool add_new_to_filter_selection);
void filter_notes(Log& logger, std::istringstream& iss, const PATHS& paths, std::vector<std::string>& filter_selection, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>> tag_map, std::vector<std::string> mode_tags);
void find_notes(Log& logger, std::istringstream& iss, const PATHS& paths, std::vector<std::string>& filter_selection, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>> tag_map, std::vector<std::string> mode_tags);
void show_filtered_notes(Log& logger, std::istringstream& iss, Config& conf, int& active_mode, const PATHS& paths, const std::string& tmp_filename, std::vector<std::string>& filter_selection, const bool& has_pandoc);
void add_note(Log& logger, std::istringstream& iss, const PATHS& paths, const std::string& file_ending, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>>& tag_map, std::vector<std::string>& mode_tags, std::vector<std::string>& filter_selection);
void add_data(Log& logger, std::istringstream& iss, const PATHS& paths, std::vector<std::string> filter_selection);
void show_details(Log& logger, std::istringstream& iss, const PATHS& paths, Config& conf, int& active_mode, std::vector<std::string> filter_selection, std::map<std::string, time_t> file_map, std::map<std::string, std::vector<std::string>> tag_map);
void open_selection(Log& logger, const PATHS& paths, std::vector<std::string> filter_selection);
void create_mode(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, int& num_modes, std::unordered_map<int, std::string>& mode_names, std::vector<std::string>& mode_tags, int& active_mode, const std::string& file_ending);
void delete_mode(Log& logger, std::istringstream& iss, Config& conf, int& num_modes, std::unordered_map<int, std::string>& mode_names, std::vector<std::string>& mode_tags, int& active_mode);
void edit_mode(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, std::unordered_map<int, std::string>& mode_names, std::vector<std::string>& mode_tags, int& active_mode,  const std::string& file_ending);
void activate_mode_command(Log& logger, std::istringstream& iss, Config& conf, const PATHS& paths, std::unordered_map<int, std::string>& mode_names, int& active_mode, std::vector<std::string>& mode_tags,  const std::string& file_ending);
void deactivate_mode(Log& logger, Config& conf, int& active_mode, std::vector<std::string>& mode_tags);
void activate_mode(Log& logger, Config& conf, const PATHS& paths, int& active_mode, std::vector<std::string>& mode_tags,  const std::string& file_ending);
void show_modes(Log& logger, std::istringstream& iss, Config& conf, std::unordered_map<int, std::string>& mode_names, int& active_mode);