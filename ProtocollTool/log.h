#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include "conversions.h"



void activateVirtualTerminal();


enum COLORS {
	NC = -1,
	BLACK,
	RED,
	GREEN,
	YELLOW,
	BLUE,
	MAGENTA,
	CYAN,
	WHITE,
};

/**
* Colorize terminal colors ANSI escape sequences.
*
* @param font font color (-1 to 7), see COLORS enum
* @param back background color (-1 to 7), see COLORS enum
* @param style font style (1==bold, 4==underline)
**/
const char* colorize(int font, int back = -1, int style = -1);

/**
* Create a logger that outputs to cout and saves the output to a file
*
* @param path path to the logfile (filesystem::path)
* @param log_to_file activate the logging to a file 
**/
class Log
{
public:
	Log(std::filesystem::path path, bool log_to_file = true) : my_fstream(path, std::fstream::in | std::fstream::out | std::fstream::app), write_to_file(log_to_file), color(colorize(BLACK, WHITE))
	{
		if (!my_fstream) {
			std::cerr << "Error opening the log file " << path.string() << std::endl;
		}
		if (write_to_file) {
			time_t now = time(NULL);
			std::string now_str;
			date2str_long(now_str, now);
			my_fstream << "\n------------------------------------\n" << now_str << "\n------------------------------------\n";
		}
		
	}
	~Log() { my_fstream.close(); }
	template<typename T> Log& operator<<(const T& something)
	{
		std::cout << color << something;
		if (this->write_to_file)
			my_fstream << something;
		return *this;
	}

	template<typename T> Log& input(const T& something)
	{
		if (this->write_to_file)
			my_fstream << something << '\n';
		return *this;
	}
	// for manipulators like std::endl
	typedef std::ostream& (*stream_function)(std::ostream&);
	Log& operator<<(stream_function func)
	{
		func(std::cout);
		if (this->write_to_file)
			func(my_fstream);
		return *this;
	}
	/**
	* Colorize terminal colors ANSI escape sequences.
	*
	* @param font font color (-1 to 7), see COLORS enum
	* @param back background color (-1 to 7), see COLORS enum
	* @param style font style (1==bold, 4==underline)
	**/
	void setColor(int font, int back = -1, int style = -1) {
		this->color = colorize(font, back, style);
	}
	const char* color;
private:
	std::ofstream my_fstream;
	bool write_to_file;
	
};

