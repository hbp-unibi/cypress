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

#include <sstream>

#include <cypress/backend/pynn/pynn.hpp>

namespace cypress {
TEST(pynn, normalisation)
{
	EXPECT_EQ("NEST", PyNN("NEST").simulator());
	EXPECT_EQ("nest", PyNN("NEST").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.NEST", "pyNN.nest"}),
	          PyNN("NEST").imports());
	EXPECT_EQ("", PyNN("NEST").nmpi_platform());

	EXPECT_EQ("pynn.NEST", PyNN("pynn.NEST").simulator());
	EXPECT_EQ("nest", PyNN("pynn.NEST").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.NEST", "pyNN.nest"}),
	          PyNN("pynn.NEST").imports());
	EXPECT_EQ("", PyNN("pynn.NEST").nmpi_platform());

	EXPECT_EQ("spinnaker", PyNN("spinnaker").simulator());
	EXPECT_EQ("nmmc1", PyNN("spinnaker").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.spinnaker", "pyNN.spiNNaker"}),
	          PyNN("spinnaker").imports());
	EXPECT_EQ("NM-MC1", PyNN("spinnaker").nmpi_platform());

	EXPECT_EQ("spiNNaker", PyNN("spiNNaker").simulator());
	EXPECT_EQ("nmmc1", PyNN("spiNNaker").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.spiNNaker"}),
	          PyNN("spiNNaker").imports());
	EXPECT_EQ("NM-MC1", PyNN("spiNNaker").nmpi_platform());

	EXPECT_EQ("pyhmf", PyNN("pyhmf").simulator());
	EXPECT_EQ("nmpm1", PyNN("pyhmf").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.pyhmf", "pyhmf"}),
	          PyNN("pyhmf").imports());
	EXPECT_EQ("NM-PM1", PyNN("pyhmf").nmpi_platform());

	EXPECT_EQ("Spikey", PyNN("Spikey").simulator());
	EXPECT_EQ("spikey", PyNN("Spikey").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.Spikey", "pyNN.hardware.spikey"}),
	          PyNN("Spikey").imports());
	EXPECT_EQ("Spikey", PyNN("Spikey").nmpi_platform());

	EXPECT_EQ("pyNN.hardware.brainscales", PyNN("pyNN.hardware.brainscales").simulator());
	EXPECT_EQ("ess", PyNN("pyNN.hardware.brainscales").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.hardware.brainscales"}),
	          PyNN("pyNN.hardware.brainscales").imports());
	EXPECT_EQ("ESS", PyNN("pyNN.hardware.brainscales").nmpi_platform());
}

TEST(pynn, timestep)
{
	EXPECT_FLOAT_EQ(42.0, PyNN("bla", {{"timestep", 42.0}}).timestep());
	EXPECT_FLOAT_EQ(0.1, PyNN("nest").timestep());
	EXPECT_FLOAT_EQ(1.0, PyNN("nmmc1").timestep());
	EXPECT_FLOAT_EQ(0.0, PyNN("nmpm1").timestep());
	EXPECT_FLOAT_EQ(0.0, PyNN("spikey").timestep());
}

}
