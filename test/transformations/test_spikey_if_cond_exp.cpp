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

#include "gtest/gtest.h"

#include <cypress/transformations/spikey_if_cond_exp.hpp>

namespace cypress {

TEST(spikey_if_cond_exp, IFFH1ToLIF_Simple)
{
	Network net_src = Network().add_population<IfFacetsHardware1>("test", 10, IfFacetsHardware1::Parameters{});

	spikey::IFFH1ToLIF trafo;
	TransformationAuxData aux;
	NetworkBase net_tar = trafo.transform(net_src, aux);
}
}
