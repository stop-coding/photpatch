
#include<string>
#include <vector>
#include <unistd.h>

#pragma once

std::vector<std::string>  string_split(const std::string& str,const std::string& delim);
static inline bool  is_file_exsit(const std::string& name) {
	return (access( name.c_str(), F_OK) != -1 );
}
static inline bool is_number(const std::string& str)
{
    for (char const &c : str) {
        if (std::isdigit(c) == 0) return false;
    }
    return true;
}

static inline bool is_hexnum(const std::string& str)
{
    for (char const &c : str) {
        if (std::isxdigit(c) == 0) return false;
    }
    return true;
}

std::string  string_strip(const std::string& str);
std::string  string_strip(const std::string& str, char c);
