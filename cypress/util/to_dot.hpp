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

#pragma once

#include <cypress/core/network_base.hpp>

namespace cypress {
/**
 * @brief Generates a dot file for the given network. Calls "dot" to generate a
 * pdf with a visualization of the network and list connectors
 *
 * @param netw network instance
 * @param graph_label label of the graph, Defaults to "Network Architecture".
 * @param filename filename to store dot file Defaults to "graph.dot".
 * @param call_dot call dot? output will be filename".pdf" Defaults to true.
 */
void create_dot(const NetworkBase &netw,
                const std::string graph_label = "Network Architecture",
                const std::string filename = "graph.dot",
                const bool call_dot = true);

}  // namespace cypress
