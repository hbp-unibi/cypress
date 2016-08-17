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
 * @file spikey_if_cond_exp.hpp
 *
 * Contains transformations from IfFacetsHardware1 to IfCondExp and the other
 * way round. This allows to use spikey without constantly having to change the
 * neuron type being used in the network.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_TRANSFORMATIONS_SPIKEY_IF_COND_EXP_HPP
#define CYPRESS_TRANSFORMATIONS_SPIKEY_IF_COND_EXP_HPP

#include <cypress/core/transformation_util.hpp>

namespace cypress {
namespace transformations {
/**
 * Transforms an IfFacetsHardware1 neuron to a standard IfCondExp neuron.
 */
class IFFH1ToLIF
    : public NeuronTypeTransformation<IfFacetsHardware1, IfCondExp> {
protected:
	void do_transform_parameters(const IfFacetsHardware1Parameters &src,
	                             IfCondExpParameters tar) override;

	void do_transform_signals(const IfFacetsHardware1Signals &src,
	                          IfCondExpSignals tar) override;

public:
	std::string id() const override { return "IfFacetsHardware1ToIfCondExp"; }

	~IFFH1ToLIF(){};
};

/**
 * Adapts the Spikey neuron parameter unit scales -- for spikey, g_leak is
 * measured in nS, whereas it is measured uS in all other simulations. This
 * transformation takes care of this discrepancy.
 */
class IFFH1UnitScale : public Transformation {
protected:
	NetworkBase do_transform(const NetworkBase &src,
	                                 TransformationAuxData &aux) override;

public:
	std::string id() const override { return "IFFH1UnitScale"; }

	~IFFH1UnitScale(){};
};
}
}

#endif /* CYPRESS_TRANSFORMATIONS_SPIKEY_IF_COND_EXP_HPP */
