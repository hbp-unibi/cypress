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

#include <cypress/core/connector.hpp>

namespace cypress {

// 	for (auto c : connections) {
// 		std::cout << c.psrc << ", " << c.ptar << ", " << c.n.src << ", "
// 		          << c.n.tar << ", " << c.n.SynapseParameters[0] << ", "
// 		          << c.n.SynapseParameters[1] << std::endl;
// 	}

TEST(connector, all_to_all)
{
	std::vector<Connection> connections = instantiate_connections({
	    {0, 0, 2, 1, 0, 5, Connector::all_to_all(0.16, 0.1)},
	    {0, 2, 4, 2, 0, 5, Connector::all_to_all(0.1, 0.05)},
	});

	EXPECT_EQ(std::vector<Connection>({
	              {0, 1, 0, 0, 0.16, 0.1}, {0, 1, 0, 1, 0.16, 0.1},
	              {0, 1, 0, 2, 0.16, 0.1}, {0, 1, 0, 3, 0.16, 0.1},
	              {0, 1, 0, 4, 0.16, 0.1}, {0, 1, 1, 0, 0.16, 0.1},
	              {0, 1, 1, 1, 0.16, 0.1}, {0, 1, 1, 2, 0.16, 0.1},
	              {0, 1, 1, 3, 0.16, 0.1}, {0, 1, 1, 4, 0.16, 0.1},

	              {0, 2, 2, 0, 0.1, 0.05}, {0, 2, 2, 1, 0.1, 0.05},
	              {0, 2, 2, 2, 0.1, 0.05}, {0, 2, 2, 3, 0.1, 0.05},
	              {0, 2, 2, 4, 0.1, 0.05}, {0, 2, 3, 0, 0.1, 0.05},
	              {0, 2, 3, 1, 0.1, 0.05}, {0, 2, 3, 2, 0.1, 0.05},
	              {0, 2, 3, 3, 0.1, 0.05}, {0, 2, 3, 4, 0.1, 0.05},
	          }),
	          connections);

	connections = instantiate_connections({
	    {0, 0, 2, 1, 0, 5, Connector::all_to_all(0.16, 0.1, false)},
	    {0, 2, 4, 2, 0, 5, Connector::all_to_all(0.1, 0.05, false)},
	});

	EXPECT_EQ(std::vector<Connection>({
	              {0, 1, 0, 0, 0.16, 0.1}, {0, 1, 0, 1, 0.16, 0.1},
	              {0, 1, 0, 2, 0.16, 0.1}, {0, 1, 0, 3, 0.16, 0.1},
	              {0, 1, 0, 4, 0.16, 0.1}, {0, 1, 1, 0, 0.16, 0.1},
	              {0, 1, 1, 1, 0.16, 0.1}, {0, 1, 1, 2, 0.16, 0.1},
	              {0, 1, 1, 3, 0.16, 0.1}, {0, 1, 1, 4, 0.16, 0.1},
	              {0, 2, 2, 0, 0.1, 0.05}, {0, 2, 2, 1, 0.1, 0.05},
	              {0, 2, 2, 2, 0.1, 0.05}, {0, 2, 2, 3, 0.1, 0.05},
	              {0, 2, 2, 4, 0.1, 0.05}, {0, 2, 3, 0, 0.1, 0.05},
	              {0, 2, 3, 1, 0.1, 0.05}, {0, 2, 3, 2, 0.1, 0.05},
	              {0, 2, 3, 3, 0.1, 0.05}, {0, 2, 3, 4, 0.1, 0.05},
	          }),
	          connections);

	connections = instantiate_connections(
	    {{0, 0, 2, 0, 0, 5, Connector::all_to_all(0.16, 0.1, true)}});

	EXPECT_EQ(std::vector<Connection>({{0, 0, 0, 0, 0.16, 0.1},
	                                   {0, 0, 0, 1, 0.16, 0.1},
	                                   {0, 0, 0, 2, 0.16, 0.1},
	                                   {0, 0, 0, 3, 0.16, 0.1},
	                                   {0, 0, 0, 4, 0.16, 0.1},
	                                   {0, 0, 1, 0, 0.16, 0.1},
	                                   {0, 0, 1, 1, 0.16, 0.1},
	                                   {0, 0, 1, 2, 0.16, 0.1},
	                                   {0, 0, 1, 3, 0.16, 0.1},
	                                   {0, 0, 1, 4, 0.16, 0.1}}),
	          connections);

	// pid_src nid_src0 nid_src1 pid_tar nid_tar0 nid_tar1 connector
	connections = instantiate_connections(
	    {{0, 0, 2, 0, 0, 5, Connector::all_to_all(0.16, 0.1, false)}});

	EXPECT_EQ(std::vector<Connection>({{0, 0, 0, 1, 0.16, 0.1},
	                                   {0, 0, 0, 2, 0.16, 0.1},
	                                   {0, 0, 0, 3, 0.16, 0.1},
	                                   {0, 0, 0, 4, 0.16, 0.1},
	                                   {0, 0, 1, 0, 0.16, 0.1},
	                                   {0, 0, 1, 2, 0.16, 0.1},
	                                   {0, 0, 1, 3, 0.16, 0.1},
	                                   {0, 0, 1, 4, 0.16, 0.1}}),
	          connections);
}

TEST(connector, one_to_one)
{
	std::vector<Connection> connections = instantiate_connections({
	    {0, 0, 2, 1, 3, 5, Connector::one_to_one(0.16, 0.1)},
	    {0, 2, 4, 2, 10, 12, Connector::one_to_one(0.1, 0.05)},
	});

	EXPECT_EQ(std::vector<Connection>({
	              {0, 1, 0, 3, 0.16, 0.1},
	              {0, 1, 1, 4, 0.16, 0.1},
	              {0, 2, 2, 10, 0.1, 0.05},
	              {0, 2, 3, 11, 0.1, 0.05},
	          }),
	          connections);
}

TEST(connector, from_list)
{
	std::vector<Connection> connections = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::from_list({
	         {0, 1, 0.16, 0.0},
	         {10, 8, 0.1, 0.0},
	         {11, 3, 0.12},
	         {12, 3, 0.00, 0.0},
	         {12, 3, 0.05},
	         {14, 3, -0.05, 0.0},
	         {12, 2, 0.00, -1.0},
	     })},
	});

	EXPECT_EQ(std::vector<Connection>({
	              {0, 1, 0, 1, 0.16, 0.0},
	              {0, 1, 10, 8, 0.1, 0.0},
	              {0, 1, 11, 3, 0.12},
	              {0, 1, 12, 3, 0.05},
	              {0, 1, 14, 3, -0.05, 0.0},
	          }),
	          connections);
}

TEST(connector, from_list_limited_range)
{
	std::vector<Connection> connections = instantiate_connections({
	    {0, 0, 16, 1, 0, 4,
	     Connector::from_list({
	         {0, 1, 0.16, 0.0},
	         {10, 8, 0.1, 0.0},
	         {11, 3, 0.12},
	         {12, 3, 0.00, 0.0},
	         {12, 3, 0.05},
	         {14, 3, -0.05, 0.0},
	         {12, 2, 0.00, -1.0},
	     })},
	});

	EXPECT_EQ(std::vector<Connection>({
	              {0, 1, 0, 1, 0.16, 0.0},
	              {0, 1, 11, 3, 0.12},
	              {0, 1, 12, 3, 0.05},
	              {0, 1, 14, 3, -0.05, 0.0},
	          }),
	          connections);
}

TEST(connector, functor)
{
	std::vector<Connection> connections = instantiate_connections({
	    {0, 0, 16, 1, 0, 4,
	     Connector::functor([](NeuronIndex src, NeuronIndex tar) {
		     if (src == tar) {
			     return Synapse(0.16, 0.1);
		     }
		     return Synapse();
	     })},
	});

	EXPECT_EQ(std::vector<Connection>({
	              {0, 1, 0, 0, 0.16, 0.1},
	              {0, 1, 1, 1, 0.16, 0.1},
	              {0, 1, 2, 2, 0.16, 0.1},
	              {0, 1, 3, 3, 0.16, 0.1},
	          }),
	          connections);
}

TEST(connector, uniform_functor)
{
	std::vector<Connection> connections = instantiate_connections({
	    {0, 0, 16, 1, 0, 4,
	     Connector::functor(
	         [](NeuronIndex src, NeuronIndex tar) { return src == tar; }, 0.16,
	         0.1)},
	});

	EXPECT_EQ(std::vector<Connection>({
	              {0, 1, 0, 0, 0.16, 0.1},
	              {0, 1, 1, 1, 0.16, 0.1},
	              {0, 1, 2, 2, 0.16, 0.1},
	              {0, 1, 3, 3, 0.16, 0.1},
	          }),
	          connections);
}

TEST(connector, uniform_functor_huge_matrix)
{
	std::vector<Connection> connections = instantiate_connections({
	    {0, 0, 10000, 1, 0, 10000,
	     Connector::functor(
	         [](NeuronIndex src, NeuronIndex tar) {
		         return src == tar && src < 4 && tar < 4;
	         },
	         0.16, 0.1)},
	});

	EXPECT_EQ(std::vector<Connection>({
	              {0, 1, 0, 0, 0.16, 0.1},
	              {0, 1, 1, 1, 0.16, 0.1},
	              {0, 1, 2, 2, 0.16, 0.1},
	              {0, 1, 3, 3, 0.16, 0.1},
	          }),
	          connections);
}

TEST(connector, fixed_probability)
{
	std::vector<Connection> connections1 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::fixed_probability(Connector::all_to_all(), 0.1)},
	});
	std::vector<Connection> connections2 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::fixed_probability(Connector::all_to_all(), 0.1)},
	});

	EXPECT_TRUE(connections1.size() < 16 * 16 * 0.3);
	EXPECT_TRUE(connections2.size() < 16 * 16 * 0.3);
	EXPECT_TRUE(connections1 != connections2);

	StaticSynapse synap({0.015, 1});
	connections1 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::fixed_probability(Connector::all_to_all(synap), 0.1)},
	});
	EXPECT_TRUE(connections1.size() < 16 * 16 * 0.3);

	for (size_t i = 0; i < 20; i++) {
		connections1 = instantiate_connections({
		    {0, 0, 16, 0, 0, 16,
		     Connector::fixed_probability(Connector::all_to_all(), 0.1, true)},
		});
		EXPECT_TRUE(connections1.size() < 16 * 16 * 0.3);

		connections1 = instantiate_connections({
		    {0, 0, 16, 0, 0, 16,
		     Connector::fixed_probability(Connector::all_to_all(), 1.0, true)},
		});
		EXPECT_TRUE(connections1.size() == 16 * 16);

		connections1 = instantiate_connections({
		    {0, 0, 16, 0, 0, 16,
		     Connector::fixed_probability(Connector::all_to_all(), 1.0, false)},
		});
		EXPECT_TRUE(connections1.size() == 15 * 16);
		for (auto conn : connections1) {
			EXPECT_TRUE(conn.n.src != conn.n.tar);
		}

		connections1 = instantiate_connections({
		    {0, 0, 16, 0, 0, 16,
		     Connector::fixed_probability(Connector::all_to_all(), 0.1, false)},
		});
		EXPECT_TRUE(connections1.size() < 16 * 16 * 0.3);
		for (auto conn : connections1) {
			EXPECT_TRUE(conn.n.src != conn.n.tar);
		}
	}
}

TEST(connector, fixed_probability_seed)
{
	std::vector<Connection> connections1 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::fixed_probability(Connector::all_to_all(), 0.1,
	                                  size_t(436283))},
	});
	std::vector<Connection> connections2 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::fixed_probability(Connector::all_to_all(), 0.1,
	                                  size_t(436283))},
	});

	EXPECT_TRUE(connections1.size() < 16 * 16 * 0.3);
	EXPECT_TRUE(connections2.size() < 16 * 16 * 0.3);
	EXPECT_TRUE(connections1 == connections2);

	for (size_t i = 0; i < 20; i++) {
		std::vector<Connection> connections3 = instantiate_connections({
		    {0, 0, 16, 0, 0, 16,
		     Connector::fixed_probability(Connector::all_to_all(), 0.1, 436283,
		                                  false)},
		});
		EXPECT_TRUE(connections1.size() < 16 * 16 * 0.3);
		for (auto conn : connections3) {
			EXPECT_TRUE(conn.n.src != conn.n.tar);
		}
		EXPECT_TRUE(connections3 != connections2);
	}
}

TEST(connector, fixed_fan_in)
{
	std::vector<Connection> connections1 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::fixed_fan_in(8, 0.1, 1.0)},
	});
	std::vector<Connection> connections2 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::fixed_fan_in(16, 0.1, 1.0)},
	});
	std::vector<Connection> connections3 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::all_to_all(0.1, 1.0)},
	});

	std::vector<size_t> fan_in(16);
	for (auto c : connections1) {
		fan_in[c.n.tar]++;
	}
	for (size_t fi : fan_in) {
		EXPECT_EQ(8U, fi);
	}

	EXPECT_TRUE(connections1.size() == 8 * 16);
	EXPECT_TRUE(connections2.size() == 16 * 16);
	EXPECT_TRUE(connections2 == connections3);

	// No self Connections
	connections1 = instantiate_connections({
	    {0, 0, 16, 0, 0, 16, Connector::fixed_fan_in(8, 0.1, 1.0, false)},
	});
	connections2 = instantiate_connections({
	    {0, 0, 16, 0, 0, 16, Connector::fixed_fan_in(16, 0.1, 1.0, false)},
	});
	connections3 = instantiate_connections({
	    {0, 0, 16, 0, 0, 16, Connector::all_to_all(0.1, 1.0, false)},
	});
	std::vector<size_t> fan_in2(16);
	for (auto c : connections1) {
		fan_in2[c.n.tar]++;
	}
	for (size_t fi : fan_in2) {
		EXPECT_EQ(8U, fi);
	}
	for (auto conn : connections1) {
		EXPECT_TRUE(conn.n.src != conn.n.tar);
	}

	for (auto conn : connections2) {
		EXPECT_TRUE(conn.n.src != conn.n.tar);
	}
	EXPECT_EQ(size_t(8 * 16), connections1.size());
	EXPECT_EQ(size_t(16 * 15), connections2.size());
	EXPECT_TRUE(connections2 == connections3);

	// No self connections, different pops
	connections1 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::fixed_fan_in(8, 0.1, 1.0, false)},
	});
	connections2 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::fixed_fan_in(16, 0.1, 1.0, false)},
	});
	connections3 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::all_to_all(0.1, 1.0, false)},
	});
	std::vector<size_t> fan_in3(16);
	for (auto c : connections1) {
		fan_in3[c.n.tar]++;
	}
	for (size_t fi : fan_in3) {
		EXPECT_EQ(8U, fi);
	}

	EXPECT_EQ(size_t(8 * 16), connections1.size());
	EXPECT_EQ(size_t(16 * 16), connections2.size());
	EXPECT_TRUE(connections2 == connections3);
}

TEST(connector, fixed_fan_in_seed)
{
	std::vector<Connection> connections1 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::fixed_fan_in(8, 0.1, 1.0, size_t(8791))},
	});
	std::vector<Connection> connections2 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::fixed_fan_in(8, 0.1, 1.0, size_t(8791))},
	});
	std::vector<Connection> connections3 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::fixed_fan_in(8, 0.1, 1.0, size_t(8792))},
	});

	EXPECT_TRUE(connections1 == connections2);
	EXPECT_TRUE(connections2 != connections3);

	connections1 = instantiate_connections({
	    {0, 0, 16, 0, 0, 16,
	     Connector::fixed_fan_in(8, 0.1, 1.0, size_t(8791), false)},
	});
	connections2 = instantiate_connections({
	    {0, 0, 16, 0, 0, 16,
	     Connector::fixed_fan_in(8, 0.1, 1.0, size_t(8791), false)},
	});
	EXPECT_TRUE(connections1 == connections2);
}

TEST(connector, fixed_fan_out)
{
	std::vector<Connection> connections1 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::fixed_fan_out(8, 0.1, 1.0)},
	});
	std::vector<Connection> connections2 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::fixed_fan_out(16, 0.1, 1.0)},
	});
	std::vector<Connection> connections3 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::all_to_all(0.1, 1.0)},
	});

	std::vector<size_t> fan_out(16);
	for (auto c : connections1) {
		fan_out[c.n.src]++;
	}
	for (size_t fo : fan_out) {
		EXPECT_EQ(8U, fo);
	}

	EXPECT_TRUE(connections1.size() == 8 * 16);
	EXPECT_TRUE(connections2.size() == 16 * 16);
	EXPECT_TRUE(connections2 == connections3);

	// No self Connections
	connections1 = instantiate_connections({
	    {0, 0, 16, 0, 0, 16, Connector::fixed_fan_out(8, 0.1, 1.0, false)},
	});
	connections2 = instantiate_connections({
	    {0, 0, 16, 0, 0, 16, Connector::fixed_fan_out(16, 0.1, 1.0, false)},
	});
	connections3 = instantiate_connections({
	    {0, 0, 16, 0, 0, 16, Connector::all_to_all(0.1, 1.0, false)},
	});
	std::vector<size_t> fan_out2(16);
	for (auto c : connections1) {
		fan_out2[c.n.src]++;
	}
	for (size_t fo : fan_out2) {
		EXPECT_EQ(8U, fo);
	}
	for (auto conn : connections1) {
		EXPECT_TRUE(conn.n.src != conn.n.tar);
	}

	for (auto conn : connections2) {
		EXPECT_TRUE(conn.n.src != conn.n.tar);
	}
	EXPECT_EQ(size_t(8 * 16), connections1.size());
	EXPECT_EQ(size_t(16 * 15), connections2.size());
	EXPECT_TRUE(connections2 == connections3);

	// No self connections, different pops
	connections1 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::fixed_fan_out(8, 0.1, 1.0, false)},
	});
	connections2 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::fixed_fan_out(16, 0.1, 1.0, false)},
	});
	connections3 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, Connector::all_to_all(0.1, 1.0, false)},
	});
	std::vector<size_t> fan_out3(16);
	for (auto c : connections1) {
		fan_out3[c.n.src]++;
	}
	for (size_t fo : fan_out3) {
		EXPECT_EQ(8U, fo);
	}

	EXPECT_EQ(size_t(8 * 16), connections1.size());
	EXPECT_EQ(size_t(16 * 16), connections2.size());
	EXPECT_TRUE(connections2 == connections3);
}

TEST(connector, fixed_fan_out_seed)
{
	std::vector<Connection> connections1 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::fixed_fan_out(8, 0.1, 1.0, size_t(8791))},
	});
	std::vector<Connection> connections2 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::fixed_fan_out(8, 0.1, 1.0, size_t(8791))},
	});
	std::vector<Connection> connections3 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     Connector::fixed_fan_out(8, 0.1, 1.0, size_t(8792))},
	});

	EXPECT_TRUE(connections1 == connections2);
	EXPECT_TRUE(connections2 != connections3);

	connections1 = instantiate_connections({
	    {0, 0, 16, 0, 0, 16,
	     Connector::fixed_fan_out(8, 0.1, 1.0, size_t(8791), false)},
	});
	connections2 = instantiate_connections({
	    {0, 0, 16, 0, 0, 16,
	     Connector::fixed_fan_out(8, 0.1, 1.0, size_t(8791), false)},
	});
	EXPECT_TRUE(connections1 == connections2);
}
}  // namespace cypress
