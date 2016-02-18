/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2016  Andreas Stöckel
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifndef CYPRESS_UTIL_FILESYSTEM_HPP
#define CYPRESS_UTIL_FILESYSTEM_HPP

#include <algorithm>
#include <string>
#include <unordered_set>

namespace cypress {
namespace filesystem {
/**
 * Canonicalises the given filename, returns an empty string on error.
 */
std::string canonicalise(const std::string &file);

/**
 * Canonicalises the given list of filenames.
 */
template <typename T>
void canonicalise_files(T &files)
{
	for (std::string &file : files) {
		file = canonicalise(file);
	}
}

/**
 * Returns the canonical directories the given files are stored in as unordered
 * set.
 */
std::unordered_set<std::string> dirs(const std::vector<std::string> &files);

/**
 * Calculates the longest common path from a set of files
 */
template <typename T>
std::string longest_common_path(const T &dirs, char sep = '/')
{
	auto it = dirs.cbegin();
	size_t n = it->size();
	const std::string &cmp = *it;
	for (it++; it != dirs.end(); it++) {
		auto p = std::mismatch(cmp.begin(), cmp.end(), it->begin());
		if ((p.first - cmp.begin()) < ssize_t(n)) {
			n = p.first - cmp.begin();
		}
	}
	return cmp.substr(
	    0, std::max(1UL, (n == dirs.cbegin()->size())
	                         ? ((dirs.cbegin()->back() == '/') ? n - 1 : n)
	                         : cmp.rfind(sep, n)));
}
}
}

#endif /* CYPRESS_UTIL_FILESYSTEM_HPP */
