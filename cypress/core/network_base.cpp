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
#include <memory>

#include <cypress/core/backend.hpp>
#include <cypress/core/data.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/core/network_base_objects.hpp>
#include <cypress/core/transformation.hpp>

#include <cypress/backend/nest/nest.hpp>
#include <cypress/backend/nmpi/nmpi.hpp>
#include <cypress/backend/pynn/pynn.hpp>

#include <cypress/transformations/registry.hpp>

#include <cypress/util/json.hpp>

namespace cypress {
namespace internal {
/**
 * Spiking neural network data container. Stores all the data describing the
 * network.
 */
class NetworkData {
private:
	friend NetworkBase;

	/**
	 * Contains information about the runtime.
	 */
	NetworkRuntime m_runtime;

	/**
	 * Vector containing the PopulationData instances.
	 */
	std::vector<std::shared_ptr<PopulationData>> m_populations;

	/**
	 * Vector containing all connections.
	 */
	mutable std::vector<ConnectionDescriptor> m_connections;

	/**
	 * Flag indicating whether the connections are currently sorted.
	 */
	mutable bool m_connections_sorted;

public:
	/**
	 * Flag indicating whether lossy transformations should be used when
	 * executing the network.
	 */
	bool use_lossy_trafos;

	/**
	 * Set of disabled transformation identifiers.
	 */
	std::unordered_set<std::string> disabled_trafo_ids;

	/**
	 * Default constructor of the NetworkData class.
	 */
	NetworkData() : m_runtime({}), m_connections_sorted(true){};

	/**
	 * Creates an independent NetworkData instance.
	 */
	NetworkData clone()
	{
		NetworkData res = *this;
		for (auto &sp : res.m_populations) {
			sp = std::make_shared<PopulationData>(*sp);
		}
		return res;
	}

	/**
	 * Returns a reference at the vector containing the population data.
	 */
	std::vector<std::shared_ptr<PopulationData>> &populations()
	{
		return m_populations;
	}

	/**
	 * Returns the indices fo the populations which match the given search
	 * criteria.
	 */
	std::vector<PopulationIndex> populations(const std::string &name,
	                                         const NeuronType &type)
	{
		std::vector<PopulationIndex> res;
		for (size_t pid = 0; pid < m_populations.size(); pid++) {
			const PopulationData &pop = *m_populations[pid];
			if ((name.empty() || pop.name() == name) &&
			    (&type == &NullNeuron::inst() || pop.type() == &type)) {
				res.push_back(pid);
			}
		}
		return res;
	}

	/**
	 * Adds the given connector to the connection list.
	 */
	void connect(const ConnectionDescriptor &descr)
	{
		// Make sure the target population is not a spike source
		if (m_populations[descr.pid_tar()]->type()->spike_source) {
			throw InvalidConnectionException(
			    "Spike sources are not valid connection targets.");
		}

		// Assemble the connection descriptor and check its validity
		if (!descr.valid()) {
			throw InvalidConnectionException(
			    "The source and target population sizes do not match the size "
			    "expected by the chosen connector.");
		}

		// Append the descriptor to the connection list, update the sorted flag
		m_connections.emplace_back(std::move(descr));
		m_connections_sorted = (m_connections.size() <= 1) ||
		                       (m_connections_sorted &&
		                        m_connections[m_connections.size() - 2] <
		                            m_connections[m_connections.size() - 1]);
	}

	/**
	 * Returns the list of connections. Makes sure the returned connections are
	 * sorted.
	 */
	const std::vector<ConnectionDescriptor> &connections() const
	{
		if (!m_connections_sorted) {
			std::sort(m_connections.begin(), m_connections.end());
			m_connections_sorted = true;
		}
		return m_connections;
	}
};
}

/*
 * Class NetworkBase
 */

NetworkBase::NetworkBase() : m_impl(std::make_shared<internal::NetworkData>())
{
	// Do nothing here
}

NetworkBase::~NetworkBase() = default;

NetworkBase NetworkBase::clone() const
{
	return NetworkBase(
	    std::make_shared<internal::NetworkData>(m_impl->clone()));
}

void NetworkBase::connect(PopulationIndex pid_src, NeuronIndex nid_src0,
                          NeuronIndex nid_src1, PopulationIndex pid_tar,
                          NeuronIndex nid_tar0, NeuronIndex nid_tar1,
                          std::unique_ptr<Connector> connector)
{
	m_impl->connect(ConnectionDescriptor(pid_src, nid_src0, nid_src1, pid_tar,
	                                     nid_tar0, nid_tar1,
	                                     std::move(connector)));
}

PopulationIndex NetworkBase::create_population_index(
    size_t size, const NeuronType &type, const NeuronParameters &params,
    const NeuronSignals &signals, const std::string &name)
{
	// Create a new population data store
	auto data = std::make_shared<PopulationData>(size, &type, name);

	// Copy the given parameters to the new population
	NeuronParameters(data, 0, size) = params;
	NeuronSignals(data, 0, size) = signals;

	// Append the population the the existing list of populations
	m_impl->populations().emplace_back(data);
	return population_count() - 1;
}

size_t NetworkBase::population_count() const
{
	return m_impl->populations().size();
}

size_t NetworkBase::neuron_count() const
{
	size_t res = 0;
	for (const auto &pop : m_impl->populations()) {
		res += pop->size();
	}
	return res;
}

std::shared_ptr<PopulationData> NetworkBase::population_data(
    PopulationIndex pid)
{
	return m_impl->populations()[pid];
}

std::shared_ptr<const PopulationData> NetworkBase::population_data(
    PopulationIndex pid) const
{
	return m_impl->populations()[pid];
}

PopulationBase NetworkBase::population(const std::string &name)
{
	auto pops = populations(name);
	if (pops.empty()) {
		throw NoSuchPopulationException(std::string("Population with name \"") +
		                                name + "\" does not exist");
	}
	return pops.back();
}

PopulationBase NetworkBase::population(PopulationIndex pid)
{
	return PopulationBase(m_impl, pid);
}

const PopulationBase NetworkBase::population(PopulationIndex pid) const
{
	return PopulationBase(m_impl, pid);
}

PopulationBase NetworkBase::operator[](PopulationIndex pid)
{
	return population(pid);
}

const PopulationBase NetworkBase::operator[](PopulationIndex pid) const
{
	return population(pid);
}

const std::vector<PopulationBase> NetworkBase::populations(
    const std::string &name, const NeuronType &type) const
{
	std::vector<PopulationIndex> pids = m_impl->populations(name, type);
	std::vector<PopulationBase> res;
	for (PopulationIndex pid : pids) {
		res.emplace_back(*this, pid);
	}
	return res;
}

const std::vector<PopulationBase> NetworkBase::populations(
    const NeuronType &type) const
{
	return populations(std::string(), type);
}

const std::vector<ConnectionDescriptor> &NetworkBase::connections() const
{
	return m_impl->connections();
}

static std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	std::stringstream ss(s);
	std::string elem;
	while (getline(ss, elem, delim)) {
		elems.push_back(elem);
	}
	return elems;
}

static std::string join(const std::vector<std::string> &elems, char delim)
{
	bool first = true;
	std::stringstream ss;
	for (const std::string &elem : elems) {
		if (!first) {
			ss << delim;
		}
		first = false;
		ss << elem;
	}
	return ss.str();
}

std::unique_ptr<Backend> NetworkBase::make_backend(std::string backend_id,
                                                   int argc, const char *argv[],
                                                   Json setup)
{
	// Split the input string at the "=" -- the lefthand part contains the
	// backend identifier, the right-hand part possible Json simulator setup
	// data.
	size_t config_pos = backend_id.find("=");
	if (config_pos != std::string::npos) {
		if (!setup.is_null()) {
			throw std::invalid_argument(
			    "Setup data present in the backend identifier, but explicit "
			    "setup given in the call to make_backend()!");
		}
		setup = Json::parse(backend_id.substr(
		    config_pos + 1, backend_id.size() - config_pos + 1));
		backend_id = backend_id.substr(0, config_pos);
	}
	std::vector<std::string> elems = split(backend_id, '.');

	// Make sure the backend identifier is not empty
	if (elems.empty()) {
		throw std::invalid_argument("Backend ID must not be empty!");
	}

	// Forward calls starting with "nmpi" to the NMPI platform
	if (elems[0] == "nmpi") {
		if (argc <= 0 || !argv) {
			throw std::invalid_argument(
			    "Command line arguments were not passed to run(), but these "
			    "are required for the NMPI backend!");
		}
		elems.erase(elems.begin());  // Remove the first element
		if (elems.empty()) {
			throw std::invalid_argument(
			    "Expected another backend name following \"nmpi\"!");
		}
		auto backend =
		    std::move(make_backend(join(elems, '.'), argc, argv, setup));
		if (dynamic_cast<PyNN *>(backend.get()) == nullptr) {
			throw std::invalid_argument(
			    "NMPI backend only works in conjunction with PyNN backends!");
		}
		std::unique_ptr<PyNN> pynn_backend(dynamic_cast<PyNN *>(backend.get()));
		backend.release();
		return std::move(
		    std::make_unique<NMPI>(std::move(pynn_backend), argc, argv));
	}
	else if (elems[0] == "pynn") {
		elems.erase(elems.begin());  // Remove the first element
		if (elems.empty()) {
			throw std::invalid_argument(
			    "Expected another backend name following \"pynn\"!");
		}
		return std::make_unique<PyNN>(join(elems, '.'), setup);
	}
	else if (elems[0] == "nest") {
		return std::move(std::make_unique<NEST>(setup));
	}
	else {
		return std::move(std::make_unique<PyNN>(join(elems, '.'), setup));
	}
	return nullptr;
}

bool NetworkBase::use_lossy_trafos() const { return m_impl->use_lossy_trafos; }

void NetworkBase::use_lossy_trafos(bool use_lossy) const
{
	m_impl->use_lossy_trafos = use_lossy;
}

const std::unordered_set<std::string> &NetworkBase::disabled_trafo_ids() const
{
	return m_impl->disabled_trafo_ids;
}

std::unordered_set<std::string> &NetworkBase::disabled_trafo_ids()
{
	return m_impl->disabled_trafo_ids;
}

void NetworkBase::run(const Backend &backend, float duration)
{
	// Automatically deduce the duration if none was given
	if (duration <= 0) {
		duration = this->duration();
	}

	// Run the network through the transformation machinery, make sure all
	// transformations are registered
	transformations::register_();
	Transformations::run(backend, *this, TransformationAuxData{duration},
	                     m_impl->disabled_trafo_ids, m_impl->use_lossy_trafos);
}

void NetworkBase::run(const std::string &backend_id, float duration, int argc,
                      const char *argv[])
{
	auto backend = make_backend(backend_id, argc, argv);
	run(*backend, duration);
}

float NetworkBase::duration() const
{
	float res = 0.0;
	for (const auto &population : populations()) {
		if (&population.type() == &SpikeSourceArray::inst()) {
			const NeuronIndex nid_end =
			    population.homogeneous_parameters() ? 1 : population.size();
			for (NeuronIndex nid = 0; nid < nid_end; nid++) {
				auto &params = population[nid].parameters().parameters();
				if (params.size() > 0) {
					// Note: the spike times are supposed to be sorted!
					res = std::max(res, params[params.size() - 1]);
				}
			}
		}
	}
	return res;
}

NetworkRuntime NetworkBase::runtime() const { return m_impl->m_runtime; }

void NetworkBase::runtime(const NetworkRuntime &runtime)
{
	m_impl->m_runtime = runtime;
}
}
