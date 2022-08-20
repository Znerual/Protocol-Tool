#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <array>
#include <set>
#include <regex>

#ifdef _WIN32
#include <shlwapi.h>
#include <WinUser.h>
#include "file_manager.h"
#else
#include "../ProtocollToolLinux/file_manager_linux.h"
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#endif

#include "utils.h"
#include "conversions.h"
#include "Config.h"
#include "trietree.h"

using namespace std;

template<typename T>
void pad(basic_string<T>& s, typename basic_string<T>::size_type n, T c, const bool cap_right, const ALIGN align) {
	if (n > s.length()) {
		switch (align)
		{
		case LEFT:
			s.append(n - s.length(), c);
			break;
		case RIGHT:
			s.insert(0, n - s.length(), c);
			break;
		case MIDDLE:
			s.insert(0, (n - s.length()) / 2, c);
			s.append((n - s.length()) - (n - s.length()) / 2, c);
			break;
		default:
			break;
		}
		
	}
	else {
		if (cap_right) {
			s = s.substr(0, n - 2) + "..";
		}
		else {
			s = ".." + s.substr(s.length() - n - 1);
		}
	}
}

std::string wrap(const std::string s, const int margin, const ALIGN align) {
	string res = s;
	int width, height;
	get_console_size(height, width);
	if (s.size() + 2 * margin > width) {
		// wrap
		istringstream iss(s);
		ostringstream oss;
		string word, margin_str(margin, ' ');
		int pos = margin;
		oss << margin_str;
		while (iss >> word) {
			if (pos + word.size() < width - margin)
			{
				oss << word << " ";
				pos += word.size() + 1;
			}
			else {
				oss << '\n' << margin_str << word << " ";
				pos = margin + word.size() + 1;
			}
		}
		res = oss.str();
	}
	else {
		// pad
		
		res.insert(0, margin, ' ');
		pad(res, width - 2 * margin, ' ', true, align);
		res.append(margin, ' ');
	}

	return res;
}


std::string ltrim(const std::string& s, const string& characters)
{
	size_t start = s.find_first_not_of(characters);
	return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(const std::string& s, const string& characters)
{
	size_t end = s.find_last_not_of(characters);
	return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string& s, const string& characters) {
	return rtrim(ltrim(s, characters), characters);
}


void load_config(const std::string init_path, Config& conf)
{
	try {
		//conf = Config("D:\\Code\\C++\\VisualStudioProjects\\ProtocollTool\\ProtocollTool\\para.conf");
		conf = Config(init_path);
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

}
void check_base_path(Config& conf, PATHS& paths)
{
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
			else 
			{ // use existing folder
				cout << colorize(BLACK, WHITE) << "  Found the folder " << base_path_str << endl;

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
}
void check_standard_paths(PATHS& paths)
{
	if (!filesystem::exists(paths.base_path / paths.file_path))
	{
		cout << colorize(YELLOW, WHITE) << "  No folder found at the file path " << paths.base_path / paths.file_path << ".\n  Creating a new folder..." << endl;
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
}

#ifdef _WIN32
void get_console_size(int& rows, int& columns)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

}
#else


void get_console_size(int& rows, int& columns)
{
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	rows = w.ws_row;
	columns = w.ws_col;
}
#endif

void add(FORMAT_OPTIONS& to, FORMAT_OPTIONS& from) {
	if (from.docx)
		to.docx = true;
	if (from.pdf)
		to.pdf = true;
	if (from.markdown)
		to.markdown = true;
	if (from.tex)
		to.tex = true;
	if (from.html)
		to.html = true;
	
}

void add(SHOW_OPTIONS& to, SHOW_OPTIONS& from) {
	if (from.show_data)
		to.show_data = true;
	if (from.show_metadata)
		to.show_metadata = true;
	if (from.show_tags)
		to.show_tags = true;
	if (from.show_table_of_content)
		to.show_table_of_content = true;
	if (from.hide_date)
		to.hide_date = true;
	if (from.open_image_data)
		to.open_image_data = true;
	

}

void add(DETAIL_OPTIONS& to, DETAIL_OPTIONS& from) {
	if (from.detail_content)
		to.detail_content = true;
	if (from.detail_path)
		to.detail_path = true;
	if (from.detail_long_path)
		to.detail_long_path = true;
	if (from.detail_last_modified)
		to.detail_last_modified = true;
	if (from.detail_tags)
		to.detail_tags = true;

}

void remove(FORMAT_OPTIONS& to, FORMAT_OPTIONS& from) {
	if (from.docx)
		to.docx = false;
	if (from.pdf)
		to.pdf = false;
	if (from.markdown)
		to.markdown = false;
	if (from.tex)
		to.tex = false;
	if (from.html)
		to.html = false;

}

void remove(SHOW_OPTIONS& to, SHOW_OPTIONS& from) {
	if (from.show_data)
		to.show_data = false;
	if (from.show_metadata)
		to.show_metadata = false;
	if (from.show_tags)
		to.show_tags = false;
	if (from.show_table_of_content)
		to.show_table_of_content = false;
	if (from.hide_date)
		to.hide_date = false;
	if (from.open_image_data)
		to.open_image_data = false;


}

void remove(DETAIL_OPTIONS& to, DETAIL_OPTIONS& from) {
	if (from.detail_content)
		to.detail_content = false;
	if (from.detail_path)
		to.detail_path = false;
	if (from.detail_long_path)
		to.detail_long_path = false;
	if (from.detail_last_modified)
		to.detail_last_modified = false;
	if (from.detail_tags)
		to.detail_tags = false;

}

void add(MODE_OPTIONS& to, MODE_OPTIONS& from) {
	add(to.format_options, from.format_options);
	add(to.detail_options, from.detail_options);
	add(to.show_options, from.show_options);
}

void remove(MODE_OPTIONS& to, MODE_OPTIONS& from) {
	remove(to.format_options, from.format_options);
	remove(to.detail_options, from.detail_options);
	remove(to.show_options, from.show_options);
}

void set_mode_options(Config& conf, MODE_OPTIONS& mode_options, const int& active_mode)
{
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_TAGS", mode_options.show_options.show_tags);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_METADATA", mode_options.show_options.show_metadata);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_TABLE_OF_CONTENT", mode_options.show_options.show_table_of_content);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_DATA", mode_options.show_options.show_data);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_HIDE_DATE", mode_options.show_options.hide_date);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_IMAGES", mode_options.show_options.open_image_data);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_HTML", mode_options.format_options.html);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_TEX", mode_options.format_options.tex);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_PDF", mode_options.format_options.pdf);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_DOCX", mode_options.format_options.docx);
	conf.set("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_MD", mode_options.format_options.markdown);

	conf.set("MODE_" + to_string(active_mode) + "_DETAIL_TAGS", mode_options.detail_options.detail_tags);
	conf.set("MODE_" + to_string(active_mode) + "_DETAIL_PATH", mode_options.detail_options.detail_path);
	conf.set("MODE_" + to_string(active_mode) + "_DETAIL_LONG_PATH", mode_options.detail_options.detail_long_path);
	conf.set("MODE_" + to_string(active_mode) + "_DETAIL_LAST_MODIFIED", mode_options.detail_options.detail_last_modified);
	conf.set("MODE_" + to_string(active_mode) + "_DETAIL_SHOW_CONTENT", mode_options.detail_options.detail_content);
}

void get_mode_options(Config& conf, MODE_OPTIONS& mode_options, const int& active_mode)
{
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_TAGS", mode_options.show_options.show_tags);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_METADATA", mode_options.show_options.show_metadata);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_TABLE_OF_CONTENT", mode_options.show_options.show_table_of_content);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_DATA", mode_options.show_options.show_data);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_HIDE_DATE", mode_options.show_options.hide_date);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_IMAGES", mode_options.show_options.open_image_data);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_HTML", mode_options.format_options.html);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_TEX", mode_options.format_options.tex);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_PDF", mode_options.format_options.pdf);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_DOCX", mode_options.format_options.docx);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_MD", mode_options.format_options.markdown);
		 
	conf.get("MODE_" + to_string(active_mode) + "_DETAIL_TAGS", mode_options.detail_options.detail_tags);
	conf.get("MODE_" + to_string(active_mode) + "_DETAIL_PATH", mode_options.detail_options.detail_path);
	conf.get("MODE_" + to_string(active_mode) + "_DETAIL_LONG_PATH", mode_options.detail_options.detail_long_path);
	conf.get("MODE_" + to_string(active_mode) + "_DETAIL_LAST_MODIFIED", mode_options.detail_options.detail_last_modified);
	conf.get("MODE_" + to_string(active_mode) + "_DETAIL_SHOW_CONTENT", mode_options.detail_options.detail_content);
}



void parse_show_args(Log& logger, std::istringstream& iss, Config& conf, int& active_mode, SHOW_OPTIONS& show_options, FORMAT_OPTIONS& format_options)
{
	

	string argument;
	while (iss >> argument)
	{
		if (argument == "t" || argument == "-t" || argument == "-tags" || argument == "-show_tags")
		{
			show_options.show_tags = true;
		}
		else if (argument == "m" || argument == "-m" || argument == "-metadata" || argument == "-show_metadata")
		{
			show_options.show_metadata = true;
		}
		else if (argument == "c" || argument == "-c" || argument == "-table_of_content" || argument == "-show_table_of_content")
		{
			if (show_options.hide_date)
			{
				logger << "Can't create a table of content when hide_date is set." << endl;
				continue;
			}

			show_options.show_table_of_content = true;
		}
		else if (argument == "d" || argument == "-d" || argument == "-data" || argument == "-show_data")
		{
			show_options.show_data = true;
		}
		else if (argument == "hd" || argument == "-hd" || argument == "-date" || argument == "-hide_date")
		{
			show_options.hide_date = true;
			if (show_options.show_table_of_content)
			{
				cout << "Can't create a table of content when hide_date is set." << endl;
				show_options.show_table_of_content = false;
			}
		}
		else if (argument == "i" || argument == "-i" || argument == "-images" || argument == "-open_images")
		{
			show_options.show_data = true;
			show_options.open_image_data = true;
		}
		else if (parse_format(logger, argument, format_options)) {
			continue;
		}
		else {
			logger << "Input argument " << argument;
			logger << " could not be parsed. Expected one of the following inputs:\n-show_(t)ags, show_(m)etadata, -show_table_of_(c)ontent, ";
			logger << "-show_(d)ata, -(h)ide_(d)ate, -open_(i)mage_data, -(html), -la(tex), -(pdf) ";
			logger << "-(docx), -(m)ark(d)own" << endl;
			return;
		}
	}

	conf.get("MODE_" + to_string(active_mode) + "_SHOW_TAGS", show_options.show_tags);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_METADATA", show_options.show_metadata);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_TABLE_OF_CONTENT", show_options.show_table_of_content);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_DATA", show_options.show_data);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_HIDE_DATE", show_options.hide_date);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_IMAGES", show_options.open_image_data);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_HTML", format_options.html);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_TEX", format_options.tex);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_PDF", format_options.pdf);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_DOCX", format_options.docx);
	conf.get("MODE_" + to_string(active_mode) + "_SHOW_OPEN_AS_MD", format_options.markdown);


	if (!format_options.docx && !format_options.html && !format_options.markdown && !format_options.pdf && !format_options.tex)
	{
		int default_open_int;
		conf.get("DEFAULT_OPEN_MODE", default_open_int);
		OPEN_MODE default_open = static_cast<OPEN_MODE>(default_open_int);
		switch (default_open)
		{
		case HTML:
			format_options.html = true;
			break;
		case MARKDOWN:
			format_options.markdown = true;
			break;
		case LATEX:
			format_options.tex = true;
			break;
		case PDF:
			format_options.pdf = true;
			break;
		case DOCX:
			format_options.docx = true;
			break;
		default:
			cout << "Error: Unknown default open mode " << default_open << endl;
			break;
		}
	}
}

bool parse_mode_option(string& argument, MODE_OPTIONS& mode_options)
{
	if (argument == "-show_tags" || argument == "-st")
	{
		mode_options.show_options.show_tags = true;
		return true;
	}
	else if (argument == "-show_metadata" || argument == "-sm")
	{
		mode_options.show_options.show_metadata = true;
		return true;
	}
	else if (argument == "-show_table_of_content" || argument == "-sc")
	{
		mode_options.show_options.show_table_of_content = true;
		return true;
	}
	else if (argument == "-show_data" || argument == "-sd")
	{
		mode_options.show_options.show_data = true;
		return true;
	}
	else if (argument == "-show_hide_date" || argument == "-shd")
	{
		mode_options.show_options.hide_date = true;
		return true;
	}
	else if (argument == "-show_open_images" || argument == "-show_images" || argument == "-si")
	{
		mode_options.show_options.open_image_data = true;
		return true;
	}
	else if (argument == "-show_open_as_html" || argument == "-shtml")
	{
		mode_options.format_options.html = true;
		return true;
	}
	else if (argument == "-show_open_as_pdf" || argument == "-spdf")
	{
		mode_options.format_options.pdf = true;
		return true;
	}
	else if (argument == "-show_open_as_markdown" || argument == "-smd")
	{
		mode_options.format_options.markdown = true;
		return true;
	}
	else if (argument == "-show_open_as_docx" || argument == "-sdocx")
	{
		mode_options.format_options.docx = true;
		return true;
	}
	else if (argument == "-show_open_as_tex" || argument == "-show_open_as_latex" || argument == "-stex")
	{
		mode_options.format_options.tex = true;
		return true;
	}
	else if (argument == "-detail_tags" || argument == "-dt")
	{
		mode_options.detail_options.detail_tags = true;
		return true;
	}
	else if (argument == "-detail_path" || argument == "-dp")
	{
		mode_options.detail_options.detail_path = true;
		return true;
	}
	else if (argument == "-detail_long_path" || argument == "-dlt")
	{
		mode_options.detail_options.detail_long_path = true;
		return true;
	}
	else if (argument == "-detail_last_modified" || argument == "-dlm")
	{
		mode_options.detail_options.detail_last_modified = true;
		return true;
	}
	else if (argument == "-detail_content" || argument == "-dc")
	{
		mode_options.detail_options.detail_content = true;
		return true;
	}
	else {
		return false;
	}

}

bool parse_folder_watcher(string& argument, FOLDER_WATCHER_MODE& mode, string& current_folder, unordered_map<string, vector<string>>& folder_watcher_tags)
{
	if (argument == "-watch_folder" || argument == "-watch" || argument == "-folder" || argument == "-wf") {
		mode = READ_FOLDER;
		current_folder = "";
		return true;
	}
	else if (mode == READ_FOLDER) {
		if (!filesystem::exists(argument)) {
			mode = READ_NONE;
			return false;
		}
		folder_watcher_tags[argument] = vector<string>();
		current_folder = argument;
		mode = READ_TAGS;
		return true;
	}
	else if (mode == READ_TAGS) {
		folder_watcher_tags[current_folder].push_back(argument);
		return true;
	}
	return false;
}

#ifdef _WIN32
void set_console_background(const int& width, const int& height) {
	DWORD dwBytesWritten;
	WORD colors[3]{ BACKGROUND_BLUE, BACKGROUND_GREEN, BACKGROUND_RED };
	FillConsoleOutputAttribute(GetStdHandle(STD_OUTPUT_HANDLE), BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED, width * height, { 0,0 }, &dwBytesWritten);
}

void set_console_font() {
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
}
#endif
void print_greetings(const int& width) {
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
#ifdef _WIN32


void get_default_applications(PATHS& paths) {
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


void get_pandoc_installed(Log& logger, Config& conf, bool& ask_pandoc, bool& has_pandoc)
{
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
}
#else
void get_pandoc_installed(Log& logger, Config& conf, bool& ask_pandoc, bool& has_pandoc) {
	// check if pandoc is installed
	FILE* pipe = popen("pandoc -v", "r");
	if (!pipe)
	{
		cerr << "Could not start command" << endl;
	}
	int returnCode = pclose(pipe);
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
			system("xdg-open https://pandoc.org/installing.html");
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
}
#endif // _WIN32
bool parse_format(Log& logger, string& argument, FORMAT_OPTIONS& format_options) {
	if (argument == "html" || argument == "-html")
	{
		format_options.html = true;
		return true;
	}
	else if (argument == "docx" || argument == "-docx") {
		format_options.docx = true;
		return true;
	}
	else if (argument == "tex" || argument == "latex" || argument == "-tex" || argument == "-latex") {
		format_options.tex = true;
		return true;
	}
	else if (argument == "pdf" || argument == "-pdf") {
		format_options.pdf = true;
		return true;
	}
	else if (argument == "md" || argument == "markdown" || argument == "-md" || argument == "-markdown") {
		format_options.markdown = true;
		return true;
	}
	else {
		logger << "Format " << argument << " was not recognized. Available options are:\n html, docx, tex, pdf, markdown." << endl;
		return false;
	}
}

void parse_find_args(Log& logger, istringstream& iss, bool& data_only, vector<time_t>& date_args, vector<string>& ctags_args, vector<string>& catags_args, vector<string>& ntags_args, string& regex, string& regex_content, vector<char>& version_args) {
	
	data_only = false;
	vector<string> _date_args;
	vector<string> _vers_args;

	string argument;
	ARGUMENT_MODE mode = NONE;
	// parse arguments
	while (iss >> argument)
	{
		if (argument == "-d" || argument == "-date")
		{
			mode = ARGUMENT_MODE::ARG_DATE;
		}
		else if (argument == "-ct" || argument == "-contains_tags")
		{
			mode = CONTAINS_TAGS;
		}
		else if (argument == "-cat" || argument == "-contains_all_tags")
		{
			mode = CONTAINS_ALL_TAGS;
		}
		else if (argument == "-nt" || argument == "-no_tags")
		{
			mode = NO_TAGS;
		}
		else if (argument == "-rt" || argument == "-reg_text")
		{
			mode = REG_TEXT;
		}
		else if (argument == "-rtc" || argument == "-reg_text_content")
		{
			mode = REG_TEXT_CONTENT;
		}
		else if (argument == "-do" || argument == "-data_only")
		{
			data_only = true;
		}
		else if (argument == "-v" || argument == "-versions")
		{
			mode = VERSIONS;
		}
		else
		{
			if (argument.size() > 1 && argument[0] == '-') {
				logger << "The argument " << argument << " was not recognized. It should be one of the following options: \n";
				logger << "-d, -date: finds notes between start and end date. \n";
				logger << "-v, -versions: selectes all notes in the given versions range. \n";
				logger << "-ct, -contains_tags: selects notes containing at least one of the specified tags.\n";
				logger << "-cat, -contains_all_tags: selectes notes containing all of the specified tags.\n";
				logger << "-nt, -no_tags: selects notes that do not contain the specified tags.\n";
				logger << "-rt, -reg_text: uses regular expressions on the note tags to select notes that match the given pattern.\n";
				logger << "-rtc, -reg_text_content: uses regular expressions on the note content to select notes that match the given pattern.\n";
				logger << "-do, -data_only: only select notes that have data associated with them." << endl;
				return;
			}
			switch (mode)
			{
			case NONE:
				logger << "Could not parse the find command " << argument << ". It should be one of the following: \n";
				logger << "-d, -date: finds notes between start and end date. \n";
				logger << "-v, -versions: selectes all notes in the given versions range. \n";
				logger << "-ct, -contains_tags: selects notes containing at least one of the specified tags.\n";
				logger << "-cat, -contains_all_tags: selectes notes containing all of the specified tags.\n";
				logger << "-nt, -no_tags: selects notes that do not contain the specified tags.\n";
				logger << "-rt, -reg_text: uses regular expressions on the note content to select notes that match the given pattern.\n";
				logger << "-do, -data_only: only select notes that have data associated with them." << endl;
				return;
			case ARGUMENT_MODE::ARG_DATE:
				_date_args.push_back(argument);
				break;
			case CONTAINS_TAGS:
				ctags_args.push_back(argument);
				break;
			case VERSIONS:
				_vers_args.push_back(argument);
				break;
			case CONTAINS_ALL_TAGS:
				catags_args.push_back(argument);
				break;
			case NO_TAGS:
				ntags_args.push_back(argument);
				break;
			case REG_TEXT:
				regex = argument;
				break;
			case REG_TEXT_CONTENT:
				regex_content = argument;
				break;
			default:
				logger << "Something went wrong. Unexpected mode in find with argument " << argument << endl;
				return;
			}
		}
	}

	
	// parse date input
	time_t start_date, end_date;
	CONV_ERROR ret1, ret2;
	size_t delim_pos;
	switch (_date_args.size())
	{
	case 0:
		// cout << "The -(d)ate argument expects input in the format of start_date-end_date or start_date to end_date, where start_data and end_date are in the form of dd, dd.mm, dd.mm.yy or special dates (t)oday, (y)esterday. \n";
		break;
	case 1:
		
		delim_pos = _date_args.at(0).find('-');

		// one date given as start and end date
		if (delim_pos == string::npos) {
			ret1 = str2date(start_date, _date_args.at(0).c_str());

			if (ret1 != CONV_SUCCESS)
			{
				logger << "The date " << _date_args.at(0) << " could not be parsed to start_date and end_date, because the separator (-) could not be found and the input is not a date." << endl;
				break;
			}
			date_args.push_back(start_date);
			date_args.push_back(start_date);
			break;
		}
		
		// dates given separated by a -
		ret1 = str2date(start_date, _date_args.at(0).substr(0, delim_pos).c_str());
		ret2 = str2date(end_date, _date_args.at(0).substr(delim_pos+1).c_str());

		if (ret1 != CONV_SUCCESS) {
			logger << "The start date " << start_date << " could not be parsed to a date." << endl;
			break;
		}
		else if (ret2 != CONV_SUCCESS) {
			logger << "The end date " << end_date << " could not be parsed to a date." << endl;
			break;
		}

		date_args.push_back(start_date);
		date_args.push_back(end_date);
		break;
	case 3:
		ret1 = str2date(start_date, _date_args.at(0).c_str());
		ret2 = str2date(end_date, _date_args.at(2).c_str());
		if (ret1 != CONV_SUCCESS) {
			logger << "The start date " << start_date << " could not be parsed to a date." << endl;
			break;
		}
		else if (ret2 != CONV_SUCCESS) {
			logger << "The end date " << end_date << " could not be parsed to a date." << endl;
			break;
		}

		date_args.push_back(start_date);
		date_args.push_back(end_date);
		break;
	default:
		logger << "Unexpected white spaces in the date parameter. Found " << _date_args.size() << " whitespaces and expects zero or two." << endl;
		return;
		
	}
	
	// parse version input
	for (const string& vers_arg : _vers_args)
	{
		if (vers_arg.size() == 1) {
			version_args.push_back(vers_arg[0]);
			continue;
		}

		if (vers_arg.size() == 3 && (vers_arg[1] == '-' || vers_arg[1] == ':'))
		{
			for (char i = vers_arg[0]; i <= vers_arg[2]; i++)
			{
				version_args.push_back(i);
			}
			continue;
		}

		logger << "Could not parse the version input " << vers_arg << ". It should either be a character, number or range in the form a-c or a:c." << endl;
		return;
	}
}
#ifdef _WIN32
void parse_create_mode(Log& logger, std::istringstream& iss, Config& conf, string& mode_name, std::vector<std::string>& mode_tags, unordered_map<string, vector<string>>& folder_watcher_tags, 
	MODE_OPTIONS& mode_options)
{
	// set default values
	mode_name = "default";
	mode_tags = vector<string>();

	if (iss)
	{
		iss >> mode_name;
		mode_name = trim(mode_name);
	}
	else {
		return;
	}

	string argument;
	string current_folder { "" };
	FOLDER_WATCHER_MODE mode {READ_NONE};
	while (iss >> argument)
	{
		boost::to_lower(argument);
		if (argument[0] == '-' || mode == READ_TAGS || mode == READ_FOLDER)
		{
			if (!parse_mode_option(argument, mode_options) && !parse_folder_watcher(argument,  mode, current_folder, folder_watcher_tags)) {
				logger << "Did not recognize parameter " << argument << ".\n It should be one of the following options controlling the (s)how command:\n";
				logger << "[-(s)how_(t)ags] [-(s)how_(m)etadata] [-(s)how_table_of_(c)ontent] ";
				logger << "[-(s)how_(d)ata] [-(s)how_(h)ide_(d)ate] [-(s)how_open_(i)mages] [-(s)how_open_as_(html)] ";
				logger << "[-(s)how_open_as_(pdf)] [-(s)how_open_as_(docx)] [-(s)how_open_as_(m)ark(d)own] [-(s)how_open_as_la(tex)]\n";
				logger << "And one of the following for controlling the (d)etails command:\n";
				logger << "[-(d)etail_tags] [-(d)etail_(p)ath] [-(d)etail_(l)ong_(p)ath] [-(d)etail_(l)ast_(m)odified] [-(d)etail_(c)ontent]\n";
				logger << "Or a path to a folder that should be observed such that when a file is created, the newly created file is added to the ";
				logger << "data folder and a new note is created. Additionally, tags that will be added when this occures can be entered: \n";
				logger << "[-(w)atch_(f)older path_to_folder tag1 tag2 ...]\n";
				logger << "Note that multiple folders can be specified by repeating the [-(w)atch_(f)older path_to_folder tag1 tag2 ...] command ";
				logger << "With different paths (and tags).\n\n";
			}
		}
		else if (argument[0] != '-') {
			mode_tags.push_back(argument);
		}
		else {
			logger << "Error in parse_create_mode, this condition should not have been reached. Input " << argument << " with current mode: " << mode_name << endl;
		}
		
	}
}

#else
void parse_create_mode(Log& logger, std::istringstream& iss, Config& conf, string& mode_name, std::vector<std::string>& mode_tags, MODE_OPTIONS& mode_options)
{
	// set default values
	mode_name = "default";
	mode_tags = vector<string>();

	if (iss)
	{
		iss >> mode_name;
		mode_name = trim(mode_name);
	}
	else {
		return;
	}

	string argument;
	string current_folder{ "" };
	while (iss >> argument)
	{
		boost::to_lower(argument);
		if (argument[0] == '-')
		{
			if (!parse_mode_option(argument, mode_options)) {
				logger << "Did not recognize parameter " << argument << ".\n It should be one of the following options controlling the (s)how command:\n";
				logger << "[-(s)how_(t)ags] [-(s)how_(m)etadata] [-(s)how_table_of_(c)ontent] ";
				logger << "[-(s)how_(d)ata] [-(s)how_(h)ide_(d)ate] [-(s)how_open_(i)mages] [-(s)how_open_as_(html)] ";
				logger << "[-(s)how_open_as_(pdf)] [-(s)how_open_as_(docx)] [-(s)how_open_as_(m)ark(d)own] [-(s)how_open_as_la(tex)]\n";
				logger << "And one of the following for controlling the (d)etails command:\n";
				logger << "[-(d)etail_tags] [-(d)etail_(p)ath] [-(d)etail_(l)ong_(p)ath] [-(d)etail_(l)ast_(m)odified] [-(d)etail_(c)ontent]\n";
				logger << "Or a path to a folder that should be observed such that when a file is created, the newly created file is added to the ";
				logger << "data folder and a new note is created. Additionally, tags that will be added when this occures can be entered: \n\n";
			}
		}
		else if (argument[0] != '-') {
			mode_tags.push_back(argument);
		}
		else {
			logger << "Error in parse_create_mode, this condition should not have been reached. Input " << argument << " with current mode: " << mode_name << endl;
		}

	}
}
#endif
void parse_details_args(Log& logger, istringstream& iss, Config& conf, int& active_mode, DETAIL_OPTIONS& detail_options)
{

	string argument;
	while (iss >> argument)
	{
		if (argument == "t" || argument == "-t" || argument == "-tags" || argument == "-show_tags")
		{
			detail_options.detail_tags = true;
		}
		else if (argument == "p" || argument == "-p" || argument == "-path" || argument == "-show_path")
		{
			detail_options.detail_path = true;
		}
		else if (argument == "lp" || argument == "-lp" || argument == "-long_path" || argument == "-show_long_path")
		{
			detail_options.detail_long_path = true;
		}
		else if (argument == "m" || argument == "-m" || argument == "-last_modified" || argument == "-show_last_modified")
		{
			detail_options.detail_last_modified = true;
		}
		else if (argument == "c" || argument == "-c" || argument == "-content" || argument == "-show_content")
		{
			detail_options.detail_content = true;
		}
		else {
			logger << "Input argument " << argument;
			logger << " could not be parsed. Expected one of the following inputs:\n-show_(t)ags, show_(p)ath, -show_(l)ong_(p)ath, ";
			logger << "-show_last_(m)odified, -show_(c)ontent" << endl;
			return;
		}
	}

	if (active_mode != -1)
	{
		conf.get("MODE_" + to_string(active_mode) + "_DETAIL_TAGS", detail_options.detail_tags);
		conf.get("MODE_" + to_string(active_mode) + "_DETAIL_PATH", detail_options.detail_path);
		conf.get("MODE_" + to_string(active_mode) + "_DETAIL_LONG_PATH", detail_options.detail_long_path);
		conf.get("MODE_" + to_string(active_mode) + "_DETAIL_LAST_MODIFIED", detail_options.detail_last_modified);
		conf.get("MODE_" + to_string(active_mode) + "_DETAIL_SHOW_CONTENT", detail_options.detail_content);
	}
}

void parse_add_note(Log& logger, std::istringstream& iss, const PATHS& paths, const std::string& file_ending, std::string& filename, time_t& date_t, std::vector<std::string>& tags, bool& add_data)
{
	add_data = false;

	string date_str;
	CONV_ERROR ret;
	iss >> date_str;
	ret = str2date(date_t, date_str);
	if (ret != CONV_ERROR::CONV_SUCCESS) {
		logger << "First argument of the new command has to be the date: ((t)oday, (y)esterday, dd, dd.mm, dd.mm.yyyy), set date to today" << endl;
		date_t = time(NULL);
	}

	// read tags and optional add_data argument
	string argument;
	while (iss >> argument) {
		if (argument == "-d" || argument == "-add_data") {
			add_data = true;
			continue;
		}
		tags.push_back(argument);
	}

	filename = get_filename(paths, date_t, file_ending);
}

#ifdef _WIN32
void RunExternalProgram(Log& logger, std::filesystem::path executeable, std::filesystem::path file, HANDLE& hExit)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES saAttr;

	HANDLE m_hChildStd_OUT_Wr = NULL;
	HANDLE m_hChildStd_OUT_Rd = NULL;
	HANDLE dwChangeHandle[2];

	ZeroMemory(&saAttr, sizeof(saAttr));
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
	if (!CreatePipe(&m_hChildStd_OUT_Rd, &m_hChildStd_OUT_Wr, &saAttr, 0))
	{
		// log error
		logger << to_string(GetLastError()) << endl;
		return;
	}

	// Ensure the read handle to the pipe for STDOUT is not inherited.

	if (!SetHandleInformation(m_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
	{
		// log error
		logger << to_string(GetLastError()) << endl;
		return;
	}

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.hStdError = m_hChildStd_OUT_Wr;
	si.hStdOutput = m_hChildStd_OUT_Wr;
	si.dwFlags |= STARTF_USESTDHANDLES;

	ZeroMemory(&pi, sizeof(pi));

	wstring wexec{ executeable.wstring() }, wfile{ file.wstring() };
	vector<wchar_t> exec(wexec.begin(), wexec.end());
	exec.push_back(L'\0');

	vector<wchar_t> command_line(wfile.begin(), wfile.end());
	command_line.push_back(L'\0');

	// Start the child process. 
	if (!CreateProcess(
		&exec[0],           // No module name (use command line)
		&command_line[0],    // Command line
		NULL,                           // Process handle not inheritable
		NULL,                           // Thread handle not inheritable
		TRUE,                           // Set handle inheritance
		0,                              // No creation flags
		NULL,                           // Use parent's environment block
		NULL,                           // Use parent's starting directory 
		&si,                            // Pointer to STARTUPINFO structure
		&pi)                            // Pointer to PROCESS_INFORMATION structure
		)
	{
		logger << to_string(GetLastError()) << endl;
		return;
	}
	else
	{
		// could read data with, for this purpose, ignore childs cout
		//m_hreadDataFromExtProgram = CreateThread(0, 0, readDataFromExtProgram, NULL, 0, NULL);
	}

	dwChangeHandle[0] = pi.hProcess;
	dwChangeHandle[1] = hExit;

	WaitForMultipleObjects(2, dwChangeHandle, FALSE, INFINITE);

	CloseHandle(m_hChildStd_OUT_Rd);
	CloseHandle(m_hChildStd_OUT_Wr);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

}

#endif
/*
DWORD __stdcall readDataFromExtProgram(void* argh)
{
	DWORD dwRead;
	CHAR chBuf[1024];
	BOOL bSuccess = FALSE;

	for (;;)
	{
		bSuccess = ReadFile(m_hChildStd_OUT_Rd, chBuf, 1024, &dwRead, NULL);
		if (!bSuccess || dwRead == 0) continue;

		// Log chBuf

		if (!bSuccess) break;
	}
	return 0;
}
*/

#ifdef _WIN32
int getinput(string& c)
{

	DWORD cNumRead,  i;
	INPUT_RECORD irInBuf[128];

	
	/*
	DWORD fdwSaveOldMode, fdwMode,;
	if (!GetConsoleMode(hStdin, &fdwSaveOldMode)) {
		cout << "Error get console mode" << endl;
		SetConsoleMode(hStdin, fdwSaveOldMode);
		ExitProcess(0);
	}
	 
	fdwMode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
	if (!SetConsoleMode(hStdin, fdwMode)) {
		cout << "Error set console mode" << endl;
		SetConsoleMode(hStdin, fdwSaveOldMode);
		ExitProcess(0);
	}
	*/	
	

	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	while (true)
	{
		// Wait for the events.

		ReadConsoleInput(
			hStdin,      // input buffer handle
			irInBuf,     // buffer to read into
			128,         // size of read buffer
			&cNumRead); // number of records read
		

		// Dispatch the events to the appropriate handler.
		for (i = 0; i < cNumRead; i++)
		{
			if (irInBuf[i].EventType == KEY_EVENT) {
				if (irInBuf[i].Event.KeyEvent.bKeyDown) {
					if (irInBuf[i].Event.KeyEvent.wVirtualKeyCode == VK_TAB || irInBuf[i].Event.KeyEvent.wVirtualKeyCode == VK_RETURN || irInBuf[i].Event.KeyEvent.wVirtualKeyCode == VK_UP || irInBuf[i].Event.KeyEvent.wVirtualKeyCode == VK_DOWN) {
						return irInBuf[i].Event.KeyEvent.wVirtualKeyCode;
					}
					else {
						if (irInBuf[i].Event.KeyEvent.uChar.AsciiChar == '\b') { // delete last character
							if (c.size() > 0) {
								c.pop_back();
								cout << '\b' << ' ' << '\b';
							}
						}
						else {
							cout << irInBuf[i].Event.KeyEvent.uChar.AsciiChar;
							c += irInBuf[i].Event.KeyEvent.uChar.AsciiChar; // input new character
						}
						
					}
				}
				
			}
			//else if (irInBuf[i].EventType == MENU_EVENT) {
			//	cout << irInBuf[i].Event.MenuEvent.dwCommandId;
			//}
				
			/*
			case MOUSE_EVENT: // mouse input
				#ifndef MOUSE_HWHEELED
				#define MOUSE_HWHEELED 0x0008
				#endif
				printf("Mouse event: ");

				switch (irInBuf[i].Event.MouseEvent.dwEventFlags)
				{
				case 0:

					if (irInBuf[i].Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
					{
						printf("left button press \n");
					}
					else if (irInBuf[i].Event.MouseEvent.dwButtonState == RIGHTMOST_BUTTON_PRESSED)
					{
						printf("right button press \n");
					}
					else
					{
						printf("button press\n");
					}
					break;
				case DOUBLE_CLICK:
					printf("double click\n");
					break;
				case MOUSE_HWHEELED:
					printf("horizontal mouse wheel\n");
					break;
				case MOUSE_MOVED:
					printf("mouse moved\n");
					break;
				case MOUSE_WHEELED:
					printf("vertical mouse wheel\n");
					break;
				default:
					printf("unknown\n");
					break;
				}
				break;

			case WINDOW_BUFFER_SIZE_EVENT: // scrn buf. resizing
				printf("Resize event\n");
				printf("Console screen buffer is %d columns by %d rows.\n", irInBuf[i].Event.WindowBufferSizeEvent.dwSize.X, irInBuf[i].Event.WindowBufferSizeEvent.dwSize.Y);
				break;

			case FOCUS_EVENT:  // disregard focus events

			case MENU_EVENT:   // disregard menu events
				break;

			default:
				SetConsoleMode(hStdin, fdwSaveOldMode);
				ExitProcess(0);
				break;
			}
			*/
		}
	}

		
}


#endif





void read_mode_names(Config& conf, std::list<std::string>& mode_names)
{
	int num_modes = 0;
	conf.get("NUM_MODES", num_modes);

	string mode_name;
	for (auto i = 0; i < num_modes; i++) {
		conf.get("MODE_" + to_string(i) + "_NAME", mode_name);
		mode_names.push_back(mode_name);
	}
}


AUTOCOMPLETE::AUTOCOMPLETE(const CMD_NAMES& cmd_names, const std::list<std::string>& tags, const std::list<std::string>& mode_names)
{
	this->tags = TrieTree(tags);
	this->mode_names = TrieTree(mode_names);
	this->cmd_names = TrieTree();
	for (const auto& [name, cmd] : cmd_names.cmd_names) {
		this->cmd_names.insert(name);
	}
}

AUTOCOMPLETE::AUTOCOMPLETE(const CMD_NAMES& cmd_names, const std::map<std::string, int>& tag_count, const std::unordered_map<int, std::string>& mode_names)
{
	this->mode_names = TrieTree();
	for (const auto& [_, mode_name] : mode_names) {
		this->mode_names.insert(mode_name);
	}
	this->cmd_names = TrieTree();
	for (const auto& [name, cmd] : cmd_names.cmd_names) {
		this->cmd_names.insert(name);
	}
	this->tags = TrieTree();
	for (const auto& [tag, _] : tag_count) {
		this->tags.insert(tag);
	}
}
