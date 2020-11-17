/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2020 Christoph Ostrau
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

#include <cypress/util/matrix.hpp>

#include "gtest/gtest.h"
namespace cypress {

TEST(Matrix, submatrix)
{
	auto mat = Matrix<int>({1, 2, 3, 4, 5, 6, 7, 8, 9});
	mat.reshape(3, 3);

	auto test = mat.submatrix(0, 0);
	EXPECT_EQ(5, test(0, 0));
	EXPECT_EQ(6, test(0, 1));
	EXPECT_EQ(8, test(1, 0));
	EXPECT_EQ(9, test(1, 1));

	test = mat.submatrix(1, 1);
	EXPECT_EQ(1, test(0, 0));
	EXPECT_EQ(3, test(0, 1));
	EXPECT_EQ(7, test(1, 0));
	EXPECT_EQ(9, test(1, 1));

	test = mat.submatrix(2, 1);
	EXPECT_EQ(1, test(0, 0));
	EXPECT_EQ(3, test(0, 1));
	EXPECT_EQ(4, test(1, 0));
	EXPECT_EQ(6, test(1, 1));

	test = mat.submatrix(2, 2);
	EXPECT_EQ(1, test(0, 0));
	EXPECT_EQ(2, test(0, 1));
	EXPECT_EQ(4, test(1, 0));
	EXPECT_EQ(5, test(1, 1));

	mat = Matrix<int>({1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12});
	mat.reshape(3, 4);

	test = mat.submatrix(2, 1);
	EXPECT_EQ(1, test(0, 0));
	EXPECT_EQ(3, test(0, 1));
	EXPECT_EQ(4, test(0, 2));
	EXPECT_EQ(5, test(1, 0));
	EXPECT_EQ(7, test(1, 1));
	EXPECT_EQ(8, test(1, 2));

	mat = Matrix<int>({1, 2, 3, 4});
	mat.reshape(2, 2);
	test = mat.submatrix(1, 1);
	EXPECT_EQ(1, test(0, 0));
	EXPECT_EQ(size_t(1), test.size());

	mat.reshape(1, 2);
	EXPECT_ANY_THROW(mat.submatrix(0, 0));
	EXPECT_ANY_THROW(mat.submatrix(2, 1));
	mat.reshape(2, 2);
	EXPECT_ANY_THROW(mat.submatrix(2, 2));
	EXPECT_ANY_THROW(mat.submatrix(2, 1));
	EXPECT_ANY_THROW(mat.submatrix(4, 4));
}

TEST(Matrix, determinant)
{
	auto mat = Matrix<int>({1, 2, 3, 4, 5, 6, 7, 8, 9});
	mat.reshape(3, 3);
	EXPECT_EQ(0, mat.determinant());
	mat = Matrix<int>({1, 2, 3, 2, 3, 1, 3, 1, 2});
	mat.reshape(3, 3);
	EXPECT_EQ(-18, mat.determinant());
	mat = Matrix<int>({1, 2, 3, 4, 5, 6, 7, 8});
	mat.reshape(2, 4);
	ASSERT_DEBUG_DEATH(mat.determinant(), "");

	// TODO test in debug
}

TEST(Matrix, adjoint)
{
	auto mat = Matrix<int>({1, 2, 3, 2, 3, 1, 3, 1, 2});
	mat.reshape(3, 3);
	auto mat2 = mat.adjoint();
	EXPECT_EQ(5, mat2(0, 0));
	EXPECT_EQ(-1, mat2(0, 1));
	EXPECT_EQ(-7, mat2(0, 2));
	EXPECT_EQ(-1, mat2(1, 0));
	EXPECT_EQ(-7, mat2(1, 1));
	EXPECT_EQ(5, mat2(1, 2));
	EXPECT_EQ(-7, mat2(2, 0));
	EXPECT_EQ(5, mat2(2, 1));
	EXPECT_EQ(-1, mat2(2, 2));

	mat = Matrix<int>({1, 2, 2, 3});
	mat.reshape(2, 2);
	mat2 = mat.adjoint();
	EXPECT_EQ(3, mat2(0, 0));
	EXPECT_EQ(-2, mat2(0, 1));
	EXPECT_EQ(-2, mat2(1, 0));
	EXPECT_EQ(1, mat2(1, 1));

	mat.reshape(2, 4);
	ASSERT_DEBUG_DEATH(mat.adjoint(), "");
}

TEST(Matrix, inverse)
{
	auto mat = Matrix<int>({1, 2, 3, 2, 3, 1, 3, 1, 2});
	mat.reshape(3, 3);
	auto mat2 = mat.inverse();
	EXPECT_FLOAT_EQ(5. / -18., mat2(0, 0));
	EXPECT_FLOAT_EQ(-1. / -18., mat2(0, 1));
	EXPECT_FLOAT_EQ(-7. / -18., mat2(0, 2));
	EXPECT_FLOAT_EQ(-1. / -18., mat2(1, 0));
	EXPECT_FLOAT_EQ(-7. / -18., mat2(1, 1));
	EXPECT_FLOAT_EQ(5. / -18., mat2(1, 2));
	EXPECT_FLOAT_EQ(-7. / -18., mat2(2, 0));
	EXPECT_FLOAT_EQ(5. / -18., mat2(2, 1));
	EXPECT_FLOAT_EQ(-1. / -18., mat2(2, 2));

	mat = Matrix<int>({1, 2, 2, 3});
	mat.reshape(2, 2);
	mat2 = mat.inverse();
	EXPECT_FLOAT_EQ(3. / -1., mat2(0, 0));
	EXPECT_FLOAT_EQ(-2. / -1., mat2(0, 1));
	EXPECT_FLOAT_EQ(-2. / -1., mat2(1, 0));
	EXPECT_FLOAT_EQ(1. / -1., mat2(1, 1));

#ifndef NDEBUG
	mat.reshape(2, 4);
	ASSERT_DEBUG_DEATH(mat.inverse(), "");
#endif
	mat = Matrix<int>({1, 2, 3, 4, 5, 6, 7, 8, 9});
	mat.reshape(3, 3);
	EXPECT_ANY_THROW(mat.inverse());
}
}  // namespace cypress
