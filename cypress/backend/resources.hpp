/*
 *  Cypress -- C++ Neural Associative Memory Simulator
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

/**
 * @file resources.hpp
 *
 * File containing declarations of all the resources embedded in the Cypress
 * library.
 *
 * @author Andreas Stöckel
 */

#pragma once

#ifndef CYPRESS_BACKEND_RESOURCES_HPP
#define CYPRESS_BACKEND_RESOURCES_HPP

#include <cypress/util/resource.hpp>

namespace cypress {

/**
 * Structure collecting all the globally available resource files.
 */
struct Resources {
	static const Resource PYNN_INTERFACE;
};

}

#endif /* CYPRESS_BACKEND_RESOURCES_HPP */
