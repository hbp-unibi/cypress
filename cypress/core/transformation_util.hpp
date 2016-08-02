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
 * @file transformation_util.hpp
 *
 * Contains some helper classes for dealing with network transformations.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_CORE_TRANSFORMATION_UTIL_HPP
#define CYPRESS_CORE_TRANSFORMATION_UTIL_HPP

#include <cypress/core/network.hpp>
#include <cypress/core/transformation.hpp>

namespace cypress {
/**
 * Abstract base class which facilitates the implementation of neuron type
 * transformations.
 */
template <typename SourceType, typename TargetType>
class NeuronTypeTransformation : public Transformation {
protected:
	const NeuronType *src_type() { return &SourceType::inst(); }
	const NeuronType *tar_type() { return &TargetType::inst(); }

	NetworkBase do_transform(const NetworkBase &src,
	                         TransformationAuxData &) override
	{
		NetworkBase tar = src.clone();
		for (size_t i = 0; i < src.population_count(); i++) {
			auto src_data = src.population_data(i);
			if (src_data->type() == src_type()) {
				// Fetch the pointer at the source data
				auto tar_data = tar.population_data(i);

				// Create a new PopulationData instance without the source data
				// and adapted target type at the pointer location
				const size_t size = src_data->size();
				*tar_data = PopulationData(size, tar_type(), src_data->name());

				// Initialize the parameters and signals of the new population
				NeuronParameters(tar_data, 0, size) =
				    typename TargetType::Parameters();
				NeuronSignals(tar_data, 0, size) =
				    typename TargetType::Signals();

				// Fetch the source and target population
				auto src_pop = Population<SourceType>(PopulationBase(src, i));
				auto tar_pop = Population<TargetType>(PopulationBase(tar, i));

				// Perform the parameter transformation -- only perform the
				// parameter transformation once in case the parameters are
				// homogeneous -- otherwise iterate over each individual neuron
				if (src_data->homogeneous_parameters() &&
				    !do_dehomogenise_parameters(src_pop.parameters())) {
					do_transform_parameters(src_pop.parameters(),
					                        tar_pop.parameters());
				}
				else {
					for (size_t j = 0; j < src_pop.size(); j++) {
						do_transform_parameters(src_pop[j].parameters(),
						                        tar_pop[j].parameters());
					}
				}

				// Similarly to above, transform the record flags
				if (src_data->homogeneous_record()) {
					do_transform_signals(src_pop.signals(), tar_pop.signals());
				}
				else {
					for (size_t j = 0; j < src_pop.size(); j++) {
						do_transform_signals(src_pop[j].signals(),
						                     tar_pop[j].signals());
					}
				}
			}
		}
		return tar;
	}

	virtual void do_transform_parameters(
	    const typename SourceType::Parameters &src,
	    typename TargetType::Parameters tar) = 0;

	virtual bool do_dehomogenise_parameters(
	    const typename SourceType::Parameters &)
	{
		return false;
	}

	virtual void do_transform_signals(const typename SourceType::Signals &src,
	                                  typename TargetType::Signals tar) = 0;

public:
	virtual ~NeuronTypeTransformation(){};
};
}

#endif /* CYPRESS_CORE_TRANSFORMATION_UTIL_HPP */
