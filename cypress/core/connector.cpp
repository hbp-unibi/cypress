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

#include <cypress/core/connector.hpp>

namespace cypress {

/**
 * Class Connector
 */

Connector::Connector() = default;

Connector::~Connector() = default;

std::unique_ptr<AllToAllConnector> Connector::all_to_all(float weight,
                                                         float delay)
{
	return std::move(std::make_unique<AllToAllConnector>(weight, delay));
}

/**
 * Class AllToAllConnector
 */

const std::string AllToAllConnector::m_name = "AllToAllConnector";

AllToAllConnector::AllToAllConnector(float weight, float delay)
    : m_weight(weight), m_delay(delay)
{
}

AllToAllConnector::~AllToAllConnector() {}

size_t AllToAllConnector::connect(const ConnectionDescriptor &descr,
                                  Connection tar_mem[])
{
	size_t i = 0;
	for (NeuronIndex src = descr.nid_src0(); src < descr.nid_src1(); src++) {
		for (NeuronIndex tar = descr.nid_tar0(); tar < descr.nid_tar1(); tar++) {
			tar_mem[i++] = Connection(descr.pid_src(), descr.pid_tar(), src,
			                          tar, m_weight, m_delay);
		}
	}
	return i;
}

size_t AllToAllConnector::connection_count(const ConnectionDescriptor &descr)
{
	return descr.nsrc() * descr.ntar();
}

bool AllToAllConnector::connection_valid(const ConnectionDescriptor &)
{
	return true;
}

const std::string &AllToAllConnector::name() { return m_name; }
}
