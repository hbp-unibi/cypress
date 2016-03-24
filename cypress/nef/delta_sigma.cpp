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

#include <cypress/nef/delta_sigma.hpp>

namespace cypress {
namespace nef {

template <typename Window>
std::pair<float, float>
DeltaSigma::DiscreteWindow::calculate_alpha_and_response_time(
    float spike_interval, float sigma, float step, float eps, float p)
{
	DiscreteWindow wnd =
	    DiscreteWindow::create_manual<Window>(1.0f, sigma, step, eps);
	std::vector<float> sum(wnd.begin(), wnd.end());
	sum.resize(2 * sum.size());
	const float max_t = sum.size() * step;
	const float i_step = 1.0f / step;
	float max = 0;
	for (float t = wnd.size() * step * 0.5f; t < max_t; t += spike_interval) {
		size_t offs = std::round(t * i_step);
		for (size_t i = offs; i < sum.size(); i++) {
			if (i - offs < wnd.size()) {
				sum[i] += wnd[i - offs];
				if (sum[i] > max) {
					max = sum[i];
				}
			}
		}
	}

	float response_time = 0;
	size_t i0 = 0;
	for (size_t i = 0; i < sum.size(); i++) {
		if (sum[i] > max * p && i0 == 0) {
			i0 = i;
		}
		if (sum[i] > max * (1.0f - p)) {
			response_time = (i - i0) * step;
			break;
		}
	}

	return std::make_pair(1.0f / max, response_time);
}

template <typename Window>
std::pair<float, float> DeltaSigma::DiscreteWindow::choose_params(
    float min_spike_interval, float response_time, float step, float eps)
{
	float sigma = 0.1 * response_time;
	float min_sigma = 0.0;
	float max_sigma = sigma;

	while (max_sigma - min_sigma > eps) {
		const float cur_response_time =
		    calculate_alpha_and_response_time<Window>(min_spike_interval, sigma,
		                                              step, eps)
		        .second;

		// Abort if the response time is reached with enough precision
		if (std::fabs(response_time - cur_response_time) <= step) {
			break;
		}

		if (cur_response_time > response_time) {
			max_sigma = sigma;
			sigma = (min_sigma + sigma) * 0.5;
		}
		else if (cur_response_time < response_time) {
			min_sigma = sigma;
			if (sigma >= max_sigma) {
				sigma = sigma * 2;
			}
			else {
				sigma = (max_sigma + sigma) * 0.5f;
			}
			if (sigma > max_sigma) {
				max_sigma = sigma;
			}
		}
	}

	const float alpha = calculate_alpha_and_response_time<Window>(
	                        min_spike_interval, sigma, step, eps)
	                        .first;
	return std::make_pair(alpha, sigma);
}

template <typename Window>
DeltaSigma::DiscreteWindow DeltaSigma::DiscreteWindow::create_manual(
    float alpha, float sigma, float step, float eps)
{
	// Calculate the number of samples that should be calculated from the
	// window function -- make sure n_samples is a odd number.
	size_t n_samples =
	    alpha == 0.0f ? 1 : size_t(std::ceil(2.0f * Window::limit(eps / alpha) *
	                                         sigma / step));
	if (n_samples % 2 == 0) {
		n_samples++;
	}

	// Sample the window function, calculate the integral of the discretised
	// window function
	float integral = 0.0;
	float integral_to_zero = 0.0;
	std::vector<float> values(n_samples);
	const float x_offs = -float((n_samples - 1) / 2) * step;
	const float i_sigma = 1.0f / sigma;
	for (size_t i = 0; i < n_samples; i++) {
		const float x = x_offs + i * step;
		const float y = Window::value(x * i_sigma) * alpha;
		if (x < 0.0) {
			integral_to_zero += y;
		}
		integral += y;
		values[i] = y;
	}
	integral *= step;
	integral_to_zero *= step;

	return std::move(DiscreteWindow(alpha, sigma, step, integral,
	                                integral_to_zero, std::move(values)));
}

template <typename Window>
DeltaSigma::DiscreteWindow DeltaSigma::DiscreteWindow::create(
    float min_spike_interval, float response_time, float step, float eps)
{
	auto res =
	    choose_params<Window>(min_spike_interval, response_time, step, eps);
	return create_manual<Window>(res.first, res.second, step, eps);
}

std::vector<float> DeltaSigma::encode(const std::vector<float> &values,
                                      const DiscreteWindow &window, float t0,
                                      float min_val, float max_val,
                                      float min_spike_interval)
{
	const float scale = 1.0 / (max_val - min_val);
	const float integral = window.integral() / window.step();
	const float integral_to_zero = window.integral_to_zero() / window.step();
	const float eps = 1e-6;

	// Result list
	std::vector<float> spikes;

	// Ring buffer storing the current function approximation
	std::vector<float> approx(window.size());
	size_t approx_i = 0;

	// Current accumulation error and spike time
	float err = 0.0f;
	float last_spike_t = -min_spike_interval;

	// Iterate over the values list
	for (size_t i = 0; i < values.size(); i++) {
		// Accumulate the error
		const float val =
		    std::max(min_val, std::min(max_val, (values[i] - min_val) * scale));
		err += val - approx[approx_i];

		// If the error surpasses the window integral, and the minimum interval
		// has passed, issue a new spike
		const float cur_t = t0 + i * window.step();
		if (err > integral &&
		    cur_t - last_spike_t + eps >= min_spike_interval) {
			spikes.emplace_back(cur_t);  // Emit the spike
			last_spike_t = cur_t;        // Update the last spike time
			err -= integral_to_zero;     // Update the error

			// Update the approximation in its ring buffer
			for (size_t j = (window.size() - 1) / 2, approx_j = approx_i;
			     j < window.size(); j++, approx_j++) {
				if (approx_j >= approx.size()) {
					approx_j = 0;
				}
				approx[approx_j] += window[j];
			}
		}

		// Reset the current ring buffer entry and advance the cursor
		approx[approx_i] = 0.0;
		approx_i = (approx_i + 1) % approx.size();
	}
	return spikes;
}

std::vector<float> DeltaSigma::decode(const std::vector<float> &spikes,
                                      const DiscreteWindow &window, float t0,
                                      float t1, float min_val, float max_val)
{
	// Calculate the number of samples in the result
	const float i_step = 1.0f / window.step();
	const size_t n_samples = std::ceil((t1 - t0) * i_step);
	const size_t size = window.size();
	const size_t center = (size - 1) / 2;

	// Iterate over the input spikes and accumulate the windows for each
	// spike
	std::vector<float> res(n_samples);
	for (float spike : spikes) {
		const ptrdiff_t offs = (spike - t0) * i_step - center;
		const size_t i0 = std::max<ptrdiff_t>(0, offs);
		const size_t i1 = std::max<ptrdiff_t>(
		    0, std::min<ptrdiff_t>(res.size(), offs + size));
		const size_t j0 = std::max<ptrdiff_t>(0, i0 - offs);
		for (size_t i = i0, j = j0; i < i1; i++, j++) {
			res[i] += window[j];
		}
	}

	// Rescale the output
	const float scale = max_val - min_val;
	for (size_t i = 0; i < res.size(); i++) {
		res[i] = res[i] * scale + min_val;
	}
	return res;
}

/*
 * Template instantiations
 */

template std::pair<float, float>
DeltaSigma::DiscreteWindow::calculate_alpha_and_response_time<
    DeltaSigma::ExponentialWindow>(float, float, float, float, float);
template std::pair<float, float>
DeltaSigma::DiscreteWindow::calculate_alpha_and_response_time<
    DeltaSigma::GaussWindow>(float, float, float, float, float);

template std::pair<float, float> DeltaSigma::DiscreteWindow::choose_params<
    DeltaSigma::ExponentialWindow>(float, float, float, float);
template std::pair<float, float>
DeltaSigma::DiscreteWindow::choose_params<DeltaSigma::GaussWindow>(float, float,
                                                                   float,
                                                                   float);

template DeltaSigma::DiscreteWindow DeltaSigma::DiscreteWindow::create_manual<
    DeltaSigma::ExponentialWindow>(float, float, float, float);
template DeltaSigma::DiscreteWindow
DeltaSigma::DiscreteWindow::create_manual<DeltaSigma::GaussWindow>(float, float,
                                                                   float,
                                                                   float);

template DeltaSigma::DiscreteWindow
DeltaSigma::DiscreteWindow::create<DeltaSigma::ExponentialWindow>(float, float,
                                                                  float, float);
template DeltaSigma::DiscreteWindow
DeltaSigma::DiscreteWindow::create<DeltaSigma::GaussWindow>(float, float, float,
                                                            float);
}
}
