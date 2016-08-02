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
 * @file spike_sources.hpp
 *
 * Contains transformations from the various spike source neuron types to the
 * generic SpikeSourceArray neuron type.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_TRANSFORMATIONS_SPIKE_SOURCES_HPP
#define CYPRESS_TRANSFORMATIONS_SPIKE_SOURCES_HPP

#include <cypress/core/transformation_util.hpp>

namespace cypress {
namespace transformations {
/**
 * Transforms a SpikeSourceConstInterval neuron to a SpikeSourceConstFreq
 * neuron by converting the interval to a frequency.
 */
class CIToCF : public NeuronTypeTransformation<SpikeSourceConstInterval,
                                               SpikeSourceConstFreq> {
protected:
	void do_transform_parameters(const SpikeSourceConstIntervalParameters &src,
	                             SpikeSourceConstFreqParameters tar) override;

	void do_transform_signals(const SpikeSourceConstIntervalSignals &src,
	                          SpikeSourceConstFreqSignals tar) override;

public:
	std::string id() const override { return "CIToCF"; }

	~CIToCF(){};
};

/**
 * Transforms a SpikeSourceConstFreq neuron to a SpikeSourceConstInterval
 * neuron by converting the frequency to an interval.
 */
class CFToCI : public NeuronTypeTransformation<SpikeSourceConstFreq,
                                               SpikeSourceConstInterval> {
protected:
	void do_transform_parameters(
	    const SpikeSourceConstFreqParameters &src,
	    SpikeSourceConstIntervalParameters tar) override;

	void do_transform_signals(const SpikeSourceConstFreqSignals &src,
	                          SpikeSourceConstIntervalSignals tar) override;

public:
	std::string id() const override { return "CFToCI"; }

	~CFToCI(){};
};

/**
 * Transforms a SpikeSourcePoisson neuron to a SpikeSourceArray neuron.
 */
class PoissonToSA
    : public NeuronTypeTransformation<SpikeSourcePoisson, SpikeSourceArray> {
protected:
	void do_transform_parameters(const SpikeSourcePoissonParameters &src,
	                             SpikeSourceArrayParameters tar) override;

	bool do_dehomogenise_parameters(
	    const SpikeSourcePoissonParameters &src) override;

	void do_transform_signals(const SpikeSourcePoissonSignals &src,
	                          SpikeSourceArraySignals tar) override;

public:
	std::string id() const override { return "PoissonToSA"; }

	~PoissonToSA(){};
};

/**
 * Transforms a SpikeSourceConstFreq neuron to a SpikeSourceArray neuron.
 */
class CFToSA
    : public NeuronTypeTransformation<SpikeSourceConstFreq, SpikeSourceArray> {
protected:
	void do_transform_parameters(const SpikeSourceConstFreqParameters &src,
	                             SpikeSourceArrayParameters tar) override;

	bool do_dehomogenise_parameters(
	    const SpikeSourceConstFreqParameters &src) override;

	void do_transform_signals(const SpikeSourceConstFreqSignals &src,
	                          SpikeSourceArraySignals tar) override;

public:
	std::string id() const override { return "CFToSA"; }

	~CFToSA(){};
};
}
}

#endif /* CYPRESS_TRANSFORMATIONS_SPIKE_SOURCES_HPP */
