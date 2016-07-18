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

# CMake version and project metadata
cmake_minimum_required(VERSION 3.0)
project(cypress)

add_executable(raw_binnf raw_binnf)
target_link_libraries(raw_binnf cypress)

add_executable(simple simple)
target_link_libraries(simple cypress)

add_executable(measure_tuning_curves measure_tuning_curves)
target_link_libraries(measure_tuning_curves cypress)

add_executable(inhibitory_synapses inhibitory_synapses)
target_link_libraries(inhibitory_synapses cypress)

add_executable(synfire synfire)
target_link_libraries(synfire cypress)

