#include <fstream>
#include <iostream>
#include <sstream>

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
        istringstream conf_pair(line);
        conf_pair >> _name >> _value;
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

            // separate name and value of the parameter
            istringstream iss(line);
            string name, value;
            if (!(iss >> name >> value)) {
                cout << "Error in reading line " << line_counter << ": " << line << '\n' << "Skipp to next line...";
                line_counter += 1;
                continue;
            }

            // check conversion to int
            if (str2int(res_int, value.c_str()) == CONV_SUCCESS) {
                this->para_int.push_back(make_tuple(name, res_int));
                line_counter += 1;
                continue;
            }

            // check conversion to float
            if (str2float(res_float, value.c_str()) == CONV_SUCCESS) {
                this->para_float.push_back(make_tuple(name, res_float));
                line_counter += 1;
                continue;
            }

            // if nothing worked, add as string
            this->para_string.push_back(make_tuple(name, value));
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
    for (auto const& t : as_const(this->para_int)) {
        if (std::get<0>(t) == name) {
            value = std::get<1>(t);
            return CONF_SUCCESS;
        }
    }
    return CONF_NOT_FOUND;
}

CONFIG_ERROR Config::get(const string& name, bool& value)
{
    for (const tuple<string, int>& t : as_const(this->para_int)) {
        if (std::get<0>(t) == name) {
            value = std::get<1>(t);
            return CONF_SUCCESS;
        }
    }
    return CONF_NOT_FOUND;
}

CONFIG_ERROR Config::get(const string& name, float& value)
{
    for (auto const& t : as_const(this->para_float)) {
        if (std::get<0>(t) == name) {
            value = std::get<1>(t);
            return CONF_SUCCESS;
        }
    }
    return CONF_NOT_FOUND;
}

CONFIG_ERROR Config::get(const string& name, string& value)
{
    for (auto const& t : as_const(this->para_string)) {
        if (std::get<0>(t) == name) {
            value = std::get<1>(t);
            return CONF_SUCCESS;
        }
    }
    return CONF_NOT_FOUND;
}

CONFIG_ERROR Config::set(const string& name, int value)
{
    this->para_int.push_back(make_tuple(name, value));
    string value_str = to_string(value);
    return set_file(name, value_str);
}

CONFIG_ERROR Config::set(const string& name, bool value)
{
    this->para_int.push_back(make_tuple(name, value));
    string value_str = to_string(value);
    return set_file(name, value_str);
}

CONFIG_ERROR Config::set(const string& name, float value)
{
    this->para_float.push_back(make_tuple(name, value));
    string value_str = to_string(value);
    return set_file(name, value_str);
}

CONFIG_ERROR Config::set(const string& name, std::string value)
{
    this->para_string.push_back(make_tuple(name, value));
    return set_file(name, value);
}

CONFIG_ERROR Config::remove(const std::string& name)
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

