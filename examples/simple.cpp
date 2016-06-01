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

#include <iostream>
#include <fstream>

#include <cypress/cypress.hpp>

using namespace cypress;

int main(int argc, const char *argv[])
{
	if (argc != 2 && !NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0] << " <SIMULATOR>" << std::endl;
		return 1;
	}

	auto net =
	    Network()
	        .add_population<SpikeSourceArray>(
	            "source", 4, {{100.0, 200.0, 300.0},
	                          {400.0, 500.0, 600.0},
	                          {700.0, 800.0, 900.0},
	                          {1000.0, 1100.0, 1200.0}},
	            SpikeSourceArraySignals().record_spikes())
	        .add_population<IfCondExp>("neuron", 4,
	                                   IfCondExpParameters().v_rest(-60.0),
	                                   IfCondExpSignals().record_spikes())
	        .add_connection("source", "neuron", Connector::one_to_one(0.16))
	        .run(PyNN(argv[1]));

	// Print the runtimes
	std::cout << "Runtime statistic: total " << net.runtime().total
	          << "s, simulation " << net.runtime().sim
	          << "s, initialization " << net.runtime().initialize
	          << "s, finalization " << net.runtime().finalize << "s"
	          << std::endl;

	// Print the spike times for each neuron
	for (auto neuron : net.population<IfCondExp>("neuron")) {
		std::cout << "Spike times for neuron " << neuron.nid() << std::endl;
		std::cout << neuron.signals().get_spikes();
	}

	return 0;
}
