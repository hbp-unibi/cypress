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

#include <random>

#include <cypress/cypress.hpp>
#include <cypress/nef.hpp>

using namespace cypress;
using namespace cypress::nef;

int main(int, char **)
{
	// Setup the tuning curve generator/evaluator
	const size_t n_samples = 100;
	const size_t n_repeat = 10;
	const float min_spike_interval = 5e-3;
	TuningCurveEvaluator evaluator(n_samples, n_repeat, min_spike_interval);

	// Setup a simple network which gets the spikes from the tuning curve
	// generator as input
	const size_t n_population = 196;
	const float synaptic_weight = 0.015;
	Network net;
	auto pop_src = net.create_population<SpikeSourceArray>(
	    1, evaluator.input_spike_train());
	auto pop_tar = net.create_population<IfFacetsHardware1>(
	    n_population,
	    IfFacetsHardware1Parameters().v_thresh(-65.0).g_leak(1.0),
	    IfFacetsHardware1Signals().record_spikes());
	pop_src.connect_to(pop_tar, Connector::all_to_all(synaptic_weight));

	// Randomly select g_leak and tau_refrac on each neuron
	/*	std::random_device rd;
	    std::default_random_engine re(rd());
	    std::uniform_real_distribution<float> gleak_distr(0.02, 20.0);
	    std::uniform_real_distribution<float> tau_refrac_distr(1.0, 100.0);
	    for (size_t i = 0; i < pop_tar.size(); i++) {
	        pop_tar[i]
	            .parameters()
	            .g_leak(gleak_distr(re))
	            .tau_refrac(tau_refrac_distr(re));
	    }*/

	// Run the network on spikey
	net.run(PyNN("spikey"));

	// Evaluate the neurons, create a result matrix containing the tuning curve
	// of each neuron in the population as column
	Matrix<float> result(n_samples, n_population + 1);
	for (auto neuron : pop_tar) {
		// Calculate the tuning curve for this neuron in the population
		std::vector<std::pair<float, float>> res =
		    evaluator.evaluate_output_spike_train(
		        neuron.signals().get_spikes());

		// Write the results to the result matrix
		for (const auto &r : res) {
			const size_t row = std::round(r.first * n_samples);
			result(row, 0) = r.first;
			result(row, neuron.nid() + 1) = r.second;
		}
	}
	std::cout << result << std::endl;
}

