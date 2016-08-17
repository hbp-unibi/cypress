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

#include <cypress/transformations/spikey_if_cond_exp.hpp>
#include <cypress/transformations/spike_sources.hpp>

namespace cypress {
namespace transformations {
namespace {
struct Registry {
	Registry()
	{
		// Transformation responsible for losslessly transforming spikey neurons
		// to standard IfCondExp neurons
		Transformations::register_neuron_type_transformation<IfFacetsHardware1,
		                                                     IfCondExp>(
		    [] { return std::move(std::make_unique<IFFH1ToLIF>()); });

		// Transformation responsible for fixing Spikey units
		Transformations::register_general_transformation(
		    [] { return std::move(std::make_unique<IFFH1UnitScale>()); },
		    [](const Backend &, const NetworkBase &net) {
			    return net.population_count<IfFacetsHardware1>() > 0;
			});

		// Transformation responsible for losslessly converting between constant
		// frequency and constant interval neurons
		Transformations::register_neuron_type_transformation<
		    SpikeSourceConstInterval, SpikeSourceConstFreq>(
		    [] { return std::move(std::make_unique<CIToCF>()); });
		Transformations::register_neuron_type_transformation<
		    SpikeSourceConstFreq, SpikeSourceConstInterval>(
		    [] { return std::move(std::make_unique<CIToCF>()); });

		// Transformations responsible for converting from poisson and constant
		// frequency neurons to plain spike source arrays
		Transformations::register_neuron_type_transformation<SpikeSourcePoisson,
		                                                     SpikeSourceArray>(
		    [] { return std::move(std::make_unique<PoissonToSA>()); });
		Transformations::register_neuron_type_transformation<
		    SpikeSourceConstFreq, SpikeSourceArray>(
		    [] { return std::move(std::make_unique<CFToSA>()); });
	}
};
}

void *register_()
{
	static Registry registry;
	return &registry;
}
}
}
