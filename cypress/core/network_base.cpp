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

#include <algorithm>

#include <cypress/core/internal/network_impl.hpp>
#include <cypress/core/backend.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/core/network_base_objects.hpp>

namespace cypress {
/*
 * Class NetworkBase
 */

NetworkBase::NetworkBase() : m_impl(std::make_shared<internal::NetworkImpl>())
{
	// Do nothing here
}

NetworkBase::~NetworkBase() = default;

NetworkBase NetworkBase::clone() const
{
	return NetworkBase(std::make_shared<internal::NetworkImpl>(*m_impl));
}

size_t NetworkBase::population_count() const
{
	return m_impl->populations().size();
}

size_t NetworkBase::population_size(PopulationIndex pid) const
{
	return m_impl->populations()[pid].size;
}

const NeuronType &NetworkBase::population_type(PopulationIndex pid) const
{
	return *(m_impl->populations()[pid].type);
}

const std::string &NetworkBase::population_name(PopulationIndex pid) const
{
	return m_impl->populations()[pid].name;
}

void NetworkBase::population_name(PopulationIndex pid, const std::string &name)
{
	m_impl->populations()[pid].name = name;
}

bool NetworkBase::homogeneous(PopulationIndex pid)
{
	return m_impl->populations()[pid].parameters.size() <= 1;
}

const NeuronParametersBase &NetworkBase::parameters(PopulationIndex pid,
                                                    NeuronIndex nid) const
{
	const internal::PopulationImpl &pop = m_impl->populations()[pid];
	const size_t n_params = pop.parameters.size();
	if (n_params == 0) {
		return pop.type->parameter_defaults;
	}
	if (n_params == 1 || nid >= NeuronIndex(n_params)) {
		return pop.parameters[0];
	}
	return pop.parameters[nid];
}

static void heterogenise(internal::PopulationImpl &pop, NeuronIndex nid0,
                         NeuronIndex nid1)
{
	// If the parameter vector is currently homogeneous, resize it such that it
	// contains an entry for each neuron in the population. Then store the old
	// parameter set in the neurons
	if (pop.size > 1 && pop.parameters.size() <= 1) {
		// Fetch the old paramter set
		NeuronParametersBase old = (pop.parameters.size() == 0)
		                               ? pop.type->parameter_defaults
		                               : pop.parameters[0];

		// Resize the parameter storage
		pop.parameters.resize(pop.size);

		// Fill the old parameters in, leave a gap for the new parameters
		std::fill(pop.parameters.begin(), pop.parameters.begin() + nid0, old);
		std::fill(pop.parameters.begin() + nid1, pop.parameters.end(), old);
	}
}

void NetworkBase::parameters(PopulationIndex pid, NeuronIndex nid0,
                             NeuronIndex nid1,
                             const std::vector<NeuronParametersBase> &params)
{
	internal::PopulationImpl &pop = m_impl->populations()[pid];

	if (params.size() == 0) {
		parameters(pid, nid0, nid1, pop.type->parameter_defaults);
	}
	else if (params.size() == 1) {
		parameters(pid, nid0, nid1, params[0]);
	}
	else {
		if (NeuronIndex(params.size()) != (nid1 - nid0)) {
			throw InvalidParameterArraySize(
			    "The parameter array must have the same size as the target "
			    "Population, PopulationView or Neuron.");
		}

		heterogenise(pop, nid0, nid1);
		std::copy(params.begin(), params.end(), pop.parameters.begin() + nid0);
	}
}

void NetworkBase::parameters(PopulationIndex pid, NeuronIndex nid0,
                             NeuronIndex nid1,
                             const NeuronParametersBase &params)
{
	internal::PopulationImpl &pop = m_impl->populations()[pid];

	// If the parameters of all neurons in the population are set, only use
	// a single entry in the parameters vector to indicate that the
	// parameters should be set homogeneously
	if (nid0 == 0 && nid1 == NeuronIndex(pop.size)) {
		pop.parameters.clear();  // No parameters => default parameters
		if (&params != &pop.type->parameter_defaults) {
			pop.parameters.emplace_back(params);  // One entry => homogeneous
		}
	}
	else {
		heterogenise(pop, nid0, nid1);
		std::fill(pop.parameters.begin() + nid0, pop.parameters.begin() + nid1,
		          params);
	}
}

void NetworkBase::parameter(PopulationIndex pid, NeuronIndex nid0,
                            NeuronIndex nid1, size_t index,
                            const std::vector<float> &values)
{
	if (NeuronIndex(values.size()) != (nid1 - nid0)) {
		throw InvalidParameterArraySize(
		    "The values array must have the same size as the target "
		    "Population, PopulationView or Neuron.");
	}

	internal::PopulationImpl &pop = m_impl->populations()[pid];
	heterogenise(pop, 0, pop.size);
	for (size_t i = 0; i < values.size(); i++) {
		auto &params = pop.parameters[nid0 + i];
		if (index < params.size()) {
			params[index] = values[i];
		}
		else {
			throw std::out_of_range("Given parameter index is out of range.");
		}
	}
}

void NetworkBase::parameter(PopulationIndex pid, NeuronIndex nid0,
                            NeuronIndex nid1, size_t index, float value)
{
	internal::PopulationImpl &pop = m_impl->populations()[pid];
	if (nid0 == 0 && nid1 == NeuronIndex(pop.size) &&
	    pop.parameters.size() <= 1) {
		if (pop.parameters.size() == 0) {
			pop.parameters.emplace_back(pop.type->parameter_defaults);
		}
		nid0 = 0;
		nid1 = 0;
	}
	else {
		heterogenise(pop, 0, pop.size);
	}
	for (NeuronIndex nid = nid0; nid < nid1; nid++) {
		if (index < pop.parameters[nid].size()) {
			pop.parameters[nid][index] = value;
		}
		else {
			throw std::out_of_range("Given parameter index is out of range.");
		}
	}
}

PopulationBase NetworkBase::population(PopulationIndex pid)
{
	return PopulationBase(*this, pid);
}

void NetworkBase::connect(PopulationIndex pid_src, NeuronIndex nid_src0,
                          NeuronIndex nid_src1, PopulationIndex pid_tar,
                          NeuronIndex nid_tar0, NeuronIndex nid_tar1,
                          std::unique_ptr<Connector> connector)
{
	m_impl->connect(ConnectionDescriptor(pid_src, nid_src0, nid_src1, pid_tar,
	                                     nid_tar0, nid_tar1,
	                                     std::move(connector)));
}

PopulationIndex NetworkBase::create_population_index(
    size_t size, const NeuronType &type,
    const std::vector<NeuronParametersBase> &params, const std::string &name)
{
	if (params.size() > 1 && params.size() != size) {
		throw InvalidParameterArraySize(
		    "The parameter array must have as many entries as there are "
		    "neurons in the population.");
	}
	m_impl->populations().emplace_back(size, type, params, name);
	return population_count() - 1;
}

const std::vector<PopulationBase> NetworkBase::populations(
    const std::string &name, const NeuronType &type) const
{
	std::vector<PopulationIndex> pids = m_impl->populations(name, type);
	std::vector<PopulationBase> res;
	for (PopulationIndex pid : pids) {
		res.emplace_back(*this, pid);
	}
	return res;
}

const std::vector<PopulationBase> NetworkBase::populations(
    const NeuronType &type) const
{
	return populations(std::string(), type);
}

PopulationBase NetworkBase::population(const std::string &name)
{
	auto pops = populations(name);
	if (pops.empty()) {
		throw NoSuchPopulationException(std::string("Population with name \"") +
		                                name + "\" does not exist");
	}
	return pops.back();
}

const std::vector<ConnectionDescriptor> &NetworkBase::connections() const
{
	return m_impl->connections();
}

void NetworkBase::run(const Backend &backend, float duration)
{
	backend.run(*this, duration);
}

float NetworkBase::duration() const
{
	float res = 0.0;
	for (const auto &population : populations()) {
		if (&population.type() == &SpikeSourceArray::inst()) {
			const NeuronIndex nid_end =
			    population.homogeneous() ? 1 : population.size();
			for (NeuronIndex nid = 0; nid < nid_end; nid++) {
				auto &params = population.parameters(nid);
				if (params.size() > 0) {
					// Note: the spike times are supposed to be sorted!
					res = std::max(res, params[params.size() - 1]);
				}
			}
		}
	}
	return res;
}
}
