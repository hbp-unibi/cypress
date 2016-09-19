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

#include <cypress/nef/delta_sigma.hpp>

namespace cypress {
namespace nef {

TEST(discrete_window, gauss)
{
	constexpr Real SQRT_PI = 1.772453851;
	for (Real alpha = 0.0; alpha < 2.0; alpha += 0.1) {
		for (Real sigma = 0.01; sigma < 0.1; sigma += 0.01) {
			auto window = DeltaSigma::DiscreteWindow::create_manual<
			    DeltaSigma::GaussWindow>(alpha, sigma);
			EXPECT_NEAR(SQRT_PI * alpha * sigma, window.integral(), 0.01);
			EXPECT_NEAR(0.5f * SQRT_PI * alpha * sigma,
			            window.integral_to_zero(), 0.01);
		}
	}
}

TEST(discrete_window, exponential)
{
	for (Real alpha = 0.0; alpha < 2.0; alpha += 0.1) {
		for (Real sigma = 0.01; sigma < 0.1; sigma += 0.01) {
			auto window = DeltaSigma::DiscreteWindow::create_manual<
			    DeltaSigma::ExponentialWindow>(alpha, sigma);
			EXPECT_NEAR(alpha * sigma, window.integral(), 0.01);
			EXPECT_EQ(0.0, window.integral_to_zero());
		}
	}
}

TEST(delta_sigma, decode)
{
	const std::vector<Real> spikes = {-20e-3, 10e-3, 50e-3, 110e-3};

	const Real alpha = 1.0;
	const Real sigma = 0.01;
	const Real step = 0.1e-3;

	std::vector<Real> vals = DeltaSigma::decode(
	    spikes,
	    DeltaSigma::DiscreteWindow::create_manual<DeltaSigma::GaussWindow>(
	        alpha, sigma, step),
	    0.0, 60e-3, 0.0, 1.0);
	Real x = 0.0;
	for (Real val : vals) {
		Real y = 0;
		for (Real spike : spikes) {
			y += DeltaSigma::GaussWindow::value((x - spike) / sigma) * alpha;
		}
		EXPECT_NEAR(y, val, 0.001);
		x += step;
	}
}

TEST(delta_sigma, encode)
{
	// Step function
	auto f = [](Real x) -> Real { return x < 0.5 ? 0.0 : 1.0; };

	const DeltaSigma::DiscreteWindow wnd =
	    DeltaSigma::DiscreteWindow::create<DeltaSigma::GaussWindow>();

	// Expect full spike rate after the step function goes to one
	const std::vector<Real> spikes =
	    DeltaSigma::encode(f, wnd, 0.0, 1.0, 0.0, 1.0);
	EXPECT_LT(499U, spikes.size());
	for (size_t i = 0; i < spikes.size(); i++) {
		EXPECT_NEAR((i + 501.0) * 1e-3, spikes[i], 1e-3);
	}
}

template <typename Window, typename Fun>
static void test_fun(const Fun &f, Real t0, Real t1, Real min_val,
                     Real max_val)
{
	const DeltaSigma::DiscreteWindow wnd =
	    DeltaSigma::DiscreteWindow::create<Window>();
	const std::vector<Real> spikes =
	    DeltaSigma::encode(f, wnd, t0, t1, min_val, max_val);
	const std::vector<Real> values =
	    DeltaSigma::decode(spikes, wnd, t0, t1, min_val, max_val);

	Real rmse = 0.0;
	for (size_t i = 0; i < values.size(); i++) {
		const Real x = t0 + i * wnd.step();
		const Real err = values[i] - f(x);
		rmse += err * err;
	}
	rmse = std::sqrt(rmse / Real(values.size())) / (max_val - min_val);
	EXPECT_GT(wnd.alpha(), rmse);
}

TEST(delta_sigma, encode_decode_sine)
{
	test_fun<DeltaSigma::GaussWindow>([](Real x) { return sin(x); }, 0.0, 10.0,
	                                  -1.0, 1.0);
	test_fun<DeltaSigma::ExponentialWindow>([](Real x) { return sin(x); }, 0.0,
	                                        10.0, -1.0, 1.0);
}

TEST(delta_sigma, encode_decode_fast_sine)
{
	test_fun<DeltaSigma::GaussWindow>([](Real x) { return sin(10.0 * x); },
	                                  0.0, 10.0, -1.0, 1.0);
	test_fun<DeltaSigma::ExponentialWindow>(
	    [](Real x) { return sin(10.0 * x); }, 0.0, 10.0, -1.0, 1.0);
}

TEST(delta_sigma, encode_decode_very_fast_sine)
{
	test_fun<DeltaSigma::GaussWindow>([](Real x) { return sin(64.0 * x); },
	                                  0.0, 10.0, -1.0, 1.0);
	test_fun<DeltaSigma::ExponentialWindow>(
	    [](Real x) { return sin(64.0 * x); }, 0.0, 10.0, -1.0, 1.0);
}

TEST(delta_sigma, encode_decode_cosine)
{
	test_fun<DeltaSigma::GaussWindow>([](Real x) { return cos(x); }, 0.0, 10.0,
	                                  -1.0, 1.0);
	test_fun<DeltaSigma::ExponentialWindow>([](Real x) { return cos(x); }, 0.0,
	                                        10.0, -1.0, 1.0);
}

TEST(delta_sigma, encode_decode_step)
{
	test_fun<DeltaSigma::GaussWindow>([](Real x) {
		return (x > 0.25 && x <= 0.75) ? 1.0 : 1.0;
	}, 0.0, 1.0, 0.0, 1.0);
	test_fun<DeltaSigma::ExponentialWindow>([](Real x) {
		return (x > 0.25 && x <= 0.75) ? 1.0 : 1.0;
	}, 0.0, 1.0, 0.0, 1.0);
}

TEST(delta_sigma, encode_decode_linear)
{
	test_fun<DeltaSigma::GaussWindow>([](Real x) { return 30.0 * x - 10.0; },
	                                  0.0, 1.0, -10.0, 20.0);
	test_fun<DeltaSigma::ExponentialWindow>(
	    [](Real x) { return 30.0 * x - 10.0; }, 0.0, 1.0, -10.0, 20.0);
}
}
}
