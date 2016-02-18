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

#include <memory>

#include <limits.h>
#include <libgen.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cypress/util/filesystem.hpp>

namespace cypress {
namespace filesystem {
std::string canonicalise(const std::string &file)
{
	std::unique_ptr<char> resolved(realpath(file.c_str(), nullptr));
	if (!resolved) {
		return std::string();
	}
	return std::string(resolved.get());
}

std::unordered_set<std::string> dirs(const std::vector<std::string> &files)
{
	std::unordered_set<std::string> res;
	for (const auto &file : files) {
		// Canonicalise the filename
		std::string canonical = canonicalise(file);
		if (canonical.empty()) {
			continue;
		}

		// Make sure the path points at a regular file
		struct stat stat;
		if (lstat(canonical.c_str(), &stat) != 0 || !S_ISREG(stat.st_mode)) {
			continue;
		}

		// Calculate the dirname and add it to the result
		res.emplace(dirname(&canonical[0]));
	}
	return res;
}

}
}

