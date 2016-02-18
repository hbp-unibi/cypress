/*
 *  Cypress -- C++ Neural Associative Memory Simulator
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
 * @file process.hpp
 *
 * Contains the class used from inter-process communication between the Python
 * and the C++ code.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_PROCESS_HPP
#define CYPRESS_PROCESS_HPP

#include <iosfwd>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace cypress {

/**
 * Forward declaration of the internal implementation of the Process class.
 */
class ProcessImpl;

/**
 * The Process class represents a child process and its input/output streams.
 */
class Process {
private:
	/**
	 * Pointer at the implementation of the Process class (Pimpl idiom.)
	 */
	std::unique_ptr<ProcessImpl> impl;

public:
	/**
	 * Constructor of the process class. Executes the given command with the
	 * given arguments.
	 *
	 * @param cmd is the command that should be executed.
	 * @param args is a vector of arguments that should be passed to the
	 * command.
	 */
	Process(const std::string &cmd, const std::vector<std::string> &args);

	/**
	 * Destroys the process instance. Waits for the child process to exit.
	 */
	~Process();

	/**
	 * Returns a reference at the child process standard out stream.
	 *
	 * @return a reference at a C++ input stream from which the standard output
	 * of the child process can be read.
	 */
	std::istream &child_stdout();

	/**
	 * Returns a reference at the child process standard error stream.
	 *
	 * @return a reference at a C++ input stream from which the standard error
	 * of the child process can be read.
	 */
	std::istream &child_stderr();

	/**
	 * Returns a reference at the child process standard input stream. Note:
	 * use the close_child_stdin() method to close this stream.
	 *
	 * @return a reference at a C++ output stream which can be used to write
	 * data to the standard input of the child process.
	 */
	std::ostream &child_stdin();

	/**
	 * Closes the childs standard in stream.
	 */
	void close_child_stdin();

	/**
	 * Returns true if the child process is still running, false otherwise.
	 */
	bool running();

	/**
	 * Returns the exitcode of the process, as a value between 0 and 255. If the
	 * child process was killed by a signal, returns the signal number which
	 * killed the process as a negative number. The result is only valid if the
	 * child process is no longer running (running() return false, or wait()
	 * returned).
	 *
	 * @return the exit code (positive number) or the signal number which killed
	 * the child process (negative number).
	 */
	int exitcode();

	/**
	 * Waits for the child process to exit. Returns the exitcode. See exitcode()
	 * for the exact semantics.
	 */
	int wait();

	/**
	 * Sends a UNIX signal to the child process, corresponding to the kill
	 * system call.
	 *
	 * @param signal is the UNIX signal that should be sent to the child
	 * process.
	 * @return true if the operation was successful, false if not, e.g. because
	 * the child no longer exists.
	 */
	bool signal(int signal);

	/**
	 * Convenience method for executing a child process and sendings its stdout
	 * and stderr streams to the given streams.
	 *
	 * @param cmd is the command that should be executed.
	 * @param args is a vector of arguments that should be given to the command.
	 * @param input is a string that should be sent to the child process via
	 * its stdin.
	 * @param cout is the target stream to which the child process cout should
	 * be written.
	 * @param cerr is the target stream to which the child process cerr should
	 * be written.
	 * @return the process return code.
	 */
	static int exec(const std::string &cmd,
	                const std::vector<std::string> &args, std::ostream &cout,
	                std::ostream &cerr,
	                const std::string &input = std::string());

	/**
	 * Convenience method for executing a child process, sending data via stdin
	 * and receiving its output on std::cout and std::cerr.
	 *
	 * @param cmd is the command that should be executed.
	 * @param args is a vector of arguments that should be given to the command.
	 * @param input is a string that should be sent to the child process via
	 * its stdin.
	 * @return a tuple containing the exit code and the output from stdout and
	 * stderr (in this order).
	 */
	static std::tuple<int, std::string, std::string> exec(
	    const std::string &cmd, const std::vector<std::string> &args,
	    const std::string &input = std::string());
};
}

#endif /* CYPRESS_PROCESS_HPP */
