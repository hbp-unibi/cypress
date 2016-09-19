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

try:
    from binnf import *
except:
    pass

import sys

# Simply read data from stdin and serialise it back
while True:
    block_type, res = deserialise(sys.stdin)
    if block_type == BLOCK_TYPE_MATRIX:
        name, header, matrix = res
        serialise_matrix(sys.stdout, name, matrix)
    elif block_type == BLOCK_TYPE_LOG:
        time, severity, module, msg = res
        serialise_log(sys.stdout, time, severity, module, msg)
    if res is None:
        break
