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

/**
 * @file spike_train.hpp
 *
 * Contains utility functions for generating input spike trains and for decoding
 * output spike trains.
 *
 * @author Andreas Stöckel
 */

#include <iostream>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace cypress {
namespace nef {

/**
 * Function representing a Gaussian window normalised to a range from zero to
 * one with a standard-deviation of one. Convoluting a spike train with this
 * Window function corresponds to calculating a sliding average firing rate.
 */
struct GaussWindow {
	/**
	 * Value of the window function at position x. Multiply x with the standard
	 * deviation to get values for standard-deviations other than one.
	 *
	 * @param x is the position at which the window function should be
	 * evaluated.
	 */
	static constexpr float value(float x) { return std::exp(-x * x); }

	/**
	 * Returns the value of x for which until the value of the window function
	 * reaches "epsilon". Multiply the result with the standard deviation to
	 * get values for standard-deviations other than one.
	 */
	static constexpr float limit(float eps = 1e-6f)
	{
		return std::sqrt(-std::log(eps));
	}

	static constexpr float width(float x) { return 2.0f * limit(x); }
};

/**
 * Function representing an exponential decay starting at x = 0.0;
 */
struct ExponentialWindow {
	/**
	 * Value of the window function at position x. Multiply x with the standard
	 * deviation to get values for standard-deviations other than one.
	 *
	 * @param x is the position at which the window function should be
	 * evaluated.
	 */
	static constexpr float value(float x)
	{
		return x < 0.0f ? 0.0f : std::exp(-x);
	}

	/**
	 * Returns the value of x for which until the value of the window function
	 * reaches "epsilon". Multiply the result with the standard deviation to
	 * get values for standard-deviations other than one.
	 */
	static constexpr float limit(float eps = 1e-6f) { return -std::log(eps); }

	static constexpr float width(float x) { return limit(x); }
};

/**
 * Class representing a discretised window function.
 */
class DiscreteWindow {
private:
	float m_step;
	float m_integral;
	float m_integral_to_zero;
	std::vector<float> m_values;

	DiscreteWindow(float step, float integral, float integral_to_zero,
	               std::vector<float> &&values)
	    : m_step(step),
	      m_integral(integral),
	      m_integral_to_zero(integral_to_zero),
	      m_values(values)
	{
	}

public:
	/**
	 * Constructor of the DiscreteWindow class. Discretises the window given as
	 * template parameter.
	 *
	 * @param alpha is a scale factor the samples are scaled by.
	 * @param sigma is the standard deviation determining the x-axis scale.
	 * @param step is the sampling interval.
	 * @param eps is the epsilon determining up to which value the window is
	 * sampled.
	 */
	template <typename Window>
	static DiscreteWindow create(float alpha, float sigma, float step = 1e-4f,
	                             float eps = 1e-6f)
	{
		// Calculate the number of samples that should be calculated from the
		// window function -- make sure n_samples is a odd number.
		size_t n_samples =
		    alpha == 0.0f ? 1
		                  : size_t(std::ceil(2.0f * Window::limit(eps / alpha) *
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

		return std::move(DiscreteWindow(step, integral, integral_to_zero,
		                                std::move(values)));
	}

	template <typename Window>
	static std::pair<float, float> calculate_alpha_and_response_time(
	    float spike_interval, float sigma, float step, float eps,
	    float p = 0.05f)
	{
		DiscreteWindow wnd =
		    DiscreteWindow::create<Window>(1.0f, sigma, step, eps);
		std::vector<float> sum(wnd.begin(), wnd.end());
		sum.resize(2 * sum.size());
		const float max_t = sum.size() * step;
		const float i_step = 1.0f / step;
		float max = 0;
		for (float t = wnd.size() * step * 0.5f; t < max_t;
		     t += spike_interval) {
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

	/**
	 * Selects the parameters alpha and sigma depending on the given window
	 * function, the minimum spike interval and the desired response time.
	 */
	template <typename Window>
	static std::pair<float, float> choose_params(
	    float min_spike_interval = 1e-3, float response_time = 10e-3,
	    float step = 1e-4f, float eps = 1e-6f)
	{
		float sigma = 0.1 * response_time;
		float min_sigma = 0.0;
		float max_sigma = sigma;

		while (true) {
			auto res = calculate_alpha_and_response_time<Window>(
			    min_spike_interval, sigma, step, eps);
			const float alpha = res.first;
			const float cur_response_time = res.second;

			// Abort if the response time is reached with enough precision
			if (std::fabs(response_time - cur_response_time) <= 2.0f * step) {
				return std::make_pair(alpha, sigma);
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
	}

	/**
	 * Creates a DiscreteWindow with automatically chosen parameters alpha and
	 * sigma.
	 */
	template <typename Window>
	static DiscreteWindow create_auto(float min_spike_interval = 1e-3,
	                                  float response_time = 10e-3,
	                                  float step = 1e-4f, float eps = 1e-6f)
	{
		auto res =
		    choose_params<Window>(min_spike_interval, response_time, step, eps);
		return create<Window>(res.first, res.second, step, eps);
	}

	/**
	 * Returns the sample interval.
	 */
	float step() const { return m_step; }

	/**
	 * Returns the integral of the window function from minus infinity to plus
	 * infinity. Multiply the result with the standard deviation to get values
	 * for standard-deviations other than one.
	 */
	float integral() const { return m_integral; }

	/**
	 * Returns the integral of the window function from minus infinity to zero.
	 */
	float integralToZero() const { return m_integral_to_zero; }

	/**
	 * Returns the number of samples until the value of the window function
	 * reaches "epsilon". Multiply the result with the standard deviation to
	 * get values for standard-deviations other than one.
	 */
	float limit(float eps = 1e-6f) const { return std::sqrt(-std::log(eps)); }

	/**
	 * Returns the number of samples stored in the window.
	 */
	size_t size() const { return m_values.size(); }

	/**
	 * Returns the value of the i-th sample.
	 */
	float operator[](size_t i) const { return m_values[i]; }

	/**
	 * Returns a constant iterator allowing to access the first sample.
	 */
	auto begin() const { return m_values.begin(); }

	/**
	 * Returns an iterator pointing at the last-plus-one sample.
	 */
	auto end() const { return m_values.end(); }
};

/**
 * Structure providing static methods for encoding continuous-valued functions
 * as a series of spikes and vice versa.
 */
struct SpikeTrain {
	static std::vector<float> encode(const std::vector<float> &values,
	                                 const DiscreteWindow &window,
	                                 float t0 = 0.0, float min_val = -1.0,
	                                 float max_val = 1.0);

	/**
	 * Converts the given input spike train into a continuous-valued function
	 * by convoluting the spikes with the given window function. Note that the
	 * result sampling interval is equivalent to that specified in the window
	 * function.
	 *
	 * @param spikes is an array containing the input spike times.
	 * @param window is the discretised window function that should be used.
	 * @param t0 is the timestamp the first element in the result vector
	 * corresponds to.
	 * @param t1 is the time the last timestamp in the result vector should
	 * correspond to.
	 */
	static std::vector<float> decode(const std::vector<float> &spikes,
	                                 const DiscreteWindow &window, float t0,
	                                 float t1, float min_val = -1.0,
	                                 float max_val = 1.0);
};

/*
class SpikeTrain {

    template <typename Function, typename Window = GaussWindow>
    static std::vector<float> encode_function_delta_sigma(
        const Function &fun, const Window &window, float min_x, float max_x,
        float resolution = 1e-3, float min_val = -1.0, float max_val = 1.0)
    {
        const float scale = 1.0 / (max_val - min_val);

        // Result list
        std::vector<float> spikes;

        // Sample the window function once
        std::vector<float> window(Window::sample_count(resolution));
        for (size_t i = 0; i < window.size(); i++) {
            window[i] = Window::value(i * resolution);
        }

        // Ring buffer storing the current function approximation
        std::vector<float> approx(Window::sample_count(resolution));
        size_t approx_i = 0;
        float err = 0;

        for (float x = min_x; x < max_x; x += resolution) {
            const float v = (fun(x) - min_val) * scale;
            err += (v - approx[approx_i]) * resolution;
            if (err > Window::integral()) {
                spikes.emplace_back(x);
                for (size_t i = 0; i < approx.size(); i++) {
                    approx[(approx_i + i) % approx.size()] =
                }
                if
                    err > integral spikes = [ spikes, xs(i) ];
                approx += window_fun(xs, xs(i));
                err -= integral0;
                end
            }
            errs(i) = err;
        }
        return spikes;
    }
};*/
}
}
