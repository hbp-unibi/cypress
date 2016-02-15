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

#include <cypress/core/backend.hpp>
#include <cypress/core/network.hpp>

namespace cypress {

static constexpr float AUTO_TIME_EXTENSION = 1000.0;

Backend::~Backend()
{
	// Do nothing here
}

void Backend::run(Network &network, float duration) const
{
	if (duration <= 0.0) {
		duration =
		    network.duration() + AUTO_TIME_EXTENSION;  // Auto time extension
	}
	do_run(network, duration);  // Now simply execute the network
}
}
