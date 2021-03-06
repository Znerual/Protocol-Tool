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
#include "watcher.h"
#include "file_manager.h"

#include <Windows.h>
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
#include <shlwapi.h>
#include <ShlObj.h>
#include <thread>



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

    
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof cfi;
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 18;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
   

    wcscpy_s(cfi.FaceName, L"Lucida Console"); // Consolas
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
    //SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 240);
    
    
    DWORD dwBytesWritten;
    WORD colors[3]{ BACKGROUND_BLUE, BACKGROUND_GREEN, BACKGROUND_RED };
    FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED, width * height, { 0,0 }, &dwBytesWritten);
   
    //system("color d7");
    // TODO: Add file system watcher for updating the state when files are changed:
    // https://docs.microsoft.com/en-us/previous-versions/chzww271(v=vs.140)?redirectedfrom=MSDN
    {
        string fillerb{ "|_____________________________|" }, fillerm{ "|                             |" }, fillert{ "_____________________________" }, welcome{ "|Welcome to the Protocol Tool!|" };
        pad(fillert, width - 1, ' ', true, MIDDLE);
        pad(fillerm, width - 1, ' ', true, MIDDLE);
        pad(fillerb, width - 1, ' ', true, MIDDLE);
        pad(welcome, width - 1, ' ', true, MIDDLE);
    cout << colorize(BLUE, WHITE) << fillert << '\n';
    cout << colorize(BLUE, WHITE) << fillerm << '\n';
    cout << colorize(BLUE, WHITE) << welcome << '\n';
    cout << colorize(BLUE, WHITE) << fillerb << '\n';
    }
    Config conf;

    try {
        //conf = Config("D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\para.conf");
        conf = Config("para.conf");
        cout << colorize(YELLOW, WHITE) << "  Loaded the configuration file\n";
    }
    catch (IOException& e) {
        string config_path;
        cout << colorize(RED, WHITE) << "  Error " << e.what() << " while loading the configuration file from " << "D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\para.conf" << '\n';
        cout << colorize(BLACK, WHITE) << "  Please specify the path where the configuration file (para.conf) can be found:" << endl;
        cout << colorize(BLACK, WHITE) << "  Path to para.conf: ";
        
        bool found_config = false;
        while (!found_config)
        {
            cin >> config_path;
            cin.ignore(10000, '\n');
            cin.clear();
            try {
                conf = Config(config_path);
                found_config = true;
                break;
            }
            catch (IOException& e2) {
                cout << colorize(RED, WHITE) << "  Error " << e2.what() << " while loading the configuration file " << "para.conf" << '\n';
                cout << colorize(BLACK, WHITE) << "  Please specify the path where the configuration file (para.conf) can be found. \n  In order to avoid this error, place the conf.para file ";
                cout << colorize(BLACK, WHITE) << "  in the same directory as the .exe file.\nPath to conf.para:";
            }
        }
        
    }
    
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
        wstring res_wstr;
        TCHAR szBuf[1000];
        DWORD cbBufSize = sizeof(szBuf);

        HRESULT hr = AssocQueryString(0, ASSOCSTR_EXECUTABLE,
            L".md", NULL, szBuf, &cbBufSize);
        //if (FAILED(hr)) { /* handle error */ }
        res_wstr = wstring(szBuf, cbBufSize);
        
        paths.md_exe = filesystem::path(res_wstr);

        hr = AssocQueryString(0, ASSOCSTR_EXECUTABLE,
            L".html", NULL, szBuf, &cbBufSize);
        res_wstr = wstring(szBuf, cbBufSize);

        paths.html_exe = filesystem::path(res_wstr);

        hr = AssocQueryString(0, ASSOCSTR_EXECUTABLE,
            L".docx", NULL, szBuf, &cbBufSize);
        res_wstr = wstring(szBuf, cbBufSize);

        paths.docx_exe = filesystem::path(res_wstr);

        hr = AssocQueryString(0, ASSOCSTR_EXECUTABLE,
            L".pdf", NULL, szBuf, &cbBufSize);
        res_wstr = wstring(szBuf, cbBufSize);

        paths.pdf_exe = filesystem::path(res_wstr);

        hr = AssocQueryString(0, ASSOCSTR_EXECUTABLE,
            L".tex", NULL, szBuf, &cbBufSize);
        res_wstr = wstring(szBuf, cbBufSize);

        paths.tex_exe = filesystem::path(res_wstr);
    }
    // TODO check error if all paths do not exist
    
    if (!filesystem::exists(paths.base_path))
    {
        
        
        bool found_base_path = false;
        while (!found_base_path)
        {
            string base_path_str;
            cout << colorize(RED, WHITE) << "  No path for the note files was set. Set to an existing directory to include those notes or chose a new, empty directory" << endl;
            cout << colorize(BLACK, WHITE) << "  Base Path: ";
            cin >> base_path_str;
            cin.clear();
            cin.ignore(10000, '\n');
            

            if (filesystem::create_directories(base_path_str)) // created new folder
            {
                cout << colorize(BLACK, WHITE) << "  Created a new folder at " << base_path_str << endl;
                found_base_path = true;
            }
            else { // use existing folder
                cout << colorize(BLACK, WHITE)<< "  Found the folder " << base_path_str << endl;

                // check for existing files
                if (filesystem::exists(filesystem::path(base_path_str) / paths.file_path)) {
                    bool invalid_files = false;
                    for (const auto& entry : filesystem::directory_iterator(filesystem::path(base_path_str) / paths.file_path))
                    {
                        if (entry.path().extension() != ".md" || entry.path().stem().string().size() != 9)
                        {
                            cout << colorize(RED, WHITE) << "  It seems as if invalid files are in the existing file folder!\nFound the file " << entry.path().filename() << " in the folder, which is not an appropiate note markdown file" << endl;
                            invalid_files = true;
                            break;
                        }
                    }
                    if (!invalid_files)
                    {
                        found_base_path = true;
                    }
                    else {
                        continue;
                    }
                }
                else { // no file folder in selected base_path, can be used as new base_path
                    found_base_path = true;
                }
                
            }

            conf.set("BASE_PATH", base_path_str);
            paths.base_path = filesystem::path(base_path_str);
        }
        
    }
    
    
    if (!filesystem::exists(paths.base_path / paths.file_path))
    {
        cout << colorize(YELLOW ,WHITE) << "  No folder found at the file path " << paths.base_path / paths.file_path << ".\n  Creating a new folder..." << endl;
        filesystem::create_directories(paths.base_path / paths.file_path);
    }
    if (!filesystem::exists(paths.base_path / paths.data_path))
    {
        cout << colorize(YELLOW, WHITE) << "  No folder found at the data path " << paths.base_path / paths.data_path << ".\n  Creating a new folder..." << endl;
        filesystem::create_directories(paths.base_path / paths.data_path);
    }
    if (!filesystem::exists(paths.base_path / paths.tmp_path))
    {
        cout << colorize(YELLOW, WHITE) << "  No folder found at the tmp path " << paths.base_path / paths.tmp_path << ".\n  Creating a new folder..." << endl;
        filesystem::create_directories(paths.base_path / paths.tmp_path);
    }

    Log logger(paths.base_path / log_path, write_log);
    logger.setColor(YELLOW, WHITE);
    logger << "  The base path for all files is: " << paths.base_path.string() << ".\n  The path to the notes is " << paths.file_path.string() << " and to the data is " << paths.data_path.string() << '\n' << endl;

    // check if pandoc is installed
    FILE* pipe = _popen("pandoc -v", "r");
    if (!pipe)
    {
        cerr << "Could not start command" << endl;
    }
    int returnCode = _pclose(pipe);
    if (returnCode != 0 && ask_pandoc)
    {
        logger.setColor(BLUE, WHITE);
        logger << "  Could not find pandoc. It is not required but highly recommended for this application, since html, tex and pdf output is only possible with the package.\n";
        logger << "  Do you want to install it? (y/n) or never ask again (naa)?" << endl;
        logger << "  The program needs to be restarted after the installation and will close when you decide to install pandoc." << endl;
        string choice;
        cin >> choice;
        cin.clear();
        cin.ignore(100000, '\n');
        logger.input(choice);

        if (choice == "y" || choice == "yes")
        {
            ShellExecute(NULL, L"open", L"https://pandoc.org/installing.html", NULL, NULL, SW_SHOW);
            exit(0);
        }
        else if (choice == "naa" || choice == "never_ask_again")
        {
            conf.set("ASK_PANDOC", false);
        }
    }
    else if (returnCode == 0) {
        has_pandoc = true;
        conf.set("HAS_PANDOC", true);
    }
    else {
        has_pandoc = false;
        conf.set("HAS_PANDOC", false);
    }

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

    
    
    CMD_NAMES cmd_names;
    CMD_STRUCTURE cmd_structure;
    read_cmd_structure(filesystem::path("cmd.dat"), cmd_structure);
    read_cmd_names(filesystem::path("cmd_names.dat"), cmd_names);
    AUTOCOMPLETE auto_comp(cmd_names, tag_count, mode_names); // update trietrees when tags and/or modes are added/changed

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
        list<string> auto_sugs;
        list<string>::iterator auto_sugs_pos = auto_sugs.begin();
        string auto_sug;
        string input, command;
        while (true) {
            int return_key;
         
            return_key = getinput(input);
            logger.input(input);

            if (return_key == VK_TAB) {
                parse_cmd(input, cmd_structure, cmd_names, auto_comp, auto_sug, auto_sugs);
                logger.input('\t');
                logger << auto_sug;
                input += auto_sug; // add prediction to console input
                auto_sugs_pos = auto_sugs.begin();
                continue;
            }
            else if (return_key == VK_UP) {
                // run the tab command
                if (auto_sugs.empty()) {
                    parse_cmd(input, cmd_structure, cmd_names, auto_comp, auto_sug, auto_sugs);
                    auto_sugs_pos = auto_sugs.begin();
                }
                logger.input("<UP>");

                size_t prev_length = (auto_sug).length();
                // no suggestion available
                if (auto_sugs.size() == 1) {
                    input += auto_sug;
                    logger << auto_sug;
                    auto_sug = "";
                    auto_sugs = list<string>();
                    auto_sugs_pos = auto_sugs.begin();

                    continue;
                }
                else if (auto_sugs.empty()) {
                    continue;
                }
                else {
                    if (auto_sugs_pos == prev(auto_sugs.end())) {
                        auto_sugs_pos = auto_sugs.begin();
                    }
                    else {
                        ++auto_sugs_pos;
                    }
                }

                size_t next_length = (*(auto_sugs_pos)).length();
               

                


                // delete previous suggestion
                input.erase(input.end() - prev_length, input.end());

                auto_sug = *auto_sugs_pos;
                input += auto_sug;

                // set cursor position back
                for (auto i = 0; i < prev_length; i++) {
                    logger << '\b';
                }

                // overwrite output with new suggestion
                logger << auto_sug;

                // overwrite old data 
                if (prev_length > next_length) {
                    for (auto i = 0; i < prev_length - next_length; i++) {
                        logger << ' ';
                    }
                    for (auto i = 0; i < prev_length - next_length; i++) {
                        logger << '\b';
                    }
                }
            }
            else if (return_key == VK_DOWN) {
                if (auto_sugs.empty()) {
                    parse_cmd(input, cmd_structure, cmd_names, auto_comp, auto_sug, auto_sugs);
                    auto_sugs_pos = auto_sugs.begin();
                }
                logger.input("<DOWN>");
                

                // no suggestion available
                if (auto_sugs.size() == 1) {
                    logger << auto_sug;
                    input += auto_sug; // add prediction to console input
                    auto_sugs_pos = auto_sugs.begin();
                    continue;
                }
                else if (auto_sugs.empty()) {
                    continue;
                }

                // delete previous suggestion
                size_t prev_length = auto_sug.length();

                if (auto_sugs_pos == auto_sugs.begin()) {
                    auto_sugs_pos = --auto_sugs.end();
                }
                else {
                    --auto_sugs_pos;
                }
            

                // delete previous suggestion
                input.erase(input.end() - prev_length);

                size_t next_length = (*(auto_sugs_pos)).length();
                auto_sug = *auto_sugs_pos;
                input += auto_sug;
                // set cursor position back
                for (auto i = 0; i < prev_length; i++) {
                    logger << '\b';
                }

                // overwrite output with new suggestion
                logger << auto_sug;

                // overwrite old data 
                if (prev_length > next_length) {
                    for (auto i = 0; i < prev_length - next_length; i++) {
                        logger << ' ';
                    }
                    for (auto i = 0; i < prev_length - next_length; i++) {
                        logger << '\b';
                    }
                }
            }
            else if (return_key == VK_RETURN) {
                logger << endl;
                auto_sug = "";
                auto_sugs = list<string>();
                auto_sugs_pos = auto_sugs.begin();
                break;
            }
            else {
                logger << "Error in parse_cmd, invalid return " << return_key << endl;
            }
        }
       



        istringstream iss(input);
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

        input = "";
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
