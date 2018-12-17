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

#ifndef CYPRESS_UTILS_NEURON_PARAMETERS_HPP
#define CYPRESS_UTILS_NEURON_PARAMETERS_HPP


#include <array>
#include <iostream>
#include <vector>
#include <string>


#include <cypress/core/network.hpp>
#include "logger.hpp"

/**
 * Macro used for defining the getters and setters associated with a parameter
 * value.
 */
#define NAMED_PARAMETER(NAME, IDX)            \
	static constexpr size_t idx_##NAME = IDX; \
	void NAME(Real x) { arr[IDX] = x; }       \
	Real &NAME() { return arr[IDX]; }         \
	Real NAME() const { return arr[IDX]; }

namespace cypress {

class NeuronParameter {
private:
	std::vector<Real> m_params;
	std::vector<std::string> m_parameter_names;

public:
	/**
	 * Construct from Json, logging values to DEBUG
	 */
	NeuronParameter(const NeuronType &type,
	                 const Json &json);

	NeuronParameter(){};

	const std::vector<Real> &parameter() const { return m_params; };

	/**
	 * Set parameter with name @param name to @param value
	 */
	NeuronParameter &set(std::string name, cypress::Real value)
	{
		for (size_t i = 0; i < m_parameter_names.size(); i++) {
			if (m_parameter_names[i] == name) {
				m_params[i] = value;
				return *this;
			}
		}
		throw std::invalid_argument("Unknown neuron parameter" + name);
	}

	cypress::Real get(std::string name)
	{
		for (size_t i = 0; i < m_parameter_names.size(); i++) {
			if (m_parameter_names[i] == name) {
				return m_params[i];
			}
		}
		throw std::invalid_argument("Unknown neuron parameter" + name);
	}
	void print(std::ostream &out = std::cout)
	{
		out << "# Neuron Parameters: " << std::endl;
		for (size_t i = 0; i < m_params.size(); i++) {
			out << m_parameter_names[i] << ": " << m_params[i] << std::endl;
		}
	}
};

/**
 * Store input form json in a map to check everything!
 */
template <typename T>
std::map<std::string, T> json_to_map(const cypress::Json &obj)
{
	std::map<std::string, T> input;
	for (auto i = obj.begin(); i != obj.end(); i++) {
		const Json value = i.value();
		// Suppress entries like Neuron_Type and sub-lists
		if (value.is_number()) {
			input.emplace(i.key(), value);
		}
	}
	return input;
}

template <typename T>
std::vector<T> read_check(std::map<std::string, T> &input,
                          const std::vector<std::string> &names,
                          const std::vector<T> &defaults)
{
	std::vector<T> res(names.size());
	for (size_t i = 0; i < res.size(); i++) {
		auto it = input.find(names[i]);
		if (it != input.end()) {
			res[i] = it->second;
			input.erase(it);
		}
		else {
			res[i] = defaults.at(i);
			global_logger().debug("Cypress",
			                      "For " + names[i] + " the default value " +
			                          std::to_string(defaults[i]) + " is used");
		}
	}
	for (auto elem : input) {
		throw std::invalid_argument("Unknown parameter \"" + elem.first + "\"");
	}

	return res;
}


}
#undef NAMED_PARAMETER
#endif /* CYPRESS_UTILS_NEURON_PARAMETERS_HPP */
