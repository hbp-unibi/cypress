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
 * @file exceptions.hpp
 *
 * Contains exception classes used throughout Cypress.
 *
 * @author Andreas Stöckel
 */

#pragma once

#ifndef CYPRESS_CORE_EXCEPTIONS_HPP
#define CYPRESS_CORE_EXCEPTIONS_HPP

#include <stdexcept>

namespace cypress {
/**
 * Base class of all exception classes.
 */
class CypressException : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/**
 * Exception that should be thrown by implementations of the Backend class if
 * do_run fails.
 */
class ExecutionError : public CypressException {
public:
	using CypressException::CypressException;
};

/**
 * Exception thrown if a population referenced by name is not found.
 */
class NoSuchPopulationException : public CypressException {
public:
	using CypressException::CypressException;
};

/**
 * Exception thrown if an invalid connection is performed, for example if the
 * source and target network are different or the neuron count in source and
 * target is not supported by the chosen connector.
 */
class InvalidConnectionException : public CypressException {
public:
	using CypressException::CypressException;
};

/**
 * Exception thrown if the given parameter array size does not match the size of
 * the target Population, PopulationView or Neuron.
 */
class InvalidParameterArraySize : public CypressException {
public:
	using CypressException::CypressException;
};

/**
 * Exception thrown if a non-existing signal is accessed.
 */
class InvalidParameter : public CypressException {
public:
	using CypressException::CypressException;
};

/**
 * Exception thrown if a non-existing signal is accessed.
 */
class InvalidSignal : public CypressException {
public:
	using CypressException::CypressException;
};
}

#endif /* CYPRESS_CORE_EXCEPTIONS_HPP */
