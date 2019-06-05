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
 * @file cypress.hpp
 *
 * Main header of cypress which includes all important headers from the
 * subfolders of the project.
 *
 * @author Andreas Stöckel
 */

#pragma once
#ifndef CYPRESS_HPP
#define CYPRESS_HPP

// Include first to avoid "_POSIX_C_SOURCE redefined" warning
#include <cypress/backend/pynn/pynn.hpp>

#include <cypress/backend/serialize/to_json.hpp>
#include <cypress/backend/nest/nest.hpp>
#include <cypress/backend/nmpi/nmpi.hpp>
#include <cypress/backend/resources.hpp>
#include <cypress/core/backend.hpp>
#include <cypress/core/connector.hpp>
#include <cypress/core/network.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/core/spike_time_generators.hpp>
#include <cypress/core/synapses.hpp>
#include <cypress/core/types.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/util/matplotlib_cpp.hpp>
#include <cypress/util/neuron_parameters.hpp>
#include <cypress/util/range.hpp>
#include <cypress/util/spiking_utils.hpp>

#endif /* CYPRESS_HPP */
