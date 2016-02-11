/*
 *  Cypress -- C++ Neural Associative Memory Simulator
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

#pragma once

#ifndef CYPRESS_CORE_NETWORK_HPP
#define CYPRESS_CORE_NETWORK_HPP

#include <cypress/core/neurons.hpp>

namespace cypress {

class Network;
class PopulationBase;

class NeuronBase {
private:
	std::weak_ptr<PopulationImpl> m_impl;
	size_t m_idx;

public:
	NeuronBase(PopulationBase &population, size_t idx);

	~NeuronBase();
};

class PopulationBase {
private:
	std::weak_ptr<PopulationImpl> m_impl;

public:
	PopulationBase(std::weak_ptr<PopulationImpl> impl);

	~PopulationBase();

	const NeuronType &type();

	size_t id() const;

	size_t size() const;

	bool homogeneous() const;

	void parameters(const NeuronParametersBase &params);

	void parameters(const std::vector<NeuronParametersBase> &params);

	void parameters(size_t idx, const NeuronParametersBase &params);

	const NeuronParametersBase &parameters(size_t idx) const;
};

template <typename Type>
class Neuron {
public:
	using Parameters = Type::Parameters;

	Parameters &parameters()
}

template <typename Type>
class Population {
public:
}

class Network {
public:
	template <typename Type>
	Population<Type> addPopulation(
	    size_t count,
	    const typename Type::Parmameters &params = NeuronType::Parameters());
};
}

#endif /* CYPRESS_CORE_NETWORK_HPP */
