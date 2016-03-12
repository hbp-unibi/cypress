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

#ifndef CYPRESS_UTIL_JSON_HPP
#define CYPRESS_UTIL_JSON_HPP

#include <cypress/json.hpp>

namespace cypress {
using Json = nlohmann::json;

/**
 * Recursively joins one JSON object into another, overriding already existing
 * keys in the target.
 *
 * @param tar is the target JSON object. Keys in this object may be overriden.
 * @param src is the source JSON object. All keys from this object will be
 * available in tar.
 * @return a reference at tar.
 */
Json& join(Json &tar, const Json &src);
}

#endif /* CYPRESS_UTIL_JSON_HPP */
