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
 * @file matrix.hpp
 *
 * Extremely small matrix library, allowing to define a two dimensional memory
 * region.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_UTIL_MATRIX_HPP
#define CYPRESS_UTIL_MATRIX_HPP

#include <algorithm>
#include <cassert>
#include <memory>
#include <ostream>
#include <vector>
#ifndef NDEBUG
#include <sstream>
#include <stdexcept>
#endif

namespace cypress {

enum class MatrixFlags { NONE = 0, ZEROS = 1 };

/**
 * Base class providing storage for a 2D memory region of an arbitrary type T.
 *
 * @tparam T is the type stored in the matrix.
 */
template <typename T>
class Matrix {
private:
	/**
	 * Shared pointer referencing the memory, used for the copy on write
	 * behaviour.
	 */
	T *m_buf;

	/**
	 * Number of rows and number of columns.
	 */
	size_t m_rows, m_cols;

	/**
	 * Flag which sets the freeing in the destructor active
	 */
	bool m_destroy = true;

#ifndef NDEBUG
	/**
	 * Function used to check whether an access at x. Disabled
	 * in release mode.
	 */
	void check_range(size_t i) const
	{
		if (i > size()) {
			std::stringstream ss;
			ss << "[" << i << "] out of range for matrix of size " << size()
			   << std::endl;
			throw std::out_of_range(ss.str());
		}
	}

	/**
	 * Function used to check whether an access at x, y is correct. Disabled
	 * in release mode.
	 */
	void check_range(size_t row, size_t col) const
	{
		if (row >= m_rows || col >= m_cols) {
			std::stringstream ss;
			ss << "[" << row << ", " << col
			   << "] out of range for matrix of size " << m_rows << " x "
			   << m_cols << std::endl;
			throw std::out_of_range(ss.str());
		}
	}
#else
	/**
	 * Function used to check whether an access at x is correct. Disabled
	 * in release mode.
	 */
	void check_range(size_t) const {}
	/**
	 * Function used to check whether an access at x, y is correct. Disabled
	 * in release mode.
	 */
	void check_range(size_t, size_t) const {}
#endif

public:
	/**
	 * Default constructor. Creates an empty matrix.
	 */
	Matrix() : m_buf(nullptr), m_rows(0), m_cols(0) {}

	/**
	 * Initialiser list constructor.
	 */
	Matrix(std::initializer_list<T> init) : Matrix(init.size(), 1)
	{
		size_t i = 0;
		for (const auto &elem : init) {
			(*this)[i++] = elem;
		}
	}

	/**
	 * 2D array constructor.
	 */
	template <size_t Rows, size_t Cols>
	Matrix(const std::array<std::array<T, Cols>, Rows> &init)
	    : Matrix(Rows, Cols)
	{
		for (size_t i = 0; i < Rows; i++) {
			for (size_t j = 0; j < Cols; j++) {
				(*this)(i, j) = init[i][j];
			}
		}
	}

	Matrix(const std::vector<std::vector<T>> &init)
	    : Matrix(init.size(), init[0].size())
	{
		for (size_t i = 1; i < init.size(); i++) {
			assert(init[0].size() == init[i].size());
		}
		for (size_t i = 0; i < init.size(); i++) {
			for (size_t j = 0; j < init[0]; j++) {
				(*this)(i, j) = init[i][j];
			}
		}
	}

	Matrix(const std::vector<T> &init) : Matrix(init.size(), 1)
	{
		for (size_t i = 0; i < init.size(); i++) {
			(*this)(i, 1) = init[i];
		}
	}

	/**
	 * Constructor of the Matrix type, creates a new matrix with the given
	 * extent.
	 *
	 * @param rows is the number of rows in the matrix.
	 * @param cols is the number of columns in the matrix.
	 * @param flags set to MatrixFlags::ZEROS to force an initialisation of the
	 * memory. If not given, the memory is not initialised.
	 */
	Matrix(size_t rows, size_t cols, MatrixFlags flags = MatrixFlags::NONE)
	    : m_buf(new T[rows * cols]), m_rows(rows), m_cols(cols)
	{
		if (flags == MatrixFlags::ZEROS) {
			fill(T());
		}
	}

	/**
	 * Constructor of the Matrix type, creates a new matrix with the given
	 * extent, copying the data from the given address.
	 *
	 * @param rows is the number of rows in the matrix.
	 * @param cols is the number of columns in the matrix.
	 * @param data is a pointer at a pre-existing data region from which the
	 * data will be copied.
	 */
	Matrix(size_t rows, size_t cols, const T *data)
	    : m_buf(new T[rows * cols]), m_rows(rows), m_cols(cols)
	{
		std::copy(data, data + (rows * cols), begin());
	}

	/**
	 * Constructor of the Matrix type from an existing data structure without a
	 * copy
	 *
	 * @param rows is the number of rows in the matrix.
	 * @param cols is the number of columns in the matrix.
	 * @param data is a pointer at a pre-existing data region
	 * @param destroy True: destructor will free memory, False: Destructor will
	 * do nothing
	 */
	Matrix(size_t rows, size_t cols, T *data, bool destroy)
	    : m_buf(data), m_rows(rows), m_cols(cols), m_destroy(destroy){};

	Matrix(const Matrix &o)
	    : m_buf(new T[o.rows() * o.cols()]), m_rows(o.rows()), m_cols(o.cols())
	{
		std::copy(o.begin(), o.end(), begin());
	}

	Matrix(Matrix &&o) noexcept
	    : m_buf(o.m_buf), m_rows(o.rows()), m_cols(o.cols())
	{
		o.m_buf = nullptr;
		o.m_rows = 0;
		o.m_cols = 0;
	}

	Matrix &operator=(const Matrix &o)
	{
		delete[] m_buf;
		m_rows = o.m_rows;
		m_cols = o.m_cols;
		m_buf = new T[o.rows() * o.cols()];
		std::copy(o.begin(), o.end(), begin());
		return *this;
	}

	Matrix &operator=(Matrix &&o) noexcept
	{
		delete[] m_buf;
		m_rows = o.m_rows;
		m_cols = o.m_cols;
		m_buf = o.m_buf;
		o.m_buf = nullptr;
		o.m_rows = 0;
		o.m_cols = 0;
		return *this;
	}

	~Matrix()
	{
		if (m_destroy) {
			delete[] m_buf;
		}
	}

	/**
	 * Conversion to a std::vector
	 */
	operator std::vector<T>() const { return std::vector<T>(begin(), end()); }

	/**
	 * Tests equality between two matrices.
	 *
	 * @param o is the matrix to which this matrix should be compared.
	 */
	bool operator==(const Matrix<T> &o) const
	{
		return rows() == o.rows() && cols() == o.cols() &&
		       std::equal(begin(), end(), o.begin());
	}

	/**
	 * Fills the matrix with the given value.
	 */
	Matrix<T> &fill(const T &val)
	{
		std::fill(begin(), end(), val);
		return *this;
	}

	T *begin(size_t row = 0) { return m_buf + row * cols(); }
	T *end() { return m_buf + size(); }
	const T *begin(size_t row = 0) const { return m_buf + row * cols(); }
	const T *end() const { return m_buf + size(); }
	const T *cbegin(size_t row = 0) const { return m_buf + row * cols(); }
	const T *cend() const { return m_buf + size(); }

	/**
	 * Returns a reference at the element at position row and col.
	 */
	T &operator()(size_t row, size_t col)
	{
		check_range(row, col);
		return *(m_buf + row * m_cols + col);
	}

	/**
	 * Returns a const reference at the element at position row and col.
	 */
	const T &operator()(size_t row, size_t col) const
	{
		check_range(row, col);
		return *(m_buf + row * m_cols + col);
	}

	/**
	 * Returns a reference at the i-th element.
	 */
	T &operator()(size_t i)
	{
		check_range(i);
		return *(m_buf + i);
	}

	/**
	 * Returns a const reference at the i-th element.
	 */
	const T &operator()(size_t i) const
	{
		check_range(i);
		return *(m_buf + i);
	}

	/**
	 * Returns a copy of the i-th element.
	 */
	T &operator[](size_t i)
	{
		check_range(i);
		return *(m_buf + i);
	}

	/**
	 * Returns a copy of the i-th element.
	 */
	const T &operator[](size_t i) const
	{
		check_range(i);
		return *(m_buf + i);
	}

	/**
	 * Returns the width of the matrix.
	 */
	size_t rows() const { return m_rows; }

	/**
	 * Returns the height of the matrix.
	 */
	size_t cols() const { return m_cols; }

	/**
	 * Returns the size of the matrix.
	 */
	size_t size() const { return m_rows * m_cols; }

	/**
	 * Resizes the matrix, flushes all previously stored data, does nothing if
	 * the dimensions do not change.
	 */
	void resize(size_t rows, size_t cols)
	{
		if (rows != m_rows || cols != m_cols) {
			delete[] m_buf;
			m_buf = new T[rows * cols];
			m_rows = rows;
			m_cols = cols;
		}
	}

	/**
	 * Returns a pointer at the raw data buffer.
	 */
	T *data() { return m_buf; }

	/**
	 * Returns a const pointer at the raw data.
	 */
	const T *data() const { return m_buf; }

	/**
	 * Dumps the matrix as CSV.
	 */
	friend std::ostream &operator<<(std::ostream &os, const Matrix<T> &m)
	{
		T const *d = m.data();
		for (size_t row = 0; row < m.rows(); row++) {
			for (size_t col = 0; col < m.cols(); col++) {
				os << (col == 0 ? "" : ",") << *d;
				d++;
			}
			os << std::endl;
		}
		return os;
	}

	/**
	 * Returns true if the matrix contains no data.
	 */
	bool empty() const { return size() == 0; }
};

template <typename T, size_t Rows, size_t Cols>
Matrix<T> make_matrix(const std::array<std::array<T, Cols>, Rows> &init)
{
	return Matrix<T>(init);
}

template <typename T>
Matrix<T> make_matrix(std::initializer_list<T> list)
{
	return Matrix<T>(list);
}

template <typename T>
class Vector : public Matrix<T> {
public:
	Vector() : Matrix<T>(0, 0) {}
	Vector(size_t s, MatrixFlags flags = MatrixFlags::NONE)
	    : Matrix<T>(s, 1, flags)
	{
	}
	Vector(std::initializer_list<T> init) : Vector(init.size())
	{
		size_t i = 0;
		for (const auto &elem : init) {
			(*this)[i++] = elem;
		}
	}
	void resize(size_t s) { resize(s, 1); }
};

}  // namespace cypress

#endif /* CYPRESS_UTIL_MATRIX_HPP */
