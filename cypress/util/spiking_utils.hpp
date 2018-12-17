/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2018  Christoph Ostrau
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

#pragma once

#ifndef CYPRESS_UTIL_SPIKING_UTILS_HPP
#define CYPRESS_UTIL_SPIKING_UTILS_HPP

#include <cypress/core/network.hpp>
#include <cypress/util/neuron_parameters.hpp>

namespace cypress {
class SpikingUtils {
public:
	/**
	 * Creates a population of type T and adds them to m_net
	 *
	 * @param network instace in wich the population is placed
	 * @param neuronParams Neuron parameters of the cell in the population
	 * @param size Number of neurons in the population
	 * @param rec_signal signal to be recorded by the backend
	 * @return Network containing the add Population (same as network)
	 */
	template <typename T>
	static PopulationBase add_typed_population(
	    Network &network, const NeuronParameter &neuronParams,
	    const size_t size,
	    const typename T::Signals &rec_signal =
	        typename T::Signals().record_spikes())
	{
		using Parameters = typename T::Parameters;
		return network.create_population<T>(
		    size, Parameters(neuronParams.parameter()), rec_signal);
	}

	/**
	 * Creates a population of type T and adds them to m_net without
	 * recording
	 *
	 * @param network instace in wich the population is placed
	 * @param neuronParams Neuron parameters of the cell in the population
	 * @param size Number of neurons in the population
	 * @return Network containing the add Population (same as network)
	 */
	template <typename T>
	static cypress::PopulationBase add_typed_population_no_record(
	    Network &network, const NeuronParameter &neuronParams,
	    const size_t size);

	/**
	 * Generates a NeuronType object from a string
	 *
	 * @param neuron_type_str string naming the neuron type
	 * @return the corresponding NeuronType object
	 */
	static const NeuronType &detect_type(std::string neuron_type_str);

	/**
	 * Runs SpikingUtils::add_typed_population, but gets a string containing the
	 * neuron type instead of a template argument
	 *
	 * @param neuron_type_str string naming the neuron type
	 * @param network instace in wich the population is placed
	 * @param neuronParams Neuron parameters of the cell in the population
	 * @param size Number of neurons in the population
	 * @param record_signal string of the signal to be recorded by the backend,
	 * e.g. "spikes" or "v"
	 */
	static cypress::PopulationBase add_population(
	    const std::string neuron_type_str, Network &network,
	    const NeuronParameter &neuronParams, const size_t size,
	    const std::string record_signal = "");

	/**
	 * Tries to run the simulation on given backend several times if backend
	 * produces an unexpected error (needed for e.g. BrainScaleS).
	 * @param network network object to simulate
	 * @param backend target of the simulation
	 * @param time simulation time
	 * @param n_trials number of trials before giving up
	 * @return true if simulation was succesful
	 */
	static bool rerun_fixed_number_trials(Network &network, Backend &backend,
	                                      Real time, size_t n_trials = 3);

	/**
	 * @brief Calculate the number of spikes of spiketrain in interval
	 * [start,stop]
	 *
	 * @param spiketrain The spiketrain for which the number of spikes should be
	 * calculated
	 * @param start Timepoint from which counting starts
	 * @param end Endpoint where counting stops
	 * @return int returns the number of spikes in [start, stop]
	 */
	static int calc_num_spikes(const std::vector<cypress::Real> &spiketrain,
	                           const cypress::Real start = 0.0,
	                           const cypress::Real end = 0.0);

	/**
	 * @brief Calculate the number of spikes in a vector of spiketrains in
	 * intervall [start,stop]
	 *
	 * @param spiketrains The vector of spiketrains for which the number of
	 * spikes should be calculated
	 * @param start Timepoint from which counting starts
	 * @param end Endpoint where counting stops, leave empty to count all
	 * @return int returns the number of spikes in [start, stop]
	 */
	template <typename T>
	static std::vector<int> calc_num_spikes_vec(
	    const cypress::Matrix<T> &spiketrains, const cypress::Real start = 0.0,
	    const cypress::Real end = std::numeric_limits<cypress::Real>::max())
	{
		std::vector<int> res;
		size_t rows = spiketrains.rows();
		size_t colls = spiketrains.cols();
		for (size_t i = 0; i < rows; i++) {
			int counter = 0;
			for (size_t j = 0; j < colls; j++) {
				if ((spiketrains[i * colls + j] >= start - 0.001) &&
				    (spiketrains[i * colls + j] <= end + 0.001)) {
					counter++;
				}
			}
			res.push_back(counter);
		}
		return res;
	}

	/**
	 * Calculate the number of spikes in given intervals (bins) of a single
	 * spike train. Size of bins is calculated from start, stop and the number
	 * of bins n_bins
	 * @param start time for starting the binning. First bin is [start, start +
	 * bin_size]
	 * @param stop end time of last bin
	 * @param n_bins number of bins
	 * @param spike_times vector containing spike times as given by
	 * neuron.signals().data(0)
	 * @return A vector with each entry representing a bin, containing the
	 * number of spikes that appeared in that bin
	 */
	template <typename T>
	static std::vector<T> spike_time_binning(
	    const Real &start, const Real &stop, const size_t &n_bins,
	    const std::vector<cypress::Real> &spike_times)
	{
		Real bin_size = (stop - start) / n_bins;
		std::vector<T> bin_counts(n_bins, 0);
		for (Real spike : spike_times) {
			if (spike >= stop || spike < start) {
				continue;
			}
			size_t bin_idx = size_t((spike - start) / bin_size);
			bin_counts[bin_idx]++;
		}
		return bin_counts;
	}
};
}  // namespace cypress

#endif /* CYPRESS_UTIL_SPIKING_UTILS_HPP */
