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

#include <sstream>

#include <cypress/backend/binnf/binnf.hpp>
#include <cypress/core/exceptions.hpp>

namespace cypress {
namespace binnf {

namespace {

/* Helper methods for the calculation of the total block length */

using SizeType = uint32_t;

static constexpr uint32_t BLOCK_START_SEQUENCE = 0x665a8cda;
static constexpr uint32_t BLOCK_END_SEQUENCE = 0x420062cb;
static constexpr uint32_t BLOCK_TYPE_MATRIX = 0x01;
static constexpr uint32_t BLOCK_TYPE_LOG = 0x02;
static constexpr SizeType BLOCK_TYPE_LEN = sizeof(uint32_t);
static constexpr SizeType SIZE_LEN = sizeof(SizeType);
static constexpr SizeType TYPE_LEN = sizeof(NumberType);

SizeType str_len(const std::string &s) { return SIZE_LEN + s.size(); }
SizeType header_len(const Header &header)
{
	SizeType res = SIZE_LEN;
	for (size_t i = 0; i < header.size(); i++) {
		res += str_len(header.name(i)) + TYPE_LEN;
	}
	return res;
}

SizeType matrix_len(const Header &header, size_t rows)
{
	return SIZE_LEN + rows * header.stride();
}

SizeType matrix_block_len(const std::string &name, const Header &header,
                          size_t rows)
{
	return BLOCK_TYPE_LEN + str_len(name) + header_len(header) +
	       matrix_len(header, rows);
}

SizeType log_block_len(const std::string &module, const std::string &msg)
{
	return BLOCK_TYPE_LEN + 12 + str_len(module) + str_len(msg);
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
	write(os, SizeType(str.size()));
	os.write(str.c_str(), str.size());
}

/* Specialised read methods */

bool synchronise(std::istream &is, uint32_t marker)
{
	uint32_t sync = 0;
	uint8_t c = 0;
	while (sync != marker) {
		is.read((char *)&c, 1);
		if (is.gcount() == 0) {
			return false;
		}
		sync = (sync >> 8) | (c << 24);
	}
	return sync == marker;
}

template <typename T>
void read(std::istream &is, T &res)
{
	is.read((char *)&res, sizeof(T));
	if (is.gcount() != sizeof(T)) {
		throw BinnfDecodeException("Unexpected end of stream");
	}
}

template <>
void read(std::istream &is, std::string &str)
{
	// Read the string size, make sure it is below the maximum string size
	SizeType size;
	read(is, size);

	// Read the actual string content
	str.resize(size);
	is.read(&str[0], size);
	if (is.gcount() != size) {
		throw BinnfDecodeException("Unexpected end of stream");
	}
}

void read(std::istream &is, const Header &header, Matrix<uint8_t> &matrix)
{
	// Read the row and column count
	SizeType rows;
	read(is, rows);

	// Write the data into the matrix
	matrix.resize(rows, header.stride());
	SizeType size = matrix.size();
	is.read((char *)(matrix.data()), size);
	if (is.gcount() != size) {
		throw BinnfDecodeException("Unexpected end of stream");
	}
}
}

/* Entire block serialisation method */

void serialise_matrix(std::ostream &os, const std::string &name,
                      const Header &header, const uint8_t *data, size_t rows)
{

	write(os, BLOCK_START_SEQUENCE);  // Write the block start mark
	write(os, matrix_block_len(name, header,
	                           rows));  //  Write the total block length
	write(os, BLOCK_TYPE_MATRIX);
	write(os, name);                     // Write the name of the block
	write(os, SizeType(header.size()));  // Write the length of the header
	for (size_t i = 0; i < header.size(); i++) {
		write(os, header.name(i));  // Write the column name
		write(os, header.type(i));  // Write the column type
	}
	write(os, SizeType(rows));  // Write the number of rows
	os.write((const char *)data, rows * header.stride());
	write(os, BLOCK_END_SEQUENCE);  // Write the block end mark
}

void serialise_log(std::ostream &os, double time, LogSeverity severity,
                   const std::string &module, const std::string &msg)
{
	write(os, BLOCK_START_SEQUENCE);        // Write the block start mark
	write(os, log_block_len(module, msg));  //  Write the total block length
	write(os, BLOCK_TYPE_LOG);
	write(os, time);
	write(os, severity);
	write(os, module);
	write(os, msg);
	write(os, BLOCK_END_SEQUENCE);  // Write the block end mark
}

void serialise(std::ostream &os, const Block &block)
{
	switch (block.type) {
		case BlockType::INVALID:
			// Nothing to do
			break;
		case BlockType::MATRIX:
			serialise_matrix(os, block.name, block.header, block.matrix.data(),
			                 block.matrix.rows());
			break;
		case BlockType::LOG:
			serialise_log(os, block.time, block.severity, block.module,
			              block.msg);
			break;
	}
}

/* Entire block deserialisation method */

static void deserialise_matrix(Block &res, std::istream &is)
{
	read(is, res.name);

	// Read the number of header elements
	SizeType header_count;
	read(is, header_count);

	// Read the header elements
	std::vector<std::string> names;
	std::vector<NumberType> types;
	names.resize(header_count);
	types.resize(header_count);
	for (size_t i = 0; i < header_count; i++) {
		read(is, names[i]);
		read(is, types[i]);
	}

	// Read the matrix
	res.header = Header(names, types);
	read(is, res.header, res.matrix);
}

static void deserialise_log(Block &res, std::istream &is)
{
	read(is, res.time);
	read(is, res.severity);
	read(is, res.module);
	read(is, res.msg);
}

Block deserialise(std::istream &is)
{
	Block res;

	// Try to read the block start header
	if (!synchronise(is, BLOCK_START_SEQUENCE)) {
		return res;
	}

	// Read the block size
	SizeType block_size;
	read(is, block_size);

	// Fetch the current stream position
	const std::streampos pos0 = is.tellg();

	// Read the block type
	read(is, res.type);

	// Read either a named matrix or a log message
	switch (res.type) {
		case BlockType::MATRIX:
			deserialise_matrix(res, is);
			break;
		case BlockType::LOG:
			deserialise_log(res, is);
			break;
		default:
			throw BinnfDecodeException("Unknown block type");
	}

	// Make sure the block size is correct
	const std::streampos pos1 = is.tellg();
	if (pos0 >= 0 && pos1 >= 0 && pos1 - pos0 != block_size) {
		throw BinnfDecodeException("Invalid block size");
	}

	// Make sure the block ends with the block end sequence
	uint32_t block_end = 0;
	read(is, block_end);
	if (block_end != BLOCK_END_SEQUENCE) {
		throw BinnfDecodeException("Expected block end");
	}
	return res;
}
}
}
