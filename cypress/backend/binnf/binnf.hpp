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

#pragma once

#ifndef CYPRESS_CORE_BINNF_HPP
#define CYPRESS_CORE_BINNF_HPP

#include <algorithm>
#include <cstdint>
#include <cassert>
#include <string>
#include <vector>
#include <ostream>
#include <functional>
#include <iostream>

#include <cypress/util/matrix.hpp>
#include <cypress/util/logger.hpp>
#include <cypress/core/types.hpp>

namespace cypress {
namespace binnf {
/**
 * Enum describing the actual type in the Number enum.
 */
enum class NumberType : uint32_t {
	INT8 = 0,
	UINT8 = 1,
	INT16 = 2,
	UINT16 = 3,
	INT32 = 4,
	UINT32 = 5,
	FLOAT32 = 6,
	INT64 = 7,
	FLOAT64 = 8
};

/**
 * Size of each number type in bytes.
 */
static const int NUMBER_SIZE[] = {1, 1, 2, 2, 4, 4, 4, 8, 8};

/**
 * Enum describing the type of a block.
 */
enum class BlockType : uint32_t { INVALID = 0x00, MATRIX = 0x01, LOG = 0x02 };

/**
 * Class describing the columns of a matrix.
 */
struct Header {
private:
	/**
	 * Names of the individual matrix columns.
	 */
	std::vector<std::string> m_names;

	/**
	 * Type stored in the individual matrix columns.
	 */
	std::vector<NumberType> m_types;

	/**
	 * Byte offsets for each column.
	 */
	std::vector<uint32_t> m_offs;

public:
	Header() {}

	Header(const std::vector<std::string> &names,
	       const std::vector<NumberType> &types)
	    : m_names(names), m_types(types), m_offs(m_types.size() + 1)
	{
		assert(m_names.size() == m_types.size());
		size_t offs = 0;
		for (size_t i = 0; i < m_types.size(); i++) {
			offs += NUMBER_SIZE[int(m_types[i])];
			m_offs[i + 1] = offs;
		}
	}

	/**
	 * Returns the name of the i-th column.
	 */
	const std::string &name(size_t i) const { return m_names[i]; }

	/**
	 * Returns the type of the i-th column.
	 */
	NumberType type(size_t i) const { return m_types[i]; }

	/**
	 * Returns the number of columns in the header.
	 */
	size_t size() const { return m_names.size(); }

	/**
	 * Returns the size of a row in bytes.
	 */
	size_t stride() const { return m_offs.back(); }

	/**
	 * Returns the offset of the i-th column
	 */
	size_t offs(size_t i) const { return m_offs[i]; }

	/**
	 * Returns the index of the column with the given name.
	 */
	size_t colidx(const std::string &name) const
	{
		return std::find(m_names.begin(), m_names.end(), name) -
		       m_names.begin();
	}
};

/**
 * Block of data as being written to/read from the serialiser.
 */
struct Block {

	BlockType type = BlockType::INVALID;

	std::string name;
	Header header;
	Matrix<uint8_t> matrix;

	double time;
	LogSeverity severity;
	std::string module;
	std::string msg;

	Block() {}

	Block(const std::string &name, const Header &header, size_t rows)
	    : type(BlockType::MATRIX),
	      name(name),
	      header(header),
	      matrix(rows, header.stride())
	{
	}

	Block(const std::string &name, const Header &header, const uint8_t *data,
	      size_t rows)
	    : type(BlockType::MATRIX),
	      name(name),
	      header(header),
	      matrix(rows, header.stride(), data)
	{
	}

	Block(double time, LogSeverity severity, const std::string &module,
	      const std::string &msg)
	    : type(BlockType::LOG),
	      time(time),
	      severity(severity),
	      module(module),
	      msg(msg)
	{
	}

	template <typename T>
	void set(size_t row, size_t col, T v)
	{
		const size_t stride = header.stride();
		const size_t offs = header.offs(col);
		switch (header.type(col)) {
			case NumberType::INT8:
				*((int8_t *)&matrix[row * stride + offs]) = v;
				break;
			case NumberType::UINT8:
				*((uint8_t *)&matrix[row * stride + offs]) = v;
				break;
			case NumberType::INT16:
				*((int16_t *)&matrix[row * stride + offs]) = v;
				break;
			case NumberType::UINT16:
				*((uint16_t *)&matrix[row * stride + offs]) = v;
				break;
			case NumberType::INT32:
				*((int32_t *)&matrix[row * stride + offs]) = v;
				break;
			case NumberType::UINT32:
				*((uint32_t *)&matrix[row * stride + offs]) = v;
				break;
			case NumberType::FLOAT32:
				*((float *)&matrix[row * stride + offs]) = v;
				break;
			case NumberType::INT64:
				*((int64_t *)&matrix[row * stride + offs]) = v;
				break;
			case NumberType::FLOAT64:
				*((double *)&matrix[row * stride + offs]) = v;
				break;
			default:
				throw std::invalid_argument("Invalid number type");
		}
	}

	template <typename T>
	void get(size_t row, size_t col, T &v) const
	{
		const size_t stride = header.stride();
		const size_t offs = header.offs(col);
		switch (header.type(col)) {
			case NumberType::INT8:
				v = *((int8_t *)&matrix[row * stride + offs]);
				break;
			case NumberType::UINT8:
				v = *((uint8_t *)&matrix[row * stride + offs]);
				break;
			case NumberType::INT16:
				v = *((int16_t *)&matrix[row * stride + offs]);
				break;
			case NumberType::UINT16:
				v = *((uint16_t *)&matrix[row * stride + offs]);
				break;
			case NumberType::INT32:
				v = *((int32_t *)&matrix[row * stride + offs]);
				break;
			case NumberType::UINT32:
				v = *((uint32_t *)&matrix[row * stride + offs]);
				break;
			case NumberType::FLOAT32:
				v = *((float *)&matrix[row * stride + offs]);
				break;
			case NumberType::INT64:
				v = *((int64_t *)&matrix[row * stride + offs]);
				break;
			case NumberType::FLOAT64:
				v = *((double *)&matrix[row * stride + offs]);
				break;
			default:
				throw std::invalid_argument("Invalid number type");
		}
	}

	int64_t get_int(size_t row, size_t col) const
	{
		int64_t v;
		get(row, col, v);
		return v;
	}

	Real get_float(size_t row, size_t col) const
	{
		Real v;
		get(row, col, v);
		return v;
	}

	size_t colidx(const std::string &name) const { return header.colidx(name); }

	size_t rows() const { return matrix.rows(); }
	size_t cols() const { return header.size(); }
};

/**
 * Serialises a named matrix along with its content and the given header to
 * the given stream as a single data block.
 *
 * @param os is the output stream to which the matrix should be written.
 * @param name is the name of the block that should be written.
 * @param header is the header describing each column of the matrix.
 * @param data is a pointer at a continuous data block containing the data that
 * should be serialised.
 * @param rows is the number of data rows. The size of a single row is
 * determined from the header.
 */
void serialise_matrix(std::ostream &os, const std::string &name,
                      const Header &header, const uint8_t *data, size_t rows);

/**
 * Serialises a log message.
 *
 * @param time is the unix timestamp containing the time at which the log
 * message was issued.
 * @param severity is the log message severity.
 * @param module is a string describing the module in which the log message
 * was issued.
 * @param msg is the actual log message that should be logged.
 */
void serialise_log(std::ostream &os, double time, LogSeverity severity,
                   const std::string &module, const std::string &msg);

/**
 * Serialises a block which may either contain a log message or a named
 * matrix.
 *
 * @param os is the output stream to which the matrix should be written.
 * @param block is the data block that should be written to the given output
 * stream.
 */
void serialise(std::ostream &os, const Block &block);

/**
 * Deserialises a single block.
 */
Block deserialise(std::istream &is);
}
}

#endif /* CYPRESS_CORE_BINNF_HPP */
