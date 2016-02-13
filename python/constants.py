# -*- coding: utf-8 -*-

#   cypress -- A C++ interface to PyNN
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

# Constants for the supported neuron types
TYPE_SOURCE = "SpikeSourceArray"
TYPE_IF_COND_EXP = "IF_cond_exp"
TYPE_AD_EX = "EIF_cond_exp_isfa_ista"
TYPE_IF_SPIKEY = "IF_facets_hardware1"
TYPES = [TYPE_SOURCE, TYPE_IF_COND_EXP, TYPE_AD_EX, TYPE_IF_SPIKEY]

# Constants for the quantities that can be recorded
SIG_SPIKES = "spikes"
SIG_V = "v"
SIG_GE = "gsyn_exc"
SIG_GI = "gsyn_inh"
SIGNALS = [SIG_SPIKES, SIG_V, SIG_GE, SIG_GI]

