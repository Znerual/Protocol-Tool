// ProtocollTool.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

//TODO https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
// Manipulate conosole screen buffer to remove printout from started editor with ShellExecute
#ifdef _WIN32
#pragma comment(lib, "Shlwapi.lib")
#endif

#include "log.h"
#include "utils.h"
#include "Config.h"
#include "conversions.h"
#include "output.h"
#include "autocomplete.h"

#ifdef _WIN32
#include "commands.h"
#include "watcher_windows.h"
#include "file_manager.h"
#else
#include "../ProtocollToolLinux/commands_linux.h"
#include "../ProtocollToolLinux/file_manager_linux.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif 

#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/bimap/bimap.hpp>
#include <boost/bimap.hpp>
#include <streambuf>
#include <thread>


#ifdef _WIN32
#include <ShlObj.h>
#include <shlwapi.h>
#include <Windows.h>
#else
#include <functional>
#endif

using namespace std;

#ifndef _WIN32
COMMAND_INPUT auto_input;
map<string, int> tag_count;
unordered_map<int, string> mode_names;
#endif

int main()
{
    #ifdef _WIN32
        activateVirtualTerminal();
    #endif

    int width, height;
    get_console_size(height, width);
    
    #ifdef _WIN32
    set_console_font();
    set_console_background(width, height);
    #endif

    print_greetings(width);

    Config conf;
    load_config("para.conf", conf);
    
    // read in config arguments and setup paths
    bool ask_pandoc, has_pandoc, write_log;
    int  num_modes;
#ifdef _WIN32
    unordered_map<int, string> mode_names;
#endif
    PATHS paths;
    filesystem::path log_path;
    string file_ending, tmp_filename;
    
    {
        string base_path_str, file_path_str, data_path_str, tmp_path_str, tmp_mode_name, log_path_str;
        conf.get("ASK_PANDOC", ask_pandoc);
        conf.get("HAS_PANDOC", has_pandoc);
        conf.get("WRITE_LOG", write_log);

        conf.get("BASE_PATH", base_path_str);
        conf.get("FILE_PATH", file_path_str);
        conf.get("DATA_PATH", data_path_str);
        conf.get("TMP_PATH", tmp_path_str);
        conf.get("LOG_PATH", log_path_str);

        // no base path, search for setup files in the AppData/Roaming directory 
        if (base_path_str.empty()) {
#ifdef _WIN32
            WCHAR* fp_return;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &fp_return))) {
                filesystem::path app_data(fp_return);
                paths.base_path = app_data / filesystem::path("Notes");
                conf.set("BASE_PATH", paths.base_path.string());
            }
            else {
                paths.base_path = filesystem::path(base_path_str);
                cout << colorize(RED, WHITE) << "SHGetKnownFolderPath() failed." << endl;
            }
#else
            const char* homedir;
            if ((homedir = getenv("HOME")) == NULL) {
                homedir = getpwuid(getuid())->pw_dir;
            }
            paths.base_path = filesystem::path(homedir) / filesystem::path("Notes");
            conf.set("BASE_PATH", paths.base_path.string());
#endif
        }
        else {
            paths.base_path = filesystem::path(base_path_str);
        }

        paths.file_path = filesystem::path(file_path_str);
        paths.data_path = filesystem::path(data_path_str);
        paths.tmp_path = filesystem::path(tmp_path_str);
        log_path = filesystem::path(log_path_str);

        conf.get("TMP_FILENAME", tmp_filename);
        conf.get("FILE_ENDING", file_ending);

        conf.get("NUM_MODES", num_modes);
        for (auto i = 0; i < num_modes; i++)
        {
            conf.get("MODE_" + to_string(i) + "_NAME", tmp_mode_name);
            tmp_mode_name = trim(tmp_mode_name);
            mode_names[i] = tmp_mode_name;

        }
#ifdef _WIN32
        
        // find default applications for opening markdown files
        get_default_applications(paths);
#endif
    }

    // TODO check error if all paths do not exist
    check_base_path(conf, paths);
    check_standard_paths(paths);
    
    Log logger(paths.base_path / log_path, write_log);
    logger.setColor(YELLOW, WHITE);
    logger << "  The base path for all files is: " << paths.base_path.string() << ".\n  The path to the notes is " << paths.file_path.string() << " and to the data is " << paths.data_path.string() << '\n' << endl;

    // check if pandoc is installed
    get_pandoc_installed(logger, conf, ask_pandoc, has_pandoc);

    vector<string> filter_selection;
    map<string, time_t> file_map = list_all_files(paths);
    map<string, vector<string>> tag_map = list_all_tags(logger, paths);
    int active_mode = -1;
    filter_selection.reserve(tag_map.size());
    for (const auto& [path, tag] : tag_map)
    {
        filter_selection.push_back(path);
    }

#ifdef _WIN32
    map<string, int> tag_count = get_tag_count(tag_map, filter_selection);
#else
    tag_count = get_tag_count(tag_map, filter_selection);
#endif
    // mode tag variables
    vector<string> mode_tags;

#ifdef _WIN32
    // manage file watchers and open file handles
    vector<jthread> file_watchers;
    vector<jthread> open_files;
    HANDLE hStopEvent;
    HANDLE hExit;

    bool update_files = false;
    hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    hExit = CreateEvent(NULL, TRUE, FALSE, L"Exit");
    jthread monitor_file_changes(note_change_watcher, ref(logger), paths, ref(update_files), ref(hExit));

    COMMAND_INPUT auto_input;
#endif

    logger.setColor(BLACK, WHITE);
    
    read_cmd_structure(filesystem::path("cmd.dat"), auto_input.cmd_structure);
    read_cmd_names(filesystem::path("cmd_names.dat"), auto_input.cmd_names);
    read_cmd_help(filesystem::path("cmd_help.dat"), auto_input.cmd_names);

#ifdef _WIN32
    AUTOCOMPLETE auto_comp(auto_input.cmd_names, tag_count, mode_names); // update trietrees when tags and/or modes are added/changed
#else
    //auto completer_func = [&](const char* text, int start, int end) {return completer(text, start, end, auto_input, tag_count, mode_names)};
    //auto completer_bound = std::bind(completer, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, auto_input, tag_count, mode_names);
    rl_attempted_completion_function = completer;
    rl_bind_key('\t', rl_complete);
#endif
    // mainloop for reading commands
    bool running = true;

#ifdef _WIN32
    map<CMD, Command*> command_fn = { {CMD::NEW , new Add_note(&logger, &paths, &conf, &active_mode, &file_ending, &file_map, &tag_map, &mode_tags, &filter_selection, &open_files, &hExit)},
           {CMD::FIND, new Find_notes(&logger, &paths, &conf, &active_mode, &filter_selection, &file_map, &tag_map, &mode_tags, &mode_names)},
           {CMD::FILTER, new Filter_notes(&logger, &paths, &conf, &active_mode, &filter_selection, &file_map, &tag_map, &mode_tags, &mode_names)},
           {CMD::SHOW, new Show_filtered_notes(&logger, &paths, &conf, &active_mode, &tmp_filename, &filter_selection, &has_pandoc, &open_files, &hExit)},
           {CMD::ADD_DATA, new Add_data(&logger, &paths, &conf, &filter_selection)},
           {CMD::DETAILS, new Show_details(&logger, &paths, &conf, &active_mode, &filter_selection, &file_map, &tag_map)},
           {CMD::TAGS, new Show_tags(&logger, &paths, &conf, &active_mode, &mode_tags, &filter_selection, &tag_map)},
           {CMD::QUIT, new Quit(&logger, &paths, &conf, &hExit, &running)},
           {CMD::CREATE_MODE, new Create_mode(&logger, &paths, &conf, &num_modes, &mode_names, &mode_tags, &active_mode, &file_watchers, &file_ending, &update_files, &hStopEvent, &open_files, &hExit)},
           {CMD::DELETE_MODE, new Delete_mode(&logger, &paths, &conf, &num_modes, &mode_names, &mode_tags, &active_mode, &file_watchers, &hStopEvent)},
           {CMD::EDIT_MODE, new Edit_mode(&logger, &paths, &conf, &mode_names, &mode_tags, &active_mode, &file_watchers, &file_ending, &update_files, &hStopEvent, &open_files, &hExit)},
           {CMD::ACTIVATE, new Activate_mode_command(&logger, &paths, &conf, &mode_names, &active_mode, &mode_tags, &file_watchers, &file_ending, &update_files, &hStopEvent, &open_files, &hExit)},
           {CMD::DEACTIVATE, new Deactivate_mode(&logger, &paths, &conf, &active_mode, &mode_tags, &file_watchers, &hStopEvent)},
           {CMD::MODES, new Show_modes(&logger, &paths, &conf, &mode_names, &active_mode, &auto_input.cmd_names)},
           {CMD::UPDATE, new Update_tags(&logger, &paths, &conf, &file_map, &tag_map, &tag_count, &filter_selection, true)},
           {CMD::OPEN, new Show_filtered_notes(&logger, &paths, &conf, &active_mode, &tmp_filename, &filter_selection, &has_pandoc, &open_files, &hExit)},
           {CMD::HELP, new Help(&logger, &paths, &conf, &auto_input.cmd_names, &auto_input.cmd_structure)},
           {CMD::TODOS, new Show_todos(&logger, &paths, &conf, &open_files, &hExit)}
    };
#else
    map<CMD, Command*> command_fn = { {CMD::NEW , new Add_note(&logger, &paths, &conf, &active_mode, &file_ending, &file_map, &tag_map, &mode_tags, &filter_selection)},
           {CMD::FIND, new Find_notes(&logger, &paths, &conf, &active_mode, &filter_selection, &file_map, &tag_map, &mode_tags, &mode_names)},
           {CMD::FILTER, new Filter_notes(&logger, &paths, &conf, &active_mode, &filter_selection, &file_map, &tag_map, &mode_tags, &mode_names)},
           {CMD::SHOW, new Show_filtered_notes(&logger, &paths, &conf, &active_mode, &tmp_filename, &filter_selection, &has_pandoc)},
           {CMD::ADD_DATA, new Add_data(&logger, &paths, &conf, &filter_selection)},
           {CMD::DETAILS, new Show_details(&logger, &paths, &conf, &active_mode, &filter_selection, &file_map, &tag_map)},
           {CMD::TAGS, new Show_tags(&logger, &paths, &conf, &active_mode, &mode_tags, &filter_selection, &tag_map)},
           {CMD::QUIT, new Quit(&logger, &paths, &conf, &running)},
           {CMD::CREATE_MODE, new Create_mode(&logger, &paths, &conf, &num_modes, &mode_names, &mode_tags, &active_mode,&file_ending)},
           {CMD::DELETE_MODE, new Delete_mode(&logger, &paths, &conf, &num_modes, &mode_names, &mode_tags, &active_mode)},
           {CMD::EDIT_MODE, new Edit_mode(&logger, &paths, &conf, &mode_names, &mode_tags, &active_mode, &file_ending)},
           {CMD::ACTIVATE, new Activate_mode_command(&logger, &paths, &conf, &mode_names, &active_mode, &mode_tags, &file_ending)},
           {CMD::DEACTIVATE, new Deactivate_mode(&logger, &paths, &conf, &active_mode, &mode_tags)},
           {CMD::MODES, new Show_modes(&logger, &paths, &conf, &mode_names, &active_mode, &auto_input.cmd_names)},
           {CMD::UPDATE, new Update_tags(&logger, &paths, &conf, &file_map, &tag_map, &tag_count, &filter_selection, true)},
           {CMD::OPEN, new Show_filtered_notes(&logger, &paths, &conf, &active_mode, &tmp_filename, &filter_selection, &has_pandoc)},
           {CMD::HELP, new Help(&logger, &paths, &conf, &auto_input.cmd_names, &auto_input.cmd_structure)},
           {CMD::TODOS, new Show_todos(&logger, &paths, &conf)} 
    };
#endif

    CMD cmd;
    map<PA, vector<string>> pa = map<PA, vector<string>>();
    vector<OA> flags = vector<OA>();
    map<OA, vector<OA>> oaoa = map<OA, vector<OA>>();
    map<OA, vector<string>> oastr = map<OA, vector<string>>();

    string command;//, input;
#ifndef _WIN32
    string prompt;
    char* input;
#endif

    while (running)
    {
        
#ifdef _WIN32
        logger.setColor(BLACK, WHITE);
        logger << " Input";
        logger.setColor(YELLOW, WHITE);
        if (active_mode != -1)
        {
            logger << " (" << mode_names[active_mode] << ")";
        }
        logger.setColor(BLACK, WHITE);
        logger << ":";
#else
        logger.setColor(BLACK, WHITE);
        prompt = logger.color;
        //logger << " Input";
        prompt += " Input";
        logger.setColor(YELLOW, WHITE);
        prompt += logger.color;
        if (active_mode != -1)
        {
            prompt += " (" + mode_names[active_mode] + ")";

        }
        logger.setColor(BLACK, WHITE);
        prompt += logger.color;
        prompt += ":";
#endif
        

        // parse user input and allow for autocomplete
        AUTO_SUGGESTIONS auto_suggestions;
#ifdef _WIN32
        string last_input;
        size_t length_last_suggestion = 0;
        while (true) {
            int return_key;
            return_key = getinput(auto_input.input);
            logger.input(auto_input.input);

            if (return_key == VK_TAB) {
                logger.input("<TAB>");
                get_suggestion(logger, auto_input, auto_comp, auto_suggestions, last_input, length_last_suggestion);
            }
            else if (return_key == VK_UP) {
                logger.input("<UP>");
                cicle_suggestions(logger, auto_input, auto_comp, auto_suggestions, last_input, length_last_suggestion, true);
            }
            else if (return_key == VK_DOWN) {
                logger.input("<DOWN>");
                cicle_suggestions(logger, auto_input, auto_comp, auto_suggestions, last_input, length_last_suggestion, false);
            }
            else if (return_key == VK_RETURN) {
                logger << endl;
                auto_suggestions = AUTO_SUGGESTIONS();
                length_last_suggestion = 0;
                last_input = "";
                break;
            }
            else {
                logger << "Error in find_cmd_suggestion, invalid return " << return_key << endl;
            }
        }
#else
        input = readline(prompt.c_str());
        //getline(cin, input);
        if (strlen(input) > 0) {
            add_history(input);
        }
        auto_input.input = string(input);
        free(input);
#endif  
        // parse input to get command and arguments
        pa = map<PA, vector<string>>();
        flags = vector<OA>();
        oaoa = map<OA, vector<OA>>();
        oastr = map<OA, vector<string>>();
        parse_cmd(logger, auto_input, cmd, pa, flags, oaoa, oastr);

        // run found command
        if (command_fn.contains(cmd)) {
            command_fn.at(cmd)->run(pa, flags, oaoa, oastr);
        }

        // clean up after command
        auto_input.input = "";
        logger << endl;
#ifdef _WIN32
        if (update_files)
        {
            update_tags(logger, paths, file_map, tag_map, tag_count, filter_selection, true);
            update_files = false;
            // TODO update now
        }
#endif
    }

#ifdef _WIN32
    CloseHandle(hStopEvent);
    CloseHandle(hExit);
#endif

    // clean up allocated memory for command map
    for (auto& [cmd, fn_ptr] : command_fn) {
        delete fn_ptr;
    }
}
