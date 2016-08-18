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
import matplotlib.pyplot as plt
import sys
import re

if len(sys.argv) < 2:
    print "Usage: " + sys.argv[0] + " <EPSP_FILES...>"
    sys.exit(1)


def cm2inch(value):
    return value / 2.54

print("Loading data...")
min_v = np.inf; max_v = -np.inf
data = {}
for fn in sys.argv[1:]:
    m = re.search("epsp_([A-Za-z]+)_w_([0-9.]+)_g_leak_([0-9.]+)\\.csv", fn)
    sim = m.group(1)
    w = float(m.group(2))
    g_leak = float(m.group(3))
    values = np.genfromtxt(fn,delimiter=',')
    max_v = max(max_v, np.max(values[:, 1]))
    min_v = min(min_v, np.min(values[:, 1]))
    data[(w, g_leak, sim)] = values

ws = list(np.unique(np.array(map(lambda x: x[0], data.keys()))))
g_leaks = list(np.unique(np.array(map(lambda x: x[1], data.keys()))))

print("Plotting data...")
fig = plt.figure(figsize=(cm2inch(40), cm2inch(50)))
for key, value in sorted(data.items()):
    w, g_leak, sim = key
    i = ws.index(w) * len(g_leaks) + g_leaks.index(g_leak) + 1
    ax = fig.add_subplot(len(ws), len(g_leaks), i)
    ax.plot(value[:, 0], value[:, 1])
    ax.set_ylim(min_v, max_v)
    ax.set_title("$w = %.3f$ $g_L = %.1f$" % (w, g_leak * 1000.0))

print("Saving to 'epsps.pdf'...")
fig.tight_layout()
fig.savefig("epsps.pdf", format='pdf',
            bbox_inches='tight')

