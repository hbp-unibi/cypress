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
 * @file simple.cpp
 *
 * A simple network consisting of four source and four target neurons. The four
 * source neurons emit spikes following a Poisson distribution. The source
 * neurons are connected to the target neurons using an all-to-all projection.
 * Spikes in both the source and target neuron populations are recorded and
 * printed at the end of the program.
 *
 * @author Andreas Stöckel
 */

#include <fstream>
#include <iostream>

#include <cypress/cypress.hpp>

using namespace cypress;

int main(int argc, const char *argv[])
{
	if (argc != 2 && !NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0] << " <SIMULATOR>" << std::endl;
		return 1;
	}

	global_logger().min_level(LogSeverity::INFO);

	auto net =
	    Network()
	        // Add a named population of poisson spike sources
	        .add_population<SpikeSourceConstFreq>(
	            "source", 8, SpikeSourceConstFreqParameters()
	                             .start(100.0)
	                             .rate(1000.0)
	                             .duration(1000.0),
	            SpikeSourceConstFreqSignals().record_spikes())
	        // Add a population of IfFacetsHardware1 neurons -- those neurons
	        // are supported by all simulator backends
	        .add_population<IfFacetsHardware1>(
	            "target", 32, IfFacetsHardware1Parameters().g_leak(0.04),
	            IfFacetsHardware1Signals().record_spikes())
	        // Project each neuron in the population "source" onto each neuron
	        // in the population "target"
	        .add_connection("source", "target",
	                        /*Connector::all_to_all(0.015, 1*/Connector::fixed_fan_in( 3, 0.015,1.0));

	// Use only a part of the target population
	PopulationView<IfFacetsHardware1> popview(
	    net, net.populations("target")[0].pid(), 8, 20);
	// Inhibitory input
	/*auto a =
	    net.add_population<SpikeSourceConstFreq>(
	         "source2", 16,
	         SpikeSourceConstFreqParameters().start(100.0).rate(100.0).duration(
	             1000.0),
	         SpikeSourceConstFreqSignals().record_spikes())
	        .add_connection("source2", popview,
	                        /*Connector::all_to_all(-0.015, 1)*//*Connector::fixed_fan_in( 3, 0.015
                                                                            )                            );
	(net.population("target"))[3].signals().record(1, true);*/

	net.run(argv[1], 200.0, argc, argv);

	// Print the spike times for each source neuron
	/*	for (auto neuron : net.population<SpikeSourcePoisson>("source")) {
	        std::cout << "Spike times for source neuron " << neuron.nid()
	                  << std::endl;
	        std::cout << neuron.signals().get_spikes();
	    }*/

	std::cout << "---------" << std::endl;

	// Print the spike times for each target neuron
	for (auto neuron : net.population<IfFacetsHardware1>("target")) {
		std::cout << "Spike frequency for target neuron " << neuron.nid()
		          << ", " << neuron.signals().get_spikes().size() / 0.9
		          << std::endl;
	}
	std::cout << net.populations("target")[0][3].signals().data(1) << std::endl;

	return 0;
}
