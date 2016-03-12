/*
 *  Cypress -- C++ Neural Associative Memory Simulator
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

#include <algorithm>
#include <iterator>

#include "gtest/gtest.h"

#include <cypress/core/network.hpp>

namespace cypress {
TEST(network, create_population)
{
	Network network;

	{
		auto pop = network.create_population<IfCondExp>(10);
		EXPECT_EQ(10U, pop.size());
		EXPECT_EQ(&IfCondExp::inst(), &pop.type());
		EXPECT_EQ("", pop.name());
	}

	{
		auto pop = network.create_population<IfCondExp>(20, {}, "foo");
		EXPECT_EQ(20U, pop.size());
		EXPECT_EQ(&IfCondExp::inst(), &pop.type());
		EXPECT_EQ("foo", pop.name());
	}

	{
		Population<IfCondExp> pop(network, 30, {});
		EXPECT_EQ(30U, pop.size());
		EXPECT_EQ(&IfCondExp::inst(), &pop.type());
		EXPECT_EQ("", pop.name());
	}

	{
		Population<IfCondExp> pop(network, 40, {}, "foo2");
		EXPECT_EQ(40U, pop.size());
		EXPECT_EQ(&IfCondExp::inst(), &pop.type());
		EXPECT_EQ("foo2", pop.name());
	}
}

TEST(network, equals)
{
	Network network;
	auto pop = network.create_population<IfCondExp>(10);

	EXPECT_TRUE(pop.network() == network);
	EXPECT_TRUE(pop[0].network() == network);
	EXPECT_TRUE(pop(0).network() == network);
	EXPECT_TRUE(pop(0, 5).network() == network);
}

TEST(network, population_iterator)
{
	Network network;
	auto pop = network.create_population<IfCondExp>(10);

	// Test the default iterator
	{
		ssize_t i = 0;
		for (auto neuron : pop) {
			EXPECT_EQ(i++, neuron.nid());
		}
		EXPECT_EQ(10, i);
	}
}

TEST(network, population_iterator_post_increment)
{
	Network network;
	auto pop = network.create_population<IfCondExp>(10);

	EXPECT_EQ(0, (pop.begin()++)->nid());
	EXPECT_EQ(9, (pop.rbegin()++)->nid());
	EXPECT_EQ(0, (pop.cbegin()++)->nid());
	EXPECT_EQ(9, (pop.crbegin()++)->nid());

	// Explicit default iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = pop.begin(); it != pop.end(); it++) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i++, (*it).nid());
		}
		EXPECT_EQ(10, i);
	}

	// Explicit constant iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = pop.cbegin(); it != pop.cend(); it++) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i++, (*it).nid());
		}
		EXPECT_EQ(10, i);
	}

	// Explicit reverse iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = pop.rbegin(); it != pop.rend(); it++) {
			ASSERT_EQ(i, ssize_t(it->nid()));
			ASSERT_EQ(i--, ssize_t((*it).nid()));
		}
		EXPECT_EQ(-1, i);
	}

	// Explicit reverse constant iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = pop.crbegin(); it != pop.crend(); it++) {
			ASSERT_EQ(i, ssize_t(it->nid()));
			ASSERT_EQ(i--, ssize_t((*it).nid()));
		}
		EXPECT_EQ(-1, i);
	}
}

TEST(network, population_iterator_pre_increment)
{
	Network network;
	auto pop = network.create_population<IfCondExp>(10);

	EXPECT_EQ(1, (++pop.begin())->nid());
	EXPECT_EQ(8, (++pop.rbegin())->nid());
	EXPECT_EQ(1, (++pop.cbegin())->nid());
	EXPECT_EQ(8, (++pop.crbegin())->nid());

	// Explicit default iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = pop.begin(); it != pop.end(); ++it) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i++, (*it).nid());
		}
		EXPECT_EQ(10, i);
	}

	// Explicit constant iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = pop.cbegin(); it != pop.cend(); ++it) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i++, (*it).nid());
		}
		EXPECT_EQ(10, i);
	}

	// Explicit reverse iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = pop.rbegin(); it != pop.rend(); ++it) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i--, (*it).nid());
		}
		EXPECT_EQ(-1, i);
	}

	// Explicit reverse constant iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = pop.crbegin(); it != pop.crend(); ++it) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i--, (*it).nid());
		}
		EXPECT_EQ(-1, i);
	}
}

TEST(network, population_iterator_post_decrement)
{
	Network network;
	auto pop = network.create_population<IfCondExp>(10);

	// Explicit default iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = --pop.end(); it >= pop.begin(); it--) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i--, (*it).nid());
		}
		EXPECT_EQ(-1, i);
	}

	// Explicit constant iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = --pop.cend(); it >= pop.cbegin(); it--) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i--, (*it).nid());
		}
		EXPECT_EQ(-1, i);
	}

	// Explicit reverse iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = --pop.rend(); it >= pop.rbegin(); it--) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i++, (*it).nid());
		}
		EXPECT_EQ(10, i);
	}

	// Explicit reverse constant iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = --pop.crend(); it >= pop.crbegin(); it--) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i++, (*it).nid());
		}
		EXPECT_EQ(10, i);
	}
}

TEST(network, population_iterator_pre_decrement)
{
	Network network;
	auto pop = network.create_population<IfCondExp>(10);

	// Explicit default iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = --pop.end(); it >= pop.begin(); --it) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i--, (*it).nid());
		}
		EXPECT_EQ(-1, i);
	}

	// Explicit constant iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = --pop.cend(); it >= pop.cbegin(); --it) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i--, (*it).nid());
		}
		EXPECT_EQ(-1, i);
	}

	// Explicit reverse iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = --pop.rend(); it >= pop.rbegin(); --it) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i++, (*it).nid());
		}
		EXPECT_EQ(10, i);
	}

	// Explicit reverse constant iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = --pop.crend(); it >= pop.crbegin(); --it) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i++, (*it).nid());
		}
		EXPECT_EQ(10, i);
	}
}

TEST(network, population_iterator_plus_equal)
{
	Network network;
	auto pop = network.create_population<IfCondExp>(10);

	// Explicit default iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = pop.begin(); it < pop.end(); it += 2) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i, (*it).nid());
			i += 2;
		}
		EXPECT_EQ(10, i);
	}

	// Explicit constant iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = pop.cbegin(); it < pop.cend(); it += 2) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i, (*it).nid());
			i += 2;
		}
		EXPECT_EQ(10, i);
	}

	// Explicit reverse iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = pop.rbegin(); it < pop.rend(); it += 2) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i, (*it).nid());
			i -= 2;
		}
		EXPECT_EQ(-1, i);
	}

	// Explicit reverse constant iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = pop.crbegin(); it < pop.crend(); it += 2) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i, (*it).nid());
			i -= 2;
		}
		EXPECT_EQ(-1, i);
	}
}

TEST(network, population_iterator_minus_equal)
{
	Network network;
	auto pop = network.create_population<IfCondExp>(10);

	// Explicit default iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = pop.end() - 1; it >= pop.begin(); it -= 2) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i, (*it).nid());
			i -= 2;
		}
		EXPECT_EQ(-1, i);
	}

	// Explicit constant iterator instantiation
	{
		ssize_t i = 9;
		for (auto it = pop.cend() - 1; it >= pop.cbegin(); it -= 2) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i, (*it).nid());
			i -= 2;
		}
		EXPECT_EQ(-1, i);
	}

	// Explicit reverse iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = pop.rend() - 1; it >= pop.rbegin(); it -= 2) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i, (*it).nid());
			i += 2;
		}
		EXPECT_EQ(10, i);
	}

	// Explicit reverse constant iterator instantiation
	{
		ssize_t i = 0;
		for (auto it = pop.crend() - 1; it >= pop.crbegin(); it -= 2) {
			ASSERT_EQ(i, it->nid());
			ASSERT_EQ(i, (*it).nid());
			i += 2;
		}
		EXPECT_EQ(10, i);
	}
}

TEST(network, population_iterator_arithmetic)
{
	Network network;
	auto pop = network.create_population<IfCondExp>(10);

	EXPECT_EQ(10, std::distance(pop.begin(), pop.end()));
	EXPECT_EQ(10, std::distance(pop.rbegin(), pop.rend()));
	EXPECT_EQ(10, std::distance(pop.cbegin(), pop.cend()));
	EXPECT_EQ(10, std::distance(pop.crbegin(), pop.crend()));

	EXPECT_EQ(10, pop.end() - pop.begin());
	EXPECT_EQ(10, pop.rend() - pop.rbegin());
	EXPECT_EQ(10, pop.cend() - pop.cbegin());
	EXPECT_EQ(10, pop.crend() - pop.crbegin());

	EXPECT_EQ(8, (pop.end() - 2) - pop.begin());
	EXPECT_EQ(8, (pop.rend() - 2) - pop.rbegin());
	EXPECT_EQ(8, (pop.cend() - 2) - pop.cbegin());
	EXPECT_EQ(8, (pop.crend() - 2) - pop.crbegin());

	EXPECT_EQ(6, (pop.end() - 2) - (pop.begin() + 2));
	EXPECT_EQ(6, (pop.rend() - 2) - (pop.rbegin() + 2));
	EXPECT_EQ(6, (pop.cend() - 2) - (pop.cbegin() + 2));
	EXPECT_EQ(6, (pop.crend() - 2) - (pop.crbegin() + 2));

	EXPECT_EQ(2, pop.begin()[2].nid());
	EXPECT_EQ(7, pop.rbegin()[2].nid());
	EXPECT_EQ(2, pop.cbegin()[2].nid());
	EXPECT_EQ(7, pop.crbegin()[2].nid());

	EXPECT_TRUE(pop.end() > pop.begin());
	EXPECT_TRUE(pop.rend() > pop.rbegin());
	EXPECT_TRUE(pop.cend() > pop.cbegin());
	EXPECT_TRUE(pop.crend() > pop.crbegin());
	EXPECT_FALSE(pop.end() < pop.begin());
	EXPECT_FALSE(pop.rend() < pop.rbegin());
	EXPECT_FALSE(pop.cend() < pop.cbegin());
	EXPECT_FALSE(pop.crend() < pop.crbegin());

	EXPECT_TRUE(pop.begin() >= pop.begin());
	EXPECT_TRUE(pop.rbegin() >= pop.rbegin());
	EXPECT_TRUE(pop.cbegin() >= pop.cbegin());
	EXPECT_TRUE(pop.crbegin() >= pop.crbegin());
	EXPECT_TRUE(pop.begin() <= pop.begin());
	EXPECT_TRUE(pop.rbegin() <= pop.rbegin());
	EXPECT_TRUE(pop.cbegin() <= pop.cbegin());
	EXPECT_TRUE(pop.crbegin() <= pop.crbegin());
	EXPECT_TRUE(pop.begin() == pop.begin());
	EXPECT_TRUE(pop.rbegin() == pop.rbegin());
	EXPECT_TRUE(pop.cbegin() == pop.cbegin());
	EXPECT_TRUE(pop.crbegin() == pop.crbegin());
	EXPECT_FALSE(pop.begin() != pop.begin());
	EXPECT_FALSE(pop.rbegin() != pop.rbegin());
	EXPECT_FALSE(pop.cbegin() != pop.cbegin());
	EXPECT_FALSE(pop.crbegin() != pop.crbegin());

	EXPECT_TRUE(pop.crend() + 1 > pop.crbegin());
	EXPECT_TRUE(pop.begin() + 1 > pop.begin());
	EXPECT_TRUE(pop.rbegin() + 1 > pop.rbegin());
	EXPECT_TRUE(pop.cbegin() + 1 > pop.cbegin());
	EXPECT_TRUE(pop.crend() + 1 != pop.crbegin());
	EXPECT_TRUE(pop.begin() + 1 != pop.begin());
	EXPECT_TRUE(pop.rbegin() + 1 != pop.rbegin());
	EXPECT_TRUE(pop.cbegin() + 1 != pop.cbegin());
	EXPECT_FALSE(pop.crend() + 1 <= pop.crbegin());
	EXPECT_FALSE(pop.begin() + 1 <= pop.begin());
	EXPECT_FALSE(pop.rbegin() + 1 <= pop.rbegin());
	EXPECT_FALSE(pop.cbegin() + 1 <= pop.cbegin());
	EXPECT_FALSE(pop.crend() + 1 == pop.crbegin());
	EXPECT_FALSE(pop.begin() + 1 == pop.begin());
	EXPECT_FALSE(pop.rbegin() + 1 == pop.rbegin());
	EXPECT_FALSE(pop.cbegin() + 1 == pop.cbegin());
}

TEST(network, population_iterator_const_conversion)
{
	Network network;
	Population<IfCondExp> pop = network.create_population<IfCondExp>(10);

	Population<IfCondExp>::const_iterator it1 = pop.begin();
	Population<IfCondExp>::const_reverse_iterator it2 = pop.rbegin();

	EXPECT_EQ(0, it1->nid());
	EXPECT_EQ(9, it2->nid());
}

TEST(network, population_view_iterator)
{
	Network network;
	Population<IfCondExp> pop = network.create_population<IfCondExp>(10);

	PopulationView<IfCondExp> view = pop.range(3, 8);
	EXPECT_EQ(5U, view.size());
	EXPECT_EQ(3, view.begin()->nid());
	EXPECT_EQ(7, view.rbegin()->nid());
	EXPECT_EQ(3, view.cbegin()->nid());
	EXPECT_EQ(7, view.crbegin()->nid());

	{
		ssize_t i = 3;
		for (auto neuron : view) {
			EXPECT_EQ(i++, neuron.nid());
		}
		EXPECT_EQ(8, i);
	}

	PopulationView<IfCondExp> view2 = view.range(1, 2);
	EXPECT_EQ(1U, view2.size());
	EXPECT_EQ(4, view2.begin()->nid());
	EXPECT_EQ(4, view2.rbegin()->nid());
	EXPECT_EQ(4, view2.cbegin()->nid());
	EXPECT_EQ(4, view2.crbegin()->nid());

	{
		ssize_t i = 4;
		for (auto neuron : view2) {
			EXPECT_EQ(i++, neuron.nid());
		}
		EXPECT_EQ(5, i);
	}
}

TEST(network, duration_empty)
{
	Network network;

	ASSERT_EQ(0.0, network.duration());
}

TEST(network, duration)
{
	Network network;

	network.create_population<SpikeSourceArray>(10, {100.0, 200.0, 300.0});
	network.create_population<SpikeSourceArray>(20);
	network.create_population<SpikeSourceArray>(30, {100.0, 400.0});

	EXPECT_EQ(400.0, network.duration());
}

TEST(network, clone)
{
	Network n2;
	Network n3;
	{
		Network n1;
		n1.create_population<SpikeSourceArray>(10, {100.0, 200.0, 300.0});
		n1.create_population<SpikeSourceArray>(20);
		auto pop = n1.create_population<SpikeSourceArray>(30, {100.0, 400.0});

		EXPECT_EQ(400.0, n1.duration());

		n2 = n1.clone();
		n3 = n1;

		pop.parameters().spike_times({100.0, 200.0});
		EXPECT_EQ(300.0, n1.duration());
	}

	EXPECT_EQ(400.0, n2.duration());
	EXPECT_EQ(300.0, n3.duration());
}

TEST(network, population_by_name)
{
	Network n;
	n.create_population<SpikeSourceArray>(10, {100.0, 120.0}, "spikes1");
	n.create_population<SpikeSourceArray>(20, {100.0, 120.0}, "spikes2");
	n.create_population<IfCondExp>(30, {}, "neurons");
	n.create_population<IfCondExp>(40, {});

	ASSERT_EQ(4U, n.populations().size());
	ASSERT_EQ(4U, n.populations("").size());
	ASSERT_EQ(1U, n.populations("spikes1").size());
	ASSERT_EQ(1U, n.populations("spikes2").size());
	ASSERT_EQ(1U, n.populations("neurons").size());
	ASSERT_EQ(2U, n.populations<SpikeSourceArray>().size());
	ASSERT_EQ(2U, n.populations<IfCondExp>().size());
	ASSERT_EQ(1U, n.populations<SpikeSourceArray>("spikes1").size());
	ASSERT_EQ(1U, n.populations<SpikeSourceArray>("spikes2").size());
	ASSERT_EQ(1U, n.populations<IfCondExp>("neurons").size());

	EXPECT_EQ(10U, n.population("spikes1").size());
	EXPECT_EQ(20U, n.population("spikes2").size());
	EXPECT_EQ(30U, n.population("neurons").size());
	EXPECT_EQ(10U, n.population<SpikeSourceArray>("spikes1").size());
	EXPECT_EQ(20U, n.population<SpikeSourceArray>("spikes2").size());
	EXPECT_EQ(30U, n.population<IfCondExp>("neurons").size());
	EXPECT_EQ(20U, n.population<SpikeSourceArray>().size());
	EXPECT_EQ(40U, n.population<IfCondExp>().size());

	ASSERT_THROW(n.population("foo"), NoSuchPopulationException);
	ASSERT_THROW(n.population<IfCondExp>("foo"), NoSuchPopulationException);
}

TEST(network, connect)
{
	Network n;

	Population<SpikeSourceArray> pop1 =
	    n.create_population<SpikeSourceArray>(1, {});
	Population<IfCondExp> pop2 = n.create_population<IfCondExp>(10, {});
	Population<IfCondExp> pop3 = n.create_population<IfCondExp>(10, {});
	Population<IfCondExp> pop4 = n.create_population<IfCondExp>(20, {});

	pop1.connect_to(pop2, Connector::all_to_all(0.016, 0.01));
	pop1.connect_to(pop3, Connector::all_to_all(0.016, 0.01));
	pop2.connect_to(pop4.range(0, 10), Connector::one_to_one(0.016, 0.01));
	pop3.connect_to(pop4.range(10, 20), Connector::one_to_one(0.016, 0.01));
	pop4(0, 10).connect_to(pop3, Connector::all_to_all(0.016, 0.01));
	pop4(10, 20).connect_to(pop2, Connector::all_to_all(0.016, 0.01));
	pop1[0].connect_to(pop2(5), Connector::one_to_one(0.016, 0.01));

	const auto &cs = n.connections();
	ASSERT_EQ(7U, cs.size());

	EXPECT_EQ(ConnectionDescriptor(0, 0, 1, 1, 0, 10), cs[0]);
	EXPECT_EQ(ConnectionDescriptor(0, 0, 1, 1, 5, 6), cs[1]);
	EXPECT_EQ(ConnectionDescriptor(0, 0, 1, 2, 0, 10), cs[2]);
	EXPECT_EQ(ConnectionDescriptor(1, 0, 10, 3, 0, 10), cs[3]);
	EXPECT_EQ(ConnectionDescriptor(2, 0, 10, 3, 10, 20), cs[4]);
	EXPECT_EQ(ConnectionDescriptor(3, 10, 20, 1, 0, 10), cs[5]);
	EXPECT_EQ(ConnectionDescriptor(3, 0, 10, 2, 0, 10), cs[6]);
}

TEST(network, record)
{
	Network n;

	{
		auto pop = n.create_population<IfCondExp>(10);
		EXPECT_FALSE(pop.signals().is_recording_spikes());
		EXPECT_FALSE(pop.signals().is_recording_v());
		EXPECT_FALSE(pop.signals().is_recording_gsyn_exc());
		EXPECT_FALSE(pop.signals().is_recording_gsyn_inh());
	}

	{
		auto pop = n.create_population<IfCondExp>(
		    10, {}, IfCondExp::Signals().record_spikes());
		EXPECT_TRUE(pop.signals().is_recording_spikes());
		EXPECT_FALSE(pop.signals().is_recording_v());
		EXPECT_FALSE(pop.signals().is_recording_gsyn_exc());
		EXPECT_FALSE(pop.signals().is_recording_gsyn_inh());
	}

	{
		auto pop = n.create_population<IfCondExp>(10, {}, {"spikes", "v"});
		EXPECT_TRUE(pop.signals().is_recording_spikes());
		EXPECT_TRUE(pop.signals().is_recording_v());
		EXPECT_FALSE(pop.signals().is_recording_gsyn_exc());
		EXPECT_FALSE(pop.signals().is_recording_gsyn_inh());
	}

	{
		auto pop = n.create_population<IfCondExp>(
		    10, {}, IfCondExp::Signals().record_spikes().record_v());
		EXPECT_TRUE(pop.signals().is_recording_spikes());
		EXPECT_TRUE(pop.signals().is_recording_v());
		EXPECT_FALSE(pop.signals().is_recording_gsyn_exc());
		EXPECT_FALSE(pop.signals().is_recording_gsyn_inh());
	}

	{
		auto pop = n.create_population<IfCondExp>(10, {});
		pop.signals().record_v();
		EXPECT_FALSE(pop.signals().is_recording_spikes());
		EXPECT_TRUE(pop.signals().is_recording_v());
		EXPECT_FALSE(pop.signals().is_recording_gsyn_exc());
		EXPECT_FALSE(pop.signals().is_recording_gsyn_inh());
	}
}

TEST(network, parameters_from_list_initialisation)
{
	Network n;

	auto pop = n.create_population<IfCondExp>(
	    3, {
	           IfCondExpParameters().v_rest(-55.0),
	           IfCondExpParameters().v_rest(-50.0),
	           IfCondExpParameters().v_rest(-45.0),
	       });

	EXPECT_EQ(-55.0, pop[0].parameters().v_rest());
	EXPECT_EQ(-50.0, pop[1].parameters().v_rest());
	EXPECT_EQ(-45.0, pop[2].parameters().v_rest());

	ASSERT_THROW(
	    n.create_population<IfCondExp>(3,
	                                   {
	                                       IfCondExpParameters().v_rest(-55.0),
	                                       IfCondExpParameters().v_rest(-50.0),
	                                   }),
	    InvalidParameterArraySize);

	auto pop2 = n.create_population<IfCondExp>(
	    3, {
	           IfCondExpParameters().v_rest(-55.0),
	       });
	EXPECT_EQ(-55.0, pop2[0].parameters().v_rest());
	EXPECT_EQ(-55.0, pop2[1].parameters().v_rest());
	EXPECT_EQ(-55.0, pop2[2].parameters().v_rest());

	auto pop3 = n.create_population<IfCondExp>(3, {});
	EXPECT_EQ(-65.0, pop3[0].parameters().v_rest());
	EXPECT_EQ(-65.0, pop3[1].parameters().v_rest());
	EXPECT_EQ(-65.0, pop3[2].parameters().v_rest());
}

TEST(network, signals_from_list_initialisation)
{
	Network n;

	auto pop = n.create_population<IfCondExp>(
	    4, {}, {
	               IfCondExpSignals().record_spikes(),
	               IfCondExpSignals().record_spikes().record_v(),
	               IfCondExpSignals().record_gsyn_exc(),
	               IfCondExpSignals().record_gsyn_inh(),
	           });
}

TEST(network, copy_parameters)
{
	Network n;

	auto pop = n.create_population<IfCondExp>(
	    10, {
	            IfCondExpParameters().v_rest(-100.0),
	            IfCondExpParameters().v_rest(-95.0),
	            IfCondExpParameters().v_rest(-90.0),
	            IfCondExpParameters().v_rest(-85.0),
	            IfCondExpParameters().v_rest(-80.0),
	            IfCondExpParameters().v_rest(-75.0),
	            IfCondExpParameters().v_rest(-70.0),
	            IfCondExpParameters().v_rest(-65.0),
	            IfCondExpParameters().v_rest(-60.0),
	            IfCondExpParameters().v_rest(-55.0),
	        });

	{
		auto pop2 = n.create_population<IfCondExp>(10);
		pop2.parameters() = pop.parameters();
		for (size_t i = 0; i < pop2.size(); i++) {
			pop2[i].parameters().v_rest(pop2[i].parameters().v_rest() + 10.0);
		}

		for (size_t i = 0; i < pop2.size(); i++) {
			EXPECT_EQ(10.0, pop2[i].parameters().v_rest() -
			                    pop[i].parameters().v_rest());
		}
	}

	{
		for (size_t i = 0; i < 10; i++) {
			auto pop2 = n.create_population<IfCondExp>(1, pop[i].parameters());
			pop2.parameters().v_rest(pop2.parameters().v_rest() + 10.0);
			EXPECT_EQ(10.0, pop2.parameters().v_rest() -
			                    pop[i].parameters().v_rest());
		}
	}

	{
		auto pop2 = n.create_population<IfCondExp>(1);

		// Assigning the parameters of the first population to the second one
		// should fail
		ASSERT_THROW(pop2.parameters() = pop.parameters(),
		             InvalidParameterArraySize);
	};
}

TEST(network, homogeneous_parameters)
{
	Network n;

	auto pop = n.create_population<IfCondExp>(10);

	// Set the reseting potential
	pop.parameters().v_rest(-62.0);

	// Make sure the resting potential is set to the correct value
	EXPECT_EQ(-62.0, pop.parameters().v_rest());
	for (size_t i = 0; i < 10; i++) {
		EXPECT_EQ(-62.0, pop[i].parameters().v_rest());
	}

	// Set the reseting potential of neurons 5-8 to another value
	pop(5, 9).parameters().v_rest(-55.0);
	for (size_t i = 0; i < 10; i++) {
		if (i >= 5 && i < 9) {
			EXPECT_EQ(-55.0, pop[i].parameters().v_rest());
		}
		else {
			EXPECT_EQ(-62.0, pop[i].parameters().v_rest());
		}
	}
	EXPECT_EQ(-62.0, pop(0, 5).parameters().v_rest());
	EXPECT_EQ(-55.0, pop(5, 9).parameters().v_rest());
	EXPECT_EQ(-62.0, pop(9, 10).parameters().v_rest());

	// Trying to read a parameter from all neurons must fail...
	ASSERT_THROW(pop.parameters().v_rest(),
	             HomogeneousPopulationRequiredException);

	// ...unless we make the parameters homogeneous again
	pop(0, 5).parameters().v_rest(-55.0);
	pop(9, 10).parameters().v_rest(-55.0);
	EXPECT_EQ(-55.0, pop.parameters().v_rest());
}

TEST(network, copy_parameters_homogeneous)
{
	Network n;

	auto pop =
	    n.create_population<IfCondExp>(10, IfCondExpParameters().v_rest(-10));

	{
		auto pop2 = n.create_population<IfCondExp>(1);
		pop2.parameters() =
		    pop.parameters();  // Assignment works because source is homogeneous
		EXPECT_EQ(-10, pop2.parameters().v_rest());
	}

	pop[5].parameters().v_rest(-20);
	{
		auto pop2 = n.create_population<IfCondExp>(1);
		ASSERT_THROW(
		    pop2.parameters() = pop.parameters(),
		    InvalidParameterArraySize);  // Source is no longer homogeneous
	}

	// So make it homogeneous again!
	pop[5].parameters().v_rest(-10);
	{
		auto pop2 = n.create_population<IfCondExp>(1);
		pop2.parameters() =
		    pop.parameters();  // Assignment works because source is homogeneous
		EXPECT_EQ(-10, pop2.parameters().v_rest());
	}
}

TEST(network, comparison_operators)
{
	Network n;

	auto pop = n.create_population<IfCondExp>(10);

	EXPECT_TRUE(pop == pop);
	EXPECT_FALSE(pop != pop);
	EXPECT_FALSE(pop > pop);
	EXPECT_TRUE(pop >= pop);
	EXPECT_FALSE(pop < pop);
	EXPECT_TRUE(pop <= pop);

	EXPECT_TRUE(pop == pop(0, 10));
	EXPECT_FALSE(pop != pop(0, 10));
	EXPECT_FALSE(pop > pop(0, 10));
	EXPECT_TRUE(pop >= pop(0, 10));
	EXPECT_FALSE(pop < pop(0, 10));
	EXPECT_TRUE(pop <= pop(0, 10));

	EXPECT_FALSE(pop == pop[0]);
	EXPECT_TRUE(pop != pop[0]);
	EXPECT_TRUE(pop > pop[0]);
	EXPECT_TRUE(pop >= pop[0]);
	EXPECT_FALSE(pop < pop[0]);
	EXPECT_FALSE(pop <= pop[0]);

	EXPECT_FALSE(pop == pop[5]);
	EXPECT_TRUE(pop != pop[5]);
	EXPECT_FALSE(pop > pop[5]);
	EXPECT_FALSE(pop >= pop[5]);
	EXPECT_TRUE(pop < pop[5]);
	EXPECT_TRUE(pop <= pop[5]);
}

TEST(network, assignment_independence)
{
	Network n;

	auto pop = n.create_population<IfCondExp>(10);
	auto pop2 = n.create_population<IfCondExp>(10);

	pop[5].parameters().v_rest(-10.0);

	pop2[2].signals().record_v();

	pop[5].signals() = pop2[2].signals();

	EXPECT_TRUE(pop[5].signals().is_recording_v());
	EXPECT_EQ(-10.0, pop[5].parameters().v_rest());
}

TEST(network, non_recorded_data)
{
	Network n;

	auto pop = n.create_population<IfCondExp>(10);

	ASSERT_THROW(pop.signals().get_spikes(), SignalNotRecordedException);

	pop.signals().record_spikes();

	EXPECT_TRUE(pop.signals().get_spikes().empty());
}

}
