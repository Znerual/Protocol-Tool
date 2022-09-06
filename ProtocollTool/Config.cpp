#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

#include "Config.h"
#include "conversions.h"

using namespace std;

CONFIG_ERROR Config::set_file(const std::string& name, std::string& value)
{
    ostringstream of;

    // read config file
    ifstream ifile(this->filepath);
    if (!ifile.is_open())
    {
        return CONF_NOT_FOUND;
    }
    string line;
    string _name, _value;
    bool found = false;
    while (getline(ifile, line))
    {
        size_t pos_del = line.find(' ');
        if (pos_del != string::npos) {
            _name = line.substr(0, pos_del);
            _value = line.substr(pos_del + 1);
        }
        else {
            _name = line;
            _value = "";
        }
       
        if (_name == name) // found value to edit
        {
            of << name << " " << value << '\n';
            found = true;
        }
        else {
            of << line << '\n'; // other parameter
        }
    }
    if (!found) // parameter not present in file
    {
        of << name << " " << value << endl;
    }

    of << flush;
    ifile.close();

    ofstream ofile(this->filepath);
    if (!ofile.is_open())
    {
        return CONF_NOT_FOUND;
    }
    ofile << of.str();
    ofile << flush;
    ofile.close();

    return CONF_SUCCESS;
}

Config::Config(const string& filepath)
{
    if (filepath == "")
        throw IOException();

    this->filepath = filepath;
    ifstream file(filepath);
    string line;
    if (file.is_open()) {

        // results of conversion
        int res_int;
        float res_float;


        int line_counter = 0;
        while (getline(file, line))
        {
            line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
            // separate name and value of the parameter
            string name, value;
            
            auto pos = line.find(" ");
            if (pos == string::npos) {
                cout << "Error in reading line " << line_counter << ": " << line << '\n' << "Skipp to next line...";
                line_counter += 1;
                continue;
            }
            name = line.substr(0, pos);
            value = line.substr(pos + 1);

            // empty entry
            if (value.empty()) {
                this->para_string[name] = "";
                line_counter += 1;
                continue;
            }
           
            // check conversion to int
            if (str2int(res_int, value.c_str()) == CONV_SUCCESS) {
                this->para_int[name] = res_int;
                line_counter += 1;
                continue;
            }

            // check conversion to float
            if (str2float(res_float, value.c_str()) == CONV_SUCCESS) {
                this->para_float[name] = res_float;
                line_counter += 1;
                continue;
            }

            // if nothing worked, add as string
            // convert paths to make system invariant
            #ifdef _WIN32
            std::replace(value.begin(), value.end(), '/', '\\');
            #else
            std::replace(value.begin(), value.end(), '\\', '/');
            #endif

            this->para_string[name] = value;
            line_counter += 1;

        }
        file.close();
    }
    else {
        cout << "Error opening the configuration file" << endl;
        throw IOException();
    }
}


CONFIG_ERROR Config::get(const string& name, int& value)
{
    if (this->para_int.contains(name)) {
        value = this->para_int[name];
        return CONF_SUCCESS;
    }
    
    return CONF_NOT_FOUND;
}

CONFIG_ERROR Config::get(const string& name, bool& value)
{
    if (this->para_int.contains(name)) {
        value = static_cast<bool>(this->para_int[name]);
        return CONF_SUCCESS;
    }

    return CONF_NOT_FOUND;
}

CONFIG_ERROR Config::get(const string& name, float& value)
{
    if (this->para_float.contains(name)) {
        value = this->para_float[name];
        return CONF_SUCCESS;
    }

    return CONF_NOT_FOUND;
}

CONFIG_ERROR Config::get(const string& name, string& value)
{
    if (this->para_string.contains(name)) {
        value = this->para_string[name];
        return CONF_SUCCESS;
    }

    return CONF_NOT_FOUND;
}

CONFIG_ERROR Config::set(const string& name, int value)
{
    this->para_int[name] = value;
    string value_str = to_string(value);
    return set_file(name, value_str);
}

CONFIG_ERROR Config::set(const string& name, bool value)
{
    this->para_int[name] = static_cast<int>(value);
    string value_str = to_string(value);
    return set_file(name, value_str);
}

CONFIG_ERROR Config::set(const string& name, float value)
{
    this->para_float[name] = value;
    string value_str = to_string(value);
    return set_file(name, value_str);
}

CONFIG_ERROR Config::set(const string& name, std::string value)
{
    this->para_string[name] = value;
    return set_file(name, value);
}

CONFIG_ERROR Config::remove(const std::string& name)
{
    if (this->para_int.contains(name)) {
        this->para_int.erase(name);
    }
    else if (this->para_float.contains(name)) {
        this->para_float.erase(name);
    }
    else if (this->para_string.contains(name)) {
        this->para_string.erase(name);
    }
    else {
        return CONF_NOT_FOUND;
    }

    ostringstream of;

    // read config file
    ifstream ifile(this->filepath);
    if (!ifile.is_open())
    {
        return CONF_NOT_FOUND;
    }
    string line;
    string _name, _value;
    bool found = false;
    while (getline(ifile, line))
    {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        istringstream conf_pair(line);
        conf_pair >> _name >> _value;
        if (_name == name) // found value to edit
        {
            found = true;
        }
        else {
            of << line << '\n'; // other parameter
        }
    }
    if (!found) // parameter not present in file
    {
        ifile.close();
        return CONF_NOT_FOUND;
    }

    of << flush;
    ifile.close();

    ofstream ofile(this->filepath);
    if (!ofile.is_open())
    {
        return CONF_NOT_FOUND;
    }
    ofile << of.str();
    ofile << flush;
    ofile.close();

 

    return CONF_SUCCESS;
}

