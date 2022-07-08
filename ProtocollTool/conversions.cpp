#include "conversions.h"

#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <boost/date_time/gregorian/gregorian.hpp>



CONV_ERROR str2int(int& i, char const* s, int base)
{
    char* end;
    long  l;
    errno = 0;
    l = strtol(s, &end, base);
    if ((errno == ERANGE && l == LONG_MAX) || l > INT_MAX) {
        return CONV_OVERFLOW;
    }
    if ((errno == ERANGE && l == LONG_MIN) || l < INT_MIN) {
        return CONV_UNDERFLOW;
    }
    if (*end != '\0') {
        return CONV_INCONVERTIBLE;
    }
    i = l;
    return CONV_SUCCESS;
}

CONV_ERROR str2float(float& f, char const* s)
{
    char* end;
    float ff;
    errno = 0;
    ff = strtof(s, &end);
    if (errno == ERANGE && ff == HUGE_VALF) {
        return CONV_OVERFLOW;
    }
    if (*s == '\0' || *end != '\0') {
        return CONV_INCONVERTIBLE;
    }
    f = ff;
    return CONV_SUCCESS;
}

CONV_ERROR str2date(time_t& t, const std::string& s)
{
    // get current date
    time_t tt = std::time(NULL);
    tm tdm;
    localtime_s(&tdm, &tt);
    boost::gregorian::date today = boost::gregorian::date_from_tm(tdm);

    if (s == "now" || s == "today" || s == "t")
    {
        tdm = boost::gregorian::to_tm(today);
        t = mktime(&tdm);
        return CONV_SUCCESS;
    }
    else if (s == "yesterday" || s == "y")
    {
        boost::gregorian::date yesterday(today.year(), today.month(), today.day() - 1);
        tdm = boost::gregorian::to_tm(yesterday);
        t = mktime(&tdm);
        return CONV_SUCCESS;
    }
    else {
        std::vector<size_t> positions = std::vector<size_t>();
        size_t pos = s.find(".", 0);
        while (pos != std::string::npos) {
            positions.push_back(pos);
            pos = s.find(".", pos + 1);
        }

        boost::gregorian::date set_date;
        switch (positions.size())
        {
        
        case 0: // only day is given
            CONV_ERROR ret_day;
            int day;
            ret_day = str2int(day, s.c_str());

            if (ret_day == CONV_INCONVERTIBLE) {
                std::cout << "Given string " << s << " is not parsable to a date value." << std::endl;
                return CONV_INCONVERTIBLE;
            }
            if (day > 31 || day < 1) {
                std::cout << "Expected a day in the range of 1 to 31 but got " << day << std::endl;
                return CONV_INCONVERTIBLE;
            }
            

            set_date = boost::gregorian::date(today.year(), today.month(), day);
            tdm = boost::gregorian::to_tm(set_date);
            t = mktime(&tdm);
           
            break;
        case 1: // day and month
            CONV_ERROR ret_month;
            int month;
            ret_day = str2int(day, s.substr(0, positions.at(0)).c_str());
            ret_month = str2int(month, s.substr(positions.at(0) + 1).c_str());

            if (ret_day == CONV_INCONVERTIBLE || ret_month == CONV_INCONVERTIBLE) {
                std::cout << "Given string " << s << " is not parsable to a date value." << std::endl;
                return CONV_INCONVERTIBLE;
            }
            if (day > 31 || day < 1) {
                std::cout << "Expected a day in the range of 1 to 31 but got " << day << std::endl;
                return CONV_INCONVERTIBLE;
            }
            else if (month > 12 || month < 1) {
                std::cout << "Expected a month in the range of 1 to 12 but got " << month << std::endl;
                return CONV_INCONVERTIBLE;
            }
            

            set_date = boost::gregorian::date(today.year(), month, day);
            tdm = boost::gregorian::to_tm(set_date);
            t = mktime(&tdm);
            
            break;

        case 2: // day, month and year
            CONV_ERROR ret_year;
            int year;
            ret_day = str2int(day, s.substr(0, positions.at(0)).c_str());
            ret_month = str2int(month, s.substr(positions.at(0) + 1, positions.at(1) - positions.at(0) - 1).c_str());
            ret_year = str2int(year, s.substr(positions.at(1) + 1).c_str());

            if (ret_day == CONV_INCONVERTIBLE || ret_month == CONV_INCONVERTIBLE ||ret_year == CONV_INCONVERTIBLE) {
                std::cout << "Given string " << s << " is not parsable to a date value." << std::endl;
                return CONV_INCONVERTIBLE;
            }
            if (day > 31 || day < 1) {
                std::cout << "Expected a day in the range of 1 to 31 but got " << day << std::endl;
                return CONV_INCONVERTIBLE;
            }
            else if (month > 12 || month < 1) {
                std::cout << "Expected a month in the range of 1 to 12 but got " << month << std::endl;
                return CONV_INCONVERTIBLE;
            }

            set_date = boost::gregorian::date(year, month, day);
            tdm = boost::gregorian::to_tm(set_date);
            t = mktime(&tdm);
            

            break;
        default:
            std::cout << "Too many . separator in " << s << ". Could not parse the string to a date" << std::endl;
            return CONV_INCONVERTIBLE;
        }
  
        return CONV_SUCCESS;
    }

    
}

CONV_ERROR str2date_short(time_t& t, const std::string& s)
{
    int day, month, year;
    CONV_ERROR ret1, ret2, ret3;
    ret1 = str2int(day, s.substr(0, 2).c_str());
    ret2 = str2int(month, s.substr(2, 2).c_str());
    ret3 = str2int(year, s.substr(4, 4).c_str());

    if (ret1 != CONV_SUCCESS || ret2 != CONV_SUCCESS || ret3 != CONV_SUCCESS)
    {
        return CONV_INCONVERTIBLE;
    }

    boost::gregorian::date set_date(year, month, day);
    tm tdm = boost::gregorian::to_tm(set_date);
    t = mktime(&tdm);

    return CONV_SUCCESS;
}

CONV_ERROR date2str(std::string& s, const time_t& t)
{
    tm ptm;
    localtime_s(&ptm, &t);
    char buffer[16];
    strftime(buffer, 16, "%a, %d.%m.%Y", &ptm);
    s = std::string(buffer);
    return CONV_SUCCESS;
}


CONV_ERROR date2str_short(std::string& s, const time_t& t)
{
    tm ptm;
    localtime_s(&ptm, &t);
    char buffer[9];
    strftime(buffer, 9, "%d%m%Y", &ptm);
    s = std::string(buffer);
    return CONV_SUCCESS;
}

CONV_ERROR date2str_long(std::string& s, const time_t& t)
{
    tm ptm;
    localtime_s(&ptm, &t);
    char buffer[32];
    strftime(buffer, 32, "%a, %d.%m.%Y %H:%M:%S", &ptm);
    s = std::string(buffer);
    return CONV_SUCCESS;
}
