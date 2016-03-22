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

#include <cypress/nef/spike_train.hpp>

namespace cypress {
namespace nef {

TEST(discrete_window, gauss)
{
	constexpr float SQRT_PI = 1.772453851;
	for (float alpha = 0.0; alpha < 2.0; alpha += 0.1) {
		for (float sigma = 0.1; sigma < 2.0; sigma += 0.1) {
			DiscreteWindow window =
			    DiscreteWindow::create<GaussWindow>(alpha, sigma);
			EXPECT_NEAR(SQRT_PI * alpha * sigma, window.integral(), 0.01);
			EXPECT_NEAR(0.5f * SQRT_PI * alpha * sigma, window.integralToZero(),
			            0.01);
		}
	}
}

TEST(discrete_window, exponential)
{
	for (float alpha = 0.0; alpha < 2.0; alpha += 0.1) {
		for (float sigma = 0.1; sigma < 2.0; sigma += 0.1) {
			DiscreteWindow window =
			    DiscreteWindow::create<ExponentialWindow>(alpha, sigma);
			EXPECT_NEAR(alpha * sigma, window.integral(), 0.01);
			EXPECT_EQ(0.0, window.integralToZero());
		}
	}
}

TEST(discrete_window, choose_params)
{
	float min_spike_interval = 1e-3;
	float response_time = 10e-3;

	{
		const auto params = DiscreteWindow::choose_params<GaussWindow>(
		    min_spike_interval, response_time);
/*		EXPECT_NEAR(response_time,
		            DiscreteWindow::calculate_response_time<GaussWindow>(
		                params.first, params.second, min_spike_interval),
		            min_spike_interval);
		EXPECT_NEAR(DiscreteWindow::choose_alpha<GaussWindow>(
		                params.second, min_spike_interval),
		            params.first, 1e-6);*/

		std::cout << params.first << "; " << params.second << std::endl;
	}

/*	{
		const auto params = DiscreteWindow::choose_params<ExponentialWindow>(
		    min_spike_interval, response_time);
		EXPECT_NEAR(response_time,
		            DiscreteWindow::calculate_response_time<ExponentialWindow>(
		                params.first, params.second, min_spike_interval),
		            min_spike_interval);
		EXPECT_NEAR(DiscreteWindow::choose_alpha<ExponentialWindow>(
		                params.second, min_spike_interval),
		            params.first, 1e-6);
	}*/
}

TEST(spike_train, decode)
{
	const std::vector<float> spikes = {-20e-3, 10e-3, 50e-3, 110e-3};

	const float alpha = 1.0;
	const float sigma = 0.01;
	const float step = 0.1e-3;

	std::vector<float> vals = SpikeTrain::decode(
	    spikes, DiscreteWindow::create<GaussWindow>(alpha, sigma, step), 0.0,
	    60e-3, 0.0, 1.0);
	float x = 0.0;
	for (float val : vals) {
		float y = 0;
		for (float spike : spikes) {
			y += GaussWindow::value((x - spike) / sigma) * alpha;
		}
		EXPECT_NEAR(y, val, 0.001);
		x += step;
	}
}
}
}
