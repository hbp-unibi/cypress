#!/usr/bin/env python
# -*- coding: utf-8 -*-

#   Cypress -- A C++ interface to PyNN
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
Reads a binary neural network specification from the specified file and executes
it. Writes the recorded data to the specified binnf file. Used as a interface
between C++ code and PyNN.
"""

import sys
import os

import cypress
import binnf

# Check the script parameters
if len(sys.argv) != 4 and len(sys.argv) != 2:
    print("Usage: ./binnf.py <SIMULATOR> [<IN> <OUT>]")
    print("Where <IN> and <OUT> may be \"-\" to read from/write to "
            + " stdin/stdout (default)")
    sys.exit(1)

# Fetch the simulator name from the command line parameters
simulator = sys.argv[1]

# Fetch the input/output file
input_file = (sys.stdin if (len(sys.argv) < 3
        or sys.argv[2] == "-") else open(sys.argv[2], 'rb'))
output_file = (sys.stdout if (len(sys.argv) < 4
        or sys.argv[3] == "-") else open(sys.argv[3], 'wb'))

# Read the input data
network = binnf.read_network(input_file)

# Open the simulator and run the network
res = cypress.Cypress(simulator, "pyNN." + simulator).run(network, duration=1000.0)

print res

