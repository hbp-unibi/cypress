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

//	for (auto c : connections) {
//		std::cout << c.psrc << ", " << c.ptar << ", " << c.n.src << ", "
//		          << c.n.tar << ", " << c.n.synapse.weight << ", "
//		          << c.n.synapse.delay << std::endl;
//	}

TEST(connector, all_to_all)
{
	std::vector<Connection> connections = instantiate_connections({
	    {0, 0, 2, 1, 0, 5, std::move(Connector::all_to_all(0.16, 0.1))},
	    {0, 2, 4, 2, 0, 5, std::move(Connector::all_to_all(0.1, 0.05))},
	});

	EXPECT_EQ(std::vector<Connection>({
	              {0, 1, 0, 0, 0.16, 0.1},
	              {0, 1, 0, 1, 0.16, 0.1},
	              {0, 1, 0, 2, 0.16, 0.1},
	              {0, 1, 0, 3, 0.16, 0.1},
	              {0, 1, 0, 4, 0.16, 0.1},
	              {0, 1, 1, 0, 0.16, 0.1},
	              {0, 1, 1, 1, 0.16, 0.1},
	              {0, 1, 1, 2, 0.16, 0.1},
	              {0, 1, 1, 3, 0.16, 0.1},
	              {0, 1, 1, 4, 0.16, 0.1},
	              {0, 2, 2, 0, 0.1, 0.05},
	              {0, 2, 2, 1, 0.1, 0.05},
	              {0, 2, 2, 2, 0.1, 0.05},
	              {0, 2, 2, 3, 0.1, 0.05},
	              {0, 2, 2, 4, 0.1, 0.05},
	              {0, 2, 3, 0, 0.1, 0.05},
	              {0, 2, 3, 1, 0.1, 0.05},
	              {0, 2, 3, 2, 0.1, 0.05},
	              {0, 2, 3, 3, 0.1, 0.05},
	              {0, 2, 3, 4, 0.1, 0.05},
	          }),
	          connections);
}

TEST(connector, one_to_one)
{
	std::vector<Connection> connections = instantiate_connections({
	    {0, 0, 2, 1, 3, 5, std::move(Connector::one_to_one(0.16, 0.1))},
	    {0, 2, 4, 2, 10, 12, std::move(Connector::one_to_one(0.1, 0.05))},
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
	    {0, 0, 16, 1, 0, 16, std::move(Connector::from_list({
	                             {0, 1, 0.16, 0.0},
	                             {10, 8, 0.1, 0.0},
	                             {11, 3, 0.12},
	                             {12, 3, 0.00, 0.0},
	                             {12, 3, 0.05},
	                             {14, 3, -0.05, 0.0},
	                             {12, 2, 0.00, -1.0},
	                         }))},
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
	     std::move(Connector::from_list({
	         {0, 1, 0.16, 0.0},
	         {10, 8, 0.1, 0.0},
	         {11, 3, 0.12},
	         {12, 3, 0.00, 0.0},
	         {12, 3, 0.05},
	         {14, 3, -0.05, 0.0},
	         {12, 2, 0.00, -1.0},
	     }))},
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
	     std::move(Connector::functor([](NeuronIndex src, NeuronIndex tar) {
		     if (src == tar) {
			     return Synapse(0.16, 0.1);
		     }
		     return Synapse();
		 }))},
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
	     std::move(Connector::functor(
	         [](NeuronIndex src, NeuronIndex tar) { return src == tar; }, 0.16,
	         0.1))},
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
	     std::move(Connector::fixed_probability(Connector::all_to_all(), 0.1))},
	});
	std::vector<Connection> connections2 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16,
	     std::move(Connector::fixed_probability(Connector::all_to_all(), 0.1))},
	});

	EXPECT_TRUE(connections1.size() < 16 * 16);
	EXPECT_TRUE(connections2.size() < 16 * 16);
	EXPECT_TRUE(connections1 != connections2);
}

TEST(connector, fixed_probability_seed)
{
	std::vector<Connection> connections1 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, std::move(Connector::fixed_probability(
	                             Connector::all_to_all(), 0.1, 436283))},
	});
	std::vector<Connection> connections2 = instantiate_connections({
	    {0, 0, 16, 1, 0, 16, std::move(Connector::fixed_probability(
	                             Connector::all_to_all(), 0.1, 436283))},
	});

	EXPECT_TRUE(connections1.size() < 16 * 16);
	EXPECT_TRUE(connections2.size() < 16 * 16);
	EXPECT_TRUE(connections1 == connections2);
}
}
