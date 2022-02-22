#pragma once

#include "Windows.h"
#include <iostream>


struct Log
{
	template <typename T>
	std::ostream& operator<<(const T message)
	{
		std::cout << message;
		std::cerr << message;

		return std::cout;
	}
};