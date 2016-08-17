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

namespace cypress {
namespace transformations {

/*
 * Class IFFH1ToLIF
 */

void IFFH1ToLIF::do_transform_parameters(const IfFacetsHardware1Parameters &src,
                                         IfCondExpParameters tar)
{
	tar.cm(src.cm())
	    .tau_m(src.tau_m())
	    .tau_syn_E(src.tau_syn_E())
	    .tau_syn_I(src.tau_syn_I())
	    .tau_refrac(src.tau_refrac())
	    .v_rest(src.v_rest())
	    .v_thresh(src.v_thresh())
	    .v_reset(src.v_reset())
	    .e_rev_E(src.e_rev_E())
	    .e_rev_I(src.e_rev_I())
	    .i_offset(src.i_offset());
}

void IFFH1ToLIF::do_transform_signals(const IfFacetsHardware1Signals &src,
                                      IfCondExpSignals tar)
{
	tar.record_spikes(src.is_recording_spikes()).record_v(src.is_recording_v());
}

/*
 * Class IFFH1UnitScale
 */

NetworkBase IFFH1UnitScale::do_transform(const NetworkBase &src,
                                         TransformationAuxData &)
{
	NetworkBase res = src.clone();
	for (auto &pop : res.populations()) {
		if (&pop.type() == &IfFacetsHardware1::inst()) {
			auto iffh1_pop = Population<IfFacetsHardware1>(pop);
			iffh1_pop.parameters().g_leak(iffh1_pop.parameters().g_leak() *
			                              1000.0);  // convert uS to nS
		}
	}
	return res;
}
}
}

