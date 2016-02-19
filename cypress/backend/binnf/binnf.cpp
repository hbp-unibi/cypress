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

#include <stdexcept>
#include <sstream>

#include <cypress/backend/binnf/binnf.hpp>

namespace cypress {
namespace binnf {

namespace {

/* Helper methods for the calculation of the total block length */

using SizeType = uint32_t;

static constexpr uint32_t BLOCK_START_SEQUENCE = 0x665a8cda;
static constexpr uint32_t BLOCK_END_SEQUENCE = 0x420062cb;
static constexpr uint32_t BLOCK_TYPE_MATRIX = 0x01;
static constexpr SizeType MAX_STR_SIZE = 1024;
static constexpr SizeType BLOCK_TYPE_LEN = sizeof(uint32_t);
static constexpr SizeType SIZE_LEN = sizeof(SizeType);
static constexpr SizeType TYPE_LEN = sizeof(NumberType);
static constexpr SizeType NUMBER_LEN = sizeof(Number);

SizeType str_len(const std::string &s) { return SIZE_LEN + s.size(); }
SizeType header_len(const Header &header)
{
	SizeType res = SIZE_LEN;
	for (auto &name : header.names) {
		res += str_len(name) + TYPE_LEN;
	}
	return res;
}

SizeType matrix_len(size_t rows, size_t cols)
{
	return 2 * SIZE_LEN + rows * cols * NUMBER_LEN;
}

SizeType block_len(const std::string &name, const Header &header,
                   size_t rows)
{
	return BLOCK_TYPE_LEN + str_len(name) + header_len(header) +
	       matrix_len(rows, header.size());
}

/* Individual data write methods */

template <typename T>
void write(std::ostream &os, const T &t)
{
	os.write((char *)&t, sizeof(T));
}

template <>
void write(std::ostream &os, const std::string &str)
{
	if (str.size() > MAX_STR_SIZE) {
		std::stringstream ss;
		ss << "String exceeds string size limit of " << MAX_STR_SIZE
		   << " bytes.";
		throw std::overflow_error(ss.str());
	}
	write(os, SizeType(str.size()));
	os.write(str.c_str(), str.size());
}

template <>
void write(std::ostream &os, const Matrix<Number> &matrix)
{
	write(os, SizeType(matrix.rows()));
	write(os, SizeType(matrix.cols()));
	os.write((const char *)matrix.data(), matrix.size() * NUMBER_LEN);
}

/* Specialised read methods */

bool synchronise(std::istream &is, uint32_t marker)
{
	uint32_t sync = 0;
	uint8_t c = 0;
	while (is.good() && sync != marker) {
		is.read((char *)&c, 1);
		sync = (sync >> 8) | (c << 24);  // Requires a little endian machine
	}
	return sync == marker;
}

template <typename T>
void read(std::istream &is, T &res)
{
	is.read((char *)&res, sizeof(T));
	if (is.gcount() != sizeof(T)) {
		throw 0;
	}
}

template <>
void read(std::istream &is, std::string &str)
{
	// Read the string size, make sure it is below the maximum string size
	SizeType size;
	read(is, size);

	if (size > MAX_STR_SIZE) {
		throw 0;
	}

	// Read the actual string content
	str.resize(size);
	is.read(&str[0], size);
	if (is.gcount() != size) {
		throw 0;
	}
}

template <>
void read(std::istream &is, Matrix<Number> &matrix)
{
	// Read the row and column count
	SizeType rows, cols;
	read(is, rows);
	read(is, cols);

	// Write the data into the matrix
	matrix.resize(rows, cols);
	SizeType size = matrix.size() * NUMBER_LEN;
	is.read((char *)(matrix.data()), size);
	if (is.gcount() != size) {
		throw 0;
	}
}
}

/* Entire block serialisation method */

void serialise(std::ostream &os, const std::string &name, const Header &header,
               const Number data[], size_t rows)
{

	write(os, BLOCK_START_SEQUENCE);  // Write the block start mark
	write(os,
	      block_len(name, header, rows));  //  Write the total block length
	write(os, BLOCK_TYPE_MATRIX);
	write(os, name);                     // Write the name of the block
	write(os, SizeType(header.size()));  // Write the length of the header
	for (size_t i = 0; i < header.size(); i++) {
		write(os, header.names[i]);  // Write the column name
		write(os, header.types[i]);  // Write the column type
	}
	write(os, SizeType(rows));           // Write the number of rows
	write(os, SizeType(header.size()));  // Write the number of columns
	os.write((const char *)data, rows * header.size() * NUMBER_LEN);
	write(os, BLOCK_END_SEQUENCE);  // Write the block end mark
}

void serialise(std::ostream &os, const std::string &name, const Header &header,
               const Matrix<Number> &matrix)
{
	assert(matrix.cols() == header.size());
	serialise(os, name, header, matrix.data(), matrix.rows());
}

void serialise(std::ostream &os, const Block &block)
{
	serialise(os, block.name, block.header, block.matrix);
}

/* Entire block deserialisation method */

std::pair<bool, Block> deserialise(std::istream &is)
{
	Block res;

	try {
		// Try to read the block start header
		if (!synchronise(is, BLOCK_START_SEQUENCE)) {
			throw 0;
		}

		// Read the block size
		SizeType block_size;
		read(is, block_size);

		// Fetch the current stream position
		const std::streampos pos0 = is.tellg();

		uint32_t block_type;
		read(is, block_type);
		read(is, res.name);

		// Read the number of header elements
		SizeType header_count;
		read(is, header_count);

		// Read the header elements
		res.header.names.resize(header_count);
		res.header.types.resize(header_count);
		for (size_t i = 0; i < header_count; i++) {
			read(is, res.header.names[i]);
			read(is, res.header.types[i]);
		}

		// Read the matrix
		read(is, res.matrix);

		// Make sure the block size is correct
		const std::streampos pos1 = is.tellg();
		if (pos0 >= 0 && pos1 >= 0 && pos1 - pos0 != block_size) {
			throw 0;
		}

		// Make sure the block ends with the block end sequence
		uint32_t block_end = 0;
		read(is, block_end);
		if (block_end != BLOCK_END_SEQUENCE) {
			throw 0;
		}
	}
	catch (...) {
		return std::make_pair(false, Block());
	}
	return std::make_pair(true, std::move(res));
}

void deserialise(std::istream &is, const Callback &callback)
{
	std::pair<bool, Block> res;  // Result structure
	do {
		res = deserialise(is);
		if (res.first) {
			res.first =
			    callback(res.second.name, res.second.header, res.second.matrix);
		}
	} while (res.first);
}
}
}
