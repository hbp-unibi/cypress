/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2019 Christoph Ostrau
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
TEST(ToJson, inhom_rec_single)
{
	Network netw;
	auto pop = netw.create_population<IfCondExp>(
	    10, IfCondExpParameters(), IfCondExpSignals().record_spikes());
	pop[5].signals().record_v();
	pop[7].signals().record_v();
	pop[9].signals().record_v();
	auto pop2 = PopulationView<IfCondExp>(netw, pop.pid(), 3, 7);
	pop2.signals().record_gsyn_exc();

	auto vec = ToJson::inhom_rec_single(pop, 0);
	for (auto i : vec) {
		EXPECT_TRUE(i);
	}
	vec = ToJson::inhom_rec_single(pop, 1);
	for (size_t i = 0; i < vec.size(); i++) {
		if (i == 5 || i == 7 || i == 9) {
			EXPECT_TRUE(vec[i]);
		}
		else {
			EXPECT_FALSE(vec[i]);
		}
	}

	vec = ToJson::inhom_rec_single(pop, 2);
	for (size_t i = 0; i < 3; i++) {
		EXPECT_FALSE(vec[i]);
	}
	for (size_t i = 3; i < 7; i++) {
		EXPECT_TRUE(vec[i]);
	}
	for (size_t i = 7; i < 10; i++) {
		EXPECT_FALSE(vec[i]);
	}
	vec = ToJson::inhom_rec_single(pop, 3);
	for (size_t i = 0; i < 10; i++) {
		EXPECT_FALSE(vec[i]);
	}
}

TEST(ToJson, inhom_rec_to_json)
{
	Network netw;
	auto pop = netw.create_population<IfCondExp>(
	    10, IfCondExpParameters(), IfCondExpSignals().record_spikes());
	pop[5].signals().record_v();
	pop[7].signals().record_v();
	pop[9].signals().record_v();
	auto pop2 = PopulationView<IfCondExp>(netw, pop.pid(), 3, 7);
	pop2.signals().record_gsyn_exc();

	auto json = ToJson::inhom_rec_to_json(pop);
	EXPECT_TRUE(json.find("spikes") != json.end());
	EXPECT_TRUE(json.find("v") != json.end());
	EXPECT_TRUE(json.find("gsyn_exc") != json.end());
	EXPECT_FALSE(json.find("gsyn_inh") != json.end());

	auto vec = json["spikes"].get<std::vector<bool>>();
	for (auto i : vec) {
		EXPECT_TRUE(i);
	}
	vec = json["v"].get<std::vector<bool>>();
	for (size_t i = 0; i < vec.size(); i++) {
		if (i == 5 || i == 7 || i == 9) {
			EXPECT_TRUE(vec[i]);
		}
		else {
			EXPECT_FALSE(vec[i]);
		}
	}

	vec = json["gsyn_exc"].get<std::vector<bool>>();
	for (size_t i = 0; i < 3; i++) {
		EXPECT_FALSE(vec[i]);
	}
	for (size_t i = 3; i < 7; i++) {
		EXPECT_TRUE(vec[i]);
	}
	for (size_t i = 7; i < 10; i++) {
		EXPECT_FALSE(vec[i]);
	}
}
void compare_json_conn_desc(const ConnectionDescriptor &conn, const Json &res)
{
	EXPECT_FLOAT_EQ(res["pid_src"], conn.pid_src());
	EXPECT_FLOAT_EQ(res["nid_src0"], conn.nid_src0());
	EXPECT_FLOAT_EQ(res["nid_src1"], conn.nid_src1());
	EXPECT_FLOAT_EQ(res["pid_tar"], conn.pid_tar());
	EXPECT_FLOAT_EQ(res["nid_tar0"], conn.nid_tar0());
	EXPECT_FLOAT_EQ(res["nid_tar1"], conn.nid_tar1());

	const Connector &connector = conn.connector();
	EXPECT_TRUE(res["conn_name"].get<std::string>() == connector.name());
	EXPECT_EQ(res["allow_self_connections"],
	          connector.allow_self_connections());
	EXPECT_FLOAT_EQ(res["additional_parameter"],
	                connector.additional_parameter());
}
TEST(ToJson, connector_to_json)
{
	ConnectionDescriptor conn_desc(0, 0, 16, 1, 0, 16,
	                               Connector::all_to_all(0.15, 1, true));

	Json json = ToJson::connector_to_json(conn_desc);
	compare_json_conn_desc(conn_desc, json);
	EXPECT_FALSE(json.find("connections") != json.end());
	conn_desc = ConnectionDescriptor(0, 0, 16, 1, 0, 16,
	                                 Connector::one_to_one(0.15, 1));
	json = ToJson::connector_to_json(conn_desc);
	compare_json_conn_desc(conn_desc, json);
	EXPECT_FALSE(json.find("connections") != json.end());
	conn_desc = ConnectionDescriptor(0, 0, 16, 1, 0, 16,
	                                 Connector::fixed_fan_in(3, 0.15, 1, true));
	json = ToJson::connector_to_json(conn_desc);
	compare_json_conn_desc(conn_desc, json);
	EXPECT_FALSE(json.find("connections") != json.end());
	conn_desc = ConnectionDescriptor(
	    0, 0, 16, 1, 0, 16, Connector::fixed_fan_out(3, 0.15, 1, false));
	json = ToJson::connector_to_json(conn_desc);
	compare_json_conn_desc(conn_desc, json);
	EXPECT_FALSE(json.find("connections") != json.end());
	conn_desc = ConnectionDescriptor(0, 0, 16, 1, 0, 16,
	                                 Connector::fixed_fan_out(0, 0.15, 1));
	json = ToJson::connector_to_json(conn_desc);
	compare_json_conn_desc(conn_desc, json);
	EXPECT_FALSE(json.find("connections") != json.end());

	conn_desc = ConnectionDescriptor(
	    0, 0, 16, 1, 0, 16,
	    Connector::fixed_probability(Connector::all_to_all(0.15, 1), 0.0));
	json = ToJson::connector_to_json(conn_desc);
	compare_json_conn_desc(conn_desc, json);
	EXPECT_TRUE(json.find("connections") != json.end());
	EXPECT_EQ(json["connections"].size(), size_t(16 * 16));
}
void check_pop(PopulationBase &pop, Json &json)
{
	EXPECT_EQ(json["type"], pop.type().name);
	EXPECT_EQ(json["size"], pop.size());
	EXPECT_EQ(json["label"], pop.name());
	if (json["parameters"][0].size() > 1) {
		EXPECT_FALSE(pop.homogeneous_parameters());
		EXPECT_EQ(json["parameters"].size(), pop.size());
		for (size_t i = 0; i < pop.size(); i++) {
			EXPECT_EQ(json["parameters"][i], pop[i].parameters().parameters());
		}
	}
	else {
		EXPECT_TRUE(pop.homogeneous_parameters());
		EXPECT_EQ(json["parameters"], pop.parameters().parameters());
	}
}

TEST(ToJson, pop_to_json)
{
	Network netw;
	PopulationBase pop =
	    netw.create_population<IfCondExp>(10, IfCondExpParameters());
	Json json = ToJson::pop_to_json(pop);
	check_pop(pop, json);

	auto pop2 = netw.create_population<SpikeSourceArray>(
	    20, SpikeSourceArrayParameters().spike_times({3, 4, 5, 6}), "test");
	json = ToJson::pop_to_json(pop2);
	check_pop(pop2, json);

	auto pop3 = netw.create_population<EifCondExpIsfaIsta>(
	    1, EifCondExpIsfaIstaParameters(), "test2");
	json = ToJson::pop_to_json(pop3);
	check_pop(pop3, json);

	auto pop4 = netw.create_population<IfCurrExp>(0, IfCurrExpParameters(),
	                                              "testasdasdf");
	json = ToJson::pop_to_json(pop4);
	check_pop(pop4, json);

	auto pop5 = netw.create_population<IfFacetsHardware1>(
	    500, IfFacetsHardware1Parameters());
	json = ToJson::pop_to_json(pop5);
	check_pop(pop5, json);

	float counter = 0.0;
	for (auto neuron : pop) {
		for (size_t i = 0; i < neuron.parameters().size(); i++) {
			neuron.parameters().set(i, i * counter);
			counter += 0.5;
		}
	}
	json = ToJson::pop_to_json(pop);
	check_pop(pop, json);
}
TEST(ToJson, hom_rec_to_json)
{
	Network netw;
	PopulationBase pop = netw.create_population<IfCondExp>(
	    10, IfCondExpParameters(), IfCondExpSignals().record_spikes());
	Json json = ToJson::pop_to_json(pop);
	ToJson::hom_rec_to_json(pop, json);
	EXPECT_EQ(json["records"].size(), size_t(1));
	EXPECT_EQ(json["records"][0], "spikes");

	PopulationBase pop2 = netw.create_population<IfCondExp>(
	    10, IfCondExpParameters(), IfCondExpSignals().record_v());
	json = ToJson::pop_to_json(pop2);
	ToJson::hom_rec_to_json(pop2, json);
	EXPECT_EQ(json["records"].size(), size_t(1));
	EXPECT_EQ(json["records"][0], "v");

	PopulationBase pop3 = netw.create_population<IfCondExp>(
	    10, IfCondExpParameters(),
	    IfCondExpSignals().record_v().record_gsyn_exc());
	json = ToJson::pop_to_json(pop3);
	ToJson::hom_rec_to_json(pop3, json);
	EXPECT_EQ(json["records"].size(), size_t(2));

	PopulationBase pop4 =
	    netw.create_population<IfCondExp>(10, IfCondExpParameters(),
	                                      IfCondExpSignals()
	                                          .record_v()
	                                          .record_gsyn_exc()
	                                          .record_gsyn_inh()
	                                          .record_spikes());
	json = ToJson::pop_to_json(pop4);
	ToJson::hom_rec_to_json(pop4, json);
	EXPECT_EQ(json["records"].size(), size_t(4));

	PopulationBase pop5 = netw.create_population<IfCondExp>(
	    10, IfCondExpParameters(), IfCondExpSignals());
	json = ToJson::pop_to_json(pop5);
	ToJson::hom_rec_to_json(pop5, json);
	EXPECT_EQ(json["records"], Json());
}

TEST(ToJson, pop_vec_to_json)
{
	Network netw;
	netw.create_population<IfCondExp>(10, IfCondExpParameters(),
	                                  IfCondExpSignals().record_spikes());
	netw.create_population<IfCondExp>(10, IfCondExpParameters(),
	                                  IfCondExpSignals().record_spikes());
	netw.create_population<IfCondExp>(10, IfCondExpParameters(),
	                                  IfCondExpSignals().record_spikes());

	auto json = ToJson::pop_vec_to_json(netw.populations());
	EXPECT_EQ(json.size(), size_t(3));
}
TEST(ToJson, recs_to_json) {}

TEST(ToJson, read_recordings_from_json)
{
	auto test = {3.0, 4.0, 5.0, 6.0, 78.0};
	Network netw;
	auto pop = netw.create_population<IfCondExp>(
	    10, IfCondExpParameters(), IfCondExpSignals().record_spikes());
	Matrix<Real> mat(test);
	Json json({{"pop_id", 0}, {"signal", 0}});
	json["data"].push_back(mat);
	json["ids"].push_back(3);

	ToJson::read_recordings_from_json(json, netw);
	EXPECT_EQ(pop[0].signals().get_spikes().size(), size_t(0));
	EXPECT_EQ(pop[1].signals().get_spikes().size(), size_t(0));
	EXPECT_EQ(pop[2].signals().get_spikes().size(), size_t(0));
	EXPECT_EQ(pop[3].signals().get_spikes().size(), size_t(5));
	EXPECT_EQ(pop[4].signals().get_spikes().size(), size_t(0));
	EXPECT_EQ(pop[5].signals().get_spikes().size(), size_t(0));
	EXPECT_EQ(pop[6].signals().get_spikes().size(), size_t(0));
	EXPECT_EQ(pop[7].signals().get_spikes().size(), size_t(0));

	auto spikes = pop[3].signals().get_spikes();
	for (size_t i = 0; i < test.size(); i++) {
		EXPECT_EQ(spikes[i], *(test.begin() + i));
	}
}

TEST(ToJson, create_pop_from_json)
{
	auto test = {3.0, 4.0, 5.0, 6.0, 78.0};
	Json json({{"type", "SpikeSourceArray"},
	           {"size", 10},
	           {"parameters", test},
	           {"label", "test"},
	           {"records", {"spikes"}}});
	Network netw;
	ToJson::create_pop_from_json(json, netw);
	auto pop = netw.populations()[0];
	EXPECT_TRUE(pop.type().name == "SpikeSourceArray");
	EXPECT_EQ(pop.size(), size_t(10));
	EXPECT_EQ(pop.parameters().parameters(), std::vector<Real>(test));
	EXPECT_TRUE(pop.homogeneous_record());
	EXPECT_TRUE(pop.signals().is_recording(0));
	EXPECT_EQ(pop.name(), json["label"]);

	netw = Network();
	json = Json(
	    {{"type", "IfCondExp"},
	     {"size", 5},
	     {"label", "test2"},
	     {"parameters", IfCondExpParameters().parameters()},
	     {"records", Json({{"spikes", std::vector<bool>{0, 0, 0, 0, 1}}})}});

	ToJson::create_pop_from_json(json, netw);
	pop = netw.populations()[0];
	EXPECT_TRUE(pop.type().name == "IfCondExp");
	EXPECT_EQ(pop.size(), size_t(5));
	EXPECT_EQ(pop.parameters().parameters(),
	          IfCondExpParameters().parameters());
	EXPECT_FALSE(pop.homogeneous_record());
	EXPECT_EQ(pop.name(), json["label"]);

	for (size_t i = 0; i < pop.size(); i++) {
		if (i == 4) {
			EXPECT_TRUE(pop[i].signals().is_recording(0));
		}
		else {
			EXPECT_FALSE(pop[i].signals().is_recording(0));
		}
		EXPECT_FALSE(pop[i].signals().is_recording(1));
		EXPECT_FALSE(pop[i].signals().is_recording(2));
		EXPECT_FALSE(pop[i].signals().is_recording(3));
	}

	json = Json({{"type", "IfCondExp"},
	             {"size", 10},
	             {"label", "test3"},
	             {"parameters", IfCondExpParameters().parameters()},
	             {"records", {}}});
	netw = Network();
	ToJson::create_pop_from_json(json, netw);
	pop = netw.populations()[0];
	EXPECT_FALSE(pop.signals().is_recording(0));
	EXPECT_FALSE(pop.signals().is_recording(1));
	EXPECT_FALSE(pop.signals().is_recording(2));
	EXPECT_FALSE(pop.signals().is_recording(3));
	EXPECT_EQ(pop.name(), json["label"]);
}

TEST(ToJson, get_synapse)
{
	EXPECT_ANY_THROW(ToJson::get_synapse("dassd", {}));
	auto params = {0.5, 1.0};
	std::shared_ptr<SynapseBase> syn =
	    ToJson::get_synapse("StaticSynapse", params);
	EXPECT_TRUE(syn->name() == "StaticSynapse");
	EXPECT_EQ(syn->parameters(), std::vector<Real>(params));

	syn = ToJson::get_synapse("SpikePairRuleAdditive", params);
	EXPECT_TRUE(syn->name() == "SpikePairRuleAdditive");
	EXPECT_EQ(syn->parameters(), std::vector<Real>(params));

	syn = ToJson::get_synapse("SpikePairRuleMultiplicative", params);
	EXPECT_TRUE(syn->name() == "SpikePairRuleMultiplicative");
	EXPECT_EQ(syn->parameters(), std::vector<Real>(params));

	syn = ToJson::get_synapse("TsodyksMarkramMechanism", params);
	EXPECT_TRUE(syn->name() == "TsodyksMarkramMechanism");
	EXPECT_EQ(syn->parameters(), std::vector<Real>(params));
}
void test_conn(Json &json)
{
	for (std::string i :
	     {"AllToAllConnector", "OneToOneConnector", "RandomConnector",
	      "FixedFanInConnector", "FixedFanOutConnector"}) {
		Network netw = Network();
		auto pop = netw.create_population<IfCondExp>(
		    10, IfCondExpParameters(), IfCondExpSignals().record_spikes());
		auto pop2 = netw.create_population<IfCondExp>(
		    500, IfCondExpParameters(), IfCondExpSignals().record_spikes());
		json["conn_name"] = i;
		ToJson::create_conn_from_json(json, netw);
		auto test = netw.connections()[0];
		EXPECT_EQ(test.pid_src(), json["pid_src"].get<PopulationIndex>());
		EXPECT_EQ(test.nid_src0(), json["nid_src0"].get<NeuronIndex>());
		EXPECT_EQ(test.nid_src1(), json["nid_src1"].get<NeuronIndex>());
		EXPECT_EQ(test.pid_tar(), json["pid_tar"].get<PopulationIndex>());
		EXPECT_EQ(test.nid_tar0(), json["nid_tar0"].get<NeuronIndex>());
		EXPECT_EQ(test.nid_tar1(), json["nid_tar1"].get<NeuronIndex>());
		EXPECT_EQ(test.label(), json["label"].get<std::string>());

		auto &test_conn = test.connector();
		EXPECT_EQ(test_conn.name(), i);
		if (i != "OneToOneConnector") {
			EXPECT_EQ(test_conn.allow_self_connections(),
			          json["allow_self_connections"].get<bool>());
		}
		EXPECT_EQ(test_conn.synapse_name(),
		          json["syn_name"].get<std::string>());

		if ((i != "AllToAllConnector") && (i != "OneToOneConnector")) {
			EXPECT_EQ(test_conn.additional_parameter(),
			          json["additional_parameter"].get<Real>());
		}
	}
}

TEST(ToJson, create_conn_from_json)
{
	auto params = {0.5, 1.0};
	Json json({{"syn_name", "StaticSynapse"},
	           {"params", params},
	           {"conn_name", "AllToAllConnector"},
	           {"allow_self_connections", true},
	           {"additional_parameter", 1.0},
	           {"pid_src", 0},
	           {"nid_src0", 3},
	           {"nid_src1", 8},
	           {"pid_tar", 1},
	           {"nid_tar0", 495},
	           {"nid_tar1", 500},
	           {"label", "test"}});
	test_conn(json);
	json["allow_self_connections"] = false;
	test_conn(json);
	json["syn_name"] = "SpikePairRuleMultiplicative";
	test_conn(json);
	json["nid_src0"] = 0;
	json["nid_src1"] = 10;
	json["nid_tar0"] = 200;
	json["nid_tar1"] = 210;
	test_conn(json);
	json["connections"] = Json({{0,1,0.1,1.0},
                               {1,400,0.1,1.1},
                                {2,201,0.1,1.1}});
	json["syn_name"] = "StaticSynapse";
	Network netw = Network();
	auto pop = netw.create_population<IfCondExp>(
	    10, IfCondExpParameters(), IfCondExpSignals().record_spikes());
	auto pop2 = netw.create_population<IfCondExp>(
	    500, IfCondExpParameters(), IfCondExpSignals().record_spikes());
	ToJson::create_conn_from_json(json, netw);

	auto test = netw.connections()[0];
	EXPECT_EQ(test.pid_src(), json["pid_src"].get<PopulationIndex>());
	EXPECT_EQ(test.nid_src0(), json["nid_src0"].get<NeuronIndex>());
	EXPECT_EQ(test.nid_src1(), json["nid_src1"].get<NeuronIndex>());
	EXPECT_EQ(test.pid_tar(), json["pid_tar"].get<PopulationIndex>());
	EXPECT_EQ(test.nid_tar0(), json["nid_tar0"].get<NeuronIndex>());
	EXPECT_EQ(test.nid_tar1(), json["nid_tar1"].get<NeuronIndex>());
	EXPECT_EQ(test.label(), json["label"].get<std::string>());
	EXPECT_EQ(test.connector().synapse_name(),
	          json["syn_name"].get<std::string>());
}

void compare_netws(Network &netw, Network &test_net)
{
	EXPECT_EQ(test_net.populations(), netw.populations());
	for (size_t i = 0; i < netw.populations().size(); i++) {
		auto pop = netw.populations()[i];
		auto test_pop = test_net.populations()[i];

		EXPECT_EQ(test_pop.size(), pop.size());
		EXPECT_EQ(&test_pop.type(), &pop.type());
		EXPECT_EQ(test_pop.parameters().parameters(),
		          pop.parameters().parameters());
		EXPECT_EQ(test_pop.name(), pop.name());
		for (size_t j = 0; j < pop.signals().size(); j++) {
			EXPECT_EQ(test_pop.signals().is_recording(j),
			          pop.signals().is_recording(j));
		}

		auto size = netw[i].signals().size();
		for (size_t j = 0; j < size; j++) {
			for (size_t k = 0; k < netw[i].size(); k++) {
				if (netw[i][k].signals().is_recording(j)) {
					EXPECT_EQ(netw[i][k].signals().data(j),
					          test_net[i][k].signals().data(j));
				}
			}
		}
	}
	EXPECT_EQ(netw.connections(), test_net.connections());
	for (size_t i = 0; i < netw.connections().size(); i++) {
		EXPECT_EQ(test_net.connections()[i].label(),
		          netw.connections()[i].label());
	}
}

TEST(ToJson, roundtrip_pops)
{
	Network netw = Network();
	auto pop = netw.create_population<IfCondExp>(
	    10, IfCondExpParameters(), IfCondExpSignals().record_spikes(), "test");
	auto pop2 = netw.create_population<IfCurrExp>(
	    500, IfCurrExpParameters(), IfCurrExpSignals().record_v());
	auto pop3 = netw.create_population<IfFacetsHardware1>(
	    500, IfFacetsHardware1Parameters(),
	    IfFacetsHardware1Signals().record_spikes(), "bla");
	auto pop4 = netw.create_population<SpikeSourceArray>(
	    20, SpikeSourceArrayParameters().spike_times({3, 4, 5, 6}));

	auto json = ToJson::pop_vec_to_json(netw.populations());
	Network test_net;
	for (auto i : json) {
		ToJson::create_pop_from_json(i, test_net);
	}
	compare_netws(netw, test_net);

	auto test = {3.0, 4.0, 5.0, 6.0, 78.0};
	json = Json({{"type", "SpikeSourceArray"},
	             {"size", 10},
	             {"parameters", test},
	             {"label", "möp"},
	             {"records", {"spikes"}}});
	netw = Network();
	ToJson::create_pop_from_json(json, netw);
	auto json2 = ToJson::pop_vec_to_json(netw.populations());
	EXPECT_EQ(json2[0], json);

	netw = Network();
	json = Json(
	    {{"type", "IfCondExp"},
	     {"size", 5},
	     {"label", "möp2"},
	     {"parameters", IfCondExpParameters().parameters()},
	     {"records", Json({{"spikes", std::vector<bool>{0, 0, 0, 0, 1}}})}});
	ToJson::create_pop_from_json(json, netw);
	json2 = ToJson::pop_vec_to_json(netw.populations());
	EXPECT_EQ(json2[0], json);

	json = Json({{"type", "IfCondExp"},
	             {"size", 10},
	             {"label", "möp3"},
	             {"parameters", IfCondExpParameters().parameters()},
	             {"records", Json()}});
	netw = Network();
	ToJson::create_pop_from_json(json, netw);
	json2 = ToJson::pop_vec_to_json(netw.populations());
	EXPECT_EQ(json2[0], json);
}
Network create_net()
{
	Network netw;
	netw.create_population<IfCondExp>(16, IfCondExpParameters(),
	                                  IfCondExpSignals().record_spikes());
	netw.create_population<IfCurrExp>(16, IfCurrExpParameters(),
	                                  IfCurrExpSignals().record_v());
	return netw;
}

TEST(ToJson, roundtrip_conns)
{
	Network netw = create_net();
	ConnectionDescriptor conn_desc(0, 0, 16, 1, 0, 16,
	                               Connector::all_to_all(0.15, 1, true));
	Json json = ToJson::connector_to_json(conn_desc);
	ToJson::create_conn_from_json(json, netw);
	EXPECT_EQ(netw.connections()[0], conn_desc);

	netw = create_net();
	conn_desc = ConnectionDescriptor(0, 0, 16, 1, 0, 16,
	                                 Connector::one_to_one(0.15, 1));
	json = ToJson::connector_to_json(conn_desc);
	ToJson::create_conn_from_json(json, netw);
	EXPECT_EQ(netw.connections()[0], conn_desc);

	netw = create_net();
	conn_desc = ConnectionDescriptor(0, 0, 16, 1, 0, 16,
	                                 Connector::fixed_fan_in(3, 0.15, 1, true));
	json = ToJson::connector_to_json(conn_desc);
	ToJson::create_conn_from_json(json, netw);
	EXPECT_EQ(netw.connections()[0], conn_desc);

	netw = create_net();
	conn_desc = ConnectionDescriptor(
	    0, 0, 16, 1, 0, 16, Connector::fixed_fan_out(3, 0.15, 1, false));
	json = ToJson::connector_to_json(conn_desc);
	ToJson::create_conn_from_json(json, netw);
	EXPECT_EQ(netw.connections()[0], conn_desc);

	netw = create_net();
	conn_desc = ConnectionDescriptor(0, 0, 16, 1, 0, 16,
	                                 Connector::fixed_fan_out(0, 0.15, 1));
	json = ToJson::connector_to_json(conn_desc);
	ToJson::create_conn_from_json(json, netw);
	EXPECT_EQ(netw.connections()[0], conn_desc);

	netw = create_net();
	conn_desc = ConnectionDescriptor(
	    0, 0, 16, 1, 0, 16,
	    Connector::fixed_probability(Connector::all_to_all(0.15, 1), 0.0));
	json = ToJson::connector_to_json(conn_desc);
	ToJson::create_conn_from_json(json, netw);
	EXPECT_EQ(netw.connections()[0], conn_desc);
}
TEST(ToJson, roundtrip_records)
{
	auto test = {3.0, 4.0, 5.0, 6.0, 78.0};
	Network netw;
	auto pop = netw.create_population<IfCondExp>(
	    4, IfCondExpParameters(), IfCondExpSignals().record_spikes());
	Matrix<Real> mat(test);
	Json json({{"pop_id", 0}, {"signal", 0}});
	json["data"].push_back(mat);
	json["data"].push_back(Json());
	json["data"].push_back(mat);
	json["data"].push_back(mat);
	json["ids"] = {0, 1, 2, 3};
	ToJson::read_recordings_from_json(json, netw);
	auto json_test = ToJson::recs_to_json(pop);
	EXPECT_EQ(json_test[0], json);

	Network netw2;
	auto pop2 = netw2.create_population<IfCondExp>(
	    4, IfCondExpParameters(), IfCondExpSignals().record_spikes());
	ToJson::read_recordings_from_json(json_test[0], netw2);
	compare_netws(netw, netw2);
}

Network create_net_simple()
{
	return Network()
	    .add_population<SpikeSourceArray>(
	        "source", 3, SpikeSourceArrayParameters({20, 50}),
	        SpikeSourceArraySignals().record_spikes())
	    .add_population<IfCondExp>("target", 10,
	                               IfCondExpParameters()
	                                   .cm(0.2)
	                                   .v_reset(-70)
	                                   .v_rest(-20)
	                                   .v_thresh(-10)
	                                   .e_rev_E(60)
	                                   .tau_m(20)
	                                   .tau_refrac(0.1)
	                                   .tau_syn_E(5.0),
	                               IfCondExpSignals().record_spikes())
	    .add_connection(
	        "source", "target",
	        Connector::all_to_all(StaticSynapse().weight(15).delay(1)));
}

Network create_net_simple_inhib()
{
	Network net =
	    Network()
	        .add_population<SpikeSourceConstFreq>(
	            "source", 8,
	            SpikeSourceConstFreqParameters()
	                .start(100.0)
	                .rate(1000.0)
	                .duration(1000.0),
	            SpikeSourceConstFreqSignals().record_spikes())
	        .add_population<IfFacetsHardware1>(
	            "target", 32, IfFacetsHardware1Parameters().g_leak(0.04),
	            IfFacetsHardware1Signals().record_spikes())
	        .add_connection("source", "target",
	                        Connector::all_to_all(0.015, 1));
	PopulationView<IfFacetsHardware1> popview(
	    net, net.populations("target")[0].pid(), 8, 20);
	net.add_population<SpikeSourceConstFreq>(
	    "source2", 16,
	    SpikeSourceConstFreqParameters().start(100.0).rate(100.0).duration(
	        1000.0),
	    SpikeSourceConstFreqSignals().record_spikes());
	net.add_connection("source2", popview, Connector::all_to_all(-0.015, 1),
	                   "test_conn");
	return net;
}

TEST(ToJson, roundtrip_network)
{
	auto net = create_net_simple();
	SpikePairRuleAdditive plastic_synapse = SpikePairRuleAdditive();
	net.add_connection("source", "target",
	                   Connector::all_to_all(plastic_synapse), "conn");

	Json json(net);
	auto net_test = json.get<Network>();
	compare_netws(net, net_test);
	EXPECT_EQ(json, Json(net_test));

	bool nest_avail = false;
	try {
		net.run("pynn.nest", 100);
		nest_avail = true;
	}
	catch (...) {
		std::cout
		    << "... Skipping test using nest simulator for roundtrip testing"
		    << std::endl;
		nest_avail = false;
	}
	if (nest_avail) {
		json = Json(net);
		net_test = json.get<Network>();
		compare_netws(net, net_test);
		EXPECT_EQ(json, Json(net_test));
		auto weights =
		    net_test.connections().back().connector().learned_weights();
		EXPECT_EQ(weights.size(), net.population("source").size() *
		                              net.population("target").size());
	}

	net = create_net_simple_inhib();
	if (nest_avail) {
		net.run("nest", 0.0);
	}
	json = Json(net);
	net_test = json.get<Network>();
	compare_netws(net, net_test);
	EXPECT_EQ(json, Json(net_test));
}

TEST(ToJson, backend)
{
	auto net = create_net_simple();
	auto net2 = create_net_simple();
	try {
		net.run("nest", 100);
	}
	catch (...) {
		std::cout
		    << "... Skipping test using nest simulator for backend testing"
		    << std::endl;
		return;
	}
	net2.run("json.nest", 100);
	compare_netws(net, net2);

	net = create_net_simple_inhib();
	net2 = create_net_simple_inhib();
	net.run("nest");
	net2.run("json.nest");
	compare_netws(net, net2);
}
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

template <typename T>
void compare_mat_json(Matrix<T> &mat, Json &json)
{
	for (size_t i = 0; i < mat.rows(); i++) {
		for (size_t j = 0; j < mat.cols(); j++) {
			EXPECT_EQ(json[i][j], mat(i, j));
		}
	}
	EXPECT_EQ(mat.rows(), json.size());
	EXPECT_EQ(mat.cols(), json[0].size());
}

template <typename T>
Matrix<T> create_mat(size_t rows, size_t cols)
{
	Matrix<T> mat(rows, cols);
	for (size_t i = 0; i < rows; i++) {
		for (size_t j = 0; j < cols; j++) {
			mat(i, j) = i * j + i + 2 * j;
		}
	}
	return mat;
}

template <typename T>
void test_mat_to_json(size_t rows, size_t cols)
{
	Matrix<T> mat = create_mat<T>(rows, cols);
	Json json(mat);
	compare_mat_json(mat, json);
}

TEST(Matrix2Json, mat_to_json)
{
	test_mat_to_json<Real>(3, 4);
	test_mat_to_json<Real>(1, 4);
	test_mat_to_json<Real>(3, 1);
	test_mat_to_json<Real>(500, 5000);
	test_mat_to_json<size_t>(3, 4);
	test_mat_to_json<int>(3, 4);
	test_mat_to_json<uint64_t>(3, 4);
}

template <typename T>
Json create_json_mat(size_t rows, size_t cols)
{
	Json res;
	for (size_t i = 0; i < rows; i++) {
		std::vector<T> vec(cols);
		for (size_t j = 0; j < cols; j++) {
			vec[j] = i * j + i + 2 * j;
		}
		res.emplace_back(vec);
	}
	return res;
}

template <typename T>
void test_json_to_mat(size_t rows, size_t cols)
{
	Json json = create_json_mat<T>(rows, cols);
	Matrix<T> mat = json.get<Matrix<T>>();
	compare_mat_json(mat, json);
}

TEST(Matrix2Json, mat_from_json)
{
	test_json_to_mat<Real>(3, 4);
	test_json_to_mat<Real>(1, 4);
	test_json_to_mat<Real>(3, 1);
	test_json_to_mat<Real>(500, 5000);
	test_json_to_mat<size_t>(3, 4);
	test_json_to_mat<int>(3, 4);
	test_json_to_mat<uint64_t>(3, 4);
}

template <typename T>
void test_roundtrip(size_t rows, size_t cols)
{
	Matrix<T> mat = create_mat<T>(rows, cols);
	Json json(mat);
	Matrix<T> mat2 = json.get<Matrix<T>>();
	EXPECT_EQ(mat2, mat);

	json = create_json_mat<T>(rows, cols);
	mat = json.get<Matrix<T>>();
	Json json2(mat);
	EXPECT_EQ(json2, json);
}

TEST(Matrix2Json, roundtrip)
{
	test_roundtrip<Real>(3, 4);
	test_roundtrip<Real>(1, 4);
	test_roundtrip<Real>(3, 1);
	test_roundtrip<Real>(500, 5000);
	test_roundtrip<size_t>(3, 4);
	test_roundtrip<int>(3, 4);
	test_roundtrip<uint64_t>(3, 4);
}

TEST(Runtime2Json, runtime2json)
{
	NetworkRuntime runtime{1, 2, 3, 4};
	Json json(runtime);
	NetworkRuntime runtime2 = json.get<NetworkRuntime>();
	Json json2(runtime2);

	EXPECT_FLOAT_EQ(runtime.total, json["total"]);
	EXPECT_FLOAT_EQ(runtime.sim, json["sim"]);
	EXPECT_FLOAT_EQ(runtime.finalize, json["finalize"]);
	EXPECT_FLOAT_EQ(runtime.initialize, json["initialize"]);
	EXPECT_FLOAT_EQ(runtime2.total, json2["total"]);
	EXPECT_FLOAT_EQ(runtime2.sim, json2["sim"]);
	EXPECT_FLOAT_EQ(runtime2.finalize, json2["finalize"]);
	EXPECT_FLOAT_EQ(runtime2.initialize, json2["initialize"]);

	EXPECT_FLOAT_EQ(runtime2.total, runtime.total);
	EXPECT_FLOAT_EQ(runtime2.sim, runtime.sim);
	EXPECT_FLOAT_EQ(runtime2.finalize, runtime.finalize);
	EXPECT_FLOAT_EQ(runtime2.initialize, runtime.initialize);
	EXPECT_EQ(json, json2);
}
}  // namespace cypress
