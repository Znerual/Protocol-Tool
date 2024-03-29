#pragma once
#include "log.h"
#include "utils.h"
#include "Config.h"

#include <vector>
#include <filesystem>
#include <thread>
#include <string>

typedef int HANDLE;
enum class Event {NOT_SET, SET};

void file_change_watcher(Log& logger, const std::filesystem::path watch_path, const PATHS paths, const std::string file_ending, std::vector<std::string> watch_path_tags, bool& update_files, HANDLE& hStopEvent, std::vector<std::jthread>& open_files, HANDLE& hExit);
void note_change_watcher(Log& logger, const PATHS paths, bool& update_files, HANDLE& hStopMonitorEvent);
void get_folder_watcher(Config& conf, int& active_mode, std::unordered_map<std::string, std::vector<std::string>>& folder_watcher_tags);