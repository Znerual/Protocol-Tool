#pragma once
#include "utils.h"

std::map<std::string, time_t> list_all_files(const PATHS& paths);
std::map<std::string, std::vector<std::string>> list_all_tags(Log& logger, const PATHS& paths);
std::map<std::string, int> get_tag_count(std::map<std::string, std::vector<std::string>>& tag_map, std::vector<std::string> filter_selection);
std::vector<std::string> read_tags(Log& logger, const std::string& path);
std::string get_filename(const PATHS& paths, time_t date, const std::string& file_ending);
void open_file(Log& logger, const PATHS paths, const std::filesystem::path file, std::vector<std::jthread>& open_files, HANDLE& hExit);
void write_file(Log& logger, const PATHS& paths, const std::string& filename, time_t date, std::vector<std::string> tags, const std::string& file_ending, const bool create_data);
int convert_document_to(const std::string& format, const std::string& ending, const PATHS& paths, const std::string& filename, const std::string& output_filename = "show");