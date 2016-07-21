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

#include <cypress/util/json.hpp>

namespace cypress {
static Json &join_impl(Json &tar, const Json &src)
{
	if (tar.is_object() && src.is_object()) {
		const std::map<std::string, Json> map = src;
		for (const auto &elem : map) {
			join(tar[elem.first], elem.second);
		}
	}
	else {
		tar = src;
	}
	return tar;
}

Json &join(Json &tar, const Json &src)
{
	if (tar.is_null()) {
		tar = Json::object();
	}
	if (src.is_null()) {
		return join_impl(tar, Json::object());
	}
	return join_impl(tar, src);
}
}
