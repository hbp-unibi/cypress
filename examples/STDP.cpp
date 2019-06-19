/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2018 Christoph Ostrau
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
 * @file STDP.cpp
 *
 * This example shows the usage of synapses. Here, an STDP synapse is used to
 * observe the time dependence on the weight change
 *
 * @author Christoph Ostrau
 */

// Include first to avoid "_POSIX_C_SOURCE redefined" warning
#include <cypress/cypress.hpp>

#include <fstream>
#include <iostream>

using namespace cypress;

int main(int argc, const char *argv[])
{
	if (argc != 2 && !NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0] << " <SIMULATOR>" << std::endl;
		return 1;
	}

	global_logger().min_level(LogSeverity::INFO);

	size_t num_neurons = 150;
	Real inter_spike_intervall = 200, trigger_time = 100;

	auto net = Network();

	// Source for platic_synapses
	auto source = net.create_population<SpikeSourceArray>(
	    num_neurons, SpikeSourceArrayParameters());
	for (size_t i = 0; i < num_neurons; i++) {
		source[i].parameters().spike_times(
		    {Real(i), Real(i) + inter_spike_intervall});
	}

	// Manually trigger a post spike
	auto trigger = net.create_population<SpikeSourceArray>(
	    1, SpikeSourceArrayParameters().spike_times({trigger_time}));

	// Target Population
	auto target = net.create_population<IfFacetsHardware1>(
	    num_neurons, IfFacetsHardware1Parameters().v_thresh(-70),
	    IfFacetsHardware1Signals().record_spikes());

	// Define the synapse rules
	SpikePairRuleAdditive plasctic_synapse = SpikePairRuleAdditive()
	                                             .weight(0.005)
	                                             .delay(1)
	                                             .A_minus(0.003)
	                                             .A_plus(0.003)
	                                             .tau_minus(10)
	                                             .tau_plus(10)
	                                             .w_max(1)
	                                             .w_min(0.0);
	// This is optional, weight and delay can be directly passed to connector
	StaticSynapse trigger_synapse({0.015, 1});
	net.add_connection(source, target, Connector::one_to_one(plasctic_synapse),
	                   "plastic");
	net.add_connection(trigger, target, Connector::all_to_all(trigger_synapse));

	// Run the simulation
	net.run(argv[1], num_neurons + inter_spike_intervall + 50, argc, argv);

	// Gather spike times
	std::vector<std::vector<Real>> spikes;
	for (auto neuron : target) {
		spikes.push_back(neuron.signals().get_spikes());
	}

	// Fetch learned weights
	const auto &weights =
	    net.connection("plastic").connector().learned_weights();

	// Plot weight - time dependence
	// Calculate x: first_post_spike - pre_spike
	std::vector<Real> x_values;
	std::vector<Real> weight_change;
	for (size_t i = 0; i < num_neurons; i++) {
		if (spikes[i].size() == 0) {
			continue;
		}
		x_values.emplace_back(spikes[i][0] - i);
		weight_change.emplace_back(weights[i].SynapseParameters[0] -
		                           plasctic_synapse.weight());
	}

	pyplot::title("STDP Curve on " + std::string(argv[1]));
	pyplot::ylabel("Weight Change");
	pyplot::xlabel("t_Trigger - t_Source in ms");
	pyplot::plot(x_values, weight_change, "r.");
	pyplot::show();

	return 0;
}
