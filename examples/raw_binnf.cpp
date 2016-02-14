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

/**
 * This example demonstrates low-level communication between the C++ code and
 * the Python backend via the binnf serialisation format. You should not use
 * the interfaces exposed here, instead rely on higher level abstractions, such
 * as cypress::Network.
 */

#include <array>
#include <iostream>
#include <limits>

#include <cypress/cypress.hpp>

using namespace cypress;
using namespace cypress::binnf;

static const auto INT = NumberType::INT;
static const auto FLOAT = NumberType::FLOAT;

static const int TYPE_SOURCE = 0;
static const int TYPE_IF_COND_EXP = 1;
static const int TYPE_AD_EX = 2;
static const int TYPE_IF_SPIKEY = 3;

static const int ALL_NEURONS = std::numeric_limits<int>::max();

static const Header POPULATIONS_HEADER = {
    {"count", "type", "record_spikes", "record_v", "record_gsyn_exc",
     "record_gsyn_inh"},
    {INT, INT, INT, INT, INT, INT}};

static const Header CONNECTIONS_HEADER = {
    {"pid_src", "pid_tar", "nid_src", "nid_tar", "weight", "delay"},
    {INT, INT, INT, INT, FLOAT, FLOAT}};

static const Header PARAMETERS_HEADER = {
    {"pid", "nid", "v_rest", "cm", "tau_m", "tau_refrac", "tau_syn_E",
     "tau_syn_I", "e_rev_E", "e_rev_I", "v_thresh", "v_reset", "i_offset"},
    {INT, INT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT,
     FLOAT, FLOAT}};

static const Header PARAMETERS_SPIKEY_HEADER = {
    {"pid", "nid", "v_rest", "g_leak", "tau_refrac", "e_rev_I", "v_thresh",
     "v_reset"},
    {INT, INT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT, FLOAT}};

static const Header TARGET_HEADER = {{"pid", "nid"}, {INT, INT}};

static const Header SPIKE_TIMES_HEADER = {{"times"}, {FLOAT}};

int main()
{
	serialise(std::cout, {"populations", POPULATIONS_HEADER,
	                      make_matrix<Number, 3, 6>({{
	                          {{3, TYPE_SOURCE, false, false, false, false}},
	                          {{3, TYPE_SOURCE, false, false, false, false}},
	                          {{4, TYPE_IF_COND_EXP, true, true, false, false}},
	                      }})});

	serialise(std::cout,
	          {"connections", CONNECTIONS_HEADER, make_matrix<Number, 4, 6>({{
	                                                  {{0, 2, 0, 3, 0.1, 0.1}},
	                                                  {{1, 2, 0, 0, 0.1, 0.1}},
	                                                  {{1, 2, 1, 1, 0.1, 0.1}},
	                                                  {{1, 2, 2, 2, 0.1, 0.1}},
	                                              }})});

	serialise(std::cout, {"parameters", PARAMETERS_HEADER,
	                      make_matrix<Number, 4, 13>({{
	                          {2, 0, -65.0, 1.0, 20.0, 0.0, 5.0, 5.0, 0.0,
	                           -70.0, -50.0, -65.0, 0.0},
	                          {2, 1, -65.0, 1.0, 20.0, 0.0, 5.0, 5.0, 0.0,
	                           -70.0, -50.0, -65.0, 0.0},
	                          {2, 2, -65.0, 1.0, 20.0, 0.0, 5.0, 5.0, 0.0,
	                           -70.0, -50.0, -65.0, 0.0},
	                          {2, 3, -65.0, 1.0, 20.0, 0.0, 5.0, 5.0, 0.0,
	                           -70.0, -50.0, -65.0, 0.0},
	                      }})});

//		serialise(std::cout, {"parameters", PARAMETERS_SPIKEY_HEADER,
//		                      make_matrix<Number, 4, 8>({{
//		                          {{2, 0, -65.0, 20.0, 0.0, -70.0, -55.0, -65.0}},
//		                          {{2, 1, -65.0, 20.0, 0.0, -70.0, -55.0, -65.0}},
//		                          {{2, 2, -65.0, 20.0, 0.0, -70.0, -55.0, -65.0}},
//		                          {{2, 3, -65.0, 20.0, 0.0, -70.0, -55.0, -65.0}},
//		                      }})});

	serialise(std::cout, {"target", TARGET_HEADER,
	                      make_matrix<Number, 1, 2>({{{{0, ALL_NEURONS}}}})});

	serialise(
	    std::cout,
	    {"spike_times", SPIKE_TIMES_HEADER,
	     make_matrix<Number>({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0})});

	serialise(std::cout,
	          {"target", TARGET_HEADER, make_matrix<Number, 1, 2>({{{{1, 0}}}})});

	serialise(std::cout, {"spike_times", SPIKE_TIMES_HEADER,
	                      make_matrix<Number>({10.0, 11.0, 12.0, 13.0})});

	serialise(std::cout,
	          {"target", TARGET_HEADER, make_matrix<Number, 1, 2>({{{{1, 1}}}})});

	serialise(std::cout, {"spike_times", SPIKE_TIMES_HEADER,
	                      make_matrix<Number>({20.0, 21.0, 22.0, 23.0})});

	serialise(std::cout,
	          {"target", TARGET_HEADER, make_matrix<Number, 1, 2>({{{{1, 2}}}})});

	serialise(std::cout, {"spike_times", SPIKE_TIMES_HEADER,
	                      make_matrix<Number>({30.0, 31.0, 32.0, 33.0})});
}

