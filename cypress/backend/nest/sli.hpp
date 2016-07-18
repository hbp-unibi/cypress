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
 * @file sli.hpp
 *
 * Contains some functions to turn a network description into a NEST SLI
 * program and to parse the program response.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_BACKEND_SLI_HPP
#define CYPRESS_BACKEND_SLI_HPP

#include <iosfwd>

#include <cypress/core/network_base.hpp>

namespace cypress {
namespace sli {
/**
 * Turns the given network into an SLI program and writes it into the given
 * output stream.
 *
 * @param os is the output stream to which the SLI script should be written.
 * @param net is the network that should be turned into an SLI script.
 * @param duration is the network simulation time in seconds.
 */
void write_network(std::ostream &os, const NetworkBase &net, float duration);

/**
 * Reads the response of the previously written SLI program.
 *
 * @param is is the input stream from which the response should be read. Note
 * that the response must be produced by an SLI program previously produced by
 * write_network().
 * @param net is the network into which the response should be written.
 * @param errs is the stream to which error messages should be written.
 */
void read_response(std::istream &is, NetworkBase &net, std::ostream &errs);
}
}

#endif /* CYPRESS_BACKEND_SLI_HPP */
