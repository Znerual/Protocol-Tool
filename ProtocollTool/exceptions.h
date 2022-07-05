#pragma once

#include <exception>

struct IOException : public std::exception {
	const char* what() const throw () {
		return "IO Exception";
	}
};
