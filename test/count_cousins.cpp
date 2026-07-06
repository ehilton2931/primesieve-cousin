///
/// @file   count_cousins.cpp
/// @brief  Count cousin primes inside [10^12, 10^12 + 10^9].
///
/// Copyright (C) 2026 Evan Hilton, <ehilton2931@protonmail.com>
///
/// This file is distributed under the BSD License. See the COPYING
/// file in the top level directory.
///

#include <primesieve.hpp>

#include <stdint.h>
#include <iostream>
#include <cstdlib>

void check(bool OK)
{
	std::cout << "   " << (OK ? "OK" : "ERROR") << "\n";
	if (!OK)
		std::exit(1);
}

int main()
{
	uint64_t start = (uint64_t) 1e12;
	uint64_t stop = (uint64_t)(1e12 + 1e9);
	uint64_t count = primesieve::count_cousins(start, stop);
	std::cout << "Cousin primes inside [10^12, 10^12 + 10^9] = " << count;
	check(count == 1729479);

	std::cout << std::endl;
	std::cout << "Test passed successfully!" << std::endl;

	return 0;
}
