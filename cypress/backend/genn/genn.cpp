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

#include <../genn/include/genn/backends/cuda/backend.h>
#include <../genn/include/genn/backends/cuda/optimiser.h>
#include <../genn/include/genn/backends/single_threaded_cpu/backend.h>
#include <../genn/include/genn/genn/code_generator/generateAll.h>
#include <../genn/include/genn/genn/code_generator/generateMakefile.h>
#include <../genn/include/genn/genn/modelSpecInternal.h>  // TODO
#include <../genn/include/genn/third_party/path.h>
#include <../genn/pygenn/genn_wrapper/include/SharedLibraryModel.h>

#include <fstream>

#include <cypress/backend/genn/genn.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/util/logger.hpp>

#include <cypress/core/network_base_objects.hpp>

#define UNPACK7(v) v[0], v[1], v[2], v[3], v[4], v[5], v[6]

// Adapted from GeNN repository
class FixedNumberPostWithReplacementNoAutapse
    : public InitSparseConnectivitySnippet::Base {
public:
	DECLARE_SNIPPET(FixedNumberPostWithReplacementNoAutapse, 1);

	SET_ROW_BUILD_CODE(
	    "const scalar u = $(gennrand_uniform);\n"
	    "x += (1.0 - x) * (1.0 - pow(u, 1.0 / (scalar)($(rowLength) - c)));\n"
	    "const unsigned int postIdx = (unsigned int)(x * $(num_post));\n"
	    "if(postIdx != $(id_pre)){\n"
	    "if(postIdx < $(num_post)) {\n"
	    "   $(addSynapse, postIdx);\n"
	    "}\n"
	    "else {\n"
	    "   $(addSynapse, $(num_post) - 1);\n"
	    "}\n"
	    "c++;\n"
	    "if(c >= $(rowLength)) {\n"
	    "   $(endRow);\n"
	    "}}\n");
	SET_ROW_BUILD_STATE_VARS({{"x", "scalar", 0.0}, {"c", "unsigned int", 0}});

	SET_PARAM_NAMES({"rowLength"});

	SET_CALC_MAX_ROW_LENGTH_FUNC([](unsigned int, unsigned int,
	                                const std::vector<double> &pars) {
		return (unsigned int)pars[0];
	});
};
IMPLEMENT_SNIPPET(FixedNumberPostWithReplacementNoAutapse);

// Adapted from GeNN repository
class FixedNumberPostWithReplacement
    : public InitSparseConnectivitySnippet::Base {
public:
	DECLARE_SNIPPET(FixedNumberPostWithReplacement, 1);

	SET_ROW_BUILD_CODE(
	    "const scalar u = $(gennrand_uniform);\n"
	    "x += (1.0 - x) * (1.0 - pow(u, 1.0 / (scalar)($(rowLength) - c)));\n"
	    "const unsigned int postIdx = (unsigned int)(x * $(num_post));\n"
	    "if(postIdx < $(num_post)) {\n"
	    "   $(addSynapse, postIdx);\n"
	    "}\n"
	    "else {\n"
	    "   $(addSynapse, $(num_post) - 1);\n"
	    "}\n"
	    "c++;\n"
	    "if(c >= $(rowLength)) {\n"
	    "   $(endRow);\n"
	    "}\n");
	SET_ROW_BUILD_STATE_VARS({{"x", "scalar", 0.0}, {"c", "unsigned int", 0}});

	SET_PARAM_NAMES({"rowLength"});

	SET_CALC_MAX_ROW_LENGTH_FUNC([](unsigned int, unsigned int,
	                                const std::vector<double> &pars) {
		return (unsigned int)pars[0];
	});
};
IMPLEMENT_SNIPPET(FixedNumberPostWithReplacement);

namespace cypress {
GeNN::GeNN(const Json &setup)
{
	if (setup.count("timestep") > 0) {
		m_timestep = setup["timestep"].get<Real>();
	}
	if (setup.count("gpu") > 0) {
		m_gpu = setup["gpu"].get<bool>();
	}
	// TODO
}
std::unordered_set<const NeuronType *> GeNN::supported_neuron_types() const
{
	return std::unordered_set<const NeuronType *>{
	    &SpikeSourceArray::inst(), &IfCondExp::inst(), &IfCurrExp::inst()};
}

// Key: parameter index in genn; value: parameter index in cypress
static const std::map<size_t, size_t> ind_condlif(
    {{0, 0}, {1, 1}, {2, 5}, {3, 7}, {4, 6}, {5, 10}, {6, 4}});
static const std::map<size_t, size_t> ind_currlif(
    {{0, 0}, {1, 1}, {2, 5}, {3, 7}, {4, 6}, {5, 8}, {6, 4}});

inline std::vector<double> map_params_to_genn(
    const NeuronParameters &params_src, const std::map<size_t, size_t> &map)
{
	std::vector<double> params(map.size(), 0);
	for (size_t i = 0; i < map.size(); i++) {
		params[i] = params_src[map.at(i)];
	}
	return params;
}

bool check_param_for_homogeneity(const PopulationBase &pop, size_t param_id)
{
	auto param = pop[0].parameters()[param_id];
	for (auto neuron : pop) {
		if (neuron.parameters()[param_id] != param) {
			return false;
		}
	}
	return true;
}

std::vector<size_t> inhomogeneous_indices(const PopulationBase &pop,
                                          const std::map<size_t, size_t> &map)
{
	std::vector<size_t> indices;
	for (size_t i = 0; i < map.size(); i++) {
		if (!check_param_for_homogeneity(pop, map.at(i))) {
			indices.emplace_back(i);
		}
	}
	return indices;
}

NeuronGroup *create_neuron_group(const PopulationBase &pop,
                                 ModelSpecInternal &model, std::string name)
{
	if (pop.homogeneous_parameters()) {
		if (&pop.type() == &IfCondExp::inst()) {
			auto temp = map_params_to_genn(pop.parameters(), ind_condlif);
			NeuronModels::LIF::ParamValues params(UNPACK7(temp));
			NeuronModels::LIF::VarValues inits(pop.parameters()[5], 0.0);
			return model.addNeuronPopulation<NeuronModels::LIF>(
			    name, pop.size(), params, inits);
		}
		else if (&pop.type() == &IfCurrExp::inst()) {
			auto temp = map_params_to_genn(pop.parameters(), ind_currlif);
			NeuronModels::LIF::ParamValues params(UNPACK7(temp));
			NeuronModels::LIF::VarValues inits(pop.parameters()[5], 0.0);
			return model.addNeuronPopulation<NeuronModels::LIF>(
			    name, pop.size(), params, inits);
		}
		else if (&pop.type() == &SpikeSourceArray::inst()) {
			NeuronModels::SpikeSourceArray::ParamValues params;
			NeuronModels::SpikeSourceArray::VarValues inits(
			    0, pop.parameters().parameters().size());
			return model.addNeuronPopulation<NeuronModels::SpikeSourceArray>(
			    name, pop.size(), params, inits);
		}
		throw ExecutionError(
		    "Unexpected Neuron Type for the simulator GeNN! The neuron model "
		    "is not supported!");  // It should never reach this error message
	}
	else {
		if (&pop.type() == &SpikeSourceArray::inst()) {
			NeuronModels::SpikeSourceArray::ParamValues params;
			NeuronModels::SpikeSourceArray::VarValues inits(uninitialisedVar(),
			                                                uninitialisedVar());
			return model.addNeuronPopulation<NeuronModels::SpikeSourceArray>(
			    name, pop.size(), params, inits);
		}
		throw ExecutionError(
		    "Currently inhomogenous parameters are not supported!");
	}
}

InitSparseConnectivitySnippet::Init genn_connector(
    std::string name, SynapseMatrixType &type,
    const bool allow_self_connections, const Real additional_parameter,
    const bool full_pops, bool &list_connected)
{
	list_connected = false;
	if (full_pops) {
		type = SynapseMatrixType::BITMASK_GLOBALG;
		if (name == "AllToAllConnector") {
			if (allow_self_connections) {
				type = SynapseMatrixType::DENSE_GLOBALG;
				return initConnectivity<
				    InitSparseConnectivitySnippet::Uninitialised>();
			}
			else {
				return initConnectivity<
				    InitSparseConnectivitySnippet::FixedProbabilityNoAutapse>(
				    1.0);
			}
		}
		else if (name == "OneToOneConnector") {
			type = SynapseMatrixType::SPARSE_GLOBALG;
			return initConnectivity<InitSparseConnectivitySnippet::OneToOne>();
		}
		/*else if (conn.connector().name() == "FixedFanInConnector") {
		    *connector =
		        initConnectivity<InitSparseConnectivitySnippet::OneToOne>();
		}
		*/
		else if (name == "FixedFanOutConnector") {
			if (allow_self_connections) {
				return initConnectivity<FixedNumberPostWithReplacement>(
				    (unsigned int)additional_parameter);
			}
			else {

				return initConnectivity<
				    FixedNumberPostWithReplacementNoAutapse>(
				    (unsigned int)additional_parameter);
			}
		}
		else if (name == "RandomConnector") {
			if (additional_parameter == 1.0 && allow_self_connections) {
				type = SynapseMatrixType::DENSE_GLOBALG;
				return initConnectivity<
				    InitSparseConnectivitySnippet::Uninitialised>();
			}
			if (allow_self_connections) {
				return initConnectivity<
				    InitSparseConnectivitySnippet::FixedProbability>(
				    additional_parameter);
			}
			else {
				return initConnectivity<
				    InitSparseConnectivitySnippet::FixedProbabilityNoAutapse>(
				    additional_parameter);
			}
		}
	}
	// default case
	list_connected = true;
	type = SynapseMatrixType::DENSE_INDIVIDUALG;
	return initConnectivity<InitSparseConnectivitySnippet::Uninitialised>();
}

SynapseGroup *connect(const std::vector<PopulationBase> pops,
                      ModelSpecInternal &model,
                      const ConnectionDescriptor &conn, std::string name,
                      Real timestep, bool &list_connect)
{
	// pops[conn.pid_tar()].homogeneous_parameters();
	// TODO : inhomogenous tau_syn and E_rev, popviews

	bool cond_based = pops[conn.pid_tar()].type().conductance_based;
	Real weight = conn.connector().synapse()->parameters()[0];
	unsigned int delay =
	    (unsigned int)(conn.connector().synapse()->parameters()[1] / timestep);
	bool full_pops = (conn.nid_src0() == 0) &&
	                 (size_t(conn.nid_src1()) == pops[conn.pid_src()].size()) &&
	                 (conn.nid_tar0() == 0) &&
	                 (size_t(conn.nid_tar1()) == pops[conn.pid_tar()].size());
	bool allow_self_connections = conn.connector().allow_self_connections() ||
	                              !(conn.pid_src() == conn.pid_tar());
	SynapseMatrixType type;

	auto connector = genn_connector(
	    conn.connector().name(), type, allow_self_connections,
	    conn.connector().additional_parameter(), full_pops, list_connect);
	if (list_connect) {
		return nullptr;
	}

	if (cond_based) {
		if (!conn.connector().synapse()->learning()) {
			double rev_pot;
			double tau;
			if (weight >= 0) {  // TODO : adapt for EifCondExpIsfaIsta
				rev_pot = pops[conn.pid_tar()].parameters().parameters()[8];
				tau = pops[conn.pid_tar()].parameters().parameters()[2];
			}
			else {
				rev_pot = pops[conn.pid_tar()].parameters().parameters()[9];
				tau = pops[conn.pid_tar()].parameters().parameters()[3];
				weight = -weight;
			}

			return model.addSynapsePopulation<WeightUpdateModels::StaticPulse,
			                                  PostsynapticModels::ExpCond>(
			    name, type, delay, "pop_" + std::to_string(conn.pid_src()),
			    "pop_" + std::to_string(conn.pid_tar()), {}, {weight},
			    {tau, rev_pot}, {}, connector);
		}
	}
	else {
		if (!conn.connector().synapse()->learning()) {
			double tau;
			if (weight >= 0) {
				tau = pops[conn.pid_tar()].parameters().parameters()[2];
			}
			else {
				tau = pops[conn.pid_tar()].parameters().parameters()[3];
			}

			return model.addSynapsePopulation<WeightUpdateModels::StaticPulse,
			                                  PostsynapticModels::ExpCurr>(
			    name, type, delay, "pop_" + std::to_string(conn.pid_src()),
			    "pop_" + std::to_string(conn.pid_tar()), {}, {weight}, {tau},
			    {}, connector);
		}
	}
	throw ExecutionError("Learning synapses are currently not supported");
}

std::tuple<SynapseGroup *, SynapseGroup *,
           std::shared_ptr<std::vector<LocalConnection>>>
list_connect_pre(const std::vector<PopulationBase> pops,
                 ModelSpecInternal &model, const ConnectionDescriptor &conn,
                 std::string name, Real timestep)
{

	if (conn.connector().name() == "FromListConnector" ||
	    conn.connector().name() == "FunctorConnector") {
		global_logger().info(
		    "GeNN",
		    "List connections only allow one delay per connector! We will use "
		    "the delay provided by the default synapse object!");
	}
	auto conns_full = std::make_shared<std::vector<LocalConnection>>();
	conn.connect(*conns_full);

	size_t num_inh = 0;
	for (auto &i : *conns_full) {
		if (i.inhibitory()) {
			num_inh++;
		}
	}

	bool cond_based = pops[conn.pid_tar()].type().conductance_based;

	unsigned int delay =
	    (unsigned int)(conn.connector().synapse()->parameters()[1] / timestep);
	SynapseGroup *synex = nullptr, *synin = nullptr;

	if (conns_full->size() - num_inh > 0) {
		if (cond_based) {
			double rev_pot = pops[conn.pid_tar()].parameters().parameters()[8];
			double tau = pops[conn.pid_tar()].parameters().parameters()[2];
			synex = model.addSynapsePopulation<WeightUpdateModels::StaticPulse,
			                                   PostsynapticModels::ExpCond>(
			    name + "_ex", SynapseMatrixType::DENSE_INDIVIDUALG, delay,
			    "pop_" + std::to_string(conn.pid_src()),
			    "pop_" + std::to_string(conn.pid_tar()), {}, {0.0},
			    {tau, rev_pot}, {});
		}
		else {
			double tau = pops[conn.pid_tar()].parameters().parameters()[2];
			synex = model.addSynapsePopulation<WeightUpdateModels::StaticPulse,
			                                   PostsynapticModels::ExpCurr>(
			    name + "_ex", SynapseMatrixType::DENSE_INDIVIDUALG, delay,
			    "pop_" + std::to_string(conn.pid_src()),
			    "pop_" + std::to_string(conn.pid_tar()), {}, {0.0}, {tau}, {});
		}
	}

	// inhibitory
	if (num_inh > 0) {
		if (cond_based) {
			double rev_pot = pops[conn.pid_tar()].parameters().parameters()[9];
			double tau = pops[conn.pid_tar()].parameters().parameters()[3];
			synin = model.addSynapsePopulation<WeightUpdateModels::StaticPulse,
			                                   PostsynapticModels::ExpCond>(
			    name + "_in", SynapseMatrixType::DENSE_INDIVIDUALG, delay,
			    "pop_" + std::to_string(conn.pid_src()),
			    "pop_" + std::to_string(conn.pid_tar()), {}, {0.0},
			    {tau, rev_pot}, {});
		}
		else {
			double tau = pops[conn.pid_tar()].parameters().parameters()[3];
			synin = model.addSynapsePopulation<WeightUpdateModels::StaticPulse,
			                                   PostsynapticModels::ExpCurr>(
			    name + "_in", SynapseMatrixType::DENSE_INDIVIDUALG, delay,
			    "pop_" + std::to_string(conn.pid_src()),
			    "pop_" + std::to_string(conn.pid_tar()), {}, {0.0}, {tau}, {});
		}
	}
	return std::make_tuple(synex, synin, std::move(conns_full));
}

typedef void (*PushFunction)(bool);
void list_connect_post(
    std::string name,
    std::tuple<SynapseGroup *, SynapseGroup *,
               std::shared_ptr<std::vector<LocalConnection>>> &tuple,
    SharedLibraryModel<float> &slm, size_t size_tar, bool cond_based)
{
	float *weights_in, *weights_ex;
	if (std::get<0>(tuple)) {
		weights_ex =
		    *(static_cast<float **>(slm.getSymbol("g" + name + "_ex")));
	}

	if (std::get<1>(tuple)) {
		weights_in =
		    *(static_cast<float **>(slm.getSymbol("g" + name + "_in")));
	}

	for (auto &i : *(std::get<2>(tuple))) {
		if (i.inhibitory()) {
			weights_in[size_tar * i.src + i.tar] =
			    cond_based ? -float(i.SynapseParameters[0])
			               : float(i.SynapseParameters[0]);
		}
		else {
			weights_ex[size_tar * i.src + i.tar] =
			    float(i.SynapseParameters[0]);
		}
	}

	if (std::get<0>(tuple)) {
		((PushFunction)slm.getSymbol("pushg" + name + "_exToDevice"))(false);
	}
	if (std::get<1>(tuple)) {
		((PushFunction)slm.getSymbol("pushg" + name + "_inToDevice"))(false);
	}
	std::get<2>(tuple)->clear();
}

void GeNN::do_run(NetworkBase &network, Real duration) const
{
	// General Setup
	ModelSpecInternal model;
	model.setPrecision(FloatType::GENN_FLOAT);  // TODO: template to use double
	model.setTimePrecision(TimePrecision::DEFAULT);
	model.setDT(m_timestep);  // Timestep in ms
	model.setMergePostsynapticModels(true);
	model.setName("cypressnet");  // TODO random net

	// Create Populations
	auto &populations = network.populations();
	std::vector<NeuronGroup *> neuron_groups;
	for (size_t i = 0; i < populations.size(); i++) {
		neuron_groups.emplace_back(create_neuron_group(
		    populations[i], model, ("pop_" + std::to_string(i))));
	}

	// Create connections
	std::vector<SynapseGroup *> synapse_groups;
	std::vector<std::tuple<SynapseGroup *, SynapseGroup *,
	                       std::shared_ptr<std::vector<LocalConnection>>>>
	    synapse_groups_list;
	std::vector<size_t> list_connected_ids;
	auto &connections = network.connections();
	for (size_t i = 0; i < connections.size(); i++) {
		bool list_connected;
		synapse_groups.emplace_back(connect(populations, model, connections[i],
		                                    "conn_" + std::to_string(i),
		                                    m_timestep, list_connected));
		if (list_connected) {
			synapse_groups_list.emplace_back(
			    list_connect_pre(populations, model, connections[i],
			                     "conn_" + std::to_string(i), m_timestep));
			list_connected_ids.emplace_back(i);
		}
	}

	// Build the model and compile
	model.finalize();
	std::string path = "./" + model.getName() + "_CODE/";
	mkdir(path.c_str(), 0777);
	if (m_gpu) {
		CodeGenerator::CUDA::Preferences prefs;
#ifndef NDEBUG
		prefs.debugCode = true;
#else
		prefs.optimizeCode = true;
#endif
		auto bck = CodeGenerator::CUDA::Optimiser::createBackend(
		    model, filesystem::path("test_netw/"), 0, prefs);
		auto moduleNames = CodeGenerator::generateAll(model, bck, path, false);
		std::ofstream makefile(path + "Makefile");
		CodeGenerator::generateMakefile(makefile, bck, moduleNames);
		makefile.close();
	}
	else {
		CodeGenerator::SingleThreadedCPU::Preferences prefs;
#ifndef NDEBUG
		prefs.debugCode = true;
#else
		prefs.optimizeCode = true;
#endif
		CodeGenerator::SingleThreadedCPU::Backend bck(0, model.getPrecision(),
		                                              prefs);

		auto moduleNames = CodeGenerator::generateAll(model, bck, path, false);
		std::ofstream makefile(path + "Makefile");
		CodeGenerator::generateMakefile(makefile, bck, moduleNames);
		makefile.close();
	}
	system(("make -j 4 -C " + path + " &>/dev/null").c_str());

	//     // Open the compiled Library as dynamically loaded library
	auto slm = SharedLibraryModel<float>();
	bool open = slm.open("./", "cypressnet");
	if (!open) {
		throw ExecutionError(
		    "Could not open Shared Library Model of GeNN network. Make sure "
		    "that code generation and make were successful!");
	}
	slm.allocateMem();
	slm.initialize();

	auto *T = static_cast<float *>(slm.getSymbol("t"));
	// auto *iT = static_cast<unsigned long long *>(slm.getSymbol("iT"));

	// Set up spike times of spike source arrays
	for (size_t i = 0; i < populations.size(); i++) {
		const auto &pop = populations[i];
		if (&pop.type() == &SpikeSourceArray::inst()) {
			if (pop.homogeneous_parameters()) {
				auto &spike_times = pop.parameters().parameters();
				slm.allocateExtraGlobalParam("pop_" + std::to_string(i),
				                             "spikeTimes", spike_times.size());
				auto **spikes_ptr = (static_cast<float **>(
				    slm.getSymbol("spikeTimespop_" + std::to_string(i))));
				for (size_t spike = 0; spike < spike_times.size(); spike++) {
					(*spikes_ptr)[spike] = float(spike_times[spike]);
				}
				slm.pushExtraGlobalParam("pop_" + std::to_string(i),
				                         "spikeTimes", spike_times.size());
			}
			else {
				size_t size = 0;
				for (size_t neuron = 0; neuron < pop.size(); neuron++) {
					size += pop[neuron].parameters().parameters().size();
				}
				slm.allocateExtraGlobalParam("pop_" + std::to_string(i),
				                             "spikeTimes", size);
				size = 0;
				auto *spikes_ptr = *(static_cast<float **>(
				    slm.getSymbol("spikeTimespop_" + std::to_string(i))));
				for (size_t neuron = 0; neuron < pop.size(); neuron++) {
					const auto &spike_times =
					    pop[neuron].parameters().parameters();
					for (size_t spike = 0; spike < spike_times.size();
					     spike++) {
						spikes_ptr[size + spike] = float(spike_times[spike]);
					}

					auto *spikes_start = *(static_cast<unsigned int **>(
					    slm.getSymbol("startSpikepop_" + std::to_string(i))));
					spikes_start[neuron] = size;

					size += spike_times.size();
					auto *spikes_end = *(static_cast<unsigned int **>(
					    slm.getSymbol("endSpikepop_" + std::to_string(i))));
					spikes_end[neuron] = size;
				}
				slm.pushExtraGlobalParam("pop_" + std::to_string(i),
				                         "spikeTimes", size);
			}
		}
	}

	// List Connections
	for (size_t i = 0; i < list_connected_ids.size(); i++) {
		size_t id = list_connected_ids[i];
		bool cond_based =
		    populations[connections[id].pid_tar()].type().conductance_based;
		list_connect_post("conn_" + std::to_string(id), synapse_groups_list[i],
		                  slm, populations[connections[id].pid_tar()].size(),
		                  cond_based);
	}

	slm.initializeSparse();

	// Get Pointers to observables
	std::vector<unsigned int *> spike_ptrs;
	std::vector<unsigned int *> spike_cnt_ptrs;
	std::vector<float *> v_ptrs;
	std::vector<bool> record_full;

	for (size_t i = 0; i < populations.size(); i++) {
		const auto &pop = populations[i];
		if (pop.homogeneous_record()) {
			record_full.emplace_back(true);
			if (pop.signals().is_recording(0)) {
				spike_cnt_ptrs.emplace_back((*(static_cast<unsigned int **>(
				    slm.getSymbol("glbSpkCntpop_" + std::to_string(i))))));
				spike_ptrs.emplace_back((*(static_cast<unsigned int **>(
				    slm.getSymbol("glbSpkpop_" + std::to_string(i))))));
			}
			else {
				spike_ptrs.emplace_back(nullptr);
			}
			if (pop.signals().size() > 1 && pop.signals().is_recording(1)) {
				v_ptrs.emplace_back(*(static_cast<float **>(
				    slm.getSymbol("Vpop_" + std::to_string(i)))));
			}
			else {
				v_ptrs.emplace_back(nullptr);
			}
		}
		else {
			record_full.emplace_back(false);
			// TODO
			throw;
		}
	}

	std::vector<std::vector<std::vector<Real>>> spike_data(
	    populations.size(), std::vector<std::vector<Real>>());

	for (size_t i = 0; i < spike_ptrs.size(); i++) {

		spike_data[i] = std::vector<std::vector<Real>>(populations[i].size(),
		                                               std::vector<Real>());
	}

	// Run the simulation and pull all recorded variables
	while (*T < duration) {
		slm.stepTime();
		for (size_t i = 0; i < spike_ptrs.size(); i++) {
			if (record_full[i]) {
				if (spike_ptrs[i] != nullptr) {
					slm.pullSpikesFromDevice("pop_" + std::to_string(i));

					for (unsigned int id = 0; id < *(spike_cnt_ptrs[i]); id++) {
						spike_data[i][(spike_ptrs[i])[id]].push_back(Real(*T));
					}
				}
			}
		}

		/*slm.pullSpikesFromDevice("a");
		slm.pullSpikesFromDevice("b");
		slm.pullVarFromDevice("a", "V");*/
		/*for (size_t i = 0; i < 10; i++) {
		    std::cout << ", V: " << *(vptr + i);
		}
		std::cout << std::endl;*/
	}
	for (size_t i = 0; i < populations.size(); i++) {
		auto &pop = populations[i];
		for (size_t neuron = 0; neuron < pop.size(); neuron++) {}
	}

	// Convert spike data
	for (size_t i = 0; i < populations.size(); i++) {
		auto &pop = populations[i];
		if (pop.homogeneous_record()) {
			record_full.emplace_back(true);
			if (pop.signals().is_recording(0)) {
				for (size_t neuron = 0; neuron < pop.size(); neuron++) {
					auto data = std::make_shared<Matrix<Real>>(
					    spike_data[i][neuron].size(), 1);
					for (size_t j = 0; j < spike_data[i][neuron].size(); j++) {
						(*data)(j, 0) = spike_data[i][neuron][j];
					}
					auto neuron_inst = pop[neuron];
					neuron_inst.signals().data(0, std::move(data));
				}
			}
		}
	}
}
}  // namespace cypress
