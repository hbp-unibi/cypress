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

#include <cypress/cypress.hpp>

#include "gtest/gtest.h"

namespace cypress {

TEST(v_init, v_init)
{
	auto v_rest = [](size_t i) { return -80.0f + i * 2.0f; };

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
	for (const std::string &sim : PyNN::simulators()) {
		run("pynn." + sim);
	}

	// Run on the native NEST backend if available
	if (NEST::installed()) {
		run("nest");
	}
}
TEST(spike_source, seed)
{
	Network net;
	net.logger().min_level(LogSeverity::WARNING);
	Population<SpikeSourcePoisson> pop1(
	    net, 10,
	    SpikeSourcePoissonParameters().start(5).duration(500).rate(50).seed(
	        1234),
	    SpikeSourcePoissonSignals().record_spikes());
	Population<SpikeSourcePoisson> pop2(
	    net, 10,
	    SpikeSourcePoissonParameters().start(5).duration(500).rate(50).seed(
	        1234),
	    SpikeSourcePoissonSignals().record_spikes());
	Population<SpikeSourcePoisson> pop3(
	    net, 10,
	    SpikeSourcePoissonParameters().start(5).duration(500).rate(50).seed(
	        1236),
	    SpikeSourcePoissonSignals().record_spikes());
	Population<SpikeSourcePoisson> pop4(
	    net, 10, SpikeSourcePoissonParameters().start(5).duration(500).rate(50),
	    SpikeSourcePoissonSignals().record_spikes());
	Population<SpikeSourcePoisson> pop5(
	    net, 10, SpikeSourcePoissonParameters().start(5).duration(500).rate(50),
	    SpikeSourcePoissonSignals().record_spikes());

	net.run("genn");
	auto spikes1 = pop1[0].signals().get_spikes();
	auto spikes12 = pop1[1].signals().get_spikes();
	auto spikes2 = pop2[0].signals().get_spikes();
	auto spikes3 = pop3[0].signals().get_spikes();
	auto spikes4 = pop4[0].signals().get_spikes();
	auto spikes5 = pop5[0].signals().get_spikes();
	EXPECT_EQ(spikes1.size(), spikes2.size());
	if (spikes1.size() == spikes2.size()) {
		for (size_t i = 0; i < spikes1.size(); i++) {
			EXPECT_FLOAT_EQ(spikes1[i], spikes2[i]);
		}
	}
	EXPECT_FALSE(spikes1[0] == spikes12[0]);
	EXPECT_FALSE(spikes1[1] == spikes12[1]);
	EXPECT_FALSE(spikes1[2] == spikes12[2]);
	EXPECT_FALSE(spikes1[3] == spikes12[3]);

	EXPECT_FALSE(spikes1[0] == spikes3[0]);
	EXPECT_FALSE(spikes1[1] == spikes3[1]);
	EXPECT_FALSE(spikes1[2] == spikes3[2]);
	EXPECT_FALSE(spikes1[3] == spikes3[3]);

	EXPECT_FALSE(spikes1[0] == spikes4[0]);
	EXPECT_FALSE(spikes1[1] == spikes4[1]);
	EXPECT_FALSE(spikes1[2] == spikes4[2]);
	EXPECT_FALSE(spikes1[3] == spikes4[3]);

	EXPECT_FALSE(spikes1[0] == spikes5[0]);
	EXPECT_FALSE(spikes1[1] == spikes5[1]);
	EXPECT_FALSE(spikes1[2] == spikes5[2]);
	EXPECT_FALSE(spikes1[3] == spikes5[3]);

	EXPECT_FALSE(spikes4[0] == spikes5[0]);
	EXPECT_FALSE(spikes4[1] == spikes5[1]);
	EXPECT_FALSE(spikes4[2] == spikes5[2]);
	EXPECT_FALSE(spikes4[3] == spikes5[3]);

	Network net2;
	Population<SpikeSourcePoisson> pop1_1(
	    net2, 10,
	    SpikeSourcePoissonParameters().start(5).duration(500).rate(50).seed(
	        1234),
	    SpikeSourcePoissonSignals().record_spikes());

	net2.run("genn");
	auto spikes1_1 = pop1_1[0].signals().get_spikes();
	EXPECT_EQ(spikes1.size(), spikes1_1.size());
	if (spikes1.size() == spikes1_1.size()) {
		for (size_t i = 0; i < spikes1.size(); i++) {
			EXPECT_FLOAT_EQ(spikes1[i], spikes1_1[i]);
		}
	}

	Network net3;
	Population<SpikeSourcePoisson> pop2_1(
	    net3, 10,
	    SpikeSourcePoissonParameters().start(5).duration(500).rate(50),
	    SpikeSourcePoissonSignals().record_spikes());
	RNG::instance().seed(1234);
	net3.run("genn");
	auto spikes2_1 = pop2_1[0].signals().get_spikes();
	EXPECT_EQ(spikes1.size(), spikes2_1.size());
	if (spikes1.size() == spikes2_1.size()) {
		for (size_t i = 0; i < spikes2_1.size(); i++) {
			EXPECT_FLOAT_EQ(spikes1[i], spikes2_1[i]);
		}
	}

	Network net4;
	Population<SpikeSourcePoisson> pop3_1(
	    net4, 10,
	    SpikeSourcePoissonParameters().start(5).duration(500).rate(50),
	    SpikeSourcePoissonSignals().record_spikes());
	RNG::instance().seed(1236);
	net4.run("genn");
	auto spikes3_1 = pop3_1[0].signals().get_spikes();
	EXPECT_FALSE(spikes1[0] == spikes3_1[0]);
	EXPECT_FALSE(spikes1[1] == spikes3_1[1]);
	EXPECT_FALSE(spikes1[2] == spikes3_1[2]);
	EXPECT_FALSE(spikes1[3] == spikes3_1[3]);
}
}  // namespace cypress
