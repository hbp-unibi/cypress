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

#include <iostream>
#include <mutex>

#include <cypress/util/terminal.hpp>
#include <cypress/util/logger.hpp>

namespace cypress {
/*
 * Class LogStreamBackendImpl
 */

class LogStreamBackendImpl {
private:
	std::ostream &m_os;
	Terminal m_terminal;

public:
	LogStreamBackendImpl(std::ostream &os, bool use_color)
	    : m_os(os), m_terminal(use_color)
	{
	}

	void log(LogSeverity lvl, std::time_t time, const std::string &module,
	         const std::string &message)
	{
		{
			char time_str[41];
			std::strftime(time_str, 40, "%Y-%m-%d %H:%M:%S",
			              std::localtime(&time));
			m_os << m_terminal.italic() << time_str << m_terminal.reset();
		}

		m_os << " [" << module << "] ";

		if (lvl <= LogSeverity::DEBUG) {
			m_os << m_terminal.color(Terminal::BLACK, true) << "debug";
		}
		else if (lvl <= LogSeverity::INFO) {
			m_os << m_terminal.color(Terminal::BLUE, true) << "info";
		}
		else if (lvl <= LogSeverity::WARNING) {
			m_os << m_terminal.color(Terminal::MAGENTA, true) << "warning";
		}
		else if (lvl <= LogSeverity::ERROR) {
			m_os << m_terminal.color(Terminal::RED, true) << "error";
		}
		else {
			m_os << m_terminal.color(Terminal::YELLOW, true) << "fatal error";
		}

		m_os << m_terminal.reset() << ": " << message
		     << std::endl;
	}
};

/*
 * Class LogStreamBackend
 */

LogStreamBackend::LogStreamBackend(std::ostream &os, bool use_color)
    : m_impl(std::make_unique<LogStreamBackendImpl>(os, use_color))
{
}

void LogStreamBackend::log(LogSeverity lvl, std::time_t time,
                           const std::string &module,
                           const std::string &message)
{
	m_impl->log(lvl, time, module, message);
}

LogStreamBackend::~LogStreamBackend()
{
	// Do nothing here, just required for the unique_ptr destructor
}

/*
 * Class LoggerImpl
 */

class LoggerImpl {
private:
	std::shared_ptr<LogBackend> m_backend;
	std::mutex m_logger_mtx;
	LogSeverity m_min_level = LogSeverity::INFO;

public:
	void backend(std::shared_ptr<LogBackend> backend)
	{
		std::lock_guard<std::mutex> lock(m_logger_mtx);
		m_backend = std::move(backend);
	}

	void min_level(LogSeverity lvl)
	{
		std::lock_guard<std::mutex> lock(m_logger_mtx);
		m_min_level = lvl;
	}

	LogSeverity min_level()
	{
		std::lock_guard<std::mutex> lock(m_logger_mtx);
		return m_min_level;
	}

	void log(LogSeverity lvl, std::time_t time, const std::string &module,
	         const std::string &message)
	{
		std::lock_guard<std::mutex> lock(m_logger_mtx);
		if (lvl >= m_min_level) {
			m_backend->log(lvl, time, module, message);
		}
	}

	void log(LogSeverity lvl, const std::string &module,
	         const std::string &message)
	{
		log(lvl, time(nullptr), module, message);
	}
};

/*
 * Class Logger
 */

Logger::Logger() : Logger(std::make_shared<LogStreamBackend>(std::cerr, true))
{
}

Logger::Logger(std::shared_ptr<LogBackend> backend)
    : m_impl(std::make_unique<LoggerImpl>())
{
	m_impl->backend(backend);
}

void Logger::min_level(LogSeverity lvl) { m_impl->min_level(lvl); }

LogSeverity Logger::min_level() { return m_impl->min_level(); }

void Logger::backend(std::shared_ptr<LogBackend> backend)
{
	m_impl->backend(backend);
}

void Logger::log(LogSeverity lvl, std::time_t time, const std::string &module,
                 const std::string &message)
{
	m_impl->log(lvl, time, module, message);
}

void Logger::debug(const std::string &module, const std::string &message)
{
	m_impl->log(LogSeverity::DEBUG, module, message);
}

void Logger::info(const std::string &module, const std::string &message)
{
	m_impl->log(LogSeverity::INFO, module, message);
}

void Logger::warn(const std::string &module, const std::string &message)
{
	m_impl->log(LogSeverity::WARNING, module, message);
}

void Logger::error(const std::string &module, const std::string &message)
{
	m_impl->log(LogSeverity::ERROR, module, message);
}

void Logger::fatal_error(const std::string &module, const std::string &message)
{
	m_impl->log(LogSeverity::FATAL_ERROR, module, message);
}

/*
 * Functions
 */

Logger &global_logger()
{
	static Logger logger;
	return logger;
}
}
