/*
 *  CppNAM -- C++ Neural Associative Memory Simulator
 *  Copyright (C) 2016  Christoph Jenzen, Andreas Stöckel
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

#include <thread>
#include <chrono>
#include <sstream>
#include "gtest/gtest.h"

#include <cypress/util/process.hpp>

namespace cypress {

TEST(process, echo)
{
	auto res = Process::exec("echo", {"-n", "Hello World"});
	EXPECT_EQ(0, std::get<0>(res));
	EXPECT_EQ("Hello World", std::get<1>(res));
	EXPECT_EQ("", std::get<2>(res));
}

TEST(process, echo_stderr)
{
	auto res = Process::exec("sh", {"-c", "echo -n \"Hello World\" >&2"});
	EXPECT_EQ(0, std::get<0>(res));
	EXPECT_EQ("", std::get<1>(res));
	EXPECT_EQ("Hello World", std::get<2>(res));
}

TEST(process, cat)
{
	auto res = Process::exec("cat", {}, "Hello World");
	EXPECT_EQ(0, std::get<0>(res));
	EXPECT_EQ("Hello World", std::get<1>(res));
	EXPECT_EQ("", std::get<2>(res));
}

TEST(process, cat_large)
{
	std::stringstream ss;
	for (size_t i = 0; i < 100000; i++) {
		ss << i;
	}
	auto res = Process::exec("cat", {}, ss.str());
	EXPECT_EQ(0, std::get<0>(res));
	EXPECT_EQ(ss.str(), std::get<1>(res));
	EXPECT_EQ("", std::get<2>(res));
}

TEST(process, python)
{
	auto res = Process::exec("python", {}, "print(\"Hello World\")");
	EXPECT_EQ(0, std::get<0>(res));
	EXPECT_EQ("Hello World\n", std::get<1>(res));
	EXPECT_EQ("", std::get<2>(res));
}

TEST(process, exec_nonexisting)
{
	auto res = Process::exec("/.foobar_foobar_fuum", {});
	EXPECT_EQ(1, std::get<0>(res));
	EXPECT_EQ("Command not found: /.foobar_foobar_fuum\n", std::get<2>(res));
}

TEST(process, exitcode)
{
	EXPECT_EQ(42, std::get<0>(Process::exec("sh", {"-c", "exit 42"})));
}

TEST(process, running)
{
	using namespace std::chrono_literals;

	Process proc("sleep", {"0.1"});
	EXPECT_TRUE(proc.running());
	EXPECT_TRUE(proc.running());
	std::this_thread::sleep_for(0.2s);
	EXPECT_FALSE(proc.running());
	EXPECT_EQ(0, proc.exitcode());
}


TEST(process, signal)
{
	using namespace std::chrono_literals;

	Process proc("sleep", {"1"});
	EXPECT_TRUE(proc.running());
	EXPECT_TRUE(proc.signal(9));
	std::this_thread::sleep_for(0.1s);
	EXPECT_FALSE(proc.running());
	EXPECT_EQ(-9, proc.exitcode());
}
}

