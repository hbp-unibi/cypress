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

#include <cypress/core/network.hpp>
#include <cypress/core/backend.hpp>

namespace cypress {

namespace {
// TODO: Move to neurons.hpp
/**
 * Internally used parameter type with no parameters.
 */
class NullNeuronParameters final : public NeuronParametersBase {
public:
	NullNeuronParameters() : NeuronParametersBase(std::vector<float>{}) {}
};

/**
 * Internally used neuron type representing no neuron type.
 */
class NullNeuronType final : public NeuronType {
private:
	NullNeuronType() : NeuronType(0, "", {}, {}, {}, false, false) {}

public:
	using Parameters = NullNeuronParameters;

	static const NullNeuronType &inst()
	{
		static const NullNeuronType inst;
		return inst;
	}
};
}

/*
 * Class PopulationImpl
 */

class PopulationImpl {
private:
	/**
	 * Index of this population within the network instance.
	 */
	size_t m_idx;

	/**
	 * Size of the population.
	 */
	size_t m_size;

	/**
	 * Reference at the underlying neuron type.
	 */
	NeuronType const *m_type;

	/**
	 * Parameters shared by all neurons in the population.
	 */
	NeuronParametersBase m_parameters;

	/**
	 * Name of the population.
	 */
	std::string m_name;

public:
	/**
	 * Default constructor of the PopulationImpl class.
	 */
	PopulationImpl()
	    : m_idx(0),
	      m_size(0),
	      m_type(&NullNeuronType::inst()),
	      m_parameters(NullNeuronType::Parameters()),
	      m_name("")
	{
	}

	PopulationImpl(size_t idx, size_t size, const NeuronType &type,
	               const NeuronParametersBase &parameters,
	               const std::string &name)
	    : m_idx(idx),
	      m_size(size),
	      m_type(&type),
	      m_parameters(parameters),
	      m_name(name)
	{
	}

	const NeuronType &type() const { return *m_type; }

	size_t idx() const { return m_idx; }

	size_t size() const { return m_size; }

	const std::string &name() const { return m_name; }

	void name(const std::string &name)
	{
		m_name = name;  // TODO: Update network
	};

	const NeuronParametersBase &parameters() const { return m_parameters; }

	bool homogeneous() const { return true; }
};

/**
 * Class NetworkImpl
 */

/**
 * The NetworkImpl class contains the actual implementation of the Network
 * graph.
 */
class NetworkImpl {
private:
	/**
	 * Vector containing the PopulationImpl instances. The clon_ptr wrapper
	 * is used to make sure the PopulationImpl pointers stored in the
	 * PopulationBase instances stays valid when the vector is altered.
	 */
	std::vector<clone_ptr<PopulationImpl>> m_populations;

	/**
	 * Vector containing all connections.
	 */
	std::vector<ConnectionDescriptor> m_connections;

public:
	std::vector<PopulationImpl *> populations(const std::string &name,
	                                          const NeuronType *type)
	{
		std::vector<PopulationImpl *> res;
		for (auto &pop : m_populations) {
			if ((name.empty() || pop->name() == name) &&
			    (type == nullptr || &pop->type() == type)) {
				res.push_back(pop.ptr());
			}
		}
		return res;
	}

	void connect(size_t pid_src, size_t nid_src0, size_t nid_src1,
	             size_t pid_tar, size_t nid_tar0, size_t nid_tar1,
	             std::unique_ptr<Connector> connector)
	{
		m_connections.emplace_back(pid_src, nid_src0, nid_src1, pid_tar,
		                           nid_tar0, nid_tar1, std::move(connector));
	}

	const std::vector<ConnectionDescriptor> &connections() const
	{
		return m_connections;
	}

	PopulationImpl &create_population(size_t size, const NeuronType &type,
	                                  const NeuronParametersBase &params,
	                                  const std::string &name)
	{
		m_populations.emplace_back(make_clone<PopulationImpl>(
		    m_populations.size(), size, type, params, name));
		return *m_populations.back();
	}

	float duration() const
	{
		float res = 0.0;
		for (const auto &population : m_populations) {
			if (&population->type() == &SpikeSourceArray::inst()) {
				auto &params = population->parameters();
				if (params.size() > 0) {
					// The spike times are supposed to be sorted!
					res = std::max(res, params[params.size() - 1]);
				}
			}
		}
		return res;
	}
};

/*
 * Class PopulationBase
 */

const NeuronType &PopulationBase::type() const { return m_impl.type(); }

size_t PopulationBase::size() const { return m_impl.size(); }

const std::string &PopulationBase::name() const { return m_impl.name(); }

bool PopulationBase::homogeneous() const { return m_impl.homogeneous(); }

/**
 * Class Network
 */

Network::Network() : m_impl(make_clone<NetworkImpl>()) {}
Network::Network(const Network &) = default;
Network::Network(Network &&) noexcept = default;
Network &Network::operator=(const Network &) = default;
Network &Network::operator=(Network &&) = default;

Network::~Network()
{
	// Only required for the unique ptr to NetworkImpl
}

Network &Network::connect(PopulationBase &src, size_t nid_src0, size_t nid_src1,
                          PopulationBase &tar, size_t nid_tar0, size_t nid_tar1,
                          std::unique_ptr<Connector> connector)
{
	// Make sure both belong to the same network instance
	if (&src.network() != this || &tar.network() != this) {
		throw InvalidConnectionException(
		    "Source and target population must belong to the same network "
		    "instance!");
	}

	// Call the connect method in the network implementation
	m_impl->connect(src.impl().idx(), nid_src0, nid_src1, tar.impl().idx(),
	                nid_tar0, nid_tar1, std::move(connector));

	return *this;
}

const std::vector<ConnectionDescriptor> &Network::connections() const
{
	return m_impl->connections();
}

std::vector<PopulationImpl *> Network::populations(const std::string &name,
                                                   const NeuronType *type)
{
	return m_impl->populations(name, type);
}

float Network::duration() const { return m_impl->duration(); }

PopulationImpl &Network::create_population(size_t size, const NeuronType &type,
                                           const NeuronParametersBase &params,
                                           const std::string &name)
{
	return m_impl->create_population(size, type, params, name);
}

Network &Network::run(const Backend &backend, float duration)
{
	backend.run(*this, duration);
	return *this;
}
}
