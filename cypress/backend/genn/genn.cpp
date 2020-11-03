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

#ifdef CUDA_PATH_DEFINED
#include <genn/backends/cuda/optimiser.h>
#endif
#include <genn/backends/single_threaded_cpu/backend.h>
#include <genn/genn/code_generator/generateAll.h>
#include <genn/genn/code_generator/generateMakefile.h>
#include <genn/genn/logging.h>
#include <genn/genn/modelSpecInternal.h>
#include <genn/third_party/path.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <sharedLibraryModel.h>

#include <chrono>
#include <cypress/backend/genn/genn.hpp>
#include <cypress/backend/genn/genn_models.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/network_base_objects.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/util/filesystem.hpp>
#include <cypress/util/logger.hpp>
#include <fstream>

#define UNPACK7(v) v[0], v[1], v[2], v[3], v[4], v[5], v[6]

namespace cypress {

struct network_storage {
	std::vector<NeuronGroup *> neuron_groups;
	std::vector<SynapseGroup *> synapse_groups;
	std::vector<std::tuple<SynapseGroup *, SynapseGroup *,
	                       std::shared_ptr<std::vector<LocalConnection>>>>
	    synapse_groups_list;

	std::vector<size_t> list_connected_ids;
	std::shared_ptr<ModelSpecInternal> model;
	bool already_compiled = false;

	void reset()
	{
		neuron_groups.clear();
		synapse_groups.clear();
		synapse_groups_list.clear();
		list_connected_ids.clear();
		model = std::make_shared<ModelSpecInternal>();
		already_compiled = false;
	}
};
GeNN::GeNN(const Json &setup)
{
	if (setup.count("timestep") > 0) {
		m_timestep = setup["timestep"].get<Real>();
	}
	if (setup.count("gpu") > 0) {
		m_gpu = setup["gpu"].get<bool>();
	}
	if (setup.count("double") > 0) {
		m_double = setup["double"].get<bool>();
	}
	if (setup.count("timing") > 0) {
		m_timing = setup["timing"].get<bool>();
	}
	if (setup.count("keep_compile") > 0) {
		m_keep_compile = setup["keep_compile"].get<bool>();
	}
	if (setup.count("recording_buffer_size") > 0) {
		m_recording_buffer_size = setup["recording_buffer_size"].get<size_t>();
	}

	m_storage = std::make_shared<network_storage>();
	m_storage->model = std::make_shared<ModelSpecInternal>();
	if (global_logger().min_level() > LogSeverity::WARNING) {
		m_disable_status = true;
	}
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

/**
 * @brief Maps cypress parameters to genn parameters
 *
 * @param params_src cypress parameters
 * @param map one of the global defined maps above
 * @return std::vector< double > list of genn parameters
 */
template <typename T>
inline std::vector<T> map_params_to_genn(const NeuronParameters &params_src,
                                         const std::map<size_t, size_t> &map)
{
	std::vector<T> params(map.size(), 0);
	for (size_t i = 0; i < map.size(); i++) {
		params[i] = T(params_src[map.at(i)]);
	}
	return params;
}

/**
 * @brief Checks whether given parameter is homogeneous, although parameters are
 * set to inhomogenous
 *
 * @param pop the population to check
 * @param param_id index of parameter
 * @return true if homogeneous, false if inhomogenous
 */
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

/**
 * @brief returns a vector of all inhomogeneous indices in given population
 *
 * @param pop Population to check
 * @param map one of the globally defined maps above
 * @return std::vector< size_t > vector of inh parameters
 */
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

inline bool any_record(const PopulationBase &pop, size_t ind)
{
	for (const auto &n : pop) {
		if (n.signals().is_recording(ind)) {
			return true;
		}
	}
	return false;
}

inline bool any_record_at_all(const std::vector<PopulationBase> &pops,
                              size_t ind)
{
	for (const auto &pop : pops) {
		if (pop.homogeneous_record() && pop.signals().is_recording(ind)) {
			return true;
		}
	}
	for (const auto &pop : pops) {
		if (any_record(pop, ind)) {
			return true;
		}
	}
	return false;
}

/**
 * @brief Creates a GeNN neuron group for given population
 *
 * @param pop Cypress population
 * @param model GeNN model to add the population to
 * @param name name of the population
 * @return NeuronGroup* pointer to GeNN neuron group
 */
template <typename T>
NeuronGroup *create_neuron_group(const PopulationBase &pop,
                                 ModelSpecInternal &model, std::string name)
{
	if (pop.homogeneous_parameters()) {
		if (&pop.type() == &IfCondExp::inst()) {
			auto temp = map_params_to_genn<T>(pop.parameters(), ind_condlif);
			NeuronModels::LIF::ParamValues params(UNPACK7(temp));
			NeuronModels::LIF::VarValues inits(pop.parameters()[5], 0.0);
			auto gpop = model.addNeuronPopulation<NeuronModels::LIF>(
			    name, pop.size(), params, inits);
			if ((pop.homogeneous_record() && pop.signals().is_recording(0)) ||
			    any_record(pop, 0)) {
				gpop->setSpikeLocation(VarLocation::HOST_DEVICE);
			}
			if ((pop.homogeneous_record() && pop.signals().is_recording(1)) ||
			    any_record(pop, 1)) {
				gpop->setVarLocation("V", VarLocation::HOST_DEVICE);
			}
			return gpop;
		}
		else if (&pop.type() == &IfCurrExp::inst()) {
			auto temp = map_params_to_genn<T>(pop.parameters(), ind_currlif);
			NeuronModels::LIF::ParamValues params(UNPACK7(temp));
			NeuronModels::LIF::VarValues inits(pop.parameters()[5], 0.0);
			auto gpop = model.addNeuronPopulation<NeuronModels::LIF>(
			    name, pop.size(), params, inits);
			if ((pop.homogeneous_record() && pop.signals().is_recording(0)) ||
			    any_record(pop, 0)) {
				gpop->setSpikeLocation(VarLocation::HOST_DEVICE);
			}
			if ((pop.homogeneous_record() && pop.signals().is_recording(1)) ||
			    any_record(pop, 1)) {
				gpop->setVarLocation("V", VarLocation::HOST_DEVICE);
			}
			return gpop;
		}
		else if (&pop.type() == &SpikeSourceArray::inst()) {
			NeuronModels::SpikeSourceArray::ParamValues params;
			NeuronModels::SpikeSourceArray::VarValues inits(
			    0, pop.parameters().parameters().size());
			auto gpop =
			    model.addNeuronPopulation<NeuronModels::SpikeSourceArray>(
			        name, pop.size(), params, inits);
			gpop->setExtraGlobalParamLocation("spikeTimes",
			                                  VarLocation::HOST_DEVICE);
			gpop->setVarLocation("startSpike", VarLocation::HOST_DEVICE);
			gpop->setVarLocation("endSpike", VarLocation::HOST_DEVICE);
			return gpop;
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
			auto gpop =
			    model.addNeuronPopulation<NeuronModels::SpikeSourceArray>(
			        name, pop.size(), params, inits);
			gpop->setExtraGlobalParamLocation("spikeTimes",
			                                  VarLocation::HOST_DEVICE);
			gpop->setVarLocation("startSpike", VarLocation::HOST_DEVICE);
			gpop->setVarLocation("endSpike", VarLocation::HOST_DEVICE);
			return gpop;
		}
		throw ExecutionError(
		    "Currently inhomogenous parameters are not supported!");
	}
}

/**
 * @brief Returns the initialisation object for given connector. If the
 * connector cannot directly mapped to a GeNN connector, list_connected is set
 * to true and returns an Uninitialised object
 *
 * @param name cypress name of the connector
 * @param type will contain information on how to store connectivity in memory
 * @param allow_self_connections will remove connections going to the same
 * neuron id, IMPORTANT: GeNN connectors cannot check if src and tar pops are
 * the same, has to be done before
 * @param additional_parameter parameter of the connector (e.g. connection
 * probability)
 * @param full_pops connector acts on full population (false for popviews)
 * @param list_connected stores, whether we need fall back list connector
 * @return InitSparseConnectivitySnippet::Init initialisation object
 */
InitSparseConnectivitySnippet::Init genn_connector(
    std::string name, SynapseMatrixType &type,
    const bool allow_self_connections, const Real additional_parameter,
    const bool full_pops, bool &list_connected)
{
	list_connected = false;
	if (full_pops) {
		type = SynapseMatrixType::SPARSE_GLOBALG;
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
			return initConnectivity<InitSparseConnectivitySnippet::OneToOne>();
		}
		else if (name == "FixedFanOutConnector") {
			if (allow_self_connections) {
				return initConnectivity<
				    GeNNModels::FixedNumberPostWithReplacement>(
				    (unsigned int)additional_parameter);
			}
			else {

				return initConnectivity<
				    GeNNModels::FixedNumberPostWithReplacementNoAutapse>(
				    (unsigned int)additional_parameter);
			}
		}
		else if (name == "RandomConnector") {
			if (additional_parameter == 1.0 && allow_self_connections) {
				// AllToAll Case
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

template <typename PostsynapticModel, int NUM_VARS>
SynapseGroup *create_synapse_group(
    ModelSpecInternal &model, const ConnectionDescriptor &conn,
    const std::string name, const SynapseMatrixType type,
    const unsigned int delay, Models::VarInitContainerBase<NUM_VARS> weight,
    const typename PostsynapticModel::ParamValues &vars,
    const InitSparseConnectivitySnippet::Init &connector, bool cond_inhib)
{
	auto syn_name = conn.connector().synapse_name();
	if (syn_name == "StaticSynapse") {
		return model.addSynapsePopulation<WeightUpdateModels::StaticPulse,
		                                  PostsynapticModel>(
		    name, type, delay, "pop_" + std::to_string(conn.pid_src()),
		    "pop_" + std::to_string(conn.pid_tar()), {}, weight, vars, {},
		    connector);
	}
	else {
		// Need individual G for learning synapses
		SynapseMatrixType type2 = SynapseMatrixType::DENSE_INDIVIDUALG;
		if (type == SynapseMatrixType::SPARSE_GLOBALG) {
			// This is the SparseInit case
			type2 = SynapseMatrixType::SPARSE_INDIVIDUALG;
		}
		SynapseGroup *ret;
		if (syn_name == "SpikePairRuleAdditive") {
			auto &params = conn.connector().synapse()->parameters();
			if (cond_inhib) {
				ret = model.addSynapsePopulation<
				    GeNNModels::SpikePairRuleAdditive, PostsynapticModel>(
				    name, type2, delay, "pop_" + std::to_string(conn.pid_src()),
				    "pop_" + std::to_string(conn.pid_tar()),
				    {
				        params[4],   // Aplus
				        params[5],   // AMinus
				        -params[7],  // WMin
				        -params[6],  // WMax
				        params[2],   // tauPlus
				        params[3],   // tauMinus
				    },
				    {
				        weight[0],  // weight
				    },
				    {0.0},  // PreVar init
				    {0.0},  // PostVar init
				    vars, {}, connector);
			}
			else {
				ret = model.addSynapsePopulation<
				    GeNNModels::SpikePairRuleAdditive, PostsynapticModel>(
				    name, type2, delay, "pop_" + std::to_string(conn.pid_src()),
				    "pop_" + std::to_string(conn.pid_tar()),
				    {
				        params[4],  // Aplus
				        params[5],  // AMinus
				        params[6],  // WMin
				        params[7],  // WMax
				        params[2],  // tauPlus
				        params[3],  // tauMinus
				    },
				    {
				        weight[0],  // weight
				    },
				    {0.0},  // PreVar init
				    {0.0},  // PostVar init
				    vars, {}, connector);
			}
		}
		else if (syn_name == "SpikePairRuleMultiplicative") {
			auto &params = conn.connector().synapse()->parameters();
			if (cond_inhib) {
				ret = model.addSynapsePopulation<
				    GeNNModels::SpikePairRuleMultiplicative, PostsynapticModel>(
				    name, type2, delay, "pop_" + std::to_string(conn.pid_src()),
				    "pop_" + std::to_string(conn.pid_tar()),
				    {
				        params[4],   // Aplus
				        params[5],   // AMinus
				        -params[7],  // WMin
				        -params[6],  // WMax
				        params[2],   // tauPlus
				        params[3],   // tauMinus
				    },
				    {
				        weight[0],  // weight
				    },
				    {0.0},  // PreVar init
				    {0.0},  // PostVar init
				    vars, {}, connector);
			}
			else {
				ret = model.addSynapsePopulation<
				    GeNNModels::SpikePairRuleMultiplicative, PostsynapticModel>(
				    name, type2, delay, "pop_" + std::to_string(conn.pid_src()),
				    "pop_" + std::to_string(conn.pid_tar()),
				    {
				        params[4],  // Aplus
				        params[5],  // AMinus
				        params[6],  // WMin
				        params[7],  // WMax
				        params[2],  // tauPlus
				        params[3],  // tauMinus
				    },
				    {
				        weight[0],  // weight
				    },
				    {0.0},  // PreVar init
				    {0.0},  // PostVar init
				    vars, {}, connector);
			}
		}
		else if (syn_name == "TsodyksMarkramMechanism") {
			throw ExecutionError("SynapseModel " + syn_name +
			                     "is not supported on this backend");
			/*auto &params = conn.connector().synapse()->parameters();
			return model.addSynapsePopulation<GeNNModels::TsodyksMarkramSynapse,
			                                  PostsynapticModel>(
			    name, type2, delay, "pop_" + std::to_string(conn.pid_src()),
			    "pop_" + std::to_string(conn.pid_tar()), {},
			    {
			        params[2],  // U
			        params[3],  // tauRec
			        params[4],  // tauFacil
			        1.0,        // tauPsc, not existent in original model TODO
			        weight[0],  // weight/g
			        0.0,        // u
			        1.0,        // x
			        0.0,        // y
			        0.0         // z
			    },
			    {},  // PreVar init
			    {},  // PostVar init
			    vars, {}, connector);*/
		}
		else {
			throw ExecutionError("SynapseModel " + syn_name +
			                     "is not supported on this backend");
		}
		ret->setWUVarLocation("g", VarLocation::HOST_DEVICE);
		return ret;
	}
}

/**
 * @brief Connect two population
 *
 * @param pops list of cypress populations
 * @param model GeNN model
 * @param conn cypress connection
 * @param name name of the GeNN SynapseGroup
 * @param timestep timestep of the simulator
 * @param list_connect stores whether we need to use fall-back list connector
 * @return SynapseGroup* pointer to synapse group
 */
template <typename T>
SynapseGroup *connect(const std::vector<PopulationBase> &pops,
                      ModelSpecInternal &model,
                      const ConnectionDescriptor &conn, std::string name,
                      T timestep, bool &list_connect)
{
	bool cond_based = pops[conn.pid_tar()].type().conductance_based;
	T weight = T(conn.connector().synapse()->parameters()[0]);
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
		T rev_pot, tau;
		if (weight >= 0) {  // TODO : adapt for EifCondExpIsfaIsta
			bool test1 = check_param_for_homogeneity(pops[conn.pid_tar()], 8);
			bool test2 = check_param_for_homogeneity(pops[conn.pid_tar()], 2);
			if (!(test1 && test2)) {
				// TODO
				throw ExecutionError(
				    "Inhomogeneous synapse parameters not supported by GeNN");
			}
			rev_pot = T(pops[conn.pid_tar()].parameters().parameters()[8]);
			tau = T(pops[conn.pid_tar()].parameters().parameters()[2]);
		}
		else {
			bool test1 = check_param_for_homogeneity(pops[conn.pid_tar()], 9);
			bool test2 = check_param_for_homogeneity(pops[conn.pid_tar()], 3);
			if (!(test1 && test2)) {
				// TODO
				throw ExecutionError(
				    "Inhomogeneous synapse parameters not supported by GeNN");
			}
			rev_pot = T(pops[conn.pid_tar()].parameters().parameters()[9]);
			tau = T(pops[conn.pid_tar()].parameters().parameters()[3]);

			weight = -weight;
		}

		return create_synapse_group<PostsynapticModels::ExpCond, 1>(
		    model, conn, name, type, delay, {weight}, {tau, rev_pot}, connector,
		    weight < 0);
	}
	else {
		T tau;
		if (weight >= 0) {
			bool test1 = check_param_for_homogeneity(pops[conn.pid_tar()], 2);
			if (!test1) {
				// TODO
				throw ExecutionError(
				    "Inhomogeneous synapse parameters not supported by GeNN");
			}
			tau = T(pops[conn.pid_tar()].parameters().parameters()[2]);
		}
		else {
			bool test1 = check_param_for_homogeneity(pops[conn.pid_tar()], 3);
			if (!test1) {
				// TODO
				throw ExecutionError(
				    "Inhomogeneous synapse parameters not supported by GeNN");
			}
			tau = T(pops[conn.pid_tar()].parameters().parameters()[3]);
		}

		return create_synapse_group<PostsynapticModels::ExpCurr, 1>(
		    model, conn, name, type, delay, {weight}, {tau}, connector, false);
	}
}

/**
 * @brief List connection init before compiling GeNN
 *
 * @param pops cypress populations
 * @param model GeNN model
 * @param conn cypress connection
 * @param name name for the GeNN SynapseGroups
 * @param timestep simulator timestep
 * @return tuple with excitatory and inhibitory SynapseGroup
 */
template <typename T>
std::tuple<SynapseGroup *, SynapseGroup *,
           std::shared_ptr<std::vector<LocalConnection>>>
list_connect_pre(const std::vector<PopulationBase> pops,
                 ModelSpecInternal &model, const ConnectionDescriptor &conn,
                 std::string name, T timestep, bool full)
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

	if (conns_full->size() - num_inh > 0 || full) {
		if (cond_based) {
			T rev_pot = T(pops[conn.pid_tar()].parameters().parameters()[8]);
			T tau = T(pops[conn.pid_tar()].parameters().parameters()[2]);
			synex = create_synapse_group<PostsynapticModels::ExpCond, 1>(
			    model, conn, name + "_ex", SynapseMatrixType::DENSE_INDIVIDUALG,
			    delay, {0.0}, {tau, rev_pot},
			    initConnectivity<
			        InitSparseConnectivitySnippet::Uninitialised>(),
			    false);
		}
		else {
			T tau = T(pops[conn.pid_tar()].parameters().parameters()[2]);
			synex = create_synapse_group<PostsynapticModels::ExpCurr, 1>(
			    model, conn, name + "_ex", SynapseMatrixType::DENSE_INDIVIDUALG,
			    delay, {0.0}, {tau},
			    initConnectivity<
			        InitSparseConnectivitySnippet::Uninitialised>(),
			    false);
		}
		synex->setWUVarLocation("g", VarLocation::HOST_DEVICE);
	}

	// inhibitory
	if (num_inh > 0 || full) {
		if (cond_based) {
			T rev_pot = T(pops[conn.pid_tar()].parameters().parameters()[9]);
			T tau = T(pops[conn.pid_tar()].parameters().parameters()[3]);
			synin = create_synapse_group<PostsynapticModels::ExpCond, 1>(
			    model, conn, name + "_in", SynapseMatrixType::DENSE_INDIVIDUALG,
			    delay, {0.0}, {tau, rev_pot},
			    initConnectivity<
			        InitSparseConnectivitySnippet::Uninitialised>(),
			    true);
		}
		else {
			T tau = T(pops[conn.pid_tar()].parameters().parameters()[3]);
			synin = create_synapse_group<PostsynapticModels::ExpCurr, 1>(
			    model, conn, name + "_in", SynapseMatrixType::DENSE_INDIVIDUALG,
			    delay, {0.0}, {tau},
			    initConnectivity<
			        InitSparseConnectivitySnippet::Uninitialised>(),
			    false);
		}
		synin->setWUVarLocation("g", VarLocation::HOST_DEVICE);
	}
	return std::make_tuple(synex, synin, std::move(conns_full));
}

typedef void (*PushFunction)(bool);

/**
 * @brief List connector after allocateMem and before sparse allocation
 *
 * @param name of the synapse group
 * @param tuple tuple of synapse groups
 * @param slm pointer to dynamically loaded GeNN lib
 * @param size_tar size of the target population
 * @param cond_based true if target pop is conductance_based
 */
template <typename T>
void list_connect_post(
    std::string name,
    std::tuple<SynapseGroup *, SynapseGroup *,
               std::shared_ptr<std::vector<LocalConnection>>> &tuple,
    GeNNModels::SharedLibraryModel_<T> &slm, size_t size_tar, bool cond_based)
{
	if (std::get<0>(tuple)) {
		T *weights_ex = *(static_cast<T **>(slm.getSymbol("g" + name + "_ex")));
		for (auto &i : *(std::get<2>(tuple))) {
			if (i.excitatory()) {
				weights_ex[size_tar * i.src + i.tar] =
				    T(i.SynapseParameters[0]);
			}
		}
		((PushFunction)slm.getSymbol("pushg" + name + "_exToDevice"))(false);
	}

	if (std::get<1>(tuple)) {
		T *weights_in = *(static_cast<T **>(slm.getSymbol("g" + name + "_in")));
		for (auto &i : *(std::get<2>(tuple))) {
			if (i.inhibitory()) {
				weights_in[size_tar * i.src + i.tar] =
				    cond_based ? -T(i.SynapseParameters[0])
				               : T(i.SynapseParameters[0]);
			}
		}
		((PushFunction)slm.getSymbol("pushg" + name + "_inToDevice"))(false);
	}

	std::get<2>(tuple)->clear();
}

namespace {
inline plog::Severity get_log_level()
{
	auto level = global_logger().min_level();
	switch (level) {
		case LogSeverity::FATAL_ERROR:
			return plog::Severity::fatal;
		case LogSeverity::ERROR:
			return plog::Severity::error;
		case LogSeverity::WARNING:
			return plog::Severity::warning;
		case LogSeverity::INFO:
			return plog::Severity::info;
		/*case  LogSeverity::debug:
			return plog::Severity::DEBUG;*/
		case LogSeverity::DEBUG:
			return plog::Severity::verbose;
		default:
			return plog::Severity::none;
	}
}
}  // namespace

/**
 * @brief Build, compile and load the model
 *
 * @param gpu flag whether to use gpu
 * @param model GeNN model
 * @return SharedLibraryModel_< float > object to access loaded model
 */
template <typename T>
GeNNModels::SharedLibraryModel_<T> build_and_make(
    bool gpu, ModelSpecInternal &model,
#ifdef CUDA_PATH_DEFINED
    plog::ConsoleAppender<plog::TxtFormatter> &logger,
#else
    plog::ConsoleAppender<plog::TxtFormatter> &,
#endif
    bool compile = true)
{
	std::string path = "./" + model.getName() + "_CODE/";
	if (compile) {
		mkdir(path.c_str(), 0777);
		auto fs_path = ::filesystem::path(path);
		auto share_path = ::filesystem::path(GENN_SHARE_PATH);
		if (gpu) {
#ifdef CUDA_PATH_DEFINED
			CodeGenerator::CUDA::Preferences prefs;
#ifndef NDEBUG
			prefs.debugCode = true;
#else
			prefs.optimizeCode = true;
#endif
			prefs.generateEmptyStatePushPull = false;
			prefs.generateEmptyStatePushPull = false;
			prefs.logLevel = get_log_level();
			auto bck = CodeGenerator::CUDA::Optimiser::createBackend(
			    model, share_path, fs_path, prefs.logLevel, &logger, prefs);
			auto moduleNames = CodeGenerator::generateAll(
			    model, bck, share_path, fs_path, false);
			std::ofstream makefile(path + "Makefile");
			CodeGenerator::generateMakefile(makefile, bck,
			                                std::get<0>(moduleNames));
			makefile.close();
#else
			throw ExecutionError(
			    "GeNN has not been compiled with GPU support!");
#endif
		}
		else {
			CodeGenerator::SingleThreadedCPU::Preferences prefs;
#ifndef NDEBUG
			prefs.debugCode = true;
#else
			prefs.optimizeCode = true;
#endif
			prefs.logLevel = get_log_level();
			CodeGenerator::SingleThreadedCPU::Backend bck(model.getPrecision(),
			                                              prefs);
			auto moduleNames = CodeGenerator::generateAll(
			    model, bck, share_path, fs_path, false);
			std::ofstream makefile(path + "Makefile");
			CodeGenerator::generateMakefile(makefile, bck,
			                                std::get<0>(moduleNames));
			makefile.close();
		}
#ifndef NDEBUG
		system(("make -j 4 -C " + path).c_str());
#else
		system(("make -j 4 -C " + path + " >/dev/null 2>&1").c_str());
#endif
	}

	// Open the compiled Library as dynamically loaded library
	auto slm = GeNNModels::SharedLibraryModel_<T>();
	bool open = slm.open("./", model.getName());
	if (!open) {
		throw ExecutionError(
		    "Could not open Shared Library Model of GeNN network. Make sure "
		    "that code generation and make were successful!");
	}
	return slm;
}

/**
 * @brief Setup of spike data for spike source arrays
 *
 * @param populations cypress populations
 * @param slm loaded GeNN model
 */
template <typename T>
void setup_spike_sources(const std::vector<PopulationBase> &populations,
                         GeNNModels::SharedLibraryModel_<T> &slm)
{
	// Set up spike times of spike source arrays
	for (size_t i = 0; i < populations.size(); i++) {
		const auto &pop = populations[i];
		if (&pop.type() == &SpikeSourceArray::inst()) {
			if (pop.homogeneous_parameters()) {
				auto &spike_times = pop.parameters().parameters();
				slm.allocateExtraGlobalParam("pop_" + std::to_string(i),
				                             "spikeTimes", spike_times.size());
				auto **spikes_ptr = (static_cast<T **>(
				    slm.getSymbol("spikeTimespop_" + std::to_string(i))));
				for (size_t spike = 0; spike < spike_times.size(); spike++) {
					(*spikes_ptr)[spike] = T(spike_times[spike]);
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
				auto *spikes_ptr = *(static_cast<T **>(
				    slm.getSymbol("spikeTimespop_" + std::to_string(i))));
				for (size_t neuron = 0; neuron < pop.size(); neuron++) {
					const auto &spike_times =
					    pop[neuron].parameters().parameters();
					for (size_t spike = 0; spike < spike_times.size();
					     spike++) {
						spikes_ptr[size + spike] = T(spike_times[spike]);
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
}

template <typename T>
/**
 * @brief Teardown of spike data for spike source arrays
 *
 * @param populations cypress populations
 * @param slm loaded GeNN model
 */
void teardown_spike_sources(const std::vector<PopulationBase> &populations,
                            GeNNModels::SharedLibraryModel_<T> &slm)
{
	// Set up spike times of spike source arrays
	for (size_t i = 0; i < populations.size(); i++) {
		const auto &pop = populations[i];
		if (&pop.type() == &SpikeSourceArray::inst()) {
			slm.freeExtraGlobalParam("pop_" + std::to_string(i), "spikeTimes");
		}
	}
}

typedef unsigned int *(*CurrentSpikeFunction)();
typedef unsigned int &(*CurrentSpikeCountFunction)();

/**
 * @brief Resolve all pointers to access GeNN data
 *
 * @param spike_ptrs Pointers to access spikes
 * @param spike_cnt_ptrs pointer to access which neurons has spiked
 * @param v_ptrs Pointer to voltage state variables
 * @param record_full_spike stores which pops record all neurons
 * @param record_full_v stores which pops record all neurons
 * @param record_part_spike stores which pops record partially
 * @param record_part_v stores which pops record partially
 * @param populations list of cypress populations
 * @param slm loaded GeNN model
 */
template <typename T>
void resolve_ptrs(std::vector<CurrentSpikeFunction> &spike_ptrs,
                  std::vector<CurrentSpikeCountFunction> &spike_cnt_ptrs,
                  std::vector<T *> &v_ptrs,
                  std::vector<size_t> &record_full_spike,
                  std::vector<size_t> &record_full_v,
                  std::vector<size_t> &record_part_spike,
                  std::vector<size_t> &record_part_v,
                  std::vector<uint32_t *> &spike_record_ptrs,
                  const std::vector<PopulationBase> &populations,
                  GeNNModels::SharedLibraryModel_<T> &slm, bool record_buf)
{
	v_ptrs = std::vector<T *>(populations.size(), nullptr);
	spike_ptrs = std::vector<CurrentSpikeFunction>(populations.size(), nullptr);
	spike_cnt_ptrs =
	    std::vector<CurrentSpikeCountFunction>(populations.size(), nullptr);
	spike_record_ptrs = std::vector<uint32_t *>(populations.size(), nullptr);

	for (size_t i = 0; i < populations.size(); i++) {
		const auto &pop = populations[i];
		if (&pop.type() == &SpikeSourceArray::inst()) {
			continue;
		}
		if (pop.homogeneous_record()) {
			if (pop.signals().is_recording(0)) {
				spike_cnt_ptrs[i] = (CurrentSpikeCountFunction)(slm.getSymbol(
				    "getpop_" + std::to_string(i) + "CurrentSpikeCount"));
				spike_ptrs[i] = (CurrentSpikeFunction)(slm.getSymbol(
				    "getpop_" + std::to_string(i) + "CurrentSpikes"));
				if (record_buf) {
					spike_record_ptrs[i] = (*(static_cast<uint32_t **>(
					    slm.getSymbol("recordSpkpop_" + std::to_string(i)))));
				}
				record_full_spike.emplace_back(pop.pid());
			}
			if (pop.signals().size() > 1 && pop.signals().is_recording(1)) {
				v_ptrs[i] = (*(static_cast<T **>(
				    slm.getSymbol("Vpop_" + std::to_string(i)))));
				record_full_v.emplace_back(pop.pid());
			}
		}
		else {
			for (auto neuron : pop) {
				if (neuron.signals().is_recording(0)) {
					record_part_spike.emplace_back(neuron.pid());

					spike_cnt_ptrs[neuron.pid()] = (CurrentSpikeCountFunction)(
					    slm.getSymbol("getpop_" + std::to_string(i) +
					                  "CurrentSpikeCount"));

					spike_ptrs[neuron.pid()] =
					    (CurrentSpikeFunction)(slm.getSymbol(
					        "getpop_" + std::to_string(i) + "CurrentSpikes"));
					if (record_buf) {
						spike_record_ptrs[i] =
						    (*(static_cast<uint32_t **>(slm.getSymbol(
						        "recordSpkpop_" + std::to_string(i)))));
					}
					break;
				}
			}
			if (&pop.type() != &SpikeSourceArray::inst()) {
				for (auto neuron : pop) {
					if (neuron.signals().is_recording(1)) {
						record_part_v.emplace_back(neuron.pid());
						v_ptrs[neuron.pid()] = (*(static_cast<T **>(
						    slm.getSymbol("Vpop_" + std::to_string(i)))));
						break;
					}
				}
			}
		}
	}
}

void set_buffer_recording(const std::vector<PopulationBase> &populations,
                          std::vector<NeuronGroup *> nrn_grp)
{
	for (size_t i = 0; i < populations.size(); i++) {
		const auto &pop = populations[i];
		if (&pop.type() == &SpikeSourceArray::inst()) {
			continue;
		}
		if (pop.homogeneous_record()) {
			if (pop.signals().is_recording(0)) {
				nrn_grp[i]->setSpikeRecordingEnabled(true);
			}
		}
		else {
			for (auto neuron : pop) {
				if (neuron.signals().is_recording(0)) {
					nrn_grp[i]->setSpikeRecordingEnabled(true);
					break;
				}
			}
		}
	}
}

/**
 * @brief Prepares container for storing spikes
 *
 * @param populations list of cypress populations
 * @return container
 */
auto prepare_spike_storage(const std::vector<PopulationBase> &populations)
{
	size_t size = populations.size();
	std::vector<std::vector<std::vector<Real>>> spike_data(
	    size, std::vector<std::vector<Real>>());

	for (size_t i = 0; i < size; i++) {
		spike_data[i] = std::vector<std::vector<Real>>(populations[i].size(),
		                                               std::vector<Real>());
	}
	return spike_data;
}

/**
 * @brief Prepares container for storing voltage traces of recording neurons
 *
 * @param populations list of cypress populations
 * @param time_slots number of time slots in current simulation
 * @param record_full_v vector with indices which pops record voltage
 * @param record_part_v vector with indices which pops partially record voltage
 * @return container
 */
auto prepare_v_storage(const std::vector<PopulationBase> &populations,
                       const size_t time_slots,
                       const std::vector<size_t> &record_full_v,
                       const std::vector<size_t> &record_part_v)
{
	size_t size = populations.size();
	std::vector<std::vector<std::shared_ptr<Matrix<Real>>>> v_data(
	    size, std::vector<std::shared_ptr<Matrix<Real>>>());

	for (size_t i = 0; i < size; i++) {
		v_data[i] = std::vector<std::shared_ptr<Matrix<Real>>>(
		    populations[i].size(), std::shared_ptr<Matrix<Real>>());
	}
	for (auto id : record_full_v) {
		for (size_t nid = 0; nid < populations[id].size(); nid++) {
			v_data[id][nid] =
			    std::make_shared<Matrix<Real>>(Matrix<Real>(time_slots, 2));
		}
	}
	for (auto id : record_part_v) {
		for (size_t nid = 0; nid < populations[id].size(); nid++) {
			if (populations[id][nid].signals().is_recording(1)) {
				v_data[id][nid] =
				    std::make_shared<Matrix<Real>>(Matrix<Real>(time_slots, 2));
			}
		}
	}

	return v_data;
}

inline std::vector<Real> remove_spikes_after(std::vector<Real> spikes,
                                             Real time)
{
	auto it = std::upper_bound(spikes.begin(), spikes.end(), time);
	spikes.erase(it, spikes.end());
	return spikes;
}

void record_spike_source(NetworkBase &netw, Real duration)
{
	for (auto pop : netw.populations()) {
		if (&pop.type() == &SpikeSourceArray::inst()) {
			if (pop.homogeneous_parameters() &&
			    ((pop.homogeneous_record() && pop.signals().is_recording(0)) ||
			     any_record(pop, 0))) {
				pop.signals().data(
				    0, std::make_shared<Matrix<Real>>(remove_spikes_after(
				           pop.parameters().parameters(), duration)));
			}
			else if (any_record(pop, 0)) {
				for (auto n : pop) {
					if (n.signals().is_recording(0)) {
						n.signals().data(
						    0,
						    std::make_shared<Matrix<Real>>(remove_spikes_after(
						        n.parameters().parameters(), duration)));
					}
				}
			}
		}
	}
}

/**
 * @brief Adapted from GeNNs userproject SpikeWriter, this function converts the
 * spike buffer from GeNNs internal buffering method to a vector of spikes
 *
 * @param spikes The target container
 * @param pop_size population size
 * @param spkRecord pointer to GeNN spike buffer.
 * @param numTimesteps this is recording_buffer_size
 * @param dt timestep
 * @param offset offset in several consecutive runs
 */
void convert_spike_buffer(std::vector<std::vector<Real>> &spikes,
                          size_t pop_size, const uint32_t *spkRecord,
                          unsigned int numTimesteps, double dt, double offset)
{
	const unsigned int timestepWords = (pop_size + 31) / 32;
	for (unsigned int t = 0; t < numTimesteps; t++) {
		// Convert timestep to time
		const double time = offset + (t * dt);
		// Loop through words representing timestep
		for (unsigned int w = 0; w < timestepWords; w++) {
			// Get word
			uint32_t spikeWord = spkRecord[(t * timestepWords) + w];

			// Calculate neuron id of highest bit of this word
			unsigned int neuronID = (w * 32) + 31;

			// While bits remain
			while (spikeWord != 0) {
				// Calculate leading zeros
				const int numLZ = __builtin_clz((unsigned int)spikeWord);

				// If all bits have now been processed, zero spike word
				// Otherwise shift past the spike we have found
				spikeWord = (numLZ == 31) ? 0 : (spikeWord << (numLZ + 1));

				// Subtract number of leading zeros from neuron ID
				neuronID -= numLZ;
				spikes[neuronID].push_back(time);
				neuronID--;
			}
		}
	}
}

/**
 * @brief Adapted from GeNNs userproject SpikeWriter, this function converts the
 * spike buffer from GeNNs internal buffering method to a vector of spikes
 * This method is meant for partial recording
 *
 * @param spikes The target container
 * @param pop target population
 * @param spkRecord pointer to GeNN spike buffer.
 * @param numTimesteps this is recording_buffer_size
 * @param dt timestep
 * @param offset offset in several consecutive runs
 */
void convert_spike_buffer(std::vector<std::vector<Real>> &spikes,
                          PopulationBase pop, const uint32_t *spkRecord,
                          unsigned int numTimesteps, double dt, double offset)
{
	const unsigned int timestepWords = (pop.size() + 31) / 32;
	for (unsigned int t = 0; t < numTimesteps; t++) {
		// Convert timestep to time
		const double time = offset + (t * dt);
		// Loop through words representing timestep
		for (unsigned int w = 0; w < timestepWords; w++) {
			// Get word
			uint32_t spikeWord = spkRecord[(t * timestepWords) + w];

			// Calculate neuron id of highest bit of this word
			unsigned int neuronID = (w * 32) + 31;

			// While bits remain
			while (spikeWord != 0) {
				// Calculate leading zeros
				const int numLZ = __builtin_clz((unsigned int)spikeWord);

				// If all bits have now been processed, zero spike word
				// Otherwise shift past the spike we have found
				spikeWord = (numLZ == 31) ? 0 : (spikeWord << (numLZ + 1));

				// Subtract number of leading zeros from neuron ID
				neuronID -= numLZ;
				if (pop[neuronID].signals().is_recording(0)) {
					spikes[neuronID].push_back(time);
				}
				neuronID--;
			}
		}
	}
}

namespace {
plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;

template <typename T>
inline void progress_bar(T p)
{
	const int w = 50;
	std::cerr << std::fixed << std::setprecision(2) << std::setw(6) << p * 100.0
	          << "% [";
	const int j = p * float(w);
	for (int i = 0; i < w; i++) {
		std::cerr << (i > j ? ' ' : (i == j ? '>' : '='));
	}
	std::cerr << "]\r";
}

template <typename T>
void allocate_buffer_memory(GeNNModels::SharedLibraryModel_<T> &slm,
                            size_t buffer_size)
{
	try {
		slm.allocateRecordingBuffers(buffer_size);
	}
	catch (...) {
		throw cypress::ExecutionError(
		    "Backend is out of memory! Please restart the application and set a"
		    " smaller (zero) recording_buffer_size! Currently size is at " +
		    std::to_string(buffer_size));
	}
}

}  // namespace

template <typename T>
void do_run_templ(NetworkBase &network, Real duration, ModelSpecInternal &model,
                  T timestep, bool gpu, bool timing, bool keep_compile,
                  network_storage &store, bool disable_status,
                  size_t recording_buffer_size)
{
	Logging::init(get_log_level(), get_log_level(), &consoleAppender,
	              &consoleAppender);

	auto &neuron_groups = store.neuron_groups;
	auto &synapse_groups = store.synapse_groups;
	auto &synapse_groups_list = store.synapse_groups_list;
	auto &list_connected_ids = store.list_connected_ids;
	auto &populations = network.populations();
	auto &connections = network.connections();
	bool execute_all =
	    !(store.already_compiled &&
	      keep_compile);  // only false if keep_compile and already_compiled

	auto start_t = std::chrono::steady_clock::now();
	if (execute_all) {
		// Create Populations
		for (size_t i = 0; i < populations.size(); i++) {
			neuron_groups.emplace_back(create_neuron_group<T>(
			    populations[i], model, ("pop_" + std::to_string(i))));
		}
	}
	else {
		// Sanity Checks
		if ((neuron_groups.size() != populations.size()) ||
		    (synapse_groups.size() != connections.size())) {
			throw cypress::ExecutionError(
			    "Unexpected network dimenstion! If only changing "
			    "weights make sure to use the same network!");
		}
	}

	if (execute_all) {
		// Create connections
		for (size_t i = 0; i < connections.size(); i++) {
			bool list_connected;
			synapse_groups.emplace_back(connect<T>(
			    populations, model, connections[i], "conn_" + std::to_string(i),
			    timestep, list_connected));

			if (list_connected) {
				synapse_groups_list.emplace_back(list_connect_pre<T>(
				    populations, model, connections[i],
				    "conn_" + std::to_string(i), timestep, keep_compile));
				list_connected_ids.emplace_back(i);
			}
		}
		if (recording_buffer_size > 0) {
			set_buffer_recording(network.populations(), neuron_groups);
		}
	}
	else {
		for (size_t i = 0; i < list_connected_ids.size(); i++) {
			auto conns_full = std::make_shared<std::vector<LocalConnection>>();
			connections[list_connected_ids[i]].connect(*conns_full);
			std::get<2>(synapse_groups_list[i]) = conns_full;
		}
	}

	// Build the model and compile
	model.finalize();
	auto slm = build_and_make<T>(gpu, model, consoleAppender, execute_all);
	slm.allocateMem();
	bool allocated_memory = false;
	if (recording_buffer_size > 0) {
		if (any_record_at_all(populations, 0)) {
			allocate_buffer_memory(slm, recording_buffer_size);
			allocated_memory = true;
		}
	}
	slm.initialize();
	T *time = static_cast<T *>(slm.getSymbol("t"));

	setup_spike_sources(populations, slm);

	// List Connections
	for (size_t i = 0; i < list_connected_ids.size(); i++) {
		size_t id = list_connected_ids[i];
		bool cond_based =
		    populations[connections[id].pid_tar()].type().conductance_based;
		list_connect_post<T>(
		    "conn_" + std::to_string(id), synapse_groups_list[i], slm,
		    populations[connections[id].pid_tar()].size(), cond_based);
	}

	slm.initializeSparse();

	// Get Pointers to observables
	std::vector<CurrentSpikeFunction> spike_ptrs;
	std::vector<CurrentSpikeCountFunction> spike_cnt_ptrs;
	std::vector<T *> v_ptrs;
	std::vector<uint32_t *> spike_rec_ptrs;

	// Store which populations are recorded
	std::vector<size_t> record_full_spike, record_full_v, record_part_spike,
	    record_part_v;
	resolve_ptrs<T>(spike_ptrs, spike_cnt_ptrs, v_ptrs, record_full_spike,
	                record_full_v, record_part_spike, record_part_v,
	                spike_rec_ptrs, populations, slm,
	                recording_buffer_size > 0);

	auto spike_data = prepare_spike_storage(populations);
	auto v_data =
	    prepare_v_storage(populations, size_t(std::ceil(duration / timestep)),
	                      record_full_v, record_part_v);

	auto built_t = std::chrono::steady_clock::now();
	size_t counter = 0;
	// Run the simulation and pull all recorded variables
	while (*time < T(duration)) {
		if ((!disable_status) && counter % 50 == 0) {
			progress_bar<T>(*time / T(duration));
		}
		slm.stepTime();

		if (recording_buffer_size == 0) {
			// Fully recorded spikes
			for (auto id : record_full_spike) {
				slm.pullCurrentSpikesFromDevice("pop_" + std::to_string(id));
				for (unsigned int nid = 0; nid < spike_cnt_ptrs[id](); nid++) {
					spike_data[id][(spike_ptrs[id]())[nid]].push_back(
					    Real(*time));
				}
			}
			// Partially recorded spikes
			for (auto id : record_part_spike) {
				slm.pullCurrentSpikesFromDevice("pop_" + std::to_string(id));
				for (unsigned int nid = 0; nid < spike_cnt_ptrs[id](); nid++) {
					unsigned int n = (spike_ptrs[id]())[nid];
					if (populations[id][n].signals().is_recording(0)) {
						spike_data[id][n].push_back(Real(*time));
					}
				}
			}
		}
		else {
			if (((counter + 1) % recording_buffer_size) == 0) {
				slm.pullRecordingBuffersFromDevice();
				for (auto id : record_full_spike) {
					convert_spike_buffer(
					    spike_data[id], populations[id].size(),
					    spike_rec_ptrs[id], recording_buffer_size, timestep,
					    double(counter + 1 - recording_buffer_size) * timestep);
				}
				for (auto id : record_part_spike) {
					convert_spike_buffer(
					    spike_data[id], populations[id], spike_rec_ptrs[id],
					    recording_buffer_size, timestep,
					    double(counter + 1 - recording_buffer_size) * timestep);
				}
			}
		}

		// Fully recorded voltage
		for (auto id : record_full_v) {
			slm.pullVarFromDevice("pop_" + std::to_string(id), "V");
			for (size_t nid = 0; nid < populations[id].size(); nid++) {
				(*v_data[id][nid])(counter, 0) = Real(*time);
				(*v_data[id][nid])(counter, 1) = Real(v_ptrs[id][nid]);
			}
		}

		// Partially recorded voltage
		for (auto id : record_part_v) {
			slm.pullVarFromDevice("pop_" + std::to_string(id), "V");
			for (size_t nid = 0; nid < populations[id].size(); nid++) {
				if (populations[id][nid].signals().is_recording(1)) {
					(*v_data[id][nid])(counter, 0) = Real(*time);
					(*v_data[id][nid])(counter, 1) = Real(v_ptrs[id][nid]);
				}
			}
		}
		counter++;
	}
	if (!disable_status) {
		progress_bar<T>(1.0);
		std::cerr << std::endl;
	}
	auto sim_fin_t = std::chrono::steady_clock::now();
	if (recording_buffer_size != 0) {
		if (allocated_memory) {
			size_t left_over = counter % recording_buffer_size;
			if (left_over != 0) {
				slm.pullRecordingBuffersFromDevice();
				for (auto id : record_full_spike) {
					convert_spike_buffer(
					    spike_data[id], populations[id].size(),
					    spike_rec_ptrs[id], left_over, timestep,
					    double(counter - left_over) * timestep);
				}
				for (auto id : record_part_spike) {
					convert_spike_buffer(
					    spike_data[id], populations[id], spike_rec_ptrs[id],
					    left_over, timestep,
					    double(counter - left_over) * timestep);
				}
			}
		}
	}
	// Convert spike data
	for (auto id : record_full_spike) {
		auto &pop = populations[id];
		for (size_t neuron = 0; neuron < pop.size(); neuron++) {
			auto data = std::make_shared<Matrix<Real>>(
			    spike_data[id][neuron].size(), 1);
			for (size_t j = 0; j < spike_data[id][neuron].size(); j++) {
				(*data)(j, 0) = spike_data[id][neuron][j];
			}
			auto neuron_inst = pop[neuron];
			neuron_inst.signals().data(0, std::move(data));
		}
	}

	// Convert spike data Partially
	for (auto id : record_part_spike) {
		auto &pop = populations[id];
		for (size_t neuron = 0; neuron < pop.size(); neuron++) {
			if (pop[neuron].signals().is_recording(0)) {
				auto data = std::make_shared<Matrix<Real>>(
				    spike_data[id][neuron].size(), 1);
				for (size_t j = 0; j < spike_data[id][neuron].size(); j++) {
					(*data)(j, 0) = spike_data[id][neuron][j];
				}
				auto neuron_inst = pop[neuron];
				neuron_inst.signals().data(0, std::move(data));
			}
		}
	}

	// Store voltage trace
	for (auto id : record_full_v) {
		auto &pop = populations[id];
		for (size_t neuron = 0; neuron < pop.size(); neuron++) {
			auto neuron_inst = pop[neuron];
			neuron_inst.signals().data(1, std::move(v_data[id][neuron]));
		}
	}

	// Store voltage trace partially
	for (auto id : record_part_v) {
		auto &pop = populations[id];
		for (size_t neuron = 0; neuron < pop.size(); neuron++) {
			if (pop[neuron].signals().is_recording(1)) {
				auto neuron_inst = pop[neuron];
				neuron_inst.signals().data(1, std::move(v_data[id][neuron]));
			}
		}
	}

	for (auto pop : populations) {
		bool record_condutance = false;
		if (pop.signals().size() > 1) {
			for (auto neuron : pop) {
				if (neuron.signals().is_recording(2)) {
					neuron.signals().data(2, std::make_shared<Matrix<Real>>(
					                             2, 2, MatrixFlags::ZEROS));
					record_condutance = true;
				}
				if (neuron.signals().size() > 2 &&
				    neuron.signals().is_recording(3)) {
					neuron.signals().data(3, std::make_shared<Matrix<Real>>(
					                             2, 2, MatrixFlags::ZEROS));
					record_condutance = true;
				}
			}
		}
		if (record_condutance) {
			global_logger().warn(
			    "GeNN",
			    "Recording conductance not implemented. Inserted dummy data!");
		}
	}

	for (size_t i = 0; i < network.connections().size(); i++) {
		auto &conn = network.connections()[i];
		if (conn.connector().synapse()->learning()) {
			Real delay = conn.connector().synapse()->parameters()[1];
			bool cond_based =
			    populations[conn.pid_tar()].type().conductance_based;

			std::vector<LocalConnection> weight_store;
			if (std::find(list_connected_ids.begin(), list_connected_ids.end(),
			              i) != list_connected_ids.end()) {
				try {
					slm.pullVarFromDevice("conn_" + std::to_string(i) + "_ex",
					                      "g");
					T *weights = *(static_cast<T **>(
					    slm.getSymbol("gconn_" + std::to_string(i) + "_ex")));
					convert_learned_weights(populations[conn.pid_src()].size(),
					                        populations[conn.pid_tar()].size(),
					                        weight_store, weights, delay,
					                        false);
				}
				catch (...) {
				}
				try {
					slm.pullVarFromDevice("conn_" + std::to_string(i) + "_in",
					                      "g");
					T *weights = *(static_cast<T **>(
					    slm.getSymbol("gconn_" + std::to_string(i) + "_in")));
					convert_learned_weights(populations[conn.pid_src()].size(),
					                        populations[conn.pid_tar()].size(),
					                        weight_store, weights, delay,
					                        cond_based);
				}
				catch (...) {
				}
			}
			else {
				slm.pullVarFromDevice("conn_" + std::to_string(i), "g");
				try {
					slm.pullConnectivityFromDevice("conn_" + std::to_string(i));
				}
				catch (...) {
				}
				T *weights = *(static_cast<T **>(
				    slm.getSymbol("gconn_" + std::to_string(i))));
				cond_based =
				    (cond_based &&
				     (conn.connector().synapse()->parameters()[0] < 0));
				try {
					unsigned int *rowlengths = *(static_cast<unsigned int **>(
					    slm.getSymbol("rowLengthconn_" + std::to_string(i))));
					unsigned int *indices = *(static_cast<unsigned int **>(
					    slm.getSymbol("indconn_" + std::to_string(i))));

					const unsigned int *maxRowLenght =
					    (static_cast<const unsigned int *>(slm.getSymbol(
					        "maxRowLengthconn_" + std::to_string(i))));
					convert_learned_weights_sparse(
					    populations[conn.pid_src()].size(),
					    populations[conn.pid_tar()].size(), weight_store,
					    weights, delay, cond_based, indices, rowlengths,
					    maxRowLenght);
				}
				catch (...) {
					convert_learned_weights(populations[conn.pid_src()].size(),
					                        populations[conn.pid_tar()].size(),
					                        weight_store, weights, delay,
					                        cond_based);
				}
			}
			conn.connector()._store_learned_weights(std::move(weight_store));
		}
	}

	teardown_spike_sources(populations, slm);
	record_spike_source(network, duration);
	auto end_t = std::chrono::steady_clock::now();
	store.already_compiled = true;

	if (timing) {
		global_logger().info(
		    "GeNN",
		    "initTime: " +
		        std::to_string(*(slm.template getScalar<double>("initTime"))));
		global_logger().info(
		    "GeNN", "initSparseTime: " +
		                std::to_string(*(
		                    slm.template getScalar<double>("initSparseTime"))));
		global_logger().info(
		    "GeNN", "neuronUpdateTime: " +
		                std::to_string(*(slm.template getScalar<double>(
		                    "neuronUpdateTime"))));
		global_logger().info(
		    "GeNN", "presynapticUpdateTime: " +
		                std::to_string(*(slm.template getScalar<double>(
		                    "presynapticUpdateTime"))));
		global_logger().info(
		    "GeNN", "postsynapticUpdateTime: " +
		                std::to_string(*(slm.template getScalar<double>(
		                    "postsynapticUpdateTime"))));
		global_logger().info(
		    "GeNN", "synapseDynamicsTime: " +
		                std::to_string(*(slm.template getScalar<double>(
		                    "synapseDynamicsTime"))));
	}
	network.runtime({std::chrono::duration<Real>(end_t - start_t).count(),
	                 std::chrono::duration<Real>(sim_fin_t - built_t).count(),
	                 std::chrono::duration<Real>(built_t - start_t).count(),
	                 std::chrono::duration<Real>(end_t - sim_fin_t).count(),
	                 std::chrono::duration<Real>(sim_fin_t - built_t).count()});
}
template <typename T>
void convert_learned_weights(size_t src_size, size_t tar_size,
                             std::vector<LocalConnection> &weight_store,
                             T *weights, Real delay, bool inhib_cond)
{
	for (size_t srcnid = 0; srcnid < src_size; srcnid++) {
		for (size_t tarnid = 0; tarnid < tar_size; tarnid++) {
			weight_store.push_back(LocalConnection(
			    srcnid, tarnid,
			    inhib_cond ? -weights[tar_size * srcnid + tarnid]
			               : weights[tar_size * srcnid + tarnid],
			    delay));
		}
	}
}

template <typename T>
void convert_learned_weights_sparse(size_t src_size, size_t,
                                    std::vector<LocalConnection> &weight_store,
                                    T *weights, Real delay, bool inhib_cond,
                                    unsigned int *indices,
                                    unsigned int *rowlengths,
                                    const unsigned int *maxRowLenght)
{
	for (size_t srcnid = 0; srcnid < src_size; srcnid++) {
		auto current_row_length = rowlengths[srcnid];
		for (size_t i = 0; i < current_row_length; i++) {
			auto tarnid = indices[i];
			auto weight = weights[(*maxRowLenght) * srcnid + i];
			weight_store.push_back(LocalConnection(
			    srcnid, tarnid, inhib_cond ? -weight : weight, delay));
		}
	}
}

void GeNN::do_run(NetworkBase &network, Real duration) const
{
	// General Setup
	if (!m_keep_compile) {
		m_storage->reset();
	}
	auto &model = *(m_storage->model);
	if (m_timing) {
		model.setTiming(true);
	}

	if (!(m_storage->already_compiled && m_keep_compile)) {
		if (m_double) {
			model.setPrecision(FloatType::GENN_DOUBLE);
		}
		else {
			model.setPrecision(FloatType::GENN_FLOAT);
		}
		model.setTimePrecision(TimePrecision::DEFAULT);
		model.setDT(m_timestep);  // Timestep in ms
		model.setMergePostsynapticModels(true);

		// Keep as much data as possible only on the device (GPU)
		model.setDefaultVarLocation(VarLocation::DEVICE);
		model.setDefaultSparseConnectivityLocation(VarLocation::DEVICE);

		std::string name = "genn_XXXXXX";
		filesystem::tmpfile(name);
		model.setName(name);
	}
	if (m_double) {
		do_run_templ<double>(network, duration, model, m_timestep, m_gpu,
		                     m_timing, m_keep_compile, *m_storage,
		                     m_disable_status, m_recording_buffer_size);
	}
	else {
		do_run_templ<float>(network, duration, model, m_timestep, m_gpu,
		                    m_timing, m_keep_compile, *m_storage,
		                    m_disable_status, m_recording_buffer_size);
	}
#ifdef NDEBUG
	if (!m_keep_compile) {
		system(("rm -r " + model.getName() + "_CODE").c_str());
	}
#endif
}
}  // namespace cypress
