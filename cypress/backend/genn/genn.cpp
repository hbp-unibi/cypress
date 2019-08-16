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
#include <../genn/include/genn/backends/single_threaded_cpu/optimiser.h>
#include <../genn/include/genn/genn/code_generator/generateAll.h>
#include <../genn/include/genn/genn/code_generator/generateMakefile.h>
#include <../genn/include/genn/genn/modelSpecInternal.h>  // TODO
#include <../genn/include/genn/third_party/path.h>
#include <../genn/pygenn/genn_wrapper/include/SharedLibraryModel.h>

#include <fstream>

#include <cypress/backend/genn/genn.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/neurons.hpp>

#include <cypress/core/network_base_objects.hpp>

#define UNPACK7(v) v[0], v[1], v[2], v[3], v[4], v[5], v[6]

std::vector<float> spikeTimes{5.0, 7.0};
namespace cypress {
GeNN::GeNN(const Json &setup)
{
	if (setup.count("timestep") > 0) {
		m_timestep = setup["timestep"].get<Real>();
	}
	if (setup.count("gpu") > 0) {
		m_gpu = setup["gpu"].get<float>();
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

InitSparseConnectivitySnippet::Init genn_connector(std::string name,
                                                   SynapseMatrixType &type,
                                                   bool allow_self_connections,
                                                   Real additional_parameter)
{
	type = SynapseMatrixType::BITMASK_GLOBALG;
	if (name == "AllToAllConnector") {
		if (allow_self_connections) {
			return initConnectivity<
			    InitSparseConnectivitySnippet::FixedProbability>(1.0);
		}
		else {
			return initConnectivity<
			    InitSparseConnectivitySnippet::FixedProbabilityNoAutapse>(1.0);
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
	else if (conn.connector().name() == "FixedFanOutConnector") {
	    *connector =
	        initConnectivity<InitSparseConnectivitySnippet::OneToOne>();
	}*/
	else if (name == "RandomConnector") {
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
	else {
		// TODO List Connector
		throw ExecutionError("Chosen Connector is not yet supported by GeNN");
	}
}

SynapseGroup *group_connect(const std::vector<PopulationBase> pops,
                            ModelSpecInternal &model,
                            const ConnectionDescriptor &conn, std::string name,
                            Real timestep)
{
	// pops[conn.pid_tar()].homogeneous_parameters();
	// TODO : inhomogenous tau_syn and E_rev, popviews

	bool cond_based = pops[conn.pid_tar()].type().conductance_based;
	Real weight = conn.connector().synapse()->parameters()[0];
	unsigned int delay =
	    (unsigned int)(conn.connector().synapse()->parameters()[1] / timestep);

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
			}
			SynapseMatrixType type;
			auto connector =
			    genn_connector(conn.connector().name(), type,
			                   conn.connector().allow_self_connections(),
			                   conn.connector().additional_parameter());

			return model.addSynapsePopulation<WeightUpdateModels::StaticPulse,
			                                  PostsynapticModels::ExpCond>(
			    name, type, delay, "pop_" + std::to_string(conn.pid_src()),
			    "pop_" + std::to_string(conn.pid_tar()), {}, {weight},
			    {tau, rev_pot}, {}, connector);
		}
		else {
			throw ExecutionError(
			    "Learning synapses are currently not supported");
		}
	}
	else {
		throw ExecutionError(
		    "Current-based models are currently not supported");
	}
}

void GeNN::do_run(NetworkBase &network, Real duration) const
{
	ModelSpecInternal model;
	model.setPrecision(FloatType::GENN_FLOAT);
	model.setTimePrecision(TimePrecision::DEFAULT);
	model.setDT(m_timestep);  // Timestep in ms
	model.setMergePostsynapticModels(true);
	model.setName("cypressnet");  // TODO random net

	auto &populations = network.populations();
	std::vector<NeuronGroup *> neuron_groups;
	for (size_t i = 0; i < populations.size(); i++) {
		neuron_groups.emplace_back(create_neuron_group(
		    populations[i], model, ("pop_" + std::to_string(i))));
	}

	std::vector<SynapseGroup *> synapse_groups;
	auto &connections = network.connections();
	for (size_t i = 0; i < connections.size(); i++) {
		synapse_groups.emplace_back(
		    group_connect(populations, model, connections[i],
		                  "conn_" + std::to_string(i), m_timestep));
	}

	model.finalize();

	std::string path = "./" + model.getName() + "_CODE/";
	mkdir(path.c_str(), 0777);
	if (m_gpu) {
		auto bck = CodeGenerator::CUDA::Optimiser::createBackend(
		    model, filesystem::path("test_netw/"), 0, {});
		auto moduleNames = CodeGenerator::generateAll(model, bck, path, false);
		std::ofstream makefile(path + "Makefile");
		CodeGenerator::generateMakefile(makefile, bck, moduleNames);
		makefile.close();
	}
	else {
		auto bck = CodeGenerator::SingleThreadedCPU::Optimiser::createBackend(
		    model, filesystem::path(path), 0, {});
		auto moduleNames = CodeGenerator::generateAll(model, bck, path, false);
		std::ofstream makefile(path + "Makefile");
		CodeGenerator::generateMakefile(makefile, bck, moduleNames);
		makefile.close();
	}

	system(("make -j 4 -C " + path + " &>/dev/null").c_str());

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

	slm.initializeSparse();

	// getPointers
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
