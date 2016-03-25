#!/usr/bin/env python
# -*- coding: utf-8 -*-

#  Cypress -- C++ Spiking Neural Network Simulation Framework
#  Copyright (C) 2016  Andreas Stöckel
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""
Plots the information for an experiment result as created with ./run.py
"""

import numpy as np
import math
import matplotlib
import matplotlib.pyplot as plt
import sys

if len(sys.argv) < 2:
    print "Usage: " + sys.argv[0] + " <TUNING CURVE INPUT>"
    sys.exit(1)

def cm2inch(value):
    return value / 2.54

data = np.loadtxt(sys.argv[1], delimiter=",")

fig = plt.figure(figsize=(cm2inch(7), cm2inch(7 )))
ax = fig.gca()
n_pop = data.shape[1] - 1
for i in xrange(1, n_pop + 1):
    color = np.array([0.75, 0.0, 0.0] if i % 2 == 0 else [0.0, 0.25, 0.75])
    color = (np.array([1.0, 1.0, 1.0]) - color) * i * 0.75 / float(n_pop) + color
    ax.plot(data[:, 0], data[:, i], '-', color=color)
ax.set_xlabel("Normalised input spike rate")
ax.set_ylabel("Normalised output spike rate")
ax.set_xlim(0.0, 1.0)

fig.savefig("tuning_curves.pdf", format='pdf',
        bbox_inches='tight')

