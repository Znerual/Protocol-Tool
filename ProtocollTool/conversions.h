#pragma once

#include <ctime>
#include <string>
#include "enums.h"


tm get_localtime(const time_t& tt);

void remove_time(tm* datetime);

CONV_ERROR str2int(int& i, char const* s, int base = 10);
CONV_ERROR str2float(float& f, char const* s);
CONV_ERROR str2date(time_t& t, const std::string& s);
CONV_ERROR str2date_short(time_t& t, const std::string& s);
CONV_ERROR date2str(std::string & s, const time_t & t);
CONV_ERROR date2str_short(std::string & s, const time_t & t);
CONV_ERROR date2str_medium(std::string& s, const time_t& t);
CONV_ERROR date2str_long(std::string& s, const time_t& t);