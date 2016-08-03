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
static float avg_fire_rate(const Population &pop, float duration)
{
	float avg_rate = 0.0;
	for (auto neuron : pop) {
		avg_rate += neuron.signals().get_spikes().size();
	}
	return avg_rate / float(pop.size()) / (duration * 1e-3);
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
	static const float f = 25.0;           // Hz
	static const float duration = 1000.0;  // ms

	// Sweep over w_syn_inh and w_syn_exc, create the network and run it
	for (float w_syn_exc = 0.0; w_syn_exc < 0.016; w_syn_exc += 0.002) {
		for (float w_syn_inh = 0.0; w_syn_inh < 0.016; w_syn_inh += 0.002) {
			Network net =
			    Network()
			        .add_population<SpikeSourceConstFreq>(
			            "src1", n_src, SpikeSourceConstFreqParameters()
			                               .rate(f)
			                               .duration(duration)
			                               .sigma(2.0))
			        .add_population<SpikeSourceConstFreq>(
			            "src2", n_src, SpikeSourceConstFreqParameters()
			                               .rate(f)
			                               .duration(duration)
			                               .sigma(2.0))
			        .add_population<Neuron>("tar", n_tar, {},
			                                Signals().record_spikes())
			        .add_connection("src1", "tar",
			                        Connector::all_to_all(-w_syn_inh))
			        .add_connection("src2", "tar",
			                        Connector::all_to_all(w_syn_exc))
			        .run(argv[1], duration, argc, argv);
			std::cout << w_syn_exc << "," << w_syn_inh << ","
			          << avg_fire_rate(net.population<Neuron>("tar"), duration)
			          << std::endl;
		}
	}

	return 0;
}
