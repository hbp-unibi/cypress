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

#include <cypress/cypress.hpp>

namespace cypress {

TEST(v_init, v_init)
{
	auto v_rest = [](size_t i) { return -80.0f + i * 2.0f;};

	auto run = [v_rest](const std::string &sim) {
		std::cout << "v_init integration test running on " << sim << std::endl;
		Network net;
		net.logger().min_level(LogSeverity::DEBUG);
		Population<IfCondExp> pop1(net, 10);
		Population<IfCondExp> pop2(net, 5);
		pop1.signals().record_v();
		pop2.signals().record_v();
		for (size_t i = 0; i < pop1.size(); i++) {
			pop1[i].parameters().v_rest(v_rest(i));
		}
		pop2.parameters().v_rest(-77.0);
		net.run(sim, 1000.0);

		for (size_t i = 0; i < pop1.size(); i++) {
			const auto &data = pop1[i].signals().get_v();
			for (size_t j = 0; j < 1; j++) {
				EXPECT_NEAR(v_rest(i), data(j, 1), 1.0f);
			}
		}
		for (size_t i = 0; i < pop2.size(); i++) {
			const auto &data = pop2[i].signals().get_v();
			for (size_t j = 0; j < 1; j++) {
				EXPECT_NEAR(-77.0, data(j, 1), 1.0f);
			}
		}
	};

	// Run on all available PyNN platforms
	for (const std::string &sim: PyNN::simulators()) {
		run("pynn." + sim);
	}

	// Run on the native NEST backend if available
	if (NEST::installed()) {
		run("nest");
	}
}
}
