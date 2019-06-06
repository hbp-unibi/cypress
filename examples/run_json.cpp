/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2019  Andreas Ostrau
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

// Include first to avoid "_POSIX_C_SOURCE redefined" warning
#include <cypress/cypress.hpp>

#include <fstream>
#include <iostream>

using namespace cypress;

int main(int argc, const char *argv[])
{
	if (argc != 2 && !NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0] << " <file>" << std::endl;
		return 1;
	}

	std::ifstream file_in;
	file_in.open(std::string(argv[1]) + ".cbor", std::ios::binary);
	Json json = Json::from_cbor(file_in);
	file_in.close();

	Network netw = json["network"].get<Network>();
	auto backend = netw.make_backend(json["simulator"].get<std::string>(), argc,
	                                 argv, json["setup"]);
	netw.run(*backend, json["duration"].get<Real>());

	std::ofstream file_out;
	file_out.open(std::string(argv[1]) + "_res.cbor", std::ios::binary);
	Json::to_cbor(Json(netw), file_out);
	file_out.close();
	return 0;
}
