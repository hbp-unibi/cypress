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

#include <cypress/cypress.hpp>
#include <cypress/nef.hpp>

using namespace cypress;
using namespace cypress::nef;

int main(int argc, const char *argv[])
{
	if (argc != 2 && !NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0] << " <SIMULATOR>" << std::endl;
		return 1;
	}

	// Some aliases to save keystrokes...
	using Neuron = IfFacetsHardware1;
	using Parameters = Neuron::Parameters;
	using Signals = Neuron::Signals;

	// General parameters for the experiment
	const size_t n_samples = 100;         // Sample points on the tuning curve
	const size_t n_repeat = 100;          // Repetitions for each sample
	const size_t n_population = 32;       // Number of neurons
	const Real min_spike_interval = 5.0;  // ms
	const Real exc_synaptic_weight = 0.004;       // uS
	const Real exc_bias_synaptic_weight = 0.015;  // uS
	const Real inh_synaptic_weight = -0.015;      // uS

	// Neuron parameters
	Parameters params = Parameters()
	                        .v_thresh(-55.0)                  // mV
	                        .v_rest(-75.0)                    // mV
	                        .v_reset(-80.0)                   // mV
	                        .e_rev_I(-80.0)                   // mV
	                        .g_leak(40.0)                     // uS
	                        .tau_refrac(min_spike_interval);  // ms

	// Setup the tuning curve generator/evaluator
	TuningCurveEvaluator evaluator(n_samples, n_repeat,
	                               min_spike_interval * 1e-3);

	// Setup a simple network which gets the spikes from the tuning curve
	// generator as input
	Network net;

	// Source Population
	Population<SpikeSourceArray> pop_src(net, 8, evaluator.input_spike_train());

	// Bias source population
	const Real len = evaluator.input_spike_train_len();
	std::vector<Real> bias_spikes =
	    spikes::constant_interval(0.0, len, min_spike_interval);
	Population<SpikeSourceArray> pop_src_bias(net, 1, bias_spikes);

	// Target population
	Population<Neuron> pop_tar(net, n_population, params,
	                           Signals().record_spikes());

	// Connect the source population to the target neurons, choose inhibitory
	// connections for every second neuron
	pop_src(0, 4).connect_to(
	    pop_tar, Connector::functor([](NeuronIndex, NeuronIndex tar) {
		    return (tar % 2) == 0;
		}, exc_synaptic_weight));
	pop_src(4, 8).connect_to(
	    pop_tar, Connector::functor([](NeuronIndex, NeuronIndex tar) {
		    return (tar % 2) == 1;
		}, inh_synaptic_weight));

	// Connect the bias spike source to every second target neuron
	pop_src_bias.connect_to(
	    pop_tar, Connector::functor([](NeuronIndex, NeuronIndex tar) {
		    return (tar % 2) == 1;
		}, exc_bias_synaptic_weight));

	// Run the network
	net.run(argv[1], 0.0, argc, argv);
    create_dot(net, "Tuning Curves Network", "tuning_curve.dot");

	// Evaluate the neurons, create a result matrix containing the tuning curve
	// of each neuron in the population as column
	Matrix<Real> result(n_samples, n_population + 1);
	for (auto neuron : pop_tar) {
		// Calculate the tuning curve for this neuron in the population
		std::vector<std::pair<Real, Real>> res =
		    evaluator.evaluate_output_spike_train(
		        neuron.signals().get_spikes());

		// Write the results to the result matrix
		for (size_t i = 0; i < res.size(); i++) {
			result(i, 0) = res[i].first;
			result(i, neuron.nid() + 1) = res[i].second;
		}
	}
	std::cout << result << std::endl;
}

