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

template <typename Population>
static float avg_fire_rate(const Population &pop, float runtime)
{
	float avg_rate = 0.0;
	for (auto neuron : pop) {
		avg_rate += neuron.signals().get_spikes().size();
	}
	return avg_rate / float(pop.size()) / (runtime * 1e-3);
}

int main(int argc, const char *argv[])
{
	if (argc != 2 && !NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0] << " <SIMULATOR>" << std::endl;
		return 1;
	}

	using Neuron = IfFacetsHardware1;
	using Signals = Neuron::Signals;

	static const size_t n_src = 8;
	static const size_t n_tar = 8;
	static const float exc_frequency = 10.0;   // Hz
	static const float inh_frequency = 100.0;  // Hz
	static const float runtime = 1.0e3;        // ms
	static const float w_exc = 0.004;
	static const float w_inh = -0.007;

	// Create the network
	Network net =
	    Network()
	        .add_population<SpikeSourceArray>(
	            "src_exc", n_src, SpikeSourceArray::constant_frequency(
	                                  0.0, runtime, exc_frequency))
	        .add_population<SpikeSourceArray>(
	            "src_inh", n_src, SpikeSourceArray::constant_frequency(
	                                  0.0, runtime, inh_frequency))
	        .add_population<Neuron>("tar", n_tar, {},
	                                Signals().record_spikes());

	// Connect the excitatory input to the target and run the simulation
	net.add_connection("src_exc", "tar", Connector::all_to_all(w_exc));
	net.run(PyNN(argv[1]), runtime);
	std::cout << "Average firing rate without inhibitory synapses "
	          << avg_fire_rate(net.population<Neuron>("tar"), runtime)
	          << std::endl;

	// Connect the inhibitory input to the target and run the simulation
	net.add_connection("src_inh", "tar", Connector::all_to_all(w_inh));
	net.run(PyNN(argv[1]), runtime);
	std::cout << "Average firing rate with inhibitory synapses "
	          << avg_fire_rate(net.population<Neuron>("tar"), runtime)
	          << std::endl;

	return 0;
}
