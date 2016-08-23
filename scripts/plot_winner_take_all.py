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
import math
import matplotlib
import matplotlib.lines as mlines
import matplotlib.patches as mpatches
import matplotlib.pyplot as plt
import sys

if len(sys.argv) != 2:
    print "Usage: " + sys.argv[0] + " <WTA_OUT>"
    sys.exit(1)

def cm2inch(value):
    return value / 2.54

def load_spiketrain(fn, delimiter=","):
    res = []
    with open(fn) as f:
        for s in f:
            if (len(s) > 1):
                res.append(map(float, s[:-1].split(delimiter)))
            else:
                res.append([])
    return res

def plot_spiketrain(train, ax, offs = 0, color="k"):
    for (i, times) in enumerate(train):
        ax.plot(times, [i] * len(times), '.', color=color, markersize=2.5)

output_spikes = load_spiketrain(sys.argv[1])

fig = plt.figure(figsize=(cm2inch(17), cm2inch(8)))
ax = fig.gca()
plot_spiketrain(output_spikes, ax)

ax.set_xlabel("Simulation time [ms]");
ax.set_ylabel("Neuron index");

fig.savefig("winner_take_all_result.png", format='png',
            bbox_inches='tight', dpi=160)

