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
#include <cstdlib>
#include <cypress/backend/power/energenie.hpp>
#include <cypress/backend/resources.hpp>
#include <cypress/backend/serialize/to_json.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/network.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/core/network_base_objects.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/util/filesystem.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/util/process.hpp>
#include <exception>
#include <sstream>
#include <system_error>
#include <thread>

namespace cypress {
namespace {
class exec_json_path {
private:
	std::string m_path;
	exec_json_path()
	{
		m_path = "./cypress_from_json";  // local
		auto ret = execlp((m_path + " >/dev/null 2>&1").c_str(), "");
		if (ret) {
			global_logger().debug(
			    "cypress",
			    "Installed json executable will be used before "
			    "subproject executable");
			m_path = "cypress_from_json";  // install
			ret = std::system((m_path + " >/dev/null 2>&1").c_str());
			if (ret) {
				m_path = JSON_EXEC_PATH;  // subproject
				ret = std::system((m_path + " >/dev/null 2>&1").c_str());
				if (ret) {
					throw std::runtime_error(
					    "Could not open json executable. Make sure that the "
					    "binary "
					    "is either in local path or installed in PATH "
					    "directory");
				}
			}
		}
		global_logger().debug("cypress", "Use json exec path " + m_path);
	};

	~exec_json_path() = default;

public:
	exec_json_path(exec_json_path const &) = delete;
	void operator=(exec_json_path const &) = delete;
	static exec_json_path &instance()
	{
		static exec_json_path instance;
		return instance;
	}

	std::string path() const { return m_path; }
};
}  // namespace

ToJson::ToJson(const std::string &simulator, const Json &setup)
    : m_simulator(simulator), m_setup(setup)
{
	if (m_setup.find("save_json") != m_setup.end()) {
		m_save_json = m_setup["save_json"].get<bool>();
		m_setup.erase(m_setup.find("save_json"));
	}
	if (m_setup.find("no_output") != m_setup.end()) {
		m_no_output = m_setup["no_output"].get<bool>();
		m_setup.erase(m_setup.find("no_output"));
	}
	m_path = "experiment_XXXXX";
	filesystem::tmpfile(m_path);

	m_json_path = exec_json_path::instance().path();
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
	res["label"] = conn.label();

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
		std::vector<LocalConnection> tar;
		connector.connect(conn, tar);
		for (auto i : tar) {
			Json json = {};
			json.emplace_back(i.src);
			json.emplace_back(i.tar);
			for (auto j : i.SynapseParameters) {
				json.emplace_back(j);
			}
			res["connections"].emplace_back(json);
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

namespace {
/**
 * Helper function to open a fifo for reading (the writing process has to open
 * the fifo first. Otherwise @res will be directly closed again
 * @param file_name name of the fifo to open
 * @param res reference to a filebuffer which will point to fifo
 */
void open_fifo_to_read(std::string file_name, std::filebuf &res)
{
	while (true) {
		if (res.open(file_name, std::ios::in)) {
			break;
		}
		usleep(3000);
	}
}

/**
 * Helper function to write network description into a file
 */
void pipe_write_helper(std::string file, Json &source)
{
	std::filebuf fb_in;
	fb_in.open(file, std::ios::out);
	std::ostream data_in(&fb_in);
	if (!data_in.good()) {
		throw ExecutionError("Error in pipe to json_exec");
	}
	// Send the network description to the simulator
	data_in << source;
	fb_in.close();
	source = {};
}
}  // namespace

Json ToJson::output_json(NetworkBase &network, Real duration) const
{
	Json json_out;
	json_out["simulator"] = m_simulator;
	json_out["setup"] = m_setup;
	json_out["duration"] = duration;
	json_out["network"] = network;
	json_out["log_level"] = global_logger().min_level();

	return json_out;
}
void ToJson::read_json(Json &result, NetworkBase &network) const
{
	if (result.find("exception") != result.end()) {
		throw CypressException("Json child threw error: " +
		                       result["exception"].get<std::string>());
	}

	const auto &recs = result["recordings"];
	for (size_t i = 0; i < recs.size(); i++) {
		for (size_t j = 0; j < recs[i].size(); j++) {
			ToJson::read_recordings_from_json(recs[i][j], network);
		}
	}
	learned_weights_from_json(result, network);
	network.runtime(result["runtime"].get<NetworkRuntime>());
}

namespace {
std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string elem;
	while (getline(ss, elem, delim)) {
		elems.push_back(elem);
	}
	return elems;
}
}  // namespace
void ToJson::do_run(NetworkBase &network, Real duration) const
{
	std::shared_ptr<energenie> powermngmt;
	bool mng = false;
	std::string sim = split(m_simulator, '=')[0];
	if (sim == "spikey" || sim == "spinnaker") {
		powermngmt = std::make_shared<energenie>();
		if (powermngmt->connected()) {
			mng = true;
		}
		if (sim == "spinnaker")
			sim = "nmmc1";
	}
	for (size_t trial = 0; trial < 3; trial++) {
		auto json_out = output_json(network, duration);
		std::thread data_in(pipe_write_helper, m_path + ".json",
		                    std::ref(json_out));
		if (!m_save_json) {
			if (mkfifo((m_path + ".json").c_str(), 0666) != 0) {
				throw std::system_error(errno, std::system_category());
			}
			if (mkfifo((m_path + "_res.json").c_str(), 0666) != 0) {
				throw std::system_error(errno, std::system_category());
			}
		}

		if (mng && !powermngmt->state(sim) && powermngmt->switch_on(sim)) {
			sleep(3);  // Sleep for three seconds
		}

		Process proc(exec_json_path::instance().path(), {m_path});
		std::thread log_thread_beg, err_thread;
		std::ofstream null;
		if (m_no_output) {
			log_thread_beg =
			    std::thread(Process::generic_pipe,
			                std::ref(proc.child_stdout()), std::ref(null));
			err_thread =
			    std::thread(Process::generic_pipe,
			                std::ref(proc.child_stderr()), std::ref(null));
		}
		else {
			log_thread_beg =
			    std::thread(Process::generic_pipe,
			                std::ref(proc.child_stdout()), std::ref(std::cout));
			err_thread =
			    std::thread(Process::generic_pipe,
			                std::ref(proc.child_stderr()), std::ref(std::cerr));
		}
		Json result;
		if (!m_save_json) {
			std::filebuf fb_res;
			open_fifo_to_read(m_path + "_res.json", fb_res);
			std::istream res_fifo(&fb_res);
			result = Json::parse(res_fifo);
			fb_res.close();
		}
		data_in.join();
		err_thread.join();
		log_thread_beg.join();
		int res = proc.wait();
		if (mng && res < 0 && trial < 2) {
			if (powermngmt->switch_off(sim)) {
				global_logger().warn(
				    "PowerMngmt",
				    "Error while executing the simulation, going "
				    "to power-cycle the neuromorphic device and retry!");
				sleep(2);
				powermngmt->switch_on(sim);
				sleep(2);
				remove((m_path + "_res.json").c_str());
				remove((m_path + ".json").c_str());
				continue;
			}
		}
		if (res < 0) {
			throw ExecutionError(
			    std::string("Simulator child process killed by signal " +
			                std::to_string(-res)));
		}
		if (!m_save_json) {
			remove((m_path + "_res.json").c_str());
			remove((m_path + ".json").c_str());
		}
		else {
			std::ifstream file_in;
			file_in.open(m_path + "_res.json", std::ios::binary);
			result = Json::parse(file_in);
		}
		try {
			read_json(result, network);
		}
		catch (CypressException &e) {
			if (mng && trial < 2) {
				continue;
			}
			else {
				throw CypressException(e.what());
			}
		}
		return;
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
	PopulationIndex pop_ind;
	std::string name = pop_json["type"].get<std::string>();
	std::vector<Real> parameters;
	bool inhomogeneous = false;
	if (pop_json["parameters"][0].is_array()) {
		// inhomogeneous parameters
		if (pop_json["parameters"][0].size() > 0) {
			parameters = pop_json["parameters"][0].get<std::vector<Real>>();
		}
		inhomogeneous = true;
	}
	else {
		parameters = pop_json["parameters"].get<std::vector<Real>>();
	}
	if (name == "SpikeSourceArray") {
		pop_ind =
		    netw.create_population<SpikeSourceArray>(
		            pop_json["size"], SpikeSourceArrayParameters(parameters),
		            SpikeSourceArraySignals())
		        .pid();
	}
	else if (name == "IfCondExp") {
		pop_ind = netw.create_population<IfCondExp>(
		                  pop_json["size"], IfCondExpParameters(parameters),
		                  IfCondExpSignals())
		              .pid();
	}
	else if (name == "IfFacetsHardware1") {
		pop_ind =
		    netw.create_population<IfFacetsHardware1>(
		            pop_json["size"], IfFacetsHardware1Parameters(parameters))
		        .pid();
	}
	else if (name == "EifCondExpIsfaIsta") {
		pop_ind =
		    netw.create_population<EifCondExpIsfaIsta>(
		            pop_json["size"], EifCondExpIsfaIstaParameters(parameters))
		        .pid();
	}
	else if (name == "IfCurrExp") {
		pop_ind = netw.create_population<IfCurrExp>(
		                  pop_json["size"], IfCurrExpParameters(parameters))
		              .pid();
	}

	else if (name == "SpikeSourcePoisson") {
		pop_ind =
		    netw.create_population<SpikeSourcePoisson>(
		            pop_json["size"], SpikeSourcePoissonParameters(parameters))
		        .pid();
	}
	else if (name == "SpikeSourceConstFreq") {
		pop_ind = netw.create_population<SpikeSourceConstFreq>(
		                  pop_json["size"],
		                  SpikeSourceConstFreqParameters(parameters))
		              .pid();
	}
	else if (name == "SpikeSourceConstInterval") {
		pop_ind = netw.create_population<SpikeSourceConstInterval>(
		                  pop_json["size"],
		                  SpikeSourceConstIntervalParameters(parameters))
		              .pid();
	}
	else {
		throw CypressException("Unknown pop type " + name + "!");
	}

	PopulationBase pop = netw.population(pop_ind);
	pop.name(pop_json["label"].get<std::string>());
	if (inhomogeneous) {
		for (size_t i = 0; i < pop.size(); i++) {
			pop[i].parameters().parameters(
			    pop_json["parameters"][i].get<std::vector<Real>>());
		}
	}

	if (pop_json["records"] == Json()) {
		return;
	}
	if (pop_json["records"].is_object()) {
		for (auto &signal : pop_json["records"].items()) {
			auto index = pop.type().signal_index(signal.key());
			if (!index.valid()) {
				throw CypressException("Unknown signal type " + signal.key() +
				                       " for neuron type " + pop.type().name);
			}
			auto flags = signal.value().get<std::vector<bool>>();
			for (size_t i = 0; i < pop.size(); i++) {
				if (flags[i]) {
					pop[i].signals().record(index.value());
				}
			}
		}
	}
	else {
		for (auto &i : pop_json["records"]) {
			auto index = pop.type().signal_index(i.get<std::string>());
			if (!index.valid()) {
				throw CypressException("Unknown signal type " +
				                       i.get<std::string>() +
				                       " for neuron type " + pop.type().name);
			}
			pop.signals().record(index.value());
		}
	}
}

std::shared_ptr<SynapseBase> ToJson::get_synapse(
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
			std::vector<Real> tmp(jconn[i].size() - 2);
			for (size_t j = 2; j < jconn[i].size(); j++) {
				tmp[j - 2] = jconn[i][j].get<Real>();
			}
			syn->parameters(tmp);
			conns[i] = LocalConnection(jconn[i][0].get<NeuronIndex>(),
			                           jconn[i][1].get<NeuronIndex>(), *syn);
		}
		if (syn->learning()) {
			connector = Connector::from_list(conns, *syn);
		}
		else {
			connector = Connector::from_list(conns);
		}
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
	netw.add_connection(src, tar, std::move(connector),
	                    con_json["label"].get<std::string>().c_str());
}

NetworkBase ToJson::network_from_json(std::string path)
{
	std::ifstream ifs;
	ifs.open(path, std::ios::binary);
	Json json = Json::from_bson(ifs);
	ifs.close();

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
	          {"label", pop.name()},
	          {"records", {}}});
	if (pop.homogeneous_parameters()) {
		res["parameters"] = pop.parameters().parameters();
	}
	else {
		res["parameters"] = {};
		for (auto neuron : pop) {
			res["parameters"].push_back(neuron.parameters().parameters());
		}
	}
	return res;
}

void ToJson::hom_rec_to_json(const PopulationBase &pop, Json &json)
{
	json["records"] = Json();
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

void ToJson::learned_weights_from_json(const Json &json, NetworkBase netw)
{
	if (json.find("learned_weights") != json.end()) {
		for (size_t ind = 0; ind < json["learned_weights"]["id"].size();
		     ind++) {
			auto &connector =
			    netw.connections()[json["learned_weights"]["id"][ind]];
			if (json["learned_weights"].find("conns") ==
			    json["learned_weights"].end()) {
				continue;
			}
			auto &temp = json["learned_weights"]["conns"][ind];
			std::vector<LocalConnection> conns(temp.size());
			for (size_t conn = 0; conn < conns.size(); conn++) {
				conns[conn].src = temp[conn][0].get<NeuronIndex>();
				conns[conn].tar = temp[conn][1].get<NeuronIndex>();
				std::vector<Real> tmp(temp[conn].size() - 2);
				for (size_t i = 2; i < temp[conn].size(); i++) {
					tmp[i - 2] = temp[conn][i].get<Real>();
				}
				conns[conn].SynapseParameters = tmp;
			}
			connector.connector()._store_learned_weights(std::move(conns));
		}
	}
}

void from_json(const Json &json, NetworkRuntime &runtime)
{
	runtime.total = json["total"].get<Real>();
	runtime.sim = json["sim"].get<Real>();
	runtime.finalize = json["finalize"].get<Real>();
	runtime.initialize = json["initialize"].get<Real>();
}

void to_json(Json &json, const NetworkRuntime &runtime)
{
	json["total"] = runtime.total;
	json["sim"] = runtime.sim;
	json["finalize"] = runtime.finalize;
	json["initialize"] = runtime.initialize;
}

void to_json(Json &result, const Network &network)
{
	const std::vector<PopulationBase> &populations = network.populations();
	result["populations"] = ToJson::pop_vec_to_json(populations);

	for (size_t i = 0; i < network.connections().size(); i++) {
		result["connections"].emplace_back(
		    ToJson::connector_to_json(network.connections()[i]));
		if (network.connections()[i].connector().synapse()->learning()) {
			auto &conns =
			    network.connections()[i].connector().learned_weights();
			result["learned_weights"]["id"].emplace_back(i);
			Json tmp;
			for (auto conn : conns) {
				Json json = {};
				json.emplace_back(conn.src);
				json.emplace_back(conn.tar);
				for (auto i : conn.SynapseParameters) {
					json.emplace_back(i);
				}
				tmp.emplace_back(json);
			}
			result["learned_weights"]["conns"].emplace_back(tmp);
		}
	}
	for (size_t i = 0; i < populations.size(); i++) {
		if (populations[i].size() == 0) {
			continue;
		}
		result["recordings"].emplace_back(ToJson::recs_to_json(populations[i]));
	}

	result["runtime"] = network.runtime();
}

void from_json(const Json &json, Network &netw)
{
	const auto &pops = json["populations"];
	for (auto &i : pops) {
		ToJson::create_pop_from_json(i, netw);
	}

	if (json.find("connections") != json.end()) {
		const auto &conns = json["connections"];
		for (auto &i : conns) {
			ToJson::create_conn_from_json(i, netw);
		}
	}
	if (json.find("recordings") != json.end()) {
		const auto &recs = json["recordings"];
		for (size_t i = 0; i < recs.size(); i++) {
			for (size_t j = 0; j < recs[i].size(); j++) {
				ToJson::read_recordings_from_json(recs[i][j], netw);
			}
		}
	}

	netw.runtime(json["runtime"].get<NetworkRuntime>());
	ToJson::learned_weights_from_json(json, netw);
}

}  // namespace cypress
