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

import numpy as np
from numpy.linalg import inv
import math
import matplotlib
import matplotlib.pyplot as plt
import sys

if len(sys.argv) < 2:
    print "Usage: " + sys.argv[0] + " <TUNING CURVE INPUT>"
    sys.exit(1)

def cm2inch(value):
    return value / 2.54

input_file = sys.argv[1]
data = np.mat(np.loadtxt(input_file, delimiter=","))

# Fetch the design matrix Phi and the number of samples/functions
Phi = data[:, 1:]
n_samples = Phi.shape[0]
n_func = Phi.shape[1]

# Construct input and desired output vectors
X = data[:, 0]
Y = (np.sin(X * 2.0 * math.pi) + 1.0) * 0.5 # Target function

# Calculate the Moore-Penrose pseudo inverse of Phi
lambda_ = 0.2
PhiI = inv(Phi.T * Phi + lambda_ * np.eye(n_func)) * Phi.T

# Calculate the weights
w = PhiI * Y
print("Maximum w: " + str(np.max(w)))
print("Minimum w: " + str(np.min(w)))

# Reconstruct the function
YRec = Phi * w
print(w)

fig = plt.figure(figsize=(cm2inch(7), cm2inch(7 )))
ax = fig.gca()
n_pop = data.shape[1] - 1
ax.plot(X, Y, ':', color=[0.25, 0.25, 0.25])
ax.plot(X, YRec, '-', color=[0.0, 0.0, 0.0])
ax.set_xlabel("Input value")
ax.set_ylabel("Function value")
ax.set_title("Decoding of $\\frac{1}2 \\cdot (\\sin(x * 2\\pi) + 1)$")
ax.set_xlim(0.0, 1.0)

fig.savefig("reconstructed_function.pdf", format='pdf',
        bbox_inches='tight')

