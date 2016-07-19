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
 * @file synfire.cpp
 *
 * Implementation of a synfire chain with feedforward inhibition as described in
 *
 * Six networks on a universal neuromorphic computing substrate
 * Thomas Pfeil, Andreas Grübl, Sebastian Jeltsch, Eric Müller, Paul Müller,
 * Mihai A. Petrovici, Michael Schmuker, Daniel Brüderle, Johannes Schemmel,
 * Karlheinz Meier, Front. Neurosci. vol. 7, no. 11, 2013
 *
 * @author Andreas Stöckel
 */

#include <algorithm>
#include <fstream>

#include <cypress/cypress.hpp>

using namespace cypress;

// Enable for Spikey. TODO: Implement trafo from IfFacetsHardware1 to LIF
//#define SPIKEY

/**
 * Parameters of the inhibitory synfire chain.
 */
template <typename Neuron>
struct SynfireCainParameters {
	/**
	 * Neuron parameters that should be used for all neurons in the synfire
	 * chain.
	 */
	typename Neuron::Parameters neuron_params;

#ifdef SPIKEY
	/**
	 * Synaptic weight of the excitatory connections.
	 */
	float w_syn = 0.009;
#else
	float w_syn = 0.035;
#endif

	/**
	 * Synaptic delay.
	 */
	float delta_syn = 1.0;

	/**
	 * Number of neurons in the regular spiking population.
	 */
	size_t n_rs = 8;

	/**
	 * Number of neuron in the fast spiking (inhibitory) population.
	 */
	size_t n_fs = 8;

	/**
	 * Length of the chain.
	 */
	size_t length = 11;

	/**
	 * Number of neurons each neuron receives input from.
	 */
	size_t n_connect = 4;

	/**
	 * Flag indicating whether the synfire chain should be closed in a loop.
	 */
	bool loop = true;
};

/**
 * Class which constructs a synfire chain with the given parameters and the
 * specified neuron type.
 */
template <typename Neuron_>
class SynfireCain {
public:
	using Neuron = Neuron_;
	using Parameters = SynfireCainParameters<Neuron_>;
	using Signals = typename Neuron_::Signals;

private:
	Network m_net;
	Parameters m_params;

	Population<Neuron> m_rs_pop;
	Population<Neuron> m_fs_pop;

public:
	SynfireCain(Network net, const Parameters &params = Parameters(),
	            const Signals &neuron_signals = Signals())
	    : m_net(net),
	      m_params(params),
	      m_rs_pop(m_net, params.length * params.n_rs, params.neuron_params,
	               neuron_signals),
	      m_fs_pop(m_net, params.length * params.n_fs, params.neuron_params,
	               neuron_signals)
	{
		for (size_t i = 0; i < params.length; i++) {
			const size_t fs0 = i * params.n_fs;
			const size_t fs1 = (i + 1) * params.n_fs;
			const size_t rs0 = i * params.n_rs;
			const size_t rs1 = (i + 1) * params.n_rs;

			m_fs_pop(fs0, fs1).connect_to(  // Inhibitory connections
			    m_rs_pop(rs0, rs1),
			    Connector::fixed_fan_in(params.n_connect, -params.w_syn,
			                            params.delta_syn));
			if (i > 0) {  // Forward connections
				const size_t rsm1 = (i - 1) * params.n_rs;
				m_rs_pop(rsm1, rs0).connect_to(
				    m_rs_pop(rs0, rs1),
				    Connector::fixed_fan_in(params.n_connect, params.w_syn,
				                            params.delta_syn));
				m_rs_pop(rsm1, rs0).connect_to(
				    m_fs_pop(fs0, fs1),
				    Connector::fixed_fan_in(params.n_connect, params.w_syn,
				                            params.delta_syn));
			}
		}
		if (params.loop) {
			rs_out().connect_to(rs_in(), Connector::fixed_fan_in(
			                                 params.n_connect, params.w_syn,
			                                 params.delta_syn));
			rs_out().connect_to(fs_in(), Connector::fixed_fan_in(
			                                 params.n_connect, params.w_syn,
			                                 params.delta_syn));
		}
	}

	Population<Neuron> rs_pop() { return m_rs_pop; }
	Population<Neuron> fs_pop() { return m_fs_pop; }
	PopulationView<Neuron> rs_in() { return m_rs_pop(0, m_params.n_rs); }
	PopulationView<Neuron> fs_in() { return m_fs_pop(0, m_params.n_fs); }
	PopulationView<Neuron> rs_out()
	{
		return m_rs_pop((m_params.length - 1) * m_params.n_rs,
		                m_params.length * m_params.n_rs);
	}
	PopulationView<Neuron> fs_out()
	{
		return m_fs_pop((m_params.length - 1) * m_params.n_fs,
		                m_params.length * m_params.n_fs);
	}
};

/**
 * Writes the spike times per neuron to a CSV file.
 *
 * @param fn is the target filename.
 * @param obj is the object that should be iterated over.
 * @param f is the accessor which should be applied to each object iterated over
 * in order to access the spike time list.
 */
template <typename T, typename F>
static void write_spike_times(const char *fn, const T &obj, F f)
{
	std::ofstream os(fn);
	for (const auto &a : obj) {
		bool first = true;
		auto o = f(a);
		for (float t : o) {
			if (!first) {
				os << ",";
			}
			os << t;
			first = false;
		}
		os << std::endl;
	}
}

static const std::vector<std::vector<float>> create_input(
    size_t n, size_t a = 3, float sigma = 2.0, size_t repeat = 1,
    float delay = 50.0, float offs = 5.0)
{
	std::default_random_engine re(std::random_device{}());
	std::normal_distribution<float> distr(0.0, sigma);
	std::vector<std::vector<float>> res(n);
	for (size_t i = 0; i < n; i++) {
		res[i].resize(a * repeat);
		for (size_t j = 0; j < repeat; j++) {
			for (size_t k = 0; k < a; k++) {
				res[i][j * a + k] = offs + j * delay + distr(re);
			}
		}
		std::sort(res[i].begin(), res[i].end());
	}
	return res;
}

int main(int argc, const char *argv[])
{
	if (argc != 2 && !NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0] << " <SIMULATOR>" << std::endl;
		return 1;
	}

// Some aliases to save keystrokes...
#ifdef SPIKEY
	using Neuron = IfFacetsHardware1;
#else
	using Neuron = LIF;
#endif
	using Parameters = Neuron::Parameters;
	using Signals = Neuron::Signals;

// Neuron and synfire chain parameters
#ifdef SPIKEY
	Parameters neuron_params = Parameters();
#else
	Parameters neuron_params = Parameters()
	                               .cm(0.2)
	                               .tau_syn_E(2.0)
	                               .tau_syn_I(2.0)
	                               .g_leak(0.2)
	                               .tau_refrac(1.0)
	                               .v_rest(-75.0)
	                               .v_thresh(-55.0)
	                               .v_reset(-80.0)
	                               .e_rev_I(-80.0)
	                               .e_rev_E(0.0);
#endif
	SynfireCainParameters<Neuron> params{neuron_params};

	// Create the network and the synfire chain
	Network net;
	SynfireCain<Neuron> synfire_chain(net, params, Signals().record_spikes());

	// Create the input spikes
	const size_t a = 3;       // Number of spikes per input burst
	const float sigma = 2.0;  // Standard deviation of the input spike times
	const auto input_spikes = create_input(params.n_rs, a, sigma);

	// Create the SpikeSourceArray and connect the source population to the
	// synfire chain input population
	Population<SpikeSourceArray> source(net, params.n_rs);
	for (size_t i = 0; i < params.n_rs; i++) {
		source[i].parameters().spike_times(input_spikes[i]);
	}
	source.connect_to(
	    synfire_chain.rs_in(),
	    Connector::fixed_fan_in(params.n_connect, params.w_syn,
	                            params.delta_syn));
	source.connect_to(
	    synfire_chain.fs_in(),
	    Connector::fixed_fan_in(params.n_connect, params.w_syn,
	                            params.delta_syn));

	// Run the simulation
	std::cout << "Running the simulation..." << std::endl;
	net.run(argv[1], 2000.0, argc, argv);

	// Write the results to disk
	std::cout << "Writing results to disk..." << std::endl;
	write_spike_times("synfire_input.csv", input_spikes,
	                  [](const auto &a) { return a; });
	write_spike_times("synfire_rs.csv", synfire_chain.rs_pop(),
	                  [](const cypress::Neuron<Neuron> &a) {
		                  return a.signals().get_spikes();
		              });
	write_spike_times("synfire_fs.csv", synfire_chain.fs_pop(),
	                  [](const cypress::Neuron<Neuron> &a) {
		                  return a.signals().get_spikes();
		              });
}
