/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2016  Andreas Stöckel
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

#include <cmath>

#include <fstream>

#include "gtest/gtest.h"

#include <cypress/nef/tuning_curve.hpp>

namespace cypress {
namespace nef {

TEST(tuning_curve, generate_and_evaluate)
{
	TuningCurveEvaluator tev;
	auto res = tev.evaluate_output_spike_train(tev.input_spike_train());
	std::ofstream os("data.txt");
	for (auto p: res) {
		os << p.first << "\t" << p.second << std::endl;
	}
}

}
}

