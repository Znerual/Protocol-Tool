// ProtocollTool.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

//TODO https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
// Manipulate conosole screen buffer to remove printout from started editor with ShellExecute

#pragma comment(lib, "Shlwapi.lib")

#include "log.h"
#include "utils.h"
#include "Config.h"
#include "conversions.h"
#include "output.h"
#include "commands.h"
#include "watcher_windows.h"
#include "file_manager.h"
#include "autocomplete.h"

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
#endif

using namespace std;

int main()
{
    // use https://github.com/p-ranav/tabulate for beautiful print outs
    #ifdef _WIN32
        activateVirtualTerminal();
    #endif
        /*
    CONSOLE_SCREEN_BUFFER_INFOEX sbInfoEx;
    sbInfoEx.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

    GetConsoleScreenBufferInfoEx(GetStdHandle(STD_OUTPUT_HANDLE), &sbInfoEx);
    sbInfoEx.ColorTable[0] = RGB(255,255,255);

    SetConsoleScreenBufferInfoEx(GetStdHandle(STD_OUTPUT_HANDLE), &sbInfoEx);
    */
    


    int width, height;
    get_console_size(height, width);
    
    set_console_font();
    set_console_background(width, height);
    
    print_greetings(width);

    Config conf;
    load_config("para.conf", conf);
    
    // read in config arguments and setup paths
    bool ask_pandoc, has_pandoc, write_log;
    int  num_modes;
    unordered_map<int, string> mode_names;
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
        int num_watch_folders = 0;
        for (auto i = 0; i < num_modes; i++)
        {
            conf.get("MODE_" + to_string(i) + "_NAME", tmp_mode_name);
            tmp_mode_name = trim(tmp_mode_name);
            mode_names[i] = tmp_mode_name;

        }

        // find default applications for opening markdown files
        get_default_applications(paths);
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
    map<string, int> tag_count = get_tag_count(tag_map, filter_selection);

    // mode tag variables
    vector<string> mode_tags;

    // manage file watchers and open file handles
    vector<jthread> file_watchers;
    vector<jthread> open_files;
    HANDLE hStopEvent;
    HANDLE hExit;
    

    bool update_files = false;
    hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    hExit = CreateEvent(NULL, TRUE, FALSE, L"Exit");
    jthread monitor_file_changes(note_change_watcher, ref(logger), paths, ref(update_files), ref(hExit));

    logger.setColor(BLACK, WHITE);
    
    AUTO_INPUT auto_input;
    read_cmd_structure(filesystem::path("cmd.dat"), auto_input.cmd_structure);
    read_cmd_names(filesystem::path("cmd_names.dat"), auto_input.cmd_names);
    AUTOCOMPLETE auto_comp(auto_input.cmd_names, tag_count, mode_names); // update trietrees when tags and/or modes are added/changed

    // mainloop for reading commands
    bool running = true;
    while (running)
    {
        
        logger.setColor(BLACK, WHITE);
        logger << " Input";
        logger.setColor(YELLOW, WHITE);
        if (active_mode != -1)
        {
            logger << " (" << mode_names[active_mode] << ")";
        }
        logger.setColor(BLACK, WHITE);
        logger << ":";

        // parse user input and allow for autocomplete
        AUTO_SUGGESTIONS auto_suggestions;
        string last_input, command;
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
                logger << "Error in parse_cmd, invalid return " << return_key << endl;
            }
        }
       



        istringstream iss(auto_input.input);
        iss >> command;
        boost::algorithm::to_lower(command);

        if (command == "n" || command == "new") 
        {
            add_note(logger, iss, paths, file_ending, file_map, tag_map, mode_tags, filter_selection, open_files, hExit);
        }
        else if (command == "f") { // could be find or filter
            logger.setColor(GREEN, WHITE);
            if (filter_selection.size() == 0) {
                find_notes(logger, iss, paths, filter_selection, file_map, tag_map, mode_tags);
                logger << "  Found " << filter_selection.size() << " matching notes.\n";
            }
            else {
                filter_notes(logger, iss, paths, filter_selection, file_map, tag_map, mode_tags);
                logger << "  Found " << filter_selection.size() << " notes matching the filter.\n";
            }
            logger.setColor(BLACK, WHITE);
            if (active_mode != -1) {
                logger.setColor(GREEN, WHITE);
                logger << "  Tags ";
                for (const auto& tag : mode_tags) {
                    logger << tag << " ";
                }
                logger << "from Mode " << mode_names[active_mode] << " were added to the search.\n";
                logger.setColor(BLACK, WHITE);
            }
            logger.setColor(BLUE, WHITE);
            logger << "  Run (s)how, (d)etails, (a)dd_data, (t)ags or (f)ilter for more.\n" << endl;
            logger.setColor(BLACK, WHITE);
        }
        else if (command == "find") {
            find_notes(logger, iss, paths, filter_selection, file_map, tag_map, mode_tags);
            logger.setColor(GREEN, WHITE);
            logger << "  Found " << filter_selection.size() << " matching notes.\n";
            if (active_mode != -1) {
                logger << "  Tags ";
                for (const auto& tag : mode_tags) {
                    logger << tag << " ";
                }
                logger << "from Mode " << mode_names[active_mode] << " were added to the search.\n";
            }
            logger.setColor(BLUE, WHITE);
            logger << "  Run (s)how, (d)etails, (a)dd_data, (t)ags or (f)ilter for more.\n" << endl;
            logger.setColor(BLACK, WHITE);
        }
        else if (command == "filter") {
            filter_notes(logger, iss, paths, filter_selection, file_map, tag_map, mode_tags);
            logger.setColor(GREEN, WHITE);
            logger << "  Found " << filter_selection.size() << " notes matching the filter.\n";
            if (active_mode != -1) {
                logger << "  Tags ";
                for (const auto& tag : mode_tags) {
                    logger << tag << " ";
                }
                logger << "from Mode " << mode_names[active_mode] << " were added to the filter.\n";
            }
            logger.setColor(BLUE, WHITE);
            logger << "  Run (s)how, (d)etails, (a)dd_data, (t)ags or (f)ilter for more.\n" << endl;
            logger.setColor(BLACK, WHITE);
        }
        else if (command == "s" || command == "show") 
        {
            show_filtered_notes(logger, iss, conf, active_mode, paths, tmp_filename, filter_selection, has_pandoc, open_files, hExit);
            
        }
        else if (command == "a" || command == "add_data") {

            add_data(logger, iss, paths, filter_selection);
           
        }
        else if (command == "d" || command == "details") {
            show_details(logger, iss, paths, conf, active_mode, filter_selection, file_map, tag_map);
        }
        else if (command == "t" || command == "tags") 
        {
            if (filter_selection.size() == 0) {
                filter_selection.reserve(tag_map.size());
                for (const auto& [path, tag] : tag_map)
                {
                    filter_selection.push_back(path);
                }
            }
            logger.setColor(GREEN, WHITE);
            logger << "  Tags";
            if (active_mode != -1) {
                logger << " (Mode Tags:";
                string tag_copy;
                for (const auto& tag : mode_tags) {
                    tag_copy = trim(tag);
                    boost::to_lower(tag_copy);
                    logger << " " << tag_copy;
                }
                logger << ")";
            }
            logger << "\n";

            tag_count = get_tag_count(tag_map, filter_selection);
            for (const auto& [tag, count] : tag_count)
            {
                logger << "  " << tag << ": " << count << '\n';
            } 
            logger.setColor(BLACK, WHITE);
        }
        else if (command == "q" || command == "quit" || command == "exit") 
        {
            SetEvent(hExit);
            running = false;
            break;
        }
        else if (command == "c" || command == "create" || command == "create_mode")
        {
            create_mode(logger, iss, conf, paths, num_modes, mode_names, mode_tags, active_mode, file_watchers, file_ending, update_files, hStopEvent, open_files, hExit);
        }
        else if (command == "del" || command == "delete" || command == "delete_mode")
        {
            delete_mode(logger, iss, conf, num_modes, mode_names, mode_tags, active_mode, file_watchers, hStopEvent);
        }
        else if (command == "act" || command == "activate" || command == "activate_mode")
        {
            activate_mode_command(logger, iss, conf, paths, mode_names, active_mode, mode_tags, file_watchers, file_ending, update_files, hStopEvent, open_files, hExit);
        }
        else if (command == "deac" || command == "deactivate" || command == "deactivate_mode")
        {
            deactivate_mode(logger, conf, active_mode, mode_tags, file_watchers, hStopEvent);
        }
        else if (command == "modes" ||  command == "show_modes")
        {
            show_modes(logger, iss, conf, mode_names, active_mode);
        }
        else if (command == "edit_mode" || command == "edit")
        {
            edit_mode(logger, iss, conf, paths, mode_names, mode_tags, active_mode, file_watchers, file_ending, update_files, hStopEvent, open_files, hExit);
        }
        else if (command == "u" || command == "update")
        {
            update_tags(logger, paths, file_map, tag_map, tag_count, filter_selection, false);
        }
        else if (command == "o" || command == "open")
        {
            open_selection(logger, paths, filter_selection, open_files, hExit);
            
        }
        else if (command == "h" || command == "-h" || command == "help")
        {
            string argument;
            if (iss >> argument) 
            {
                
                if (argument == "n" || argument == "new")
                {
                    logger.setColor(BLUE, WHITE);
                    logger << "  (n)ew: date tag1 tag2 ... [-d]\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Creates a new note file and the optional parameter -d indicates that a data folder will be created. The given date can be in the format of dd or dd.mm or dd.mm.yyyy (where one digit numebers can be specified as one digit or two digits with leading zero) or the special format t for today and y for yesterday.", 3) << '\n' << '\n';
                    
                }
                else if (argument == "f") {
                    logger << wrap("The abbreviation f will be interpreted as find if no note is currently selected and as filter, if multiple (or one) note(s) are selected.", 2) << "\n";
                } 
                else if (argument == "find") {
                    logger.setColor(BLUE, WHITE);
                    logger << wrap("(f)ind: [-(d)ate start_date-end_date] [-(c)ontains_(t)ags tag1 ...] [-(c)ontains_(a)ll_(t)ags tag1 ...] [-(n)o_(t)ags tag1 ...] [-(r)eg_(t)ext text] [-(v)ersions start_version-end_version] [-(d)ata_(o)nly]", 2) << "\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Looks for notes that fall within the specified ranges (start and end dates are included in the interval). -ct describes a list of tags where one of them needs to be found in the note, while -cat describes a list of tags which all need to be find in the note. -nt describes a list of tags that are not allowed to be in matching notes and -rt stands for regular expression text, which will be used to search the content of the note. -v gives an interval of versions (a-...) that are considered and -do stands for data_only, meaning only notes with a data folder will be selected.", 3) << "\n\n";
                }
                else if (argument == "filter") {
                    logger.setColor(BLUE, WHITE);
                    logger << wrap("(f)ilter: [-(d)ate start_date-end_date] [-(c)ontains_(t)ags tag1 ...] [-(c)ontains_(a)ll_(t)ags tag1 ...] [-(n)o_(t)ags tag1 ...] [-(r)eg_(t)ext text] [-(v)ersions start_version-end_version] [-(d)ata_(o)nly]", 2) << "\n";
                    logger.setColor(BLACK, WHITE);
                    logger <<wrap("Reduces the current selection of notes with parameter similar to find. Also, intevals contain the start and end values. -ct describes a list of tags where one of them needs to be found in the note, while -cat describes a list of tags which all need to be find in the note. -nt describes a list of tags that are not allowed to be in matching notes and -rt stands for regular expression text, which will be used to search the content of the note. -v gives an interval of versions (a-...) that are considered and -do stands for data_only, meaning only notes with a data folder will be selected.", 3) << "\n\n";
                }
                else if (argument == "s" || argument == "show") {
                    logger.setColor(BLUE, WHITE);
                    logger << wrap("(s)how: [(t)ags] [(m)etadata] [table_of_(c)ontent] [(d)ata] [(h)ide_(d)ate] [open_(i)mages] [open_as_(html)] [open_as_(tex)] [open_as_(pdf)] [open_as_(docx)] [open_as_(m)ark(d)own]" ,2) << "\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Shows the current note selection (created by find and filter) in various formats (specified by the open_as_... parameter), where by default only the date and content of the note are shown. The parameters (t)ags, (m)etadata and table_of_(c)ontent add additional information to the output and (d)ata opens the data folders in the explorer. The command open_(i)mages additionally opens .jpg, .jpeg and .png files inside the data folders directly.", 3) << "\n\n";
                }
                else if (argument == "a" || argument == "add_data") {
                    logger.setColor(BLUE, WHITE);
                    logger << "  (a)dd_data:\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Adds a data folder for the notes that are currently in the selection.The selection is created by the find commandand it can be refined by the filter command.", 3) << "\n";
                }
                else if (argument == "d" || argument == "details" ||argument == "show_details") {
                    logger.setColor(BLUE, WHITE);
                    logger << wrap("(d)etails: [-(t)ags] [-(p)ath] [-(l)ong_(p)ath] [-last_(m)odified] [-(c)ontent]", 2) << '\n';
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Shows details about the current selection of notes. The parameters controll what is shown.",3) << "\n";
                }
                else if (argument == "t" || argument == "tags") {
                    logger.setColor(BLUE, WHITE);
                    logger << "  (t)ags:\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Shows the statictics of tags of the current selection.If no selection was made, it shows the statistic of all tags of all notes.", 3) << "\n";
                }
                else if (argument == "q" || argument == "quit" || argument == "exit") {
                    logger.setColor(BLUE, WHITE);
                    logger << "  (q)uit:\nEnds the programm.";
                    logger.setColor(BLACK, WHITE);
                }
                else if (argument == "c" || argument == "create" || argument == "create_mode") {
                    logger.setColor(BLUE, WHITE);
                    logger << wrap("(c)reate_mode: mode_name tag1 tag2 ... [-(s)how_(t)ags] [-(s)how_(m)etadata] [-(s)how_table_of_(c)ontent] [-(s)how_(d)ata] [-(s)how_(h)ide_(d)ate] [-(s)how_open_(i)mages] [-(s)how_open_as_(html)] [-(s)how_open_as_(m)ark(d)own] [-(s)how_open_as_(docx)] [-(s)how_open_as_(pdf)] [-(s)how_open_as_la(tex)] [-(d)etail_(t)ags] [-(d)etail_(p)ath] [-(d)etail_(l)ong_(p)ath] [-(d)etail_(l)ast_(m)odified] [-(d)etail_(c)ontent] [-(w)atch_(f)older path_to_folder1 tag1 tag2 ...] [-(w)atch_(f)older path_to_folder2 tag1 tag2 ...]", 2) << "\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("A mode allows you to set the standard behavior of the (s)how and (d)etail command, as well as influence the (n)ew, (f)ind and (f)ilter commands. Options starting with show in the name control the (s)how command and options starting with detail control the (d)etails command. All set options will always be added to the commands when the mode is active. The mode tags will be added to the tags that are manually specified in the (n)ew command and are also added to the (f)ind and (f)ilter command in the -(c)ontails_(a)ll_(t)ags parameter, meaning only notes contraining all mode tags are selected. In addition to setting default (s)how and (d)etail parameter, a path to a folder that should be observed can be added. This means that when a file inside the specified folder is created, the newly created file is automatically added to the data folder and a new note is created. Additionally, tags that will be added when this occures can be entered by the following structure of the command: [-(w)atch_(f)older path_to_folder tag1 tag2 ...]. Note that multiple folders can be specified by repeating the [-(w)atch_(f)older path_to_folder tag1 tag2 ...] command With different paths (and tags). Modes can be activated by the (act)ivate commandand and deactivated by the (deac)tivate command. Additionally, an existing mode can be edited by the (edit)_mode command and deleted by the (delete)_mode command. An overview over all modes is given by the (modes) command, which lists the name, open format, tags, all options and all observed folders with their tags of all modes." ,3) << "\n";
                }
                else if (argument == "del" || argument == "delete" || argument == "delete_mode") {
                    logger.setColor(BLUE, WHITE);
                    logger << "  (del)ete_mode: mode_name\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Deletes the specified mode. More about modes can be found by running help create_mode.", 3) << "\n";
                }
                else if (argument == "act" || argument == "activate" || argument == "activate_mode") {
                    logger.setColor(BLUE, WHITE);
                    logger << "  (act)ivate_mode: mode_name\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Activates the specified mode, such that all set options are now in effect. More about modes can be found by running help create_mode.", 3) << "\n";
                }
                else if (argument == "deac" || argument == "deactivate" || argument == "deactivate_mode") {
                    logger.setColor(BLUE, WHITE);
                    logger << "  (deac)tivate_mode:\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Deactivate the current mode and uses standard options again. More about modes can be found by running help create_mode.", 3) << "\n";
                }

                else if (argument == "modes") {
                    logger.setColor(BLUE, WHITE);
                    logger << "  (modes):\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Gives an overview over all existing modes and shows mode name, mode tags, default open format, mode options.",3) << "\n";
                }
                else if (argument == "edit" || argument == "edit_mode") {
                    logger.setColor(BLUE, WHITE);
                    logger << wrap("(edit)_mode: mode_name [-(add_opt)ion opt1 opt2 ...] [-(remove_opt)ion opt1 opt2 ...] [-(add_t)ags tag1 tag2 ...] [-(remove_t)ags tag1 tag2] [-change_name new_mode_name]", 2) << '\n';
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Where opt are the options from the (c)reate_mode command. More about modes can be found by running help create_mode", 3) << "\n";
                }
                else if (argument == "u" || argument == "update") {
                    logger.setColor(BLUE, WHITE);
                    logger << "  (u)pdate:\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Updates the tag list, tag statistics and finds externally added files. Restarting the programm does the same trick.", 3) << "\n";
                }
                else if (argument == "o" || argument == "open") {
                    logger.setColor(BLUE, WHITE);
                    logger << "  (o)pen:\n";
                    logger.setColor(BLACK, WHITE);
                    logger << wrap("Opens the current selection of notes as markdown files, which can be edited by the editor of choice.", 3) << "\n";
                }
                else {
                    logger << "  command " << argument << " could not be found. It should be one of the following:\n";
                    logger << wrap("(n)ew, (f)ind, (f)ilter, (s)how, (a)dd_data, (d)etails, (t)ags, (q)uit, (c)reate_mode, (del)ete_mode, (act)ivate_mode, (deac)tivate_mode, (modes), (edit)_mode, (u)pdate and (o)pen.", 2) << '\n';
                    logger << wrap("Detailed information about a command can be found by running: help command.", 2) << "\n";
                }
            }
            else {
                logger << wrap("This programm can be used by typing one of the following commands, where (...) indicates that the expression inside the parenthesis can be used as an abbreviation of the command or parameter. (n)ew, (f)ind, (f)ilter, (s)how, (a)dd_data, (d)etails, (t)ags, (q)uit, (c)reate_mode, (del)ete_mode, (act)ivate_mode, (deac)tivate_mode, (modes), (edit)_mode, (u)pdate and (o)pen. Detailed information about a command can be found by running: help command.", 2) << "\n";
            }
        
        
        
        }
        else if (!command.empty()) {
            logger << "  Command " << command << " could not be parsed.\n";
            logger << wrap("Available options are : (n)ew, (f)ind, (f)ilter, (s)how, (a)dd_data, (d)etails, (t)ags, (u)pdate, (e)xport and (o)pen.",3) << 'n';
        }

        auto_input.input = "";
        logger << endl;

        if (update_files)
        {
            update_tags(logger, paths, file_map, tag_map, tag_count, filter_selection, true);
            update_files = false;
            // TODO update now
        }
    }
    
    CloseHandle(hStopEvent);
    CloseHandle(hExit);
   
}


// Programm ausführen: STRG+F5 oder Menüeintrag "Debuggen" > "Starten ohne Debuggen starten"
// Programm debuggen: F5 oder "Debuggen" > Menü "Debuggen starten"

// Tipps für den Einstieg: 
//   1. Verwenden Sie das Projektmappen-Explorer-Fenster zum Hinzufügen/Verwalten von Dateien.
//   2. Verwenden Sie das Team Explorer-Fenster zum Herstellen einer Verbindung mit der Quellcodeverwaltung.
//   3. Verwenden Sie das Ausgabefenster, um die Buildausgabe und andere Nachrichten anzuzeigen.
//   4. Verwenden Sie das Fenster "Fehlerliste", um Fehler anzuzeigen.
//   5. Wechseln Sie zu "Projekt" > "Neues Element hinzufügen", um neue Codedateien zu erstellen, bzw. zu "Projekt" > "Vorhandenes Element hinzufügen", um dem Projekt vorhandene Codedateien hinzuzufügen.
//   6. Um dieses Projekt später erneut zu öffnen, wechseln Sie zu "Datei" > "Öffnen" > "Projekt", und wählen Sie die SLN-Datei aus.
