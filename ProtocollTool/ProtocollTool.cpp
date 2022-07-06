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

using namespace std;

int main()
{

    // TODO: Add file system watcher for updating the state when files are changed:
    // https://docs.microsoft.com/en-us/previous-versions/chzww271(v=vs.140)?redirectedfrom=MSDN
    cout << "Welcome to the Protocol Tool!" << '\n';
    cout << "-----------------------------" << '\n';
    Config conf;

    try {
        conf = Config("D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\para.conf");
        cout << "Loaded the configuration file from " << "D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\para.conf" << '\n';
    }
    catch (IOException& e) {
        string config_path;
        cout << "Error " << e.what() << " while loading the configuration file from " << "D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\para.conf" << '\n';
        cout << "Please specify the path where the configuration file (para.conf) can be found:" << endl;
        
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
                cout << "Error " << e2.what() << " while loading the configuration file from " << "D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\para.conf" << '\n';
                cout << "Please specify the path where the configuration file (para.conf) can be found:" << endl;
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
        for (auto i = 0; i < num_modes; i++)
        {
            conf.get("MODE_" + to_string(i) + "_NAME", tmp_mode_name);
            tmp_mode_name = trim(tmp_mode_name);
            mode_names[i] = tmp_mode_name;
        }
    }
    Log logger(paths.base_path / log_path, write_log);
    
    if (!filesystem::exists(paths.base_path))
    {
        bool found_base_path = false;
        while (!found_base_path)
        {
            string base_path_str;
            logger << "No path for the note files was set. Set to an existing directory to include those notes or chose a new, empty directory" << endl;
            cin >> base_path_str;
            cin.clear();
            cin.ignore(10000, '\n');
            logger.input(base_path_str);

            if (filesystem::create_directories(base_path_str)) // created new folder
            {
                logger << "Created a new folder at " << base_path_str << endl;
                found_base_path = true;
            }
            else { // use existing folder
                logger << "Found the folder " << base_path_str << endl;
                bool invalid_files = false;
                for (const auto& entry : filesystem::directory_iterator(filesystem::path(base_path_str)))
                {
                    if (entry.path().extension() != ".md" || entry.path().stem().string().size() != 9)
                    {
                        logger << "It seems as if invalid files are in this folder!\nFound the file " << entry.path().filename() << " in the folder, which is not an appropiate note markdown file" << endl;
                        invalid_files = true;
                        break;
                    }
                }
                if (!invalid_files)
                {
                    found_base_path = true;
                    break;
                }
            }

            conf.set("BASE_PATH", base_path_str);
            paths.base_path = filesystem::path(base_path_str);
        }
        
    }
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
    vector<string> mode_tags;

    filter_selection.reserve(tag_map.size());
    for (const auto& [path, tag] : tag_map)
    {
        filter_selection.push_back(path);
    }
    map<string, int> tag_count = get_tag_count(tag_map, filter_selection);

    bool running = true;
    while (running)
    {
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
            create_mode(logger, iss, conf, num_modes, mode_names, mode_tags, active_mode, open_mode);
        }
        else if (command == "del" || command == "delete" || command == "delete_mode")
        {
            delete_mode(logger, iss, conf, num_modes, mode_names, mode_tags, active_mode, open_mode);
        }
        else if (command == "act" || command == "activate" || command == "activate_mode")
        {
            activate_mode(logger, iss, conf, mode_names, active_mode, mode_tags, open_mode);
        }
        else if (command == "deac" || command == "deactivate" || command == "deactivate_mode")
        {
            active_mode = -1;
            mode_tags = vector<string>();
            int open_mode_int;
            conf.get("DEFAULT_OPEN_MODE", open_mode_int);
            open_mode = static_cast<OPEN_MODE>(open_mode_int);
        }
        else if (command == "modes" ||  command == "show_modes")
        {
            show_modes(logger, iss, conf, mode_names, active_mode, open_mode);
        }
        else if (command == "edit_mode" || command == "edit")
        {
            edit_mode(logger, iss, conf, mode_names, mode_tags, active_mode, open_mode);
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
        logger << "(n)ew, (f)ind, (f)ilter, (s)how, (a)dd_data, (d)etails, (t)ags, (q)uit, (c)reate_mode, (del)ete_mode, (act)ivate_mode, (deac)tivate_mode, (modes), (u)pdate and (o)pen.\n\n";
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
        }
        
        
        // TODO: Finish help, add help to individual commands
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
