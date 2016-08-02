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

#include <cypress/core/spike_time_generators.hpp>
#include <cypress/transformations/spike_sources.hpp>

namespace cypress {
namespace transformations {

/*
 * Class CIToCF
 */

void CIToCF::do_transform_parameters(
    const SpikeSourceConstIntervalParameters &src,
    SpikeSourceConstFreqParameters tar)
{
	tar.rate(1000.0 / src.interval())
	    .start(src.start())
	    .duration(src.duration());
}

void CIToCF::do_transform_signals(const SpikeSourceConstIntervalSignals &src,
                                  SpikeSourceConstFreqSignals tar)
{
	tar.record_spikes(src.is_recording_spikes());
}

/*
 * Class CFToCI
 */

void CFToCI::do_transform_parameters(const SpikeSourceConstFreqParameters &src,
                                     SpikeSourceConstIntervalParameters tar)
{
	tar.interval(1000.0 / src.rate())
	    .start(src.start())
	    .duration(src.duration());
}

void CFToCI::do_transform_signals(const SpikeSourceConstFreqSignals &src,
                                  SpikeSourceConstIntervalSignals tar)
{
	tar.record_spikes(src.is_recording_spikes());
}

/*
 * PoissonToSA
 */

void PoissonToSA::do_transform_parameters(
    const SpikeSourcePoissonParameters &src, SpikeSourceArrayParameters tar)
{
	tar.spike_times(
	    spikes::poisson(src.start(), src.start() + src.duration(), src.rate()));
}

bool PoissonToSA::do_dehomogenise_parameters(
    const SpikeSourcePoissonParameters &)
{
	return true;
}

void PoissonToSA::do_transform_signals(const SpikeSourcePoissonSignals &src,
                                       SpikeSourceArraySignals tar)
{
	tar.record_spikes(src.is_recording_spikes());
}

/**
 * Transforms a SpikeSourceConstFreq neuron to a SpikeSourceArray neuron.
 */
void CFToSA::do_transform_parameters(const SpikeSourceConstFreqParameters &src,
                                     SpikeSourceArrayParameters tar)
{
	tar.spike_times(spikes::constant_frequency(
	    src.start(), src.start() + src.duration(), src.rate(), src.sigma()));
}

bool CFToSA::do_dehomogenise_parameters(
    const SpikeSourceConstFreqParameters &src)
{
	return src.sigma() > 0.0;
}

void CFToSA::do_transform_signals(const SpikeSourceConstFreqSignals &src,
                                  SpikeSourceArraySignals tar)
{
	tar.record_spikes(src.is_recording_spikes());
}
}
}

