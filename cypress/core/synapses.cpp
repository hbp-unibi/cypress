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
#include <memory>

#include <cypress/core/synapses.hpp>

namespace cypress{
std::shared_ptr<SynapseBase> SynapseBase::make_shared(SynapseBase &synapse)
{
	if (synapse.name() == "StaticSynapse") {
		return std::make_shared<StaticSynapse>(StaticSynapse(synapse.parameters()));
	}
	else if (synapse.name() == "SpikePairRuleAdditive") {
		return std::make_shared<SpikePairRuleAdditive>(
			SpikePairRuleAdditive(synapse.parameters()));
	}
	else {
		throw "New synapse type not registered internally";
	}
}
}