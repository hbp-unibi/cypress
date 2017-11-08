/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2017 Christoph Jenzen
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <chrono>
#include <fstream>
#include <iostream>

#include <cypress/cypress.hpp>

using namespace cypress;

int main(int argc, const char *argv[])
{
	if (argc != 4 && !NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0] << " <SIMULATOR> <#SOURCE NEURONS> <#TARGET NEURONS>" << std::endl;
		return 1;
	}

	global_logger().min_level(LogSeverity::INFO);
    
    size_t n_source = std::stoi(argv[2]);
    size_t n_tar = std::stoi(argv[3]);
    
    using namespace std::chrono;
    system_clock::time_point t1, t2;
    
    
	auto net =
	    Network()
	        // Add a named population of poisson spike sources
	        .add_population<SpikeSourceArray>(
	            "source", n_source, SpikeSourceArrayParameters().spike_times(std::vector<Real>({0.0})))
	        .add_population<IfFacetsHardware1>(
	            "target", n_tar, IfFacetsHardware1Parameters(),
	            IfFacetsHardware1Signals().record_spikes())
	        .add_connection("source", "target",
	                        Connector::all_to_all(0.001, 1));

    t1 = system_clock::now();
	net.run(argv[1], 1.0, argc, argv);
    t2 = system_clock::now();
    auto time = duration_cast<milliseconds>(t2 - t1);

    net =
	    Network()
	        // Add a named population of poisson spike sources
	        .add_population<SpikeSourceArray>(
	            "source", n_source, SpikeSourceArrayParameters().spike_times(std::vector<Real>({0.0})))
	        .add_population<IfFacetsHardware1>(
	            "target", n_tar, IfFacetsHardware1Parameters(),
	            IfFacetsHardware1Signals().record_spikes())
	        .add_connection("source", "target",
	                        Connector::fixed_probability(Connector::all_to_all(0.001, 1), 1));

    t1 = system_clock::now();
	net.run(argv[1], 1.0, argc, argv);
    t2 = system_clock::now();
    auto time2 = duration_cast<milliseconds>(t2 - t1);

    std::cout << "Group Connector time : \t" << time.count()<<std::endl;
    std::cout << "FromList Connector time : \t" << time2.count()<<std::endl;

	return 0;
}

