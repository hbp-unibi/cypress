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

# Required as the Python code is usually concatenated into a single file and
# embedded in the Cypress C++ library.
try:
    from binnf import *
    from cypress import *
except:
    pass

def do_run(args):
    import sys

    # Fetch the input/output file
    in_filename = getattr(args, "in")
    in_fd = sys.stdin if in_filename == '-' else open(in_filename, "rb")
    out_filename = getattr(args, "out")
    out_fd = sys.stdout if out_filename == '-' else open(out_filename, "rb")

    # Read the input data
    network = read_network(in_fd)

    # Open the simulator and run the network
    res = Cypress(
        args.simulator,
        args.library).run(
        network,
        duration=args.duration)

    # Write the recorded data back to binnf
    write_result(out_fd, res)


def do_dump(args):
    import numpy as np
    import sys

    # Set some numpy print options
    np.set_printoptions(precision=3, suppress=True)

    # Fetch the input file
    in_filename = getattr(args, "in")
    in_fd = sys.stdin if in_filename == '-' else open(in_filename, "rb")

    # Dump the contents of the binnf stream
    while True:
        # Deserialise the input stream
        name, header, matrix = deserialise(in_fd)
        if name is None:
            return

        # Pretty-print the matrix
        sys.stdout.write("== " + name + " ==\n\n")

        # Generate the table columns
        table = map(lambda x: [x["name"]], header)
        for i in xrange(len(header)):
            table[i] = table[i] + (map(str, matrix[header[i]["name"]]))

        # Justify the columns
        max_col_len = map(lambda x: max(map(len, x)), table)
        table = map(lambda c_l: map(lambda s: " " *
                                    (c_l[1] -
                                     len(s)) +
                                    s, c_l[0]), zip(table, max_col_len))

        # Print the table
        for i in xrange(len(matrix) + 1):
            for j in xrange(len(header)):
                if j > 0:
                    sys.stdout.write(" | ")
                sys.stdout.write(table[j][i])
            sys.stdout.write("\n")
        sys.stdout.write("\n")
        sys.stdout.flush()

#
# Parse the command line
#
import argparse
parser = argparse.ArgumentParser(
    description="Command line interface to the Python part of Cypress")
sp = parser.add_subparsers()

# Options for the "run" subcommand
sp_run = sp.add_parser("run", help="Executes a neural network")
sp_run.add_argument(
    "--simulator",
    type=str,
    help="Canonical name of the simulator", required=True)
sp_run.add_argument(
    "--library",
    type=str,
    help="PyNN backend to use (e.g. pyNN.nest)", required=True)
sp_run.add_argument(
    "--duration",
    type=float,
    help="Duration of the simulation", required=True)
sp_run.add_argument(
    "--in",
    type=str,
    default="-",
    help="Input binnf filename, use \"-\" for stdin")
sp_run.add_argument(
    "--out",
    type=str,
    default="-",
    help="Output binnf filename, use \"-\" for stdin")
sp_run.set_defaults(func=do_run)

# Options for the "dump" subcommand
sp_dump = sp.add_parser("dump", help="Decodes a binnf data stream")
sp_dump.add_argument(
    "--in",
    type=str,
    default="-",
    help="Input binnf filename, use \"-\" for stdin")
sp_dump.set_defaults(func=do_dump)

# Parse the command line arguments and execute the associated function
args = parser.parse_args()
args.func(args)

