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

#include <algorithm>
#include <random>

#include <cypress/nef/tuning_curve.hpp>

namespace cypress {
namespace nef {

static std::vector<float> generate_test_values(size_t n_samples,
                                               size_t n_repeat)
{
	// Fill the list
	std::vector<float> res(n_samples * n_repeat);
	const float f = 1.0f / float(n_samples);
	for (size_t i = 0; i < n_samples; i++) {
		const float v = i * f;
		for (size_t j = 0; j < n_repeat; j++) {
			res[i * n_repeat + j] = v;
		}
	}

	// Shuffle the list
	std::random_device rd;
	std::default_random_engine re(rd());
	std::shuffle(res.begin(), res.end(), re);
	return res;
}

static std::vector<float> generate_test_spike_train(
    const DeltaSigma::DiscreteWindow &wnd, const std::vector<float> &values,
    float min_spike_interval, float t_wnd)
{
	// Calculate the start and end time of the test signal
	const float i_t_wnd = 1.0f / t_wnd;
	const float t0 = 0.0f;
	const float t1 = t_wnd * values.size();

	// Encode the test values as spike train
	std::vector<float> res =  DeltaSigma::encode([&](float t) -> float {
		return values[std::floor(t * i_t_wnd)];
	}, wnd, t0, t1, 0.0f, 1.0f, min_spike_interval);

	for (float &v: res) {
		v *= 1e3; // Convert seconds to milliseconds
	}

	return res;
}

TuningCurveEvaluator::TuningCurveEvaluator(size_t n_samples, size_t n_repeat,
                                           float min_spike_interval,
                                           float response_time, float step)
    : m_n_samples(n_samples),
      m_n_repeat(n_repeat),
      m_wnd(DeltaSigma::DiscreteWindow::create<Window>(min_spike_interval,
                                                       response_time, step)),
      m_t_wnd(1.0f * response_time),
      m_test_values(generate_test_values(m_n_samples, m_n_repeat)),
      m_test_spike_train(generate_test_spike_train(m_wnd, m_test_values,
                                                   min_spike_interval, m_t_wnd))
{
}

const std::vector<float> &TuningCurveEvaluator::input_spike_train()
{
	return m_test_spike_train;
}

std::vector<std::pair<float, float>>
    TuningCurveEvaluator::evaluate_output_spike_train(
        std::vector<float> output_spikes)
{
	for (float &v: output_spikes) {
		v *= 1e-3; // Convert milliseconds to seconds
	}

	// Decode the output spike train
	std::vector<float> decoded = DeltaSigma::decode(
	    output_spikes, m_wnd, 0.0f, m_t_wnd * m_test_values.size(), 0.0f, 1.0f);

	// Read the output values for the given test values
	const float i_step = 1.0f / m_wnd.step();
	const size_t response_offs = m_t_wnd * 0.25f;
	const size_t response_len = std::floor((0.5f * m_t_wnd) * i_step) + 1;
	const float i_response_len = 1.0f / float(response_len);
	std::vector<std::pair<float, float>> res(m_n_samples);

	for (size_t i = 0; i < m_test_values.size(); i++) {
		float v_in = m_test_values[i];
		float v_out = 0.0f;

		const size_t j0 = std::round(i * m_t_wnd * i_step) + response_offs;
		const size_t j1 = j0 + response_len;
		for (size_t j = j0; j < j1; j++) {
			v_out += decoded[j];
		}
		v_out *= i_response_len;

		const size_t result_idx = std::round(v_in * m_n_samples);
		res[result_idx].first = v_in;
		res[result_idx].second += v_out;
	}

	// Average the output values over the number of repetitions
	const float i_n_repeat = 1.0f / float(m_n_repeat);
	for (size_t i = 0; i < m_n_samples; i++) {
		res[i].second *= i_n_repeat;
	}
	return res;
}
}
}

