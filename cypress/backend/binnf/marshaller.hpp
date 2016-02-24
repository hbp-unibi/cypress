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

/**
 * @file marshaller.hpp
 *
 * This header declares the "marshaller" function which is responsible for
 * turning a Network instance into a binnf stream and reading the simulator
 * response back into the Network instance.
 *
 * @author Andreas Stöckel
 */

#pragma once

#ifndef CYPRESS_BINNF_MARSHALLER_HPP
#define CYPRESS_BINNF_MARSHALLER_HPP

#include <iosfwd>
#include <functional>

namespace cypress {

/*
 * Forward declarations.
 */
class NetworkBase;
class Connection;

namespace binnf {
/**
 * Writes the input network as binnf to the given output stream.
 *
 * @param net_in is the network description which is read and serialised into
 * the output stream.
 * @param os is the output stream to which the input network is written.
 * @param connection_trafo is a callback function that can be used to transform
 * the list of connections according to the capabilities of the target platform.
 */
void marshall_network(NetworkBase &net, std::ostream &os,
                      std::function<size_t(Connection connections[],
                                           size_t count)> connection_trafo =
                          [](Connection[], size_t count) -> size_t {
	                      return count;
	                  });

/**
 * Ten waits for a response from the simulator on the the given input stream and
 * writes the experiment result into the given entwork. Throws an exception if
 * an error occurs.
 *
 * @param net is the network description into which the results should be
 *  written.
 * @param is is the input stream from which the simulator response is read and
 * written into the output network.
 * @return true if valid result data was received from the simulator
 */
bool marshall_response(NetworkBase &net, std::istream &is);
}
}

#endif /* CYPRESS_BINNF_MARSHALLER_HPP */
