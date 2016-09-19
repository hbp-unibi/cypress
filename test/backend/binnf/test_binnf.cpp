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

#include "gtest/gtest.h"

#include <sstream>

#include <cypress/backend/binnf/binnf.hpp>
#include <cypress/backend/resources.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/util/process.hpp>

namespace cypress {
namespace binnf {

static Block generate_test_matrix_block()
{
	std::string name = "test_matrix";
	Header header = {
	    {"col0", "col1", "col2", "col3", "col4", "col5", "col6", "col7",
	     "col8"},
	    {NumberType::INT8, NumberType::UINT8, NumberType::INT16,
	     NumberType::UINT16, NumberType::INT32, NumberType::UINT32,
	     NumberType::FLOAT32, NumberType::INT64, NumberType::FLOAT64}};
	Block block("test_matrix", header, 100);
	for (size_t i = 0; i < block.rows(); i++) {
		for (size_t j = 0; j < block.cols(); j++) {
			block.set(i, j, i * (j + 1));
		}
	}
	return block;
}

static Block generate_test_log_block()
{
	return Block(1234.2, LogSeverity::WARNING, "test", "test message");
}

static void check_equal(const Block &b1, const Block &b2)
{
	EXPECT_EQ(b1.name, b2.name);
	EXPECT_EQ(b1.matrix, b2.matrix);
	EXPECT_EQ(b1.header.size(), b2.header.size());
	for (size_t i = 0; i < b2.header.size(); i++) {
		EXPECT_EQ(b1.header.name(i), b2.header.name(i));
		EXPECT_EQ(b1.header.type(i), b2.header.type(i));
	}
}

static void check_serialise_deserialise(const Block &block)
{
	std::stringstream ss;

	// Write the test data to the memory stream
	serialise(ss, block);

	// Read it back, make sure input and output data are equal
	check_equal(block, deserialise(ss));
}

TEST(binnf, read_write)
{
	check_serialise_deserialise(generate_test_matrix_block());
	check_serialise_deserialise(generate_test_log_block());
}

TEST(binnf, python_communication)
{
	// Open the python loopback as a subprocess
	Process proc("python", {Resources::PYNN_BINNF_LOOPBACK.open()});

	// Generate a test data block
	const Block block_in_1 = generate_test_matrix_block();
	const Block block_in_2 = generate_test_log_block();

	// Send the data block to python
	serialise(proc.child_stdin(), block_in_1);
	serialise(proc.child_stdin(), block_in_2);
	serialise(proc.child_stdin(), block_in_2);
	serialise(proc.child_stdin(), block_in_1);
	serialise(proc.child_stdin(), block_in_2);
	serialise(proc.child_stdin(), block_in_2);
	serialise(proc.child_stdin(), block_in_1);
	serialise(proc.child_stdin(), block_in_1);
	proc.close_child_stdin();

	// Read it back, expect the deserialisation to be successful
	try {
		check_equal(block_in_1, deserialise(proc.child_stdout()));
		check_equal(block_in_2, deserialise(proc.child_stdout()));
		check_equal(block_in_2, deserialise(proc.child_stdout()));
		check_equal(block_in_1, deserialise(proc.child_stdout()));
		check_equal(block_in_2, deserialise(proc.child_stdout()));
		check_equal(block_in_2, deserialise(proc.child_stdout()));
		check_equal(block_in_1, deserialise(proc.child_stdout()));
		check_equal(block_in_1, deserialise(proc.child_stdout()));
	}
	catch (BinnfDecodeException &e) {
		// Most likely there is an error message
		char buf[4096];
		while (proc.child_stderr().good()) {
			proc.child_stderr().read(buf, 4096);
			std::cout.write(buf, proc.child_stderr().gcount());
		}
		EXPECT_TRUE(false);
	}

	// Make sure the Python code does not crash
	EXPECT_EQ(0, proc.wait());
}
}
}
