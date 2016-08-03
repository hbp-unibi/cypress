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
 * @file logger.hpp
 *
 * Implements a simple logging system used throughout Cypress to log messages
 * and warnings originating from the remote backends and cypress itself.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_UTIL_LOGGER_HPP
#define CYPRESS_UTIL_LOGGER_HPP

#include <ctime>
#include <string>
#include <iosfwd>
#include <memory>

namespace cypress {
/*
 * Forward declarations.
 */
class Logger;
class LoggerImpl;
class LogStreamBackendImpl;

/**
 * The Severity enum holds the severity of a log message. Higher severities are
 * associated with higher values.
 */
enum Severity {
	DEBUG = 10,
	INFO = 20,
	WARNING = 30,
	ERROR = 40,
	FATAL_ERROR = 50
};

/**
 * The Backend class is the abstract base class that must be implemented by
 * any log backend.
 */
class LogBackend {
public:
	/**
	 * Pure virtual function which is called whenever a new log message should
	 * be logged.
	 */
	virtual void log(Severity lvl, std::time_t time, const std::string &module,
	                 const std::string &message) = 0;

	virtual ~LogBackend() {}
};

/**
 * Implementation of the LogBackend class which loggs to the given output
 * stream.
 */
class LogStreamBackend : public LogBackend {
private:
	std::unique_ptr<LogStreamBackendImpl> m_impl;

public:
	LogStreamBackend(std::ostream &os, bool use_color = false);

	void log(Severity lvl, std::time_t time, const std::string &module,
	         const std::string &message) override;

	virtual ~LogStreamBackend();
};

/**
 * The Logger class is the frontend class that should be used to log messages.
 * A global instance which logs to std::cerr can be accessed using the
 * global_logger() function.
 */
class Logger {
private:
	std::unique_ptr<LoggerImpl> m_impl;

public:
	/**
	 * Default constructor, logs through a LogStreamBackend backend to
	 * std::cerr.
	 */
	Logger();

	/**
	 * Creates a new Logger instance which logs into the given backend.
	 */
	Logger(std::shared_ptr<LogBackend> backend);

	/**
	 * Sets the minimum severity level that should be logged. The default is
	 * Serverity::INFO.
	 */
	void min_level(Severity lvl);

	/**
	 * Returns the current minimum log level.
	 */
	Severity min_level();

	/**
	 * Sets the backend to the given backend instance.
	 */
	void backend(std::shared_ptr<LogBackend> backend);

	void log(Severity lvl, std::time_t time, const std::string &module,
	         const std::string &message);
	void debug(const std::string &module, const std::string &message);
	void info(const std::string &module, const std::string &message);
	void warn(const std::string &module, const std::string &message);
	void error(const std::string &module, const std::string &message);
	void fatal_error(const std::string &module, const std::string &message);
};

Logger &global_logger();
}

#endif /* CYPRESS_UTIL_LOGGER_HPP */
