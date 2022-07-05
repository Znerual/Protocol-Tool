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
        cout << "Error while loading the configuration file from " << "D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\para.conf" << '\n';
        cout << "Please specify the path where the configuration file (para.conf) can be found:" << endl;
        cin >> config_path;
        conf = Config(config_path);
    }

    
    // read in config arguments and setup paths
    bool ask_pandoc, has_pandoc;
    int  num_modes;
    OPEN_MODE open_mode;
    unordered_map<int, string> mode_names;
    filesystem::path base_path, file_path, data_path, tmp_path;
    string file_ending, tmp_filename;
    {
        string base_path_str, file_path_str, data_path_str, tmp_path_str, tmp_node_name;
        int open_mode_int;
        conf.get("ASK_PANDOC", ask_pandoc);
        conf.get("HAS_PANDOC", has_pandoc);

        conf.get("BASE_PATH", base_path_str);
        conf.get("FILE_PATH", file_path_str);
        conf.get("DATA_PATH", data_path_str);
        conf.get("TMP_PATH", tmp_path_str);
        base_path = filesystem::path(base_path_str);
        file_path = filesystem::path(file_path_str);
        data_path = filesystem::path(data_path_str);
        tmp_path = filesystem::path(tmp_path_str);

        conf.get("TMP_FILENAME", tmp_filename);
        conf.get("FILE_ENDING", file_ending);

        conf.get("DEFAULT_OPEN_MODE", open_mode_int);
        open_mode = static_cast<OPEN_MODE>(open_mode_int);

        conf.get("NUM_MODES", num_modes);
        for (auto i = 0; i < num_modes; i++)
        {
            conf.get("MODE_" + to_string(i) + "_NAME", tmp_node_name);
            mode_names[i] = tmp_node_name;
        }
    }

    
    if (!filesystem::exists(base_path))
    {
        bool found_base_path = false;
        while (!found_base_path)
        {
            string base_path_str;
            cout << "No path for the note files was set. Set to an existing directory to include those notes or chose a new, empty directory" << endl;
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
                for (const auto& entry : filesystem::directory_iterator(filesystem::path(base_path_str)))
                {
                    if (entry.path().extension() != ".md" || entry.path().stem().string().size() != 9)
                    {
                        cout << "It seems as if invalid files are in this folder!\nFound the file " << entry.path().filename() << " in the folder, which is not an appropiate note markdown file" << endl;
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
            base_path = filesystem::path(base_path_str);
        }
        
    }
    cout << "The base path for all files is: " << base_path.string() << ".\nThe path to the notes is " << file_path.string() << " and to the data is " << data_path.string() << '\n' << endl;
    
    if (!filesystem::exists(base_path / file_path))
    {
        cout << "No folder found at the file path " << base_path / file_path << ".\nCreating a new folder..." << endl;
        filesystem::create_directories(base_path / file_path);
    }
    if (!filesystem::exists(base_path / data_path))
    {
        cout << "No folder found at the data path " << base_path / data_path << ".\nCreating a new folder..." << endl;
        filesystem::create_directories(base_path / data_path);
    }
    if (!filesystem::exists(base_path / tmp_path))
    {
        cout << "No folder found at the tmp path " << base_path / tmp_path << ".\nCreating a new folder..." << endl;
        filesystem::create_directories(base_path / tmp_path);
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
        cout << "Could not find pandoc. It is not required but highly recommended for this application, since html, tex and pdf output is only possible with the package.\n";
        cout << "Do you want to install it? (y/n) or never ask again (naa)?" << endl;
        cout << "The program needs to be restarted after the installation and will close when you decide to install pandoc." << endl;
        string choice;
        cin >> choice;
        cin.clear();
        cin.ignore(100000, '\n');
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
    
    
    /*
    array<char, 128> buffer;
    string result;

    while (fgets(buffer.data(), 128, pipe) != NULL)
    {
        result += buffer.data();
    }
    */
    //int returnCode = _pclose(pipe);
    //cout << "Process ended with " << returnCode << " and wrote " << result << endl;


    vector<string> filter_selection;
    map<string, time_t> file_map = list_all_files(base_path.string(), file_path.string());
    map<string, vector<string>> tag_map = list_all_tags(base_path.string(), file_path.string());
    map<string, int> tag_count = get_tag_count(tag_map);
    int active_mode = -1;
    vector<string> mode_tags;

    bool running = true;
    while (running)
    {
        cout << "Input";
        if (active_mode != -1)
        {
            cout << " (" << mode_names[active_mode] << ")";
        }
        cout << ":";

        string input, command;
        getline(cin, input);

        istringstream iss(input);
        iss >> command;
        boost::algorithm::to_lower(command);

        if (command == "n" || command == "new") 
        {
            add_note(iss, base_path, file_path, data_path, file_ending, file_map, tag_map, mode_tags);
        }
        else if (command == "f") { // could be find or filter
            if (filter_selection.size() == 0) {
                find_notes(iss, base_path.string(), file_path.string(), data_path.string(), filter_selection, file_map, tag_map, mode_tags);
                cout << "Found " << filter_selection.size() << " matching notes.\nRun (s)how, (d)etails, (a)dd data or (f)ilter." << endl;
            }
            else {
                filter_notes(iss, base_path.string(), file_path.string(), data_path.string(), filter_selection, file_map, tag_map, mode_tags);
                cout << "Found " << filter_selection.size() << " notes matching the filter.\nRun (s)how, (d)etails, (a)dd data or (f)ilter." << endl;
            }
            
        }
        else if (command == "find") {
            find_notes(iss, base_path.string(), file_path.string(), data_path.string(), filter_selection, file_map, tag_map, mode_tags);
            cout << "Found " << filter_selection.size() << " matching notes.\nRun (s)how, (d)etails, (a)dd data or (f)ilter." << endl;
        }
        else if (command == "filter") {
            filter_notes(iss, base_path.string(), file_path.string(), data_path.string(), filter_selection, file_map, tag_map, mode_tags);
            cout << "Found " << filter_selection.size() << " notes matching the filter.\nRun (s)how, (d)etails, (a)dd data or (f)ilter." << endl;
        }
        else if (command == "s" || command == "show") 
        {
            show_filtered_notes(iss, open_mode, base_path, file_path, data_path, tmp_path, tmp_filename, filter_selection, has_pandoc);
            
        }
        else if (command == "a" || command == "add_data") {

            add_data(iss, base_path, data_path, filter_selection);
           
        }
        else if (command == "d" || command == "details") {
            show_details(iss, base_path, file_path, data_path, filter_selection, file_map, tag_map);
        }
        else if (command == "t" || command == "tags") 
        {
            cout << "Tags: \n";
            for (const auto& [tag, count] : tag_count)
            {
                cout << tag << ": " << count << '\n';
            }

        }
        else if (command == "q" || command == "quit" || command == "exit") 
        {
            running = false;
            break;
        }
        else if (command == "c" || command == "create" || command == "create_mode")
        {
            create_mode(iss, conf, num_modes, mode_names, mode_tags, active_mode, open_mode);
        }
        else if (command == "del" || command == "delete" || command == "delete_mode")
        {
            delete_mode(iss, conf, num_modes, mode_names, mode_tags, active_mode, open_mode);
        }
        else if (command == "act" || command == "activate" || command == "activate_mode")
        {
            activate_mode(iss, conf, mode_names, active_mode, mode_tags, open_mode);
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
            cout << "Mode name:\tMode tags:\tOpen as default:\n";
            for (const auto& [id, name] : mode_names)
            {
                cout << name << ": ";

                int num_tags;
                conf.get("MODE_" + to_string(id) + "_NUM_TAGS", num_tags);

                string tag;
                for (auto i = 0; i < num_tags; i++) {
                    conf.get("MODE_" + to_string(id) + "_TAG_" + to_string(i), tag);
                    cout << tag << " ";
                }

                int open_as_int;
                conf.get("MODE_" + to_string(id) + "_OPEN_AS", open_as_int);
                switch (static_cast<OPEN_MODE>(open_as_int))
                {
                case HTML:
                    cout << ", html";
                    break;
                case PDF:
                    cout << ", pdf";
                    break;
                case DOCX:
                    cout << ", docx";
                    break;
                case LATEX:
                    cout << ", tex";
                    break;
                case MARKDOWN:
                    cout << ", markdown";
                    break;
                default:
                    cout << ", unknown format";
                    break;
                }
                cout << endl;
            }
        }
        else if (command == "u" || command == "update")
        {
            update_tags(base_path.string(), file_path.string(), file_map, tag_map, tag_count, filter_selection);
        }
        else if (command == "o" || command == "open")
        {
            open_selection(base_path, filter_selection);
            
        }
        else if (command == "h" || command == "-h" || command == "help")
        {
        cout << "This programm can be used by typing one of the following commands, where (...) indicates that the expression ";
        cout << "inside the parenthesis can be used as an abbreviation of the command or parameter.\n";
        cout << "(n)ew, (f)ind, (f)ilter, (s)how, (a)dd_data, (d)etails, (t)ags, (q)uit, (c)reate_mode, (del)ete_mode, (act)ivate_mode, (deac)tivate_mode, (modes), (u)pdate and (o)pen.\n\n";
        string argument;
        if (iss) 
        {
            iss >> argument;
            if (argument == "n" || argument == "new")
            {
                cout << "(n)ew: date tag1 tag2 ... [-d]\nCreates a new note file and the optional parameter -d indicates that a data folder will be created. The given date can be in ";
                cout << "the format of dd or dd.mm or dd.mm.yyyy (where one digit numebers can be specified as one digit or two digits with leading zero) or the special format t for today and ";
                cout << "y for yesterday.\n\n";
            }
            else if (argument == "f") {
                cout << "The abbreviation f will be interpreted as find if no note is currently selected and as filter, if multiple (or one) note(s) are selected.\n\n";
            } 
            else if (argument == "find") {
                cout << "(f)ind: [-(d)ate start_date-end_date] [-(c)ontains_(t)ags tag1 ...] [-(c)ontains_(a)ll_(t)ags tag1 ...] [-(n)o_(t)ags tag1 ...] [-(r)eg_(t)ext text] [-(v)ersions start_version-end_version] [-(d)ata_(o)nly]\n";
                cout << "Looks for notes that fall within the specified ranges (start and end dates are included in the interval). -ct describes a list of tags where one of them ";
                cout << "needs to be found in the note, while -cat describes a list of tags which all need to be find in the note. -nt describes a list of tags that are not allowed ";
                cout << "to be in matching notes and -rt stands for regular expression text, which will be used to search the content of the note. -v gives an interval of versions (a-...) ";
                cout << "that are considered and -do stands for data_only, meaning only notes with a data folder will be selected.\n\n";
            }
            else if (argument == "filter") {
                cout << "(f)ilter: [-(d)ate start_date-end_date] [-(c)ontains_(t)ags tag1 ...] [-(c)ontains_(a)ll_(t)ags tag1 ...] [-(n)o_(t)ags tag1 ...] [-(r)eg_(t)ext text] [-(v)ersions start_version-end_version] [-(d)ata_(o)nly]\n";
                cout << "Reduces the current selection of notes with parameter similar to find. Also, intevals contain the start and end values. -ct describes a list of tags where one of them ";
                cout << "needs to be found in the note, while -cat describes a list of tags which all need to be find in the note. -nt describes a list of tags that are not allowed ";
                cout << "to be in matching notes and -rt stands for regular expression text, which will be used to search the content of the note. -v gives an interval of versions (a-...) ";
                cout << "that are considered and -do stands for data_only, meaning only notes with a data folder will be selected.\n\n";
            }
            else if (argument == "s" || argument == "show") {
                cout << "(s)how: [(t)ags] [(m)etadata] [table_of_(c)ontent] [(d)ata] [(h)ide_(d)ate] [open_(i)mages] [open_as_(html)] [open_as_(tex)] [open_as_(pdf)] [open_as_(docx)] [open_as_(m)ark(d)own]\n";
                cout << "Shows the current note selection (created by find and filter) in various formats (specified by the open_as_... parameter), where by default only the date and content of the note are shown.";
                cout << "The parameters (t)ags, (m)etadata and table_of_(c)ontent add additional information to the output and (d)ata opens the data folders in the explorer. ";
                cout << "The command open_(i)mages additionally opens .jpg, .jpeg and .png files inside the data folders directly.\n\n";
            }
        }
        
        
        // TODO: Finish help, add help to individual commands
        }
        else {
            cout << "Command " << command << " could not be parsed. Available options are: (n)ew, (f)ind, (f)ilter, (s)how, (a)dd_data, (d)etails, (t)ags, (u)pdate, (e)xport and (o)pen." << 'n';
        }

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
