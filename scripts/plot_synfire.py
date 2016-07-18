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

if len(sys.argv) != 4:
    print "Usage: " + sys.argv[0] + " <SYNFIRE_INPUT> <SYNFIRE_RS> <SYNFIRE_FS>"
    sys.exit(1)

pop_size = 8

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

def plot_spiketrain(train, ax, offs = 0, color="k", bg_color=None):
    for (i, times) in enumerate(train):
        grp = i // pop_size
        y = grp * pop_size * 2 + i % pop_size + offs
        ax.plot(times, [y] * len(times), '.', color=color, markersize=2.5)
        if bg_color:
            ax.add_patch(mpatches.Rectangle((0.0, y // pop_size * pop_size), 2000.0, pop_size, color=bg_color, linewidth=0))

input_spikes = load_spiketrain(sys.argv[1])
output_spikes_rs = load_spiketrain(sys.argv[2])
output_spikes_fs = load_spiketrain(sys.argv[3])

fig = plt.figure(figsize=(cm2inch(17), cm2inch(8)))
ax = fig.gca()
ax.yaxis.set_ticks(np.arange(0, len(output_spikes_rs) * 2 + 1, pop_size * 2))
plot_spiketrain(input_spikes, ax, -pop_size, "#000000", "#f3f3f3")
plot_spiketrain(output_spikes_rs, ax, 0, "#cc0000")
plot_spiketrain(output_spikes_fs, ax, pop_size, "#3465a4", "#d1e2f4")

ax.set_xlabel("Simulation time [ms]");
ax.set_ylabel("Neuron index");

handles = [
    mlines.Line2D([], [], marker='.', color="#000000", markersize=6,
                  linewidth=0, label="Input spikes"),
    mlines.Line2D([], [], marker='.', color="#cc0000", markersize=6,
                  linewidth=0, label="Excitatory pop."),
    mlines.Line2D([], [], marker='.', color="#3465a4", markersize=6,
                  linewidth=0, label="Inhibitory pop."),
]
ax.legend(handles=handles, loc=9, bbox_to_anchor=(0.5, 1.1), numpoints=1, ncol=3)

fig.savefig("synfire_result.pdf", format='pdf',
            bbox_inches='tight')

