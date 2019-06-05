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
#include <cypress/backend/serialize/to_json.hpp>

#include <cypress/backend/resources.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/network.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/core/network_base_objects.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/util/filesystem.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/util/process.hpp>
#include <thread>

namespace cypress {

ToJson::ToJson(const std::string &simulator, const Json &setup)
    : m_simulator(simulator), m_setup(setup)
{
}

ToJson::~ToJson() = default;

static std::map<const NeuronType *, std::string> NeuronTypesMap = {
    {&SpikeSourceArray::inst(), "SpikeSourceArray"},
    {&IfCondExp::inst(), "IfCondExp"},
    {&EifCondExpIsfaIsta::inst(), "EifCondExpIsfaIsta"},
    {&IfCurrExp::inst(), "IfCurrExp"},
    {&IfFacetsHardware1::inst(), "IfFacetsHardware1"},
    {&SpikeSourcePoisson::inst(), "SpikeSourcePoisson"},
    {&SpikeSourceConstFreq::inst(), "SpikeSourceConstFreq"},
    {&SpikeSourceConstInterval::inst(), "SpikeSourceConstInterval"}};

std::vector<bool> ToJson::inhom_rec_single(const PopulationBase &pop,
                                           size_t index)
{
	std::vector<bool> res(pop.size(), false);
	for (size_t i = 0; i < pop.size(); i++) {
		res[i] = pop[i].signals().is_recording(index);
	}
	return res;
}

Json ToJson::inhom_rec_to_json(const PopulationBase &pop)
{
	const std::vector<std::string> &signals = pop.type().signal_names;
	Json res;
	for (size_t i = 0; i < signals.size(); i++) {
		auto vec = inhom_rec_single(pop, i);
		if (std::all_of(vec.begin(), vec.end(), [](bool v) { return !v; })) {
			continue;  // no records
		}
		res[signals[i]] = vec;
	}
	return res;
}

Json ToJson::connector_to_json(const ConnectionDescriptor &conn)
{
	Json res;
	res["pid_src"] = conn.pid_src();
	res["nid_src0"] = conn.nid_src0();
	res["nid_src1"] = conn.nid_src1();
	res["pid_tar"] = conn.pid_tar();
	res["nid_tar0"] = conn.nid_tar0();
	res["nid_tar1"] = conn.nid_tar1();

	const Connector &connector = conn.connector();
	res["conn_name"] = connector.name();
	res["allow_self_connections"] = connector.allow_self_connections();
	res["additional_parameter"] = connector.additional_parameter();

	res["syn_name"] = connector.synapse()->name();
	res["params"] = connector.synapse()->parameters();

	if (connector.name() == "FromListConnector" ||
	    connector.name() == "UniformFunctorConnector" ||
	    connector.name() == "FunctorConnector" ||
	    connector.name() == "FixedProbabilityConnector"

	) {
		std::vector<Connection> tar;
		connector.connect(conn, tar);
		for (auto i : tar) {
			Json temp = {{"src", i.n.src},
			             {"tar", i.n.tar},
			             {"par", i.n.SynapseParameters}};
			res["connections"].emplace_back(temp);
		}
	}
	return res;
}

void ToJson::read_recordings_from_json(const Json &pop_data, NetworkBase &netw)
{
	auto pop_id = pop_data["pop_id"].get<PopulationIndex>();
	auto signal = pop_data["signal"].get<size_t>();
	for (size_t i = 0; i < pop_data["data"].size(); i++) {
		auto data = std::make_shared<Matrix<Real>>(
		    pop_data["data"][i].get<Matrix<Real>>());
		auto id = pop_data["ids"][i];
		auto neuron = netw.populations()[pop_id][id];
		neuron.signals().data(signal, std::move(data));
	}
}

void ToJson::do_run(NetworkBase &network, Real duration) const
{
	Json json_out;
	json_out["simulator"] = m_simulator;
	json_out["setup"] = m_setup;
	json_out["duration"] = duration;
	json_out["network"] = network;

	std::string path = "experiment_XXXXX";
	filesystem::tmpfile(path);
	auto file = std::ofstream(path + ".cbor", std::ios::binary);
	Json::to_cbor(json_out, file);
	file.close();

	file = std::ofstream(path + ".bson", std::ios::binary);
	Json::to_bson(json_out, file);
	file.close();

	file = std::ofstream(path + ".msgpack", std::ios::binary);
	Json::to_msgpack(json_out, file);
	file.close();

	file = std::ofstream(path + ".json", std::ios::binary);
	file << json_out;
	file.close();
	json_out = {};

	Process proc("examples/json_exec", {path});

	std::thread log_thread_beg(Process::generic_pipe,
	                           std::ref(proc.child_stdout()),
	                           std::ref(std::cout));

	// Continiously track stderr of child
	std::thread err_thread(Process::generic_pipe, std::ref(proc.child_stderr()),
	                       std::ref(std::cerr));

	int res = proc.wait();

	err_thread.join();
	log_thread_beg.join();
	if (res < 0) {
		throw ExecutionError(
		    std::string("Simulator child process killed by signal " +
		                std::to_string(-res)));
	}

	auto file_in = std::ifstream(path + "_res.cbor", std::ios::binary);

	Json result = Json::from_cbor(file_in);

	const auto &recs = result["recordings"];
	for (size_t i = 0; i < recs.size(); i++) {
		for (size_t j = 0; j < recs[i].size(); j++) {
			ToJson::read_recordings_from_json(recs[i][j], network);
		}
	}
}
std::unordered_set<const NeuronType *> ToJson::supported_neuron_types() const
{
	return {&SpikeSourceArray::inst(),     &IfCondExp::inst(),
	        &EifCondExpIsfaIsta::inst(),   &IfCurrExp::inst(),
	        &IfFacetsHardware1::inst(),    &SpikeSourcePoisson::inst(),
	        &SpikeSourceConstFreq::inst(), &SpikeSourceConstInterval::inst()};
}

std::string ToJson::name() const { return "json"; }

void ToJson::create_pop_from_json(const Json &pop_json, Network &netw)
{
	std::shared_ptr<PopulationBase> pop;
	std::string name = pop_json["type"].get<std::string>();
	if (name == "SpikeSourceArray") {
		pop = std::make_shared<PopulationBase>(
		    netw.create_population<SpikeSourceArray>(
		        pop_json["size"],
		        SpikeSourceArrayParameters(
		            pop_json["parameters"].get<std::vector<Real>>()),
		        SpikeSourceArraySignals()));
	}
	else if (name == "IfCondExp") {
		pop =
		    std::make_shared<PopulationBase>(netw.create_population<IfCondExp>(
		        pop_json["size"],
		        IfCondExpParameters(
		            pop_json["parameters"].get<std::vector<Real>>()),
		        IfCondExpSignals()));
	}
	else if (name == "IfFacetsHardware1") {
		pop = std::make_shared<PopulationBase>(
		    netw.create_population<IfFacetsHardware1>(
		        pop_json["size"],
		        IfFacetsHardware1Parameters(
		            pop_json["parameters"].get<std::vector<Real>>())));
	}
	else if (name == "EifCondExpIsfaIsta") {
		pop = std::make_shared<PopulationBase>(
		    netw.create_population<EifCondExpIsfaIsta>(
		        pop_json["size"],
		        EifCondExpIsfaIstaParameters(
		            pop_json["parameters"].get<std::vector<Real>>())));
	}
	else if (name == "IfCurrExp") {
		pop =
		    std::make_shared<PopulationBase>(netw.create_population<IfCurrExp>(
		        pop_json["size"],
		        IfCurrExpParameters(
		            pop_json["parameters"].get<std::vector<Real>>())));
	}

	else if (name == "SpikeSourcePoisson") {
		pop = std::make_shared<PopulationBase>(
		    netw.create_population<SpikeSourcePoisson>(
		        pop_json["size"],
		        SpikeSourcePoissonParameters(
		            pop_json["parameters"].get<std::vector<Real>>())));
	}
	else if (name == "SpikeSourceConstFreq") {
		pop = std::make_shared<PopulationBase>(
		    netw.create_population<SpikeSourceConstFreq>(
		        pop_json["size"],
		        SpikeSourceConstFreqParameters(
		            pop_json["parameters"].get<std::vector<Real>>())));
	}
	else if (name == "SpikeSourceConstInterval") {
		pop = std::make_shared<PopulationBase>(
		    netw.create_population<SpikeSourceConstInterval>(
		        pop_json["size"],
		        SpikeSourceConstIntervalParameters(
		            pop_json["parameters"].get<std::vector<Real>>())));
	}
	else {
		throw CypressException("Unknown pop type " + name + "!");
	}

	// TODO inhomog params
	if (pop_json["records"].is_object()) {
		for (auto &signal : pop_json["records"].items()) {
			auto index = pop->type().signal_index(signal.key());
			if (!index.valid()) {
				throw CypressException("Unknown signal type " + signal.key() +
				                       " for neuron type " + pop->type().name);
			}
			auto flags = signal.value().get<std::vector<bool>>();
			for (size_t i = 0; i < pop->size(); i++) {
				if (flags[i]) {
					(*pop)[i].signals().record(index.value());
				}
			}
		}
	}
	else {
		for (auto &i : pop_json["records"]) {
			auto index = pop->type().signal_index(i.get<std::string>());
			if (!index.valid()) {
				throw CypressException("Unknown signal type " +
				                       i.get<std::string>() +
				                       " for neuron type " + pop->type().name);
			}
			pop->signals().record(index.value());
		}
	}
}

inline std::shared_ptr<SynapseBase> ToJson::get_synapse(
    const std::string &name, const std::vector<Real> &parameters)
{
	if (name == "StaticSynapse") {
		return std::make_shared<StaticSynapse>(parameters);
	}
	else if (name == "SpikePairRuleAdditive") {
		return std::make_shared<SpikePairRuleAdditive>(parameters);
	}
	else if (name == "SpikePairRuleMultiplicative") {
		return std::make_shared<SpikePairRuleMultiplicative>(parameters);
	}
	else if (name == "TsodyksMarkramMechanism") {
		return std::make_shared<TsodyksMarkramMechanism>(parameters);
	}
	else {
		throw CypressException("Unknown type of synapse " + name);
	}
}

void ToJson::create_conn_from_json(const Json &con_json, Network &netw)
{
	auto pops = netw.populations();

	auto syn = get_synapse(con_json["syn_name"].get<std::string>(),
	                       con_json["params"].get<std::vector<Real>>());
	std::unique_ptr<Connector> connector;

	if (con_json.find("connections") != con_json.end()) {
		// List connections
		const Json &jconn = con_json["connections"];
		std::vector<LocalConnection> conns(jconn.size());
		for (size_t i = 0; i < conns.size(); i++) {
			syn->parameters(jconn[i]["par"].get<std::vector<Real>>());
			conns[i] =
			    LocalConnection(jconn[i]["src"].get<NeuronIndex>(),
			                    jconn[i]["tar"].get<NeuronIndex>(), *syn);
		}
		connector = Connector::from_list(conns);
	}
	else if (con_json["conn_name"] == "AllToAllConnector") {
		connector = Connector::all_to_all(
		    *syn, con_json["allow_self_connections"].get<bool>());
	}
	else if (con_json["conn_name"] == "OneToOneConnector") {
		connector = Connector::one_to_one(*syn);
	}
	else if (con_json["conn_name"] == "RandomConnector") {
		connector = Connector::random(
		    *syn, con_json["additional_parameter"].get<Real>(),
		    con_json["allow_self_connections"].get<bool>());
	}
	else if (con_json["conn_name"] == "FixedFanInConnector") {
		connector = Connector::fixed_fan_in(
		    con_json["additional_parameter"].get<Real>(), *syn,
		    con_json["allow_self_connections"].get<bool>());
	}
	else if (con_json["conn_name"] == "FixedFanOutConnector") {
		connector = Connector::fixed_fan_out(
		    con_json["additional_parameter"].get<Real>(), *syn,
		    con_json["allow_self_connections"].get<bool>());
	}
	else {
		throw CypressException("Unknown type of Connection: " +
		                       con_json["conn_name"].get<std::string>());
	}
	auto src = pops[con_json["pid_src"].get<PopulationIndex>()].range(
	    con_json["nid_src0"].get<NeuronIndex>(),
	    con_json["nid_src1"].get<NeuronIndex>());
	auto tar = pops[con_json["pid_tar"].get<PopulationIndex>()].range(
	    con_json["nid_tar0"].get<NeuronIndex>(),
	    con_json["nid_tar1"].get<NeuronIndex>());
	netw.add_connection(src, tar, std::move(connector));
}

NetworkBase ToJson::network_from_json(std::string path)
{
	std::ifstream ifs(std::ifstream(path, std::ios::binary));
	Json json = Json::from_bson(ifs);

	Network netw;
	const auto &pops = json["populations"];
	for (auto i : pops) {
		create_pop_from_json(i, netw);
	}

	const auto &conns = json["connections"];
	for (auto i : conns) {
		create_conn_from_json(i, netw);
	}
	return netw;
}

Json ToJson::pop_to_json(const PopulationBase &pop)
{
	Json res({{"type", NeuronTypesMap.find(&pop.type())->second},
	          {"size", pop.size()},
	          {"records", {}}});
	if (pop.homogeneous_parameters()) {
		res["parameters"] = pop.parameters().parameters();
	}
	else {
		std::cerr << "TODO inhom. params" << std::endl;
		throw;  // TODO
	}
	return res;
}

void ToJson::hom_rec_to_json(const PopulationBase &pop, Json &json)
{
	const std::vector<std::string> &signals = pop.type().signal_names;
	for (size_t i = 0; i < signals.size(); i++) {
		if (pop.signals().is_recording(i)) {
			json["records"].emplace_back(signals[i]);
		}
	}
}

Json ToJson::pop_vec_to_json(const std::vector<PopulationBase> &pops)
{
	Json res;
	for (auto pop : pops) {
		res.emplace_back(pop_to_json(pop));
		if (pop.homogeneous_record()) {
			hom_rec_to_json(pop, res.back());
		}
		else {
			res.back()["records"] = inhom_rec_to_json(pop);
		}
	}
	return res;
}

Json ToJson::recs_to_json(const PopulationBase &pop)
{
	Json res;
	std::vector<std::string> signals = pop.type().signal_names;
	for (size_t j = 0; j < signals.size(); j++) {
		bool is_recording = false;
		for (auto neuron : pop) {
			if (neuron.signals().is_recording(j)) {
				is_recording = true;
				break;
			}
		}
		if (is_recording) {
			res.emplace_back(Json({{"pop_id", pop.pid()}, {"signal", j}}));
			for (auto neuron : pop) {
				if (neuron.signals().is_recording(j)) {
					res.back()["data"].push_back(neuron.signals().data(j));
					res.back()["ids"].push_back(neuron.nid());
				}
			}
		}
	}
	return res;
}

void to_json(Json &result, const Network &network)
{
	const std::vector<PopulationBase> &populations = network.populations();
	result["populations"] = ToJson::pop_vec_to_json(populations);

	for (size_t i = 0; i < network.connections().size(); i++) {
		result["connections"].emplace_back(
		    ToJson::connector_to_json(network.connections()[i]));
	}
	for (size_t i = 0; i < populations.size(); i++) {
		if (populations[i].size() == 0) {
			continue;
		}
		result["recordings"].emplace_back(ToJson::recs_to_json(populations[i]));
	}

	// Runtime TODO

	// Learned Weights TODO
}

void from_json(const Json &json, Network &netw)
{
	const auto &pops = json["populations"];
	for (auto &i : pops) {
		ToJson::create_pop_from_json(i, netw);
	}

	const auto &conns = json["connections"];
	for (auto &i : conns) {
		ToJson::create_conn_from_json(i, netw);
	}
	const auto &recs = json["recordings"];
	for (size_t i = 0; i < recs.size(); i++) {
		for (size_t j = 0; j < recs[i].size(); j++) {
			ToJson::read_recordings_from_json(recs[i][j], netw);
		}
	}

	// Runtime //TODO
}

}  // namespace cypress
