#pragma once

#include <string>
#include <map>
#include <filesystem>
#include <vector>
#include "Config.h"

enum ARGUMENT_MODE {NONE, ARG_DATE, CONTAINS_TAGS, CONTAINS_ALL_TAGS, NO_TAGS, REG_TEXT, VERSIONS};
enum OPEN_MODE {HTML, MARKDOWN, LATEX, PDF, DOCX};

const std::string WHITESPACE = " ,\n\r\t\f\v";
std::string ltrim(const std::string& s);
std::string rtrim(const std::string& s);
std::string trim(const std::string& s);

 std::map<std::string, time_t> list_all_files(const std::string& base_path, const std::string& file_path);
 std::map<std::string, std::vector<std::string>> list_all_tags(const std::string& base_path, const std::string& file_path);
 std::map<std::string, int> get_tag_count(std::map<std::string, std::vector<std::string>>& tag_map);
 std::vector<std::string> read_tags(const std::string& path);
 
 void update_tags(const std::string& base_path, const std::string& file_path, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>>& tag_map, std::map<std::string, int>& tag_count, std::vector<std::string>& filter_selection);

 std::string get_filename(const std::string& base_path, const std::string& file_path, time_t date, std::vector<std::string> tags, const std::string& file_ending);
 void write_file(const std::string& base_path, const std::string& file_path, const std::string& filename, time_t date, std::vector<std::string> tags, const std::string& file_ending, const std::string& data_path);
 void write_file(const std::string& base_path, const std::string& file_path, const std::string& filename, time_t date, std::vector<std::string> tags, const std::string& file_ending);
 
 void filter_notes(std::istringstream& iss, const std::string& base_path, const std::string& file_path, const std::string& data_path, std::vector<std::string>& filter_selection, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>> tag_map, std::vector<std::string> mode_tags);
 void find_notes(std::istringstream& iss, const std::string& base_path, const std::string& file_path, const std::string& data_path, std::vector<std::string>& filter_selection, std::map<std::string, time_t>& file_map, std::map<std::string, std::vector<std::string>> tag_map, std::vector<std::string> mode_tags);
 void show_filtered_notes(std::istringstream& iss, const OPEN_MODE default_open, const std::filesystem::path& base_path, const std::filesystem::path& file_path, const std::filesystem::path& data_path, const std::filesystem::path& tmp_path, const std::string& tmp_filename, std::vector<std::string>& filter_selection, const bool& has_pandoc);
 void add_note(std::istringstream& iss, const std::filesystem::path& base_path, const std::filesystem::path& file_path, const std::filesystem::path& data_path, const std::string& file_ending, std::map<std::string, time_t> file_map, std::map<std::string, std::vector<std::string>> tag_map, std::vector<std::string> mode_tags);
 void add_data(std::istringstream& iss, const std::filesystem::path& base_path, const std::filesystem::path& data_path, std::vector<std::string> filter_selection);
 void show_details(std::istringstream& iss, const std::filesystem::path& base_path, const std::filesystem::path& file_path, const std::filesystem::path& data_path, std::vector<std::string> filter_selection, std::map<std::string, time_t> file_map, std::map<std::string, std::vector<std::string>> tag_map);
 void open_selection(const std::filesystem::path& base_path, std::vector<std::string> filter_selection);
 void create_mode(std::istringstream& iss, Config& conf, int& num_modes, std::unordered_map<int, std::string>& mode_names, std::vector<std::string>& mode_tags, int& active_mode, OPEN_MODE& open_mode);
 void delete_mode(std::istringstream& iss, Config& conf, int& num_modes, std::unordered_map<int, std::string>& mode_names, std::vector<std::string>& mode_tags, int& active_mode, OPEN_MODE& open_mode);
 void activate_mode(std::istringstream& iss, Config& conf, std::unordered_map<int, std::string>& mode_names, int& active_mode, std::vector<std::string>& mode_tags, OPEN_MODE& open_mode);

 void parse_find_args(std::string& input, bool& data_only, std::vector<time_t>& date_args, std::vector<std::string>& ctags_args, std::vector<std::string>& catags_args, std::vector<std::string>& ntags_args, std::string& regex, std::vector<char>& version_args);
 void parse_details_args(std::istringstream& iss, bool& show_path, bool& show_long_path, bool& show_tags, bool& show_last_modified, bool& show_content);
 void parse_show_args(std::istringstream& iss, OPEN_MODE default_open, bool& show_tags, bool& show_metadata, bool& show_table_of_content, bool& show_data, bool& open_image_data, bool& hide_date, bool& open_as_html, bool& open_as_tex, bool& open_as_pdf, bool& open_as_docx, bool& open_as_markdown);
 void parse_add_note(std::istringstream& iss, const std::filesystem::path& base_path, const std::filesystem::path& file_path, const std::string& file_ending, std::string& filename, time_t& date_t, std::vector<std::string>& tags, bool& add_data);
 void parse_create_mode(std::istringstream& iss, Config& conf, std::string& mode_name, std::vector<std::string>& mode_tags, OPEN_MODE& open_mode);

 int convert_document_to(const std::string& format, const std::string& ending, const std::filesystem::path& base_path, const std::filesystem::path& file_path, const std::string& filename, const std::string& output_filename = "show");