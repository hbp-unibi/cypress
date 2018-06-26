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

/**
 * @file sweep_2d_record_v.cpp
 *
 * Iterates over the w and g_leak parameters of an IfFacetsHardware1 neuron
 * and records the EPSP for a single spike in a single neuron.
 *
 * @author Andreas Stöckel
 */

// Include first to avoid "_POSIX_C_SOURCE redefined" warning
#include <cypress/cypress.hpp>

#include <iostream>
#include <fstream>


using namespace cypress;

int main(int argc, const char *argv[])
{
	if (argc != 2 && !NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0] << " <SIMULATOR>" << std::endl;
		return 1;
	}

	// Sweep over the w and g_leak parameters and store all networks in the
	// network pool
	for (Real w : linspace(0.001, 0.015, 7)) {
		for (Real g_leak : linspace(0.001, 0.02, 5)) {
			auto net =
			    Network()
			        .add_population<SpikeSourceArray>("source", 1, {50.0})
			        .add_population<IfFacetsHardware1>(
			            "target", 1,
			            IfFacetsHardware1Parameters().g_leak(g_leak),
			            IfFacetsHardware1Signals().record_v())
			        .add_connection("source", "target",
			                        Connector::one_to_one(w))
			        .run(argv[1], 1000.0, argc, argv);
			std::ofstream fs("epsp_" + std::string(argv[1]) + "_w_" +
			                 std::to_string(w) + "_g_leak_" +
			                 std::to_string(g_leak) + ".csv");
			fs << net.population<IfFacetsHardware1>("target").signals().get_v();
		}
	}

	return 0;
}
