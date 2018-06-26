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
 * @file winner_take_all.cpp
 *
 * Implements a very simple Winner Take All network. Each population in the
 * network receives an input spike train with triangle-shaped average spike
 * frequency. Only the population with the maximum average spike frequency
 * should produce output spikes.
 *
 * @author Andreas Stöckel
 */

// Include first to avoid "_POSIX_C_SOURCE redefined" warning
#include <cypress/cypress.hpp>

#include <iostream>
#include <fstream>

#include <cypress/nef/delta_sigma.hpp>

using namespace cypress;
using namespace cypress::nef;

struct WTAParameters {
	size_t n_pops = 3;
	size_t n_pop_size = 8;
	Real w_exc = 0.015;
	Real w_inh = -0.015;
	Real p_con = 1.0;
};

template <typename Neuron_>
class WTA {
public:
	using Neuron = Neuron_;
	using Parameters = typename Neuron_::Parameters;
	using Signals = typename Neuron_::Signals;

private:
	WTAParameters m_params;
	Population<Neuron> m_pop_exc;
	Population<Neuron> m_pop_inh;

public:
	WTA(Network net, const WTAParameters &params = WTAParameters(),
	    const Parameters &neuron_params = Parameters(),
	    const Signals &neuron_signals = Signals())
	    : m_params(params),
	      m_pop_exc(net, params.n_pop_size * params.n_pops, neuron_params,
	                neuron_signals),
	      m_pop_inh(net, params.n_pop_size * params.n_pops, neuron_params,
	                neuron_signals)
	{
		// Connect the populations -- excitatory self-connection, inhibitory
		// mutual connections
		for (size_t i = 0; i < params.n_pops; i++) {
			for (size_t j = 0; j < params.n_pops; j++) {
				if (i == j) {
					pop_exc(i).connect_to(
					    pop_exc(j), Connector::fixed_fan_in(2, params.w_exc));
					pop_exc(i).connect_to(
					    pop_inh(j), Connector::fixed_fan_in(8, params.w_exc));
				}
				else {
					pop_inh(i).connect_to(
					    pop_exc(j), Connector::fixed_fan_in(8, params.w_inh));
				}
			}
		}
	}

	size_t size() const { return m_params.n_pops; }

	Population<Neuron> pop_exc() { return m_pop_exc; }

	Population<Neuron> pop_inh() { return m_pop_inh; }

	PopulationView<Neuron> pop_exc(size_t i)
	{
		return m_pop_exc(i * m_params.n_pop_size,
		                 (i + 1) * m_params.n_pop_size);
	}

	PopulationView<Neuron> pop_inh(size_t i)
	{
		return m_pop_inh(i * m_params.n_pop_size,
		                 (i + 1) * m_params.n_pop_size);
	}
};

/**
 * Writes the spike times per neuron to a CSV file. TODO: Somehow move this to
 * the Cypress core library.
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
		for (Real t : o) {
			if (!first) {
				os << ",";
			}
			os << t;
			first = false;
		}
		os << std::endl;
	}
}

int main(int argc, const char *argv[])
{
	if (argc != 2 && !NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0] << " <SIMULATOR>" << std::endl;
		return 1;
	}

	using Neuron = IfFacetsHardware1;
	using Parameters = Neuron::Parameters;
	using Signals = Neuron::Signals;

	Network net;
	WTAParameters params;
	WTA<IfFacetsHardware1> wta(net, params, Parameters(),
	                           Signals().record_spikes());

	// Create the input neurons
	const Real t_dur = 2000.0;  // ms
	const Real t_tot = params.n_pops * t_dur;

	// Sweep over a triangle function
	std::cout << "Creating input spike trains..." << std::endl;
	DeltaSigma::DiscreteWindow wnd =
	    DeltaSigma::DiscreteWindow::create<DeltaSigma::GaussWindow>();
	for (size_t i = 0; i < params.n_pops; i++) {
		const Real tMax = t_dur * (i + 0.5);
		const Real t0 = tMax - t_dur;
		const Real t1 = tMax + t_dur;
		const auto f = [&](Real t) -> Real {
			t = t * 1e3;
			if (t < t0 || t > t1) {
				return 0.0;
			}
			if (t < tMax) {
				return (t - t0) / (tMax - t0);
			}
			if (t > tMax) {
				return (t1 - t) / (t1 - tMax);
			}
			return 0.0;
		};
		std::vector<Real> spike_train =
		    DeltaSigma::encode(f, wnd, 0.0, t_tot * 1e-3, 0.0, 1.0);
		for (Real &t : spike_train) {
			t *= 1e3;  // s to ms
		}
		Population<SpikeSourceArray> src(net, 1, spike_train);
		src.connect_to(wta.pop_exc(i), Connector::all_to_all(params.w_exc));
	}

	// Run the simulation
	std::cout << "Running the simulation..." << std::endl;
	net.run(argv[1], t_tot, argc, argv);

	// Write the output spike times to disk
	std::cout << "Writing results to disk..." << std::endl;
	write_spike_times("winner_take_all_out.csv", wta.pop_exc(),
	                  [](const cypress::Neuron<Neuron> &a) {
		                  return a.signals().get_spikes();
		              });

	return 0;
}
