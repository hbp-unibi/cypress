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

#include "gtest/gtest.h"

#include <sstream>

#include <cypress/backend/binnf/binnf.hpp>
#include <cypress/backend/resources.hpp>
#include <cypress/util/process.hpp>

namespace cypress {
namespace binnf {

static Block generate_test_block()
{
	std::string name = "test_matrix";
	Header header = {
	    {"col1", "col2", "col3"},
	    {NumberType::INT, NumberType::FLOAT,
	     NumberType::INT}};
	Matrix<Number> matrix(100, 3);
	for (size_t i = 0; i < matrix.rows(); i++) {
		for (size_t j = 0; j < matrix.cols(); j++) {
			switch (header.types[j]) {
				case NumberType::INT:
					matrix(i, j).i = i * (j + 1);
					break;
				case NumberType::FLOAT:
					matrix(i, j).f = i * (j + 1);
					break;
			}
		}
	}
	return Block(name, header, matrix);
}

static void check_equal(const Block &b1, const Block &b2)
{
	EXPECT_EQ(b1.name, b2.name);
	EXPECT_EQ(b1.matrix, b2.matrix);
	EXPECT_EQ(b1.header.size(), b2.header.size());
	for (size_t i = 0; i < b2.header.size(); i++) {
		EXPECT_EQ(b1.header.names[i], b2.header.names[i]);
		EXPECT_EQ(b1.header.types[i], b2.header.types[i]);
	}
}

TEST(binnf, read_write)
{
	std::stringstream ss;

	// Generate a test data block
	const Block block_in = generate_test_block();

	// Write the test data to the memory stream
	serialise(ss, block_in);

	// Read it back, expect the deserialisation to be successful
	auto res = deserialise(ss);
	ASSERT_TRUE(res.first);

	// Make sure input and output data are equal
	check_equal(block_in, res.second);
}

TEST(binnf, python_communication)
{
	// Open the python loopback as a subprocess
	Process proc("python", {Resources::PYNN_BINNF_LOOPBACK.open()});

	// Generate a test data block
	const Block block_in = generate_test_block();

	// Send the data block to python
	serialise(proc.child_stdin(), block_in);
	proc.close_child_stdin();

	// Read it back, expect the deserialisation to be successful
	auto res = deserialise(proc.child_stdout());
	EXPECT_TRUE(res.first);

	// Make sure input and output data are equal
	if (res.first) {
		check_equal(block_in, res.second);
	} else {
		// Most likely there is an error message
		char buf[4096];
		while (proc.child_stderr().good()) {
			proc.child_stderr().read(buf, 4096);
			std::cout.write(buf, proc.child_stderr().gcount());
		}
	}

	// Make sure the Python code does not crash
	EXPECT_EQ(0, proc.wait());
}
}
}
