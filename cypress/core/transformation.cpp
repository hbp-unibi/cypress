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
#include <tuple>
#include <unordered_map>

#include <cypress/core/backend.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/transformation.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/core/network_base_objects.hpp>

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
		if (&src[i].type() != &tar[i].type()) {
			throw TransformationException(
			    "Neron type was changed, cannot copy results!");
		}
		tar[i].signals() = src[i].signals();
	}
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

static std::unordered_set<const NeuronType *> find_unsupported_neuron_types(
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
	return res;
}
}

std::vector<TransformationCtor>
Transformations::construct_neuron_type_transformation_chain(
    const std::unordered_set<const NeuronType *> &unsupported_types,
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

		// Initialize the "cost" vector
		std::vector<size_t> cost(nodes.size(), MAX_VAL);
		std::vector<size_t> prev(nodes.size(), MAX_VAL);
		std::vector<bool> visited(nodes.size(), false);
		cost[start_idx] = 0;

		// Add the start node to the priority queue, execute the Dijkstra
		// algorithm
		std::priority_queue<std::tuple<size_t, size_t>> q;
		q.emplace(0, start_idx);
		while (!q.empty()) {
			const size_t u = std::get<1>(q.top());
			q.pop();
			if (visited[u]) {
				continue;
			}
			for (size_t edge_idx : nodes[u].edges) {
				if (edges[edge_idx].lossy && !use_lossy) {
					continue;
				}
				const size_t alt = cost[u] + edges[edge_idx].cost;
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
		size_t min_cost = MAX_VAL;
		for (size_t i = 0; i < cost.size(); i++) {
			if (nodes[i].supported && cost[i] < min_cost) {
				min_cost = cost[i];
				end_idx = i;
			}
		}

		// Create the resulting path by backtracking
		std::vector<size_t> path;
		if (min_cost != MAX_VAL) {
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
	std::vector<std::vector<TransformationCtor>> paths;
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

			// Find a shortest path to a supported neurons, try again with lossy
			// transformations if this does not yield a result
			std::vector<size_t> res = dijkstra(start_idx, false);
			if (res.empty() && use_lossy) {
				res = dijkstra(start_idx, true);
			}

			// Add the path to the path list, mark visited nodes as "supported"
			if (!res.empty()) {
				std::vector<TransformationCtor> path;
				size_t idx = start_idx;
				nodes[idx].supported = true;
				for (size_t i = 0; i < res.size(); i++) {
					idx = edges[res[i]].tar_idx;
					nodes[idx].supported = true;
					path.emplace_back(std::get<0>(transformations[res[i]]));
				}
				paths.emplace_back(path);
				continue;
			}
		}
		throw NotSupportedException(
		    "The neuron type " + unsupported_type->name +
		    " is not supported by the backend and no transformation to a "
		    "supported neuron type was found.");
	}

	// Step 3: Extract the transformations from the paths
	std::vector<TransformationCtor> res;
	for (ssize_t i = paths.size() - 1; i >= 0; i--) {
		res.insert(res.end(), paths[i].begin(), paths[i].end());
	}
	return res;
}

void Transformations::execute(
    const NetworkBase &network, const Backend &backend,
    const std::unordered_set<std::string> &disabled_trafo_ids, bool use_lossy)
{
	// List of transformations that should be executed
	std::vector<TransformationCtor> trafos;

	// Fetch all neuron type transformations which are not disabled
	auto available_neuron_trafos =
	    TransformationRegistry::get_neuron_type_transformations();
	for (auto it = available_neuron_trafos.begin();
	     it != available_neuron_trafos.end(); it++) {
		if (disabled_trafo_ids.count(std::get<0> (*it)()->id()) > 0) {
			it = available_neuron_trafos.erase(it);
		}
	}

	// Iterate over the network and find neuron types which are not supported
	// by the backend
	const std::unordered_set<const NeuronType *> supported_types =
	    backend.supported_neuron_types();
	const std::unordered_set<const NeuronType *> unsupported_types =
	    find_unsupported_neuron_types(network, supported_types);
	if (!unsupported_types.empty()) {
		std::vector<TransformationCtor> neuron_trafos =
		    construct_neuron_type_transformation_chain(
		        unsupported_types, supported_types, available_neuron_trafos,
		        use_lossy);
		trafos.insert(trafos.end(), neuron_trafos.begin(), neuron_trafos.end());
	}
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
