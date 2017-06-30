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
 * @file delta_sigma.hpp
 *
 * Contains functions needed to encode a continuous-valued function to a
 * time-series of spikes using a delta-sigma encoder.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_NEF_DELTA_SIGMA
#define CYPRESS_NEF_DELTA_SIGMA

#include <cmath>
#include <cstddef>
#include <vector>

#include <cypress/core/types.hpp>

namespace cypress {
namespace nef {

/**
 * Structure providing static methods for encoding continuous-valued functions
 * as a series of spikes and vice versa.
 */
class DeltaSigma {
public:
	static constexpr Real DEFAULT_RESPONSE_TIME = 50e-3;
	static constexpr Real DEFAULT_STEP = 1e-4;
	static constexpr Real DEFAULT_EPS = 1e-6;
	static constexpr Real DEFAULT_MIN_SPIKE_INTERVAL = 1e-3;

	/**
	 * Function representing a Gaussian window normalised to a range from zero
	 * to one with a standard-deviation of one. Convoluting a spike train with
	 * this Window function corresponds to calculating a sliding average firing
	 * rate.
	 */
	struct GaussWindow {
		/**
		 * Value of the window function at position x. Multiply x with the
		 * standard deviation to get values for standard-deviations other than
		 * one.
		 *
		 * @param x is the position at which the window function should be
		 * evaluated.
		 */
		static Real value(Real x) { return std::exp(-x * x); }

		/**
		 * Returns the value of x for which until the value of the window
		 * function reaches "epsilon". Multiply the result with the standard
		 * deviation to get values for standard-deviations other than one.
		 */
		static Real limit(Real eps = 1e-6f)
		{
			return std::sqrt(-std::log(eps));
		}
	};

	/**
	 * Function representing an exponential decay starting at x = 0.0;
	 */
	struct ExponentialWindow {
		/**
		 * Value of the window function at position x. Multiply x with the
		 * standard deviation to get values for standard-deviations other than
		 * one.
		 *
		 * @param x is the position at which the window function should be
		 * evaluated.
		 */
		static Real value(Real x)
		{
			return x < 0.0f ? 0.0f : std::exp(-x);
		}

		/**
		 * Returns the value of x for which until the value of the window
		 * function reaches "epsilon". Multiply the result with the standard
		 * deviation to get values for standard-deviations other than one.
		 */
		static Real limit(Real eps = 1e-6f) { return -std::log(eps); }
	};

	/**
	 * Class representing a discretised window function.
	 */
	class DiscreteWindow {
	private:
		Real m_alpha;
		Real m_sigma;
		Real m_step;
		Real m_integral;
		Real m_integral_to_zero;
		std::vector<Real> m_values;

		DiscreteWindow(Real alpha, Real sigma, Real step, Real integral,
		               Real integral_to_zero, std::vector<Real> &&values)
		    : m_alpha(alpha),
		      m_sigma(sigma),
		      m_step(step),
		      m_integral(integral),
		      m_integral_to_zero(integral_to_zero),
		      m_values(values)
		{
		}

		template <typename Window>
		static std::pair<Real, Real> calculate_alpha_and_response_time(
		    Real spike_interval, Real sigma, Real step, Real eps,
		    Real p = 0.05f);

		/**
		 * Selects the parameters alpha and sigma depending on the given window
		 * function, the minimum spike interval and the desired response time.
		 */
		template <typename Window>
		static std::pair<Real, Real> choose_params(
		    Real min_spike_interval = DEFAULT_MIN_SPIKE_INTERVAL,
		    Real response_time = DEFAULT_RESPONSE_TIME,
		    Real step = DEFAULT_STEP, Real eps = DEFAULT_EPS);

	public:
		/**
		 * Constructor of the DiscreteWindow class. Discretises the window given
		 * as
		 * template parameter.
		 *
		 * @param alpha is a scale factor the samples are scaled by.
		 * @param sigma is the standard deviation determining the x-axis scale.
		 * @param step is the sampling interval.
		 * @param eps is the epsilon determining up to which value the window is
		 * sampled.
		 */
		template <typename Window>
		static DiscreteWindow create_manual(Real alpha, Real sigma,
		                                    Real step = DEFAULT_STEP,
		                                    Real eps = DEFAULT_EPS);

		/**
		 * Creates a DiscreteWindow with automatically chosen parameters
		 * matching
		 *
		 * @param min_spike_interval is the minimum interval in which two spikes
		 * may occur. This parameter controls the scaling of the window function
		 * values.
		 * @param response_time controls the minimum response time that can be
		 * encoded using the spike-train representation at the cost of the
		 * value resolution.
		 * @param step is the discretisation time step.
		 * @param eps controls the minimum value that should be stored in the
		 * discretised window.
		 */
		template <typename Window>
		static DiscreteWindow create(
		    Real min_spike_interval = DEFAULT_MIN_SPIKE_INTERVAL,
		    Real response_time = DEFAULT_RESPONSE_TIME,
		    Real step = DEFAULT_STEP, Real eps = DEFAULT_EPS);

		/**
		 * Returns the used scaling factor.
		 */
		Real alpha() const { return m_alpha; }

		/**
		 * Returns the used standard deviation/width of the window.
		 */
		Real sigma() const { return m_sigma; }

		/**
		 * Returns the sample interval.
		 */
		Real step() const { return m_step; }

		/**
		 * Returns the integral of the window function from minus infinity to
		 * plus
		 * infinity. Multiply the result with the standard deviation to get
		 * values
		 * for standard-deviations other than one.
		 */
		Real integral() const { return m_integral; }

		/**
		 * Returns the integral of the window function from minus infinity to
		 * zero.
		 */
		Real integral_to_zero() const { return m_integral_to_zero; }

		/**
		 * Returns the number of samples until the value of the window function
		 * reaches "epsilon". Multiply the result with the standard deviation to
		 * get values for standard-deviations other than one.
		 */
		Real limit(Real eps = DEFAULT_EPS) const
		{
			return std::sqrt(-std::log(eps));
		}

		/**
		 * Returns the number of samples stored in the window.
		 */
		size_t size() const { return m_values.size(); }

		/**
		 * Returns the value of the i-th sample.
		 */
		Real operator[](size_t i) const { return m_values[i]; }

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
	 * Encodes a time-series of continuous-valued list of values to a
	 * time-series of spikes, which can be decoded by convoluting the spike
	 * train with the given window function. Implemented as a delta-sigma
	 * encoder.
	 *
	 * @param values is a list containing the function values. The sampling
	 * interval is taken from the given discretised window function.
	 * @param t0 is the time offset that should be added to the resulting
	 * spikes.
	 * @param window is the discretised window function that should be used
	 * for encoding.
	 * @param min_spike_interval is the minimum delay between two spikes should
	 * be greater or equal to the window sampling interval.
	 * @param min_val is the minimum value occuring in the list of values.
	 * @param max_val is the maximum value occuring in the list of values.
	 */
	static std::vector<Real> encode(
	    const std::vector<Real> &values, const DiscreteWindow &window, Real t0,
	    Real min_val = -1.0, Real max_val = 1.0,
	    Real min_spike_interval = DEFAULT_MIN_SPIKE_INTERVAL);

	/**
	 * Encodes a time-series of continuous-valued function as a time-series of
	 * spikes, which can be decoded by convoluting the spike train with the
	 * given window function. Implemented as a delta-sigma encoder.
	 *
	 * @param values is a list containing the function values. The sampling
	 * interval is taken from the given discretised window function.
	 * @param window is the discretised window function that should be used
	 * for encoding.
	 * @param min_spike_interval is the minimum delay between two spikes should
	 * be greater or equal to the window sampling interval.
	 * @param t0 is the time offset that should be added to the resulting
	 * spikes.
	 * @param min_val is the minimum value occuring in the list of values.
	 * @param max_val is the maximum value occuring in the list of values.
	 */
	template <typename Fun>
	static std::vector<Real> encode(
	    const Fun &f, const DiscreteWindow &window, Real t0, Real t1,
	    Real min_val = -1.0, Real max_val = 1.0,
	    Real min_spike_interval = DEFAULT_MIN_SPIKE_INTERVAL)
	{
		const size_t n_samples = std::ceil(t1 - t0) / window.step();
		std::vector<Real> values(n_samples);
		for (size_t i = 0; i < n_samples; i++) {
			values[i] = f(t0 + i * window.step());
		}
		return encode(values, window, t0, min_val, max_val, min_spike_interval);
	}

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
	static std::vector<Real> decode(const std::vector<Real> &spikes,
	                                const DiscreteWindow &window, Real t0,
	                                Real t1, Real min_val = -1.0,
	                                Real max_val = 1.0);
};
}
}

#endif /* CYPRESS_NEF_DELTA_SIGMA */

