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

#include <random>

#include <cypress/core/neurons.hpp>

namespace cypress {
/*
 * Class SpikeSourceArray
 */

SpikeSourceArray::SpikeSourceArray()
    : NeuronTypeBase("SpikeSourceArray", {"spike_times"}, {"ms"}, {},
                     {"spikes"}, {"ms"}, false, true)
{
}

const SpikeSourceArray &SpikeSourceArray::inst()
{
	static const SpikeSourceArray inst;
	return inst;
}

/*
 * Class SpikeSourcePoisson
 */

SpikeSourcePoisson::SpikeSourcePoisson()
    : NeuronTypeBase("SpikeSourceArray", {"rate", "start", "duration", "seed"},
                     {"Hz", "ms", "ms", ""}, {{0.0, 0.0, 1e3, 0.0}}, {"spikes"},
                     {"ms"}, false, true)
{
}

const SpikeSourcePoisson &SpikeSourcePoisson::inst()
{
	static const SpikeSourcePoisson inst;
	return inst;
}

/*
 * Class SpikeSourceConstFreq
 */

SpikeSourceConstFreq::SpikeSourceConstFreq()
    : NeuronTypeBase("SpikeSourceConstFreq",
                     {"rate", "start", "duration", "sigma", "seed"},
                     {"Hz", "ms", "ms", "ms", ""}, {{0.0, 0.0, 1e3, 0.0, 0.0}},
                     {"spikes"}, {"ms"}, false, true)
{
}

const SpikeSourceConstFreq &SpikeSourceConstFreq::inst()
{
	static const SpikeSourceConstFreq inst;
	return inst;
}

/*
 * Class SpikeSourceConstInterval
 */

SpikeSourceConstInterval::SpikeSourceConstInterval()
    : NeuronTypeBase("SpikeSourceConstFreq",
                     {"interval", "start", "duration", "sigma", "seed"},
                     {"ms", "ms", "ms", "ms", ""}, {{0.0, 0.0, 1e3, 0.0, 0.0}},
                     {"spikes"}, {"ms"}, false, true)
{
}

const SpikeSourceConstInterval &SpikeSourceConstInterval::inst()
{
	static const SpikeSourceConstInterval inst;
	return inst;
}

/*
 * Class IfCondExp
 */

IfFacetsHardware1::IfFacetsHardware1()
    : NeuronTypeBase("IfFacetsHardware1", {"g_leak", "tau_refrac", "v_rest",
                                           "v_thresh", "v_reset", "e_rev_I"},
                     {"uS", "ms", "mV", "mV", "mV", "mV"},
                     {0.02, 1.0, -75.0, -55.0, -80.0, -80.0}, {"spikes", "v"},
                     {"ms", "mV"}, true, false)
{
}

const IfFacetsHardware1 &IfFacetsHardware1::inst()
{
	static const IfFacetsHardware1 inst;
	return inst;
}

/*
 * Class IfCondExp
 */

IfCondExp::IfCondExp()
    : NeuronTypeBase(
          "IfCondExp",
          {"cm", "tau_m", "tau_syn_E", "tau_syn_I", "tau_refrac", "v_rest",
           "v_thresh", "v_reset", "e_rev_E", "e_rev_I", "i_offset"},
          {"nF", "ms", "ms", "ms", "ms", "mV", "mV", "mV", "mV", "mV", "nA"},
          {1.0, 20.0, 5.0, 5.0, 0.1, -65.0, -50.0, -65.0, 0.0, -70.0, 0.0},
          {"spikes", "v", "gsyn_exc", "gsyn_inh"}, {"ms", "mV", "uS", "uS"},
          true, false)
{
}

const IfCondExp &IfCondExp::inst()
{
	static const IfCondExp inst;
	return inst;
}

/*
 * Class EifCondExpIsfaIsta
 */

EifCondExpIsfaIsta::EifCondExpIsfaIsta()
    : NeuronTypeBase("EifCondExpIsfaIsta",
                     {"cm", "tau_m", "tau_syn_E", "tau_syn_I", "tau_refrac",
                      "tau_w", "v_rest", "v_thresh", "v_reset", "e_rev_E",
                      "e_rev_I", "i_offset", "a", "b", "delta_T"},
                     {"nF", "ms", "ms", "ms", "ms", "ms", "mV", "mV", "mV",
                      "mV", "mV", "nA", "nS", "nA", "mV"},
                     {1.0, 20.0, 5.0, 5.0, 0.1, 144.0, -70.6, -50.4, -70.6, 0.0,
                      -80.0, 0.0, 4.0, 0.0805, 2.0},
                     {"spikes", "v", "gsyn_exc", "gsyn_inh"},
                     {"ms", "mV", "uS", "uS"}, true, false)
{
}

const EifCondExpIsfaIsta &EifCondExpIsfaIsta::inst()
{
	static const EifCondExpIsfaIsta inst;
	return inst;
}

/*
 * Class IfCurrExp
 */

IfCurrExp::IfCurrExp()
    : NeuronTypeBase(
          "IfCurrExp",
          {"cm", "tau_m", "tau_syn_E", "tau_syn_I", "tau_refrac", "v_rest",
           "v_thresh", "v_reset", "i_offset"},
          {"nF", "ms", "ms", "ms", "ms", "mV", "mV", "mV", "nA"},
          {1.0, 20.0, 5.0, 5.0, 0.1, -65.0, -50.0, -65.0, 0.0},
          {"spikes", "v"}, {"ms", "mV"},
          false, false)
{
}

const IfCurrExp &IfCurrExp::inst()
{
	static const IfCurrExp inst;
	return inst;
}
}
