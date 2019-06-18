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
	if (argc != 2 && argc != 3 && !NMPI::check_args(argc, argv)) {
		std::cout << "Usage: " << argv[0] << " <file> [bin]" << std::endl
		          << "Set [bin] to read a .cbor" << std::endl;
		return 0;
	}

	std::ifstream file_in;
	Json json;
	if (argc == 3) {
		file_in.open(std::string(argv[1]) + ".cbor", std::ios::binary);
		json = Json::from_cbor(file_in);
	}
	else {
		file_in.open(std::string(argv[1]) + ".json", std::ios::binary);
		json = Json::parse(file_in);
	}
	file_in.close();
	global_logger().min_level(LogSeverity(json["log_level"].get<int32_t>()));
	Network netw = json["network"].get<Network>();
	auto backend = netw.make_backend(json["simulator"].get<std::string>(), argc,
	                                 argv, json["setup"]);
	netw.run(*backend, json["duration"].get<Real>());

	std::ofstream file_out;
	if (argc == 3) {
		file_out.open(std::string(argv[1]) + "_res.cbor", std::ios::binary);
		Json::to_cbor(Json(netw), file_out);
	}
	else {
		file_out.open(std::string(argv[1]) + "_res.json", std::ios::binary);
		file_out << Json(netw);
	}
	file_out.close();
	return 0;
}
