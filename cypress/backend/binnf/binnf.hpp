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

namespace cypress {
namespace binnf {
/**
 * The Number union represents a number which either is a 32-bit integer or
 * a 32-bit float.
 */
union Number {
	int32_t i;
	float f;

	Number() : i(0){};

	Number(int32_t i) : i(i) {}

	Number(double d) : f(d) {}

	operator int32_t() const {return i;}
	operator double() const {return f;}

	bool operator==(const Number &o) const { return o.i == i; }

	friend std::ostream &operator<<(std::ostream &os, const Number &n)
	{
		return os << "(" << n.i << "/" << n.f << ")";
	}
};

/**
 * Enum describing the actual type in the Number enum.
 */
enum class NumberType : uint32_t { INT = 0, FLOAT = 1 };

/**
 * Class describing the columns of a matrix.
 */
struct Header {
	/**
	 * Names of the individual matrix columns.
	 */
	std::vector<std::string> names;

	/**
	 * Type stored in the individual matrix columns.
	 */
	std::vector<NumberType> types;

	Header() {}

	Header(const std::vector<std::string> &names,
	       const std::vector<NumberType> &types)
	    : names(names), types(types)
	{
		assert(names.size() == types.size());
	}

	size_t size() const { return names.size(); }
};

/**
 * Block of data as being written to/read from the serialiser.
 */
struct Block {
	std::string name;
	Header header;
	Matrix<Number> matrix;

	Block() {}

	Block(const std::string &name, const Header &header,
	      const Matrix<Number> &matrix)
	    : name(name), header(header), matrix(matrix)
	{
	}

	size_t colidx(const std::string &name) const
	{
		return std::find(header.names.begin(), header.names.end(), name) -
		       header.names.begin();
	}

	size_t size() const { return header.size(); }
};

/**
 * Callback function type being called whenever the "deserialise" function
 * finds and deserialises a data block.
 */
using Callback = std::function<bool(const std::string &, const Header &,
                                    const Matrix<Number> &)>;

/**
 * Serialises a named matrix along with its content and the given header to
 * the given stream as a single data block.
 *
 * @param os is the output stream to which the matrix should be written.
 * @param name is the name of the block that should be written.
 * @param header is the header describing each column of the matrix.
 * @param data is a pointer at a continuous data block containing the data that
 * should be serialised.
 * @param rows is the number of data rows.
 */
void serialise(std::ostream &os, const std::string &name, const Header &header,
               const Number data[], size_t rows);

/**
 * Serialises a named matrix along with its content and the given header to
 * the given stream as a single data block.
 *
 * @param os is the output stream to which the matrix should be written.
 * @param name is the name of the block that should be written.
 * @param header is the header describing each column of the matrix.
 * @param matrix is the matrix that should be written to file.
 */
void serialise(std::ostream &os, const std::string &name, const Header &header,
               const Matrix<Number> &matrix);

/**
 * Serialises a named matrix along with its content and the given header to
 * the given stream as a single data block.
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
