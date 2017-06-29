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

#include <algorithm>
#include <queue>
#include <limits>
#include <mutex>
#include <stack>
#include <tuple>
#include <unordered_map>

#include <cypress/core/backend.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/transformation.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/core/network_base_objects.hpp>
#include <cypress/util/logger.hpp>

namespace cypress {

/*
 * Class Transformation
 */

void Transformation::do_copy_results(const NetworkBase &src,
                                     NetworkBase &tar) const
{
	// Make sure the source and the target population are of the same size
	if (src.population_count() != tar.population_count()) {
		throw TransformationException(
		    "Source and target network do not have an equal population count, "
		    "cannot copy results!");
	}

	// Copy the recorded data from the source to the target network
	for (size_t i = 0; i < src.population_count(); i++) {
		if (&src[i].type() == &tar[i].type()) {
			tar[i].signals() = src[i].signals();
			continue;
		}
		// Map the source signals to signals with the same name in the
		// target neuron
		const NeuronType &src_type = src[i].type();
		const NeuronType &tar_type = tar[i].type();
		for (size_t j = 0; j < src_type.signal_names.size(); j++) {
			// Make sure this signal was recorded in the source neuron
			if (src[i].homogeneous_record() &&
			    !src[i].signals().is_recording(j)) {
				continue;
			}
			else if (!src[i].homogeneous_record())
            {
                bool record = false;
                for(size_t k =0; k<src[i].size(); k++){
                    if(src[i][k].signals().is_recording(j)){
                        record = true;
                        break;
                    }
                }
                if (!record){
                    continue;
                }
                    
            }

			// Find the target index
			const std::string &signal_name = src_type.signal_names[j];
			auto idx = tar_type.signal_index(signal_name);
			if (!idx.valid()) {
				throw TransformationException("Cannot find signal " +
				                              signal_name +
				                              " in target population");
			}

			// Copy the data from the source to the target neuron
			for (size_t k = 0; k < src[i].size(); k++) {
				if (!src[i][k].signals().is_recording(j)) {
					continue;
				}
				tar[i][k].signals().data(idx.value(),
				                         src[i][k].signals().data_ptr(j));
			}
		}
	}
}

NetworkBase Transformation::transform(const NetworkBase &src,
                                      TransformationAuxData &aux)
{
	return do_transform(src, aux);
}

void Transformation::copy_results(const NetworkBase &src,
                                  NetworkBase &tar) const
{
	do_copy_results(src, tar);
}

/**
 * Class Transformations
 */

namespace {
/**
 * Internally used static class which is used to access the global
 * transformation registry.
 */
struct TransformationRegistry {
	using GeneralTransformations =
	    std::vector<std::tuple<TransformationCtor, TransformationTest>>;
	using NeuronTypeTransformations = std::vector<
	    std::tuple<TransformationCtor, const NeuronType *, const NeuronType *>>;

	static std::mutex mutex;

	static RegisteredTransformation trafo_id;

	static GeneralTransformations &get_general_transformations()
	{
		static GeneralTransformations general_transformations;
		return general_transformations;
	}

	static NeuronTypeTransformations &get_neuron_type_transformations()
	{
		static NeuronTypeTransformations neuron_type_transformations;
		return neuron_type_transformations;
	}

	static RegisteredTransformation make_registered_transformation()
	{
		std::lock_guard<std::mutex> lock(mutex);
		return ++trafo_id;
	}
};

RegisteredTransformation TransformationRegistry::trafo_id;
std::mutex TransformationRegistry::mutex;

struct NeuronTypeEdge {
	size_t src_idx;
	size_t tar_idx;
	size_t cost;
	bool lossy;
};

struct NeuronTypeNode {
	const NeuronType *type;
	bool supported;
	std::vector<size_t> edges;
};

static std::vector<const NeuronType *> find_unsupported_neuron_types(
    const NetworkBase &network,
    const std::unordered_set<const NeuronType *> &supported_neuron_types)
{
	std::unordered_set<const NeuronType *> res;
	for (const PopulationBase &population : network.populations()) {
		if (supported_neuron_types.find(&population.type()) ==
		    supported_neuron_types.end()) {
			res.emplace(&population.type());
		}
	}
	return std::vector<const NeuronType *>(res.begin(), res.end());
}
}

std::vector<TransformationCtor>
Transformations::construct_neuron_type_transformation_chain(
    const std::vector<const NeuronType *> &unsupported_types,
    const std::unordered_set<const NeuronType *> &supported_types,
    const std::vector<std::tuple<TransformationCtor, const NeuronType *,
                                 const NeuronType *>> &transformations,
    bool use_lossy)
{
	std::vector<NeuronTypeNode> nodes;
	std::vector<NeuronTypeEdge> edges;
	std::unordered_map<const NeuronType *, size_t> node_idx_map;

	// Implementation of the Dijkstra algorithm to find the shortest path from
	// a source node to a supported node
	auto dijkstra = [&](const size_t &start_idx, bool use_lossy) {
		const size_t MAX_VAL = std::numeric_limits<size_t>::max();

		// Initialize the "cost" vector -- the cost consists of a flag
		// indicating whether a lossy node was crossed and the actual cost
		using Cost = std::tuple<bool, size_t>;
		std::vector<Cost> cost(nodes.size(), Cost{true, MAX_VAL});
		std::vector<size_t> prev(nodes.size(), MAX_VAL);
		std::vector<bool> visited(nodes.size(), false);
		cost[start_idx] = Cost{false, 0};

		// Add the start node to the priority queue, execute the Dijkstra
		// algorithm
		using T = std::tuple<Cost, size_t>;
		std::priority_queue<T, std::vector<T>, std::greater<T>> q;
		q.emplace(cost[start_idx], start_idx);
		while (!q.empty()) {
			const Cost cu = std::get<0>(q.top());
			const size_t u = std::get<1>(q.top());
			q.pop();
			if (visited[u]) {
				continue;
			}
			for (size_t edge_idx : nodes[u].edges) {
				if (edges[edge_idx].lossy && !use_lossy) {
					continue;
				}
				const Cost alt = Cost(edges[edge_idx].lossy || std::get<0>(cu),
				                      std::get<1>(cu) + edges[edge_idx].cost);
				const size_t v = edges[edge_idx].tar_idx;
				if (alt < cost[v]) {
					cost[v] = alt;
					prev[v] = edge_idx;
					q.emplace(alt, v);
				}
			}
			visited[u] = true;
		}

		// Find the node with minimum cost which is supported by the backend
		size_t end_idx = 0;
		Cost min_cost = Cost{true, MAX_VAL};
		for (size_t i = 0; i < cost.size(); i++) {
			if (nodes[i].supported && cost[i] < min_cost) {
				min_cost = cost[i];
				end_idx = i;
			}
		}

		// Create the resulting path by backtracking
		std::vector<size_t> path;
		if (min_cost != Cost{true, MAX_VAL}) {
			size_t idx = end_idx;
			while (idx != start_idx) {
				if (prev[idx] == MAX_VAL) {
					return std::vector<size_t>{};
				}
				path.emplace_back(prev[idx]);
				idx = edges[prev[idx]].src_idx;
			}
		}
		return path;
	};

	// Step 1: Build the search graph
	{
		// Function returning the index of a node for the given type, creates
		// the node if it does not exist yet
		auto node_idx = [&](const NeuronType *type) {
			auto it = node_idx_map.find(type);
			if (it == node_idx_map.end()) {
				it = node_idx_map.emplace(type, nodes.size()).first;
				nodes.emplace_back(
				    NeuronTypeNode{type, supported_types.count(type) > 0, {}});
			}
			return it->second;
		};

		// Iterate over all transformations and create corresponding links
		// between the nodes
		for (size_t i = 0; i < transformations.size(); i++) {
			const size_t src_idx = node_idx(std::get<1>(transformations[i]));
			const size_t tar_idx = node_idx(std::get<2>(transformations[i]));
			const TransformationProperties props =
			    std::get<0>(transformations[i])()->properties();
			edges.emplace_back(
			    NeuronTypeEdge{src_idx, tar_idx, props.cost, props.lossy});
			nodes[src_idx].edges.emplace_back(i);
		}
	}

	// Step 2: Try to find the shortest path from an unsupported type to a
	// supported type
	std::vector<TransformationCtor> path;
	for (const NeuronType *unsupported_type : unsupported_types) {
		// Find the start node
		const auto it = node_idx_map.find(unsupported_type);
		if (it != node_idx_map.end()) {
			// If the current node is already marked as supported, there is
			// nothing left to do. Continue.
			const size_t start_idx = it->second;
			if (nodes[start_idx].supported) {
				continue;
			}

			// Find a shortest path to a supported neuron
			std::vector<size_t> res = dijkstra(start_idx, use_lossy);

			// Add the path to the path list, mark visited nodes as "supported"
			if (!res.empty()) {
				size_t idx = start_idx;
				nodes[idx].supported = true;
				for (size_t i = 0; i < res.size(); i++) {
					idx = edges[res[i]].tar_idx;
					nodes[idx].supported = true;
					path.emplace_back(std::get<0>(transformations[res[i]]));
				}
				continue;
			}
		}
		throw NotSupportedException(
		    "The neuron type " + unsupported_type->name +
		    " is not supported by the backend and no transformation to a "
		    "supported neuron type was found.");
	}

	// Step 3: Reverse the execution order and return the resulting path
	std::reverse(path.begin(), path.end());
	return path;
}

void Transformations::run(const Backend &backend, NetworkBase network,
                          const TransformationAuxData &aux,
                          std::unordered_set<std::string> disabled_trafo_ids,
                          bool use_lossy)
{
	// Iterate over the network and find neuron types which are not
	// supported by the backend
	const std::unordered_set<const NeuronType *> supported_types =
	    backend.supported_neuron_types();
	const std::vector<const NeuronType *> unsupported_types =
	    find_unsupported_neuron_types(network, supported_types);

	bool done;
	do {
		// Fetch all neuron type transformations which are not disabled
		auto available_neuron_trafos =
		    TransformationRegistry::get_neuron_type_transformations();
		for (auto it = available_neuron_trafos.begin();
		     it != available_neuron_trafos.end(); it++) {
			if (disabled_trafo_ids.count(std::get<0> (*it)()->id()) > 0) {
				it = available_neuron_trafos.erase(it);
			}
		}

		// Add the neuron transformations to the transformation list
		std::vector<TransformationCtor> neuron_trafos;
		if (!unsupported_types.empty()) {
			neuron_trafos = construct_neuron_type_transformation_chain(
			    unsupported_types, supported_types, available_neuron_trafos,
			    use_lossy);
		}

		// Apply the neuron transformations, store each intermediate network on
		// the "networks" stack
		TransformationAuxData aux_cpy = aux;
		std::stack<NetworkBase> networks;
		std::stack<std::unique_ptr<Transformation>> trafos;

		// Applies a single transformation and returns true if it succeeded
		auto apply_trafo = [&](std::unique_ptr<Transformation> trafo) {
			try {
				// Try to apply the transformation. If this fails for some
				// reason, skip this transformation in the next run and try
				// again
				trafos.emplace(std::move(trafo));
				if (trafos.top()->properties().lossy) {
					networks.top().logger().warn(
					    "cypress",
					    "Executing lossy transformation " + trafos.top()->id());
				}
				else {
					networks.top().logger().info(
					    "cypress",
					    "Executing transformation " + trafos.top()->id());
				}
				networks.emplace(std::move(
				    trafos.top()->transform(networks.top(), aux_cpy)));
			}
			catch (TransformationException e) {
				networks.top().logger().warn(
				    "cypress", "Error while executing the transformation " +
				                   trafos.top()->id() + ": " + e.what());
				networks.top().logger().info(
				    "cypress",
				    "Disabling this transformation and trying again.");
				disabled_trafo_ids.emplace(trafos.top()->id());
				return false;
			}
			return true;
		};

		// Applies all transformations, runs the network, and copyies the data
		// back to the original network
		auto apply_trafos = [&]() {
			for (const auto &ctor : neuron_trafos) {
				if (!apply_trafo(std::move(ctor()))) {
					return false;
				}
			}

			// Apply all general transformations
			for (const auto &descr :
			     TransformationRegistry::get_general_transformations()) {
				std::unique_ptr<Transformation> trafo =
				    std::move(std::get<0>(descr)());
				if (disabled_trafo_ids.count(trafo->id()) > 0) {
					continue;  // Skip disabled transformations
				}

				// Check whether the transformation applies to the current
				// network and backend, if yes, execute it
				if (std::get<1>(descr)(backend, networks.top()) &&
				    !apply_trafo(std::move(trafo))) {
					return false;
				}
			}
			return true;
		};

		// Try to apply all transformations, if one fails, blacklist it and try
		// again
		networks.emplace(network);
		if ((done = apply_trafos())) {
			// Run the network
			backend.run(networks.top(), aux_cpy.duration);

			// Copy the data back to the original network
			while (!trafos.empty()) {
				// Fetch the network on top of the stack as source network
				NetworkBase network_src = networks.top();
				networks.pop();

				// Copy the results from the source network to the next network
				// on the stack
				trafos.top()->copy_results(network_src, networks.top());

				// Copy the runtime info
				networks.top().runtime(network_src.runtime());

				// Erase the current network
				trafos.pop();
			}
		}
	} while (!done);
}

RegisteredTransformation Transformations::register_general_transformation(
    TransformationCtor ctor,
    std::function<bool(const Backend &, const NetworkBase &)> test)
{
	TransformationRegistry::get_general_transformations().emplace_back(ctor,
	                                                                   test);
	return TransformationRegistry::make_registered_transformation();
}

RegisteredTransformation Transformations::register_neuron_type_transformation(
    TransformationCtor ctor, const NeuronType &src, const NeuronType &tar)
{
	TransformationRegistry::get_neuron_type_transformations().emplace_back(
	    ctor, &src, &tar);
	return TransformationRegistry::make_registered_transformation();
}
}
