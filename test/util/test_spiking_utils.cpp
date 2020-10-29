/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2016  Christoph Jenzen
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
#include <cypress/util/neuron_parameters.hpp>
#include <cypress/util/spiking_utils.hpp>
#include <string>

#include "gtest/gtest.h"

namespace cypress {
TEST(SpikingUtils, detect_type)
{
	EXPECT_EQ(&SpikingUtils::detect_type("IF_cond_exp"), &IfCondExp::inst());
	EXPECT_EQ(&SpikingUtils::detect_type("IfFacetsHardware1"),
	          &IfFacetsHardware1::inst());
	EXPECT_EQ(&SpikingUtils::detect_type("AdExp"), &EifCondExpIsfaIsta::inst());
	EXPECT_ANY_THROW(SpikingUtils::detect_type("Nothing"));
}

TEST(SpikingUtils, add_population)
{
	cypress::Network netw;
	cypress::Json json;
	NeuronParameter params(IfCondExp::inst(), json);
	std::string type = "IF_cond_exp";
	auto pop1 = SpikingUtils::add_population(type, netw, params, 1);
	EXPECT_EQ(netw.population_count(), size_t(1)

	);
	EXPECT_EQ(netw.neuron_count(), size_t(1));
	EXPECT_EQ(netw.neuron_count(IfCondExp::inst()), size_t(1)

	);
	EXPECT_EQ(netw.neuron_count(IfFacetsHardware1::inst()), size_t(0)

	);
	EXPECT_FALSE(pop1.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("v").value()));
	EXPECT_FALSE(pop1.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("spikes").value()));

	EXPECT_EQ(pop1.size(), size_t(1));
	EXPECT_EQ(&pop1.type(), &IfCondExp::inst());

	auto pop2 = SpikingUtils::add_population(type, netw, params, 1, "v");
	EXPECT_TRUE(pop2.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("v").value()));
	EXPECT_FALSE(pop2.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("spikes").value()));

	auto pop3 = SpikingUtils::add_population(type, netw, params, 1, "gsyn_exc");
	EXPECT_FALSE(pop3.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("v").value()));
	EXPECT_FALSE(pop3.signals().is_recording(
	    cypress::IfCondExp::inst().signal_index("spikes").value()));
}

TEST(SpikingUtils, calc_num_spikes)
{
	std::vector<cypress::Real> spiketrain;
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 0.0), 0);
	for (size_t i = 0; i < 50; i++) {
		spiketrain.push_back(cypress::Real(i) * 0.3);
	}
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 20.0), 0);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 15.0), 0);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 14.7), 1);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 14.4), 2);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 14.0), 3);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 13.6), 4);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 0.0), 50);

	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 11.4), 05);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 11.7), 06);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 12.0), 07);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 13.0), 10);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 14.0), 13);
	EXPECT_EQ(SpikingUtils::calc_num_spikes(spiketrain, 10.0, 15.0), 16);
}

TEST(SpikingUtils, calc_num_spikes_vec)
{
	std::array<cypress::Real, 50> spiketrain, spiketrain2;
	for (size_t i = 0; i < 50; i++) {
		spiketrain[i] = cypress::Real(i) * 0.3;
		spiketrain2[i] = cypress::Real(i) * 0.4;
	}
	cypress::Matrix<cypress::Real> spiketrains(
	    std::array<std::array<cypress::Real, 50>, 2>{
	        {spiketrain, spiketrain2}});

	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 20.0)[0], 0);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 15.0)[0], 0);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 14.7)[0], 1);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 14.4)[0], 2);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 14.0)[0], 3);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 13.6)[0], 4);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains)[0], 50);

	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 11.4)[0], 5);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 11.7)[0], 6);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 12.0)[0], 7);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 13.0)[0],
	          10);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 14.0)[0],
	          13);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 10.0, 15.0)[0],
	          16);

	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 0.0)[1], 50);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 8.0, 9.0)[1], 3);
	EXPECT_EQ(SpikingUtils::calc_num_spikes_vec(spiketrains, 8.0, 10.0)[1], 6);
}

TEST(SpikingUtils, spike_time_binning)
{
	std::vector<Real> spikes = {};
	auto bins = SpikingUtils::spike_time_binning<size_t>(0, 1, 10, spikes);
	EXPECT_EQ(size_t(10), bins.size());
	for (auto i : bins) {
		EXPECT_EQ(size_t(0), i);
	}

	for (size_t i = 0; i < 100; i++) {
		spikes.push_back(i * 0.1);
	}
	bins = SpikingUtils::spike_time_binning<size_t>(0, 10, 10, spikes);
	EXPECT_EQ(size_t(10), bins.size());
	for (auto i : bins) {
		EXPECT_EQ(size_t(10), i);
	}

	bins = SpikingUtils::spike_time_binning<size_t>(1, 10, 9, spikes);
	EXPECT_EQ(size_t(9), bins.size());
	for (auto i : bins) {
		EXPECT_EQ(size_t(10), i);
	}
	bins = SpikingUtils::spike_time_binning<size_t>(1, 9, 8, spikes);
	EXPECT_EQ(size_t(8), bins.size());
	for (auto i : bins) {
		EXPECT_EQ(size_t(10), i);
	}

	bins = SpikingUtils::spike_time_binning<size_t>(1, 9, 16, spikes);
	EXPECT_EQ(size_t(16), bins.size());
	for (auto i : bins) {
		EXPECT_EQ(size_t(5), i);
	}
	spikes.push_back(3.421);
	spikes.push_back(8.95);
	std::sort(spikes.begin(), spikes.end());
	bins = SpikingUtils::spike_time_binning<size_t>(1, 9, 8, spikes);
	EXPECT_EQ(size_t(8), bins.size());

	EXPECT_EQ(size_t(11), bins[2]);
	EXPECT_EQ(size_t(11), bins[7]);
}

TEST(SpikingUtils, spike_time_binning_TTFS)
{
	std::vector<Real> spikes = {};
	auto bins = SpikingUtils::spike_time_binning_TTFS(0, 1, 10, spikes);
	EXPECT_EQ(size_t(10), bins.size());
	for (auto i : bins) {
		EXPECT_FLOAT_EQ(std::numeric_limits<Real>::max(), i);
	}

	for (size_t i = 0; i < 100; i++) {
		spikes.push_back(i * 0.1);
	}
	bins = SpikingUtils::spike_time_binning_TTFS(0, 10, 10, spikes);
	EXPECT_EQ(size_t(10), bins.size());
	for (size_t i = 0; i < bins.size(); i++) {
		EXPECT_FLOAT_EQ(Real(i) * 1.0, bins[i]);
	}

	bins = SpikingUtils::spike_time_binning_TTFS(1, 10, 9, spikes);
	EXPECT_EQ(size_t(9), bins.size());
	for (size_t i = 0; i < bins.size(); i++) {
		EXPECT_FLOAT_EQ(1.0 + Real(i) * 1.0, bins[i]);
	}
	bins = SpikingUtils::spike_time_binning_TTFS(1, 9, 8, spikes);
	EXPECT_EQ(size_t(8), bins.size());
	for (size_t i = 0; i < bins.size(); i++) {
		EXPECT_FLOAT_EQ(1.0 + Real(i) * 1.0, bins[i]);
	}

	bins = SpikingUtils::spike_time_binning_TTFS(1, 9, 16, spikes);
	EXPECT_EQ(size_t(16), bins.size());
	for (size_t i = 0; i < bins.size(); i++) {
		EXPECT_FLOAT_EQ(1.0 + Real(i) * 0.5, bins[i]);
	}

	spikes = std::vector<Real>{3.421, 8.95};
	bins = SpikingUtils::spike_time_binning_TTFS(1, 9, 8, spikes);
	EXPECT_EQ(size_t(8), bins.size());

	EXPECT_FLOAT_EQ(std::numeric_limits<Real>::max(), bins[0]);
	EXPECT_FLOAT_EQ(std::numeric_limits<Real>::max(), bins[1]);
	EXPECT_EQ(3.421_R, bins[2]);
	EXPECT_FLOAT_EQ(std::numeric_limits<Real>::max(), bins[3]);
	EXPECT_FLOAT_EQ(std::numeric_limits<Real>::max(), bins[4]);
	EXPECT_FLOAT_EQ(std::numeric_limits<Real>::max(), bins[5]);
	EXPECT_FLOAT_EQ(std::numeric_limits<Real>::max(), bins[6]);
	EXPECT_EQ(8.95_R, bins[7]);
}

static const std::string test_json =
    "{\n"
    "\t\"data\": {\n"
    "\t\t\"n_bits_in\": 100,\n"
    "\t\t\"n_bits_out\": 100,\n"
    "\t\t\"n_ones_in\": 4,\n"
    "\t\t\"n_ones_out\": 4,\n"
    "\t\t\"n_samples\" : 1000\n"
    "\t},\n"
    "\n"
    "\t\"network\": {\n"
    "\t\t\"params\": {\n"
    "\t\t\t\"e_rev_E\": 0.0,\n"
    "\t\t\t\"v_rest\": -70.0,\n"
    "\t\t\t\"v_reset\": -80.0,\n"
    "\t\t\t\"v_thresh\": -57.0,\n"
    "\t\t\t\"tau_syn_E\": 2.0,\n"
    "\t\t\t\"tau_refrac\": 0.0,\n"
    "\t\t\t\"tau_m\": 50.0,\n"
    "\t\t\t\"cm\": 0.2\n"
    "\t\t},\n"
    "\t\t\"neuron_type\": \"IF_cond_exp\",\n"
    "\t\t\"weight\": 0.01,\n"
    "\t\t\"input_burst_size\": 1,\n"
    "\t\t\"output_burst_size\": 1,\n"
    "\t\t\"time_window\": 100.0,\n"
    "\t\t\"isi\": 2.0,\n"
    "\t\t\"sigma_t\": 2.0,\n"
    "\t\t\"sigma_offs\": 0.0,\n"
    "\t\t\"p0\": 0.0,\n"
    "\t\t\"p1\": 0.0,\n"
    "\t\t\"general_offset\" : 100\n"
    "\t}\n"
    "}\n"
    "";

TEST(NeuronParameters, json_to_map)
{
	std::stringstream ss(test_json);
	Json json = Json::parse(ss);

	auto map = json_to_map<float>(json["network"]);
	EXPECT_EQ(map.end(), map.find("neuron_type"));
	EXPECT_NEAR(0.01, map.find("weight")->second, 1e-8);
	EXPECT_NEAR(1.0, map.find("input_burst_size")->second, 1e-8);
	EXPECT_NEAR(1.0, map.find("output_burst_size")->second, 1e-8);
	EXPECT_NEAR(100.0, map.find("time_window")->second, 1e-8);
	EXPECT_NEAR(2.0, map.find("isi")->second, 1e-8);
	EXPECT_NEAR(2.0, map.find("sigma_t")->second, 1e-8);
	EXPECT_NEAR(0.0, map.find("sigma_offs")->second, 1e-8);
	EXPECT_NEAR(0.0, map.find("p0")->second, 1e-8);
	EXPECT_NEAR(0.0, map.find("p1")->second, 1e-8);
	EXPECT_NEAR(100.0, map.find("general_offset")->second, 1e-8);
}

TEST(NeuronParameters, read_check)
{
	std::stringstream ss(test_json);
	Json json = Json::parse(ss);

	auto map = json_to_map<float>(json["network"]["params"]);
	std::vector<std::string> names = {"e_rev_E",  "v_rest",    "v_reset",
	                                  "v_thresh", "tau_syn_E", "tau_refrac",
	                                  "tau_m",    "cm"};

	std::vector<float> def = {1, 1, 1, 1, 1, 1, 1, 1};

	auto res = read_check<float>(map, names, def);
	EXPECT_NEAR(res[0], 0.0, 1e-8);
	EXPECT_NEAR(res[1], -70.0, 1e-8);
	EXPECT_NEAR(res[2], -80.0, 1e-8);
	EXPECT_NEAR(res[3], -57.0, 1e-8);
	EXPECT_NEAR(res[4], 2.0, 1e-8);
	EXPECT_NEAR(res[5], 0.0, 1e-8);
	EXPECT_NEAR(res[6], 50.0, 1e-8);
	EXPECT_NEAR(res[7], 0.2, 1e-8);

	map = json_to_map<float>(json["network"]);
	EXPECT_ANY_THROW(
	    read_check<float>(map, std::vector<std::string>({"input_burst_size"}),
	                      std::vector<float>({0})));
}
}  // namespace cypress
