# -*- coding: utf-8 -*-

#   Cypress -- C++ Neural Associative Memory Simulator
#   Copyright (C) 2016 Andreas Stöckel
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
Tests the "serialise" and "deseralise" methods in the pynnless_binnf module
"""

import unittest
import numpy
import StringIO

import binnf
import numpy.testing

class TestBinnf(unittest.TestCase):

    def generate_test_block(self):
        name = "test_matrix";
        header = [
            {"name": "col1", "type": binnf.TYPE_INT},
            {"name": "col2", "type": binnf.TYPE_FLOAT},
            {"name": "col3", "type": binnf.TYPE_INT}
        ]

        fmt = binnf.header_to_dtype(header)
        matrix = numpy.zeros((100), dtype=fmt)
        for i in xrange(matrix.shape[0]):
            for j in xrange(len(fmt)):
                matrix[i][j] = i * (j + 1)
        return name, header, matrix

    def test_read_write(self):
        # Generate some test data
        name_in, header_in, matrix_in = self.generate_test_block()

        # Write the test data to the input stream
        stream = StringIO.StringIO()
        binnf.serialise(stream, name_in, header_in, matrix_in)

        # Deserialise the data
        stream.seek(0)
        name_out, header_out, matrix_out = binnf.deseralise(stream)

        # Make sure the deserialised data is correct
        self.assertEqual(name_out, name_in)
        self.assertEqual(header_out, header_in)
        numpy.testing.assert_equal(matrix_out, matrix_in)

        stream.close()

