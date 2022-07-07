// ProtocollTool.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include <iostream>
#include <filesystem>
#include <fstream>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <streambuf>
#include <shlwapi.h>
//#include <cstdlib>
//#include <array>
#include "utils.h"
#include "Config.h"
#include "conversions.h"
#include "log.h"
#include <thread>

using namespace std;

int main()
{

    // TODO: Add file system watcher for updating the state when files are changed:
    // https://docs.microsoft.com/en-us/previous-versions/chzww271(v=vs.140)?redirectedfrom=MSDN
    cout << "Welcome to the Protocol Tool!" << '\n';
    cout << "-----------------------------" << '\n';
    Config conf;

    try {
        //conf = Config("D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\para.conf");
        conf = Config("para.conf");
        cout << "Loaded the configuration file\n";
    }
    catch (IOException& e) {
        string config_path;
        cout << "Error " << e.what() << " while loading the configuration file from " << "D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\para.conf" << '\n';
        cout << "Please specify the path where the configuration file (para.conf) can be found:" << endl;
        cout << "Path to para.conf: ";
        
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
                cout << "Error " << e2.what() << " while loading the configuration file " << "para.conf" << '\n';
                cout << "Please specify the path where the configuration file (para.conf) can be found. \nIn order to avoid this error, place the conf.para file ";
                cout << "in the same directory as the .exe file.\nPath to conf.para:";
            }
        }
        
    }
    
    // read in config arguments and setup paths
    bool ask_pandoc, has_pandoc, write_log;
    int  num_modes;
    OPEN_MODE open_mode;
    unordered_map<int, string> mode_names;
    PATHS paths;
    filesystem::path log_path;
    string file_ending, tmp_filename;
    
    {
        string base_path_str, file_path_str, data_path_str, tmp_path_str, tmp_mode_name, log_path_str;
        int open_mode_int;
        conf.get("ASK_PANDOC", ask_pandoc);
        conf.get("HAS_PANDOC", has_pandoc);
        conf.get("WRITE_LOG", write_log);

        conf.get("BASE_PATH", base_path_str);
        conf.get("FILE_PATH", file_path_str);
        conf.get("DATA_PATH", data_path_str);
        conf.get("TMP_PATH", tmp_path_str);
        conf.get("LOG_PATH", log_path_str);
        paths.base_path = filesystem::path(base_path_str);
        paths.file_path = filesystem::path(file_path_str);
        paths.data_path = filesystem::path(data_path_str);
        paths.tmp_path = filesystem::path(tmp_path_str);
        log_path = filesystem::path(log_path_str);

        conf.get("TMP_FILENAME", tmp_filename);
        conf.get("FILE_ENDING", file_ending);

        conf.get("DEFAULT_OPEN_MODE", open_mode_int);
        open_mode = static_cast<OPEN_MODE>(open_mode_int);

        conf.get("NUM_MODES", num_modes);
        int num_watch_folders = 0;
        for (auto i = 0; i < num_modes; i++)
        {
            conf.get("MODE_" + to_string(i) + "_NAME", tmp_mode_name);
            tmp_mode_name = trim(tmp_mode_name);
            mode_names[i] = tmp_mode_name;

        }
    }
    // TODO check error if all paths do not exist
    
    if (!filesystem::exists(paths.base_path))
    {
        bool found_base_path = false;
        while (!found_base_path)
        {
            string base_path_str;
            cout << "No path for the note files was set. Set to an existing directory to include those notes or chose a new, empty directory" << endl;
            cout << "Base Path: ";
            cin >> base_path_str;
            cin.clear();
            cin.ignore(10000, '\n');
            

            if (filesystem::create_directories(base_path_str)) // created new folder
            {
                cout << "Created a new folder at " << base_path_str << endl;
                found_base_path = true;
            }
            else { // use existing folder
                cout << "Found the folder " << base_path_str << endl;
                bool invalid_files = false;
                for (const auto& entry : filesystem::directory_iterator(filesystem::path(base_path_str) / paths.file_path))
                {
                    if (entry.path().extension() != ".md" || entry.path().stem().string().size() != 9)
                    {
                        cout << "It seems as if invalid files are in the existing file folder!\nFound the file " << entry.path().filename() << " in the folder, which is not an appropiate note markdown file" << endl;
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

            conf.set("BASE_PATH", base_path_str);
            paths.base_path = filesystem::path(base_path_str);
        }
        
    }
    Log logger(paths.base_path / log_path, write_log);

    logger << "The base path for all files is: " << paths.base_path.string() << ".\nThe path to the notes is " << paths.file_path.string() << " and to the data is " << paths.data_path.string() << '\n' << endl;
    
    if (!filesystem::exists(paths.base_path / paths.file_path))
    {
        logger << "No folder found at the file path " << paths.base_path / paths.file_path << ".\nCreating a new folder..." << endl;
        filesystem::create_directories(paths.base_path / paths.file_path);
    }
    if (!filesystem::exists(paths.base_path / paths.data_path))
    {
        logger << "No folder found at the data path " << paths.base_path / paths.data_path << ".\nCreating a new folder..." << endl;
        filesystem::create_directories(paths.base_path / paths.data_path);
    }
    if (!filesystem::exists(paths.base_path / paths.tmp_path))
    {
        logger << "No folder found at the tmp path " << paths.base_path / paths.tmp_path << ".\nCreating a new folder..." << endl;
        filesystem::create_directories(paths.base_path / paths.tmp_path);
    }
    

    // check if pandoc is installed
    FILE* pipe = _popen("pandoc -v", "r");
    if (!pipe)
    {
        cerr << "Could not start command" << endl;
    }
    int returnCode = _pclose(pipe);
    if (returnCode != 0 && ask_pandoc)
    {
        logger << "Could not find pandoc. It is not required but highly recommended for this application, since html, tex and pdf output is only possible with the package.\n";
        logger << "Do you want to install it? (y/n) or never ask again (naa)?" << endl;
        logger << "The program needs to be restarted after the installation and will close when you decide to install pandoc." << endl;
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
    vector<jthread> file_watchers;

    /*filesystem::path desktop{"C:\\Users\\karst\\Desktop"};
    bool update_files;
    vector<string> folder_watcher_tags;
    std::jthread t1(file_change_watcher, ref(logger), ref(desktop), ref(paths), ref(file_ending),  ref(folder_watcher_tags), ref(update_files));
    */

    bool update_files = false;
    bool running = true;
    while (running)
    {
        if (update_files)
        {
            update_tags(logger, paths, file_map, tag_map, tag_count, filter_selection);
            update_files = false;
        }


        logger << "Input";
        if (active_mode != -1)
        {
            logger << " (" << mode_names[active_mode] << ")";
        }
        logger << ":";

        string input, command;
        getline(cin, input);
        logger.input(input);

        istringstream iss(input);
        iss >> command;
        boost::algorithm::to_lower(command);

        if (command == "n" || command == "new") 
        {
            add_note(logger, iss, paths, file_ending, file_map, tag_map, mode_tags, filter_selection);
        }
        else if (command == "f") { // could be find or filter
            if (filter_selection.size() == 0) {
                find_notes(logger, iss, paths, filter_selection, file_map, tag_map, mode_tags);
                logger << "Found " << filter_selection.size() << " matching notes.\nRun (s)how, (d)etails, (a)dd_data, (t)ags or (f)ilter." << endl;
            }
            else {
                filter_notes(logger, iss, paths, filter_selection, file_map, tag_map, mode_tags);
                logger << "Found " << filter_selection.size() << " notes matching the filter.\nRun (s)how, (d)etails, (a)dd_data, (t)ags or (f)ilter." << endl;
            }
            
        }
        else if (command == "find") {
            find_notes(logger, iss, paths, filter_selection, file_map, tag_map, mode_tags);
            logger << "Found " << filter_selection.size() << " matching notes.\nRun (s)how, (d)etails, (a)dd_data, (t)ags or (f)ilter." << endl;
        }
        else if (command == "filter") {
            filter_notes(logger, iss, paths, filter_selection, file_map, tag_map, mode_tags);
            logger << "Found " << filter_selection.size() << " notes matching the filter.\nRun (s)how, (d)etails, (a)dd_data, (t)ags or (f)ilter." << endl;
        }
        else if (command == "s" || command == "show") 
        {
            show_filtered_notes(logger, iss, open_mode, conf, active_mode, paths, tmp_filename, filter_selection, has_pandoc);
            
        }
        else if (command == "a" || command == "add_data") {

            add_data(logger, iss, paths, filter_selection);
           
        }
        else if (command == "d" || command == "details") {
            show_details(logger, iss, paths, conf, active_mode, filter_selection, file_map, tag_map);
        }
        else if (command == "t" || command == "tags") 
        {
            logger << "Tags: \n";
            tag_count = get_tag_count(tag_map, filter_selection);
            for (const auto& [tag, count] : tag_count)
            {
                logger << tag << ": " << count << '\n';
            }                      
        }
        else if (command == "q" || command == "quit" || command == "exit") 
        {
            running = false;
            break;
        }
        else if (command == "c" || command == "create" || command == "create_mode")
        {
            create_mode(logger, iss, conf, paths, num_modes, mode_names, mode_tags, active_mode, file_watchers, open_mode, file_ending, update_files);
        }
        else if (command == "del" || command == "delete" || command == "delete_mode")
        {
            delete_mode(logger, iss, conf, num_modes, mode_names, mode_tags, active_mode, file_watchers, open_mode);
        }
        else if (command == "act" || command == "activate" || command == "activate_mode")
        {
            activate_mode_command(logger, iss, conf, paths, mode_names, active_mode, mode_tags, file_watchers, open_mode, file_ending, update_files);
        }
        else if (command == "deac" || command == "deactivate" || command == "deactivate_mode")
        {
            deactivate_mode(logger, conf, active_mode, mode_tags, file_watchers, open_mode);
        }
        else if (command == "modes" ||  command == "show_modes")
        {
            show_modes(logger, iss, conf, mode_names, active_mode, open_mode);
        }
        else if (command == "edit_mode" || command == "edit")
        {
            edit_mode(logger, iss, conf, paths, mode_names, mode_tags, active_mode, file_watchers, open_mode, file_ending, update_files);
        }
        else if (command == "u" || command == "update")
        {
            update_tags(logger, paths, file_map, tag_map, tag_count, filter_selection);
        }
        else if (command == "o" || command == "open")
        {
            open_selection(logger, paths, filter_selection);
            
        }
        else if (command == "h" || command == "-h" || command == "help")
        {
            logger << "This programm can be used by typing one of the following commands, where (...) indicates that the expression ";
            logger << "inside the parenthesis can be used as an abbreviation of the command or parameter.\n";
            logger << "(n)ew, (f)ind, (f)ilter, (s)how, (a)dd_data, (d)etails, (t)ags, (q)uit, (c)reate_mode, (del)ete_mode, (act)ivate_mode, (deac)tivate_mode, (modes), (edit)_mode, (u)pdate and (o)pen.\n";
            logger << "Detailed information about a command can be found by running: help command.\n\n";
            string argument;
            if (iss) 
            {
                iss >> argument;
                if (argument == "n" || argument == "new")
                {
                    logger << "(n)ew: date tag1 tag2 ... [-d]\nCreates a new note file and the optional parameter -d indicates that a data folder will be created. The given date can be in ";
                    logger << "the format of dd or dd.mm or dd.mm.yyyy (where one digit numebers can be specified as one digit or two digits with leading zero) or the special format t for today and ";
                    logger << "y for yesterday.\n\n";
                }
                else if (argument == "f") {
                    logger << "The abbreviation f will be interpreted as find if no note is currently selected and as filter, if multiple (or one) note(s) are selected.\n\n";
                } 
                else if (argument == "find") {
                    logger << "(f)ind: [-(d)ate start_date-end_date] [-(c)ontains_(t)ags tag1 ...] [-(c)ontains_(a)ll_(t)ags tag1 ...] [-(n)o_(t)ags tag1 ...] [-(r)eg_(t)ext text] [-(v)ersions start_version-end_version] [-(d)ata_(o)nly]\n";
                    logger << "Looks for notes that fall within the specified ranges (start and end dates are included in the interval). -ct describes a list of tags where one of them ";
                    logger << "needs to be found in the note, while -cat describes a list of tags which all need to be find in the note. -nt describes a list of tags that are not allowed ";
                    logger << "to be in matching notes and -rt stands for regular expression text, which will be used to search the content of the note. -v gives an interval of versions (a-...) ";
                    logger << "that are considered and -do stands for data_only, meaning only notes with a data folder will be selected.\n\n";
                }
                else if (argument == "filter") {
                    logger << "(f)ilter: [-(d)ate start_date-end_date] [-(c)ontains_(t)ags tag1 ...] [-(c)ontains_(a)ll_(t)ags tag1 ...] [-(n)o_(t)ags tag1 ...] [-(r)eg_(t)ext text] [-(v)ersions start_version-end_version] [-(d)ata_(o)nly]\n";
                    logger << "Reduces the current selection of notes with parameter similar to find. Also, intevals contain the start and end values. -ct describes a list of tags where one of them ";
                    logger << "needs to be found in the note, while -cat describes a list of tags which all need to be find in the note. -nt describes a list of tags that are not allowed ";
                    logger << "to be in matching notes and -rt stands for regular expression text, which will be used to search the content of the note. -v gives an interval of versions (a-...) ";
                    logger << "that are considered and -do stands for data_only, meaning only notes with a data folder will be selected.\n\n";
                }
                else if (argument == "s" || argument == "show") {
                    logger << "(s)how: [(t)ags] [(m)etadata] [table_of_(c)ontent] [(d)ata] [(h)ide_(d)ate] [open_(i)mages] [open_as_(html)] [open_as_(tex)] [open_as_(pdf)] [open_as_(docx)] [open_as_(m)ark(d)own]\n";
                    logger << "Shows the current note selection (created by find and filter) in various formats (specified by the open_as_... parameter), where by default only the date and content of the note are shown.";
                    logger << "The parameters (t)ags, (m)etadata and table_of_(c)ontent add additional information to the output and (d)ata opens the data folders in the explorer. ";
                    logger << "The command open_(i)mages additionally opens .jpg, .jpeg and .png files inside the data folders directly.\n\n";
                }
                else if (argument == "a" || argument == "add_data") {
                    logger << "(a)dd_data:\nAdds a data folder for the notes that are currently in the selection. The selection is created by the find command and it can be refined by the filter command.\n\n";
                }
                else if (argument == "d" || argument == "details" ||argument == "show_details") {
                    logger << "(d)etails: [-(t)ags] [-(p)ath] [-(l)ong_(p)ath] [-last_(m)odified] [-(c)ontent]\n";
                    logger << "Shows details about the current selection of notes. The parameters controll what is shown.\n\n";
                }
                else if (argument == "t" || argument == "tags") {
                    logger << "(t)ags:\nShows the statictics of tags of the current selection. If no selection was made, it shows the statistic of all tags of all notes.\n\n";
                }
                else if (argument == "q" || argument == "quit") {
                    logger << "(q)uit:\nEnds the programm.";
                }
                else if (argument == "c" || argument == "create_mode") {
                    logger << "(c)reate_mode: mode_name tag1 tag2 ... [-(o)pen_as format] [-(s)how_(t)ags] [-(s)how_(m)etadata] [-(s)how_table_of_(c)ontent] [-(s)how_(d)ata] ";
                    logger << "[-(s)how_(h)ide_(d)ate] [-(s)how_open_(i)mages] [-(s)how_open_as_(html)] [-(s)how_open_as_(m)ark(d)own] [-(s)how_open_as_(docx)] ";
                    logger << "[-(s)how_open_as_(pdf)] [-(s)how_open_as_la(tex)] [-(d)etail_(t)ags] [-(d)etail_(p)ath] [-(d)etail_(l)ong_(p)ath] ";
                    logger << "[-(d)etail_(l)ast_(m)odified] [-(d)etail_(c)ontent]\n\n";
                    logger << "A mode allows you to set the standard behavior of the (s)how and (d)etail command, as well as influence the (n)ew, (f)ind and (f)ilter commands. ";
                    logger << "Options starting with show in the name control the (s)how command and options starting with detail control the (d)etails command. All set ";
                    logger << "options will always be added to the commands when the mode is active. The mode tags will be added to the tags that are manually specified in the (n)ew command ";
                    logger << "and are also added to the (f)ind and (f)ilter command in the -(c)ontails_(a)ll_(t)ags parameter, meaning only notes contraining all mode tags are selected.\n";
                    logger << "Modes can be activated by the (act)ivate commandand and deactivated by the ";
                    logger << "(deac)tivate command. Additionally, an existing mode can be edited by the (edit)_mode command and deleted by the (delete)_mode command. ";
                    logger << "An overview over all modes is given by the (modes) command, which lists the name, open format, tags and all options of all modes.\n\n";
                }
                else if (argument == "del" || argument == "delete_mode") {
                    logger << "(del)ete_mode: mode_name\n";
                    logger << "Deletes the specified mode. More about modes can be found by running help create_mode.\n\n";
                }
                else if (argument == "act" || argument == "activate" || argument == "activate_mode") {
                    logger << "(act)ivate_mode: mode_name\n";
                    logger << "Activates the specified mode, such that all set options are now in effect. More about modes can be found by running help create_mode\n\n";
                }
                else if (argument == "deac" || argument == "deactivate" || argument == "deactivate_mode") {
                    logger << "(deac)tivate_mode:\n";
                    logger << "Deactivate the current mode and uses standard options again. More about modes can be found by running help create_mode\n\n";
                }
                else if (argument == "modes") {
                    logger << "(modes):\n";
                    logger << "Gives an overview over all existing modes and shows mode name, mode tags, default open format, mode options.\n\n";
                }
                else if (argument == "edit" || argument == "edit_mode") {
                    logger << "(edit)_mode: mode_name [-(add_opt)ion opt1 opt2 ...] [-(remove_opt)ion opt1 opt2 ...] [-change_format open_as_format] [-(add_t)ags tag1 tag2 ...] [-(remove_t)ags tag1 tag2] [-change_name new_mode_name]\n";
                    logger << "Where opt are the options from the (c)reate_mode command. More about modes can be found by running help create_mode\n\n";
                }
                else if (argument == "u" || argument == "update") {
                    logger << "(u)pdate:\n";
                    logger << "Updates the tag list, tag statistics and finds externally added files. Restarting the programm does the same trick.\n\n";
                }
                else if (argument == "o" || argument == "open") {
                    logger << "(o)pen:\n";
                    logger << "Opens the current selection of notes as markdown files, which can be edited by the editor of choice.\n\n";
                }
                else {
                    logger << "command " << argument << " could not be found. It should be one of the following:\n";
                    logger << "(n)ew, (f)ind, (f)ilter, (s)how, (a)dd_data, (d)etails, (t)ags, (q)uit, (c)reate_mode, (del)ete_mode, (act)ivate_mode, (deac)tivate_mode, (modes), (edit)_mode, (u)pdate and (o)pen.\n";
                    logger << "Detailed information about a command can be found by running: help command.\n\n";
                }
            }
        
        
        
        }
        else {
            logger << "Command " << command << " could not be parsed. Available options are: (n)ew, (f)ind, (f)ilter, (s)how, (a)dd_data, (d)etails, (t)ags, (u)pdate, (e)xport and (o)pen." << 'n';
        }
        logger << endl;

    }
    
    
   
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
