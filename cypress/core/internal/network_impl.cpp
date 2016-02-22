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

#include <cypress/core/internal/network_impl.hpp>
#include <cypress/core/exceptions.hpp>

namespace cypress {
namespace internal {
NetworkImpl::NetworkImpl() : m_connections_sorted(true) {}

PopulationIndex NetworkImpl::create_population(
    size_t size, const NeuronType &type,
    const std::vector<NeuronParametersBase> &params, const std::string &name)
{
	m_populations.emplace_back(size, type, params, name);
	return m_populations.size();
}

std::vector<PopulationIndex> NetworkImpl::populations(const std::string &name,
                                                      const NeuronType &type)
{
	std::vector<PopulationIndex> res;
	for (size_t pid = 0; pid < m_populations.size(); pid++) {
		const PopulationImpl &pop = m_populations[pid];
		if ((name.empty() || pop.name == name) &&
		    (&type == &NullNeuronType::inst() || pop.type == &type)) {
			res.push_back(pid);
		}
	}
	return res;
}

void NetworkImpl::connect(const ConnectionDescriptor &descr)
{
	// Make sure the target population is not a spike source
	if (m_populations[descr.pid_tar()].type->spike_source) {
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

const std::vector<ConnectionDescriptor> &NetworkImpl::connections() const
{
	if (!m_connections_sorted) {
		std::sort(m_connections.begin(), m_connections.end());
		m_connections_sorted = true;
	}
	return m_connections;
}
}
}
