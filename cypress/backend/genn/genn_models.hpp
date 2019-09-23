/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2019 Christoph Ostrau
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

#pragma once

#include <SharedLibraryModel.h>
#include <genn/genn/modelSpecInternal.h>
#include <genn/genn/weightUpdateModels.h>

namespace GeNNModels {
template <typename T>
class SharedLibraryModel_ : public SharedLibraryModel<T> {
public:
	using SharedLibraryModel<T>::getSymbol;
};

//_______________________Connectivity Models____________________________________

// Adapted from GeNN repository
class FixedNumberPostWithReplacementNoAutapse
    : public InitSparseConnectivitySnippet::Base {
public:
	DECLARE_SNIPPET(FixedNumberPostWithReplacementNoAutapse, 1);

	SET_ROW_BUILD_CODE(
	    "const scalar u = $(gennrand_uniform);\n"
	    "x += (1.0 - x) * (1.0 - pow(u, 1.0 / (scalar)($(rowLength) - c)));\n"
	    "const unsigned int postIdx = (unsigned int)(x * $(num_post));\n"
	    "if(postIdx != $(id_pre)){\n"
	    "if(postIdx < $(num_post)) {\n"
	    "   $(addSynapse, postIdx);\n"
	    "}\n"
	    "else {\n"
	    "   $(addSynapse, $(num_post) - 1);\n"
	    "}\n"
	    "c++;\n"
	    "if(c >= $(rowLength)) {\n"
	    "   $(endRow);\n"
	    "}}\n");
	SET_ROW_BUILD_STATE_VARS({{"x", "scalar", 0.0}, {"c", "unsigned int", 0}});

	SET_PARAM_NAMES({"rowLength"});

	SET_CALC_MAX_ROW_LENGTH_FUNC([](unsigned int, unsigned int,
	                                const std::vector<double> &pars) {
		return (unsigned int)pars[0];
	});
};
IMPLEMENT_SNIPPET(FixedNumberPostWithReplacementNoAutapse);

// Adapted from GeNN repository
class FixedNumberPostWithReplacement
    : public InitSparseConnectivitySnippet::Base {
public:
	DECLARE_SNIPPET(FixedNumberPostWithReplacement, 1);

	SET_ROW_BUILD_CODE(
	    "const scalar u = $(gennrand_uniform);\n"
	    "x += (1.0 - x) * (1.0 - pow(u, 1.0 / (scalar)($(rowLength) - c)));\n"
	    "const unsigned int postIdx = (unsigned int)(x * $(num_post));\n"
	    "if(postIdx < $(num_post)) {\n"
	    "   $(addSynapse, postIdx);\n"
	    "}\n"
	    "else {\n"
	    "   $(addSynapse, $(num_post) - 1);\n"
	    "}\n"
	    "c++;\n"
	    "if(c >= $(rowLength)) {\n"
	    "   $(endRow);\n"
	    "}\n");
	SET_ROW_BUILD_STATE_VARS({{"x", "scalar", 0.0}, {"c", "unsigned int", 0}});

	SET_PARAM_NAMES({"rowLength"});

	SET_CALC_MAX_ROW_LENGTH_FUNC([](unsigned int, unsigned int,
	                                const std::vector<double> &pars) {
		return (unsigned int)pars[0];
	});
};
IMPLEMENT_SNIPPET(FixedNumberPostWithReplacement);

//_______________________Weight Update Models___________________________________
/**
 * The simulation code for all synapse models is adapted from pynn_genn
 * repository
 */
/*
class TsodyksMarkramSynapse : public WeightUpdateModels::Base {
public:
	DECLARE_WEIGHT_UPDATE_MODEL(TsodyksMarkramSynapse, 0, 9, 0, 0);

	SET_VARS({{"U", "scalar"},  // asymptotic value of probability of release
	          {"tauRec",
	           "scalar"},  // recovery time from synaptic depression [ms]
	          {"tauFacil", "scalar"},  // time constant for facilitation [ms]
	          {"tauPsc",
	           "scalar"},  // decay time constant of postsynaptic current [ms]
	          {"g", "scalar"},                           // weight
	          {"u", "scalar"},                           //  0
	          {"x", "scalar"},                           // 1.0
	          {"y", "scalar"},                           // 0.0
	          {"z", "scalar"}});  // 0.0

	SET_SIM_CODE(
	    "const scalar deltaST = $(t) - $(sT_pre);"
	    "$(z) *= exp(-deltaST / $(tauRec));"
	    "$(z) += $(y) * (exp(-deltaST / $(tauPsc)) - exp(-deltaST / "
	    "$(tauRec))) / (($(tauPsc) / $(tauRec)) - 1.0);"
	    "$(y) *= exp(-deltaST / $(tauPsc));"
	    "$(x) = 1.0 - $(y) - $(z);"
	    "$(u) *= exp(-deltaST / $(tauFacil));"
	    "$(u) += $(U) * (1.0 - $(u));"
	    "if ($(u) > $(U)) {"
	    "   $(u) = $(U);"
	    "}\n"
	    "$(y) += $(x) * $(u);\n"
	    "$(addToInSyn, $(g) * $(x) * $(u));\n");

	SET_NEEDS_PRE_SPIKE_TIME(true);
};
IMPLEMENT_SNIPPET(TsodyksMarkramSynapse);*/ // TODO Broken!

class SpikePairRuleAdditive : public WeightUpdateModels::Base {
	DECLARE_WEIGHT_UPDATE_MODEL(SpikePairRuleAdditive, 6, 1, 1, 1);
	SET_VARS({
	    {"g", "scalar", VarAccess::READ_WRITE}  // 0 - Weight
	});
    
    SET_PARAM_NAMES({
	    "Aplus",                     // 0 - Rate of potentiation
	    "Aminus",                    // 1 - Rate of depression
	    "Wmin",                      // 2 - Minimum weight
	    "Wmax",                      // 3 - Maximum weight
        "tauPlus",                   // 4 - Potentiation time constant (ms)
        "tauMinus",                  // 5 - Depression time constant (ms)
    }
    );
    
	SET_PRE_VARS({{"preTrace", "scalar"}});     // 0 - Internal, set to 0});
	SET_POST_VARS({{"postTrace", "scalar"}});  // 0 - Internal, set to 0

	SET_SIM_CODE(
	    "$(addToInSyn, $(g));\n"
	    "scalar dt = $(t) - $(sT_post);\n"
	    "if (dt > 0){\n"
	    "const scalar update = $(Aminus) * $(postTrace) * exp(-dt / "
	    "$(tauMinus));\n"
	    "$(g) = fmin($(Wmax), fmax($(Wmin), $(g) - (($(Wmax) - $(Wmin)) * "
	    "update)));\n"
	    "}\n");

	SET_LEARN_POST_CODE(
	    "scalar dt = $(t) - $(sT_pre);\n"
	    "if (dt > 0){\n"
	    "const scalar update = $(Aplus) * $(preTrace) * exp(-dt / "
	    "$(tauPlus));\n"
	    "$(g) = fmin($(Wmax), fmax($(Wmin), $(g) + (($(Wmax) - $(Wmin)) * "
	    "update)));\n"
	    "}\n");

	SET_PRE_SPIKE_CODE(
	    "const scalar dt = $(t) - $(sT_pre);\n"
	    "$(preTrace) = $(preTrace) * exp(-dt / $(tauPlus)) + 1.0;\n")
	SET_POST_SPIKE_CODE(
	    "const scalar dt = $(t) - $(sT_post);\n"
	    "$(postTrace) = $(postTrace) * exp(-dt / $(tauMinus)) + 1.0;\n");

	SET_NEEDS_PRE_SPIKE_TIME(true);
	SET_NEEDS_POST_SPIKE_TIME(true);
};
IMPLEMENT_SNIPPET(SpikePairRuleAdditive);

class SpikePairRuleMultiplicative : public WeightUpdateModels::Base {
	DECLARE_WEIGHT_UPDATE_MODEL(SpikePairRuleMultiplicative, 6, 1, 1, 1);
	SET_VARS({
	    {"g", "scalar"},                            // 0 - Weight
	});
	SET_PRE_VARS({{"preTrace", "scalar"}}); // 0 - Internal, set to 0
	SET_POST_VARS({{"postTrace", "scalar"}});  // 0 - Internal, set to 0
	                
    SET_PARAM_NAMES({
	    "Aplus",                     // 0 - Rate of potentiation
	    "Aminus",                    // 1 - Rate of depression
	    "Wmin",                      // 2 - Minimum weight
	    "Wmax",                      // 3 - Maximum weight
        "tauPlus",                   // 4 - Potentiation time constant (ms)
        "tauMinus",                  // 5 - Depression time constant (ms)
    }
    );

	SET_SIM_CODE(
	    "$(addToInSyn, $(g));\n"
	    "scalar dt = $(t) - $(sT_post);\n"
	    "if (dt > 0){\n"
	    "const scalar update = $(Aminus) * $(postTrace) * exp(-dt / "
	    "$(tauMinus));\n"
	    "$(g) -= ($(g) - $(Wmin)) * update;\n"
	    "}\n");

	SET_LEARN_POST_CODE(
	    "scalar dt = $(t) - $(sT_pre);\n"
	    "if (dt > 0){\n"
	    "const scalar update = $(Aplus) * $(preTrace) * exp(-dt / "
	    "$(tauPlus));\n"
	    "$(g) += ($(Wmax) - $(g)) * update;\n"
	    "}\n");

	SET_PRE_SPIKE_CODE(
	    "const scalar dt = $(t) - $(sT_pre);\n"
	    "$(preTrace) = $(preTrace) * exp(-dt / $(tauPlus)) + 1.0;\n")
	SET_POST_SPIKE_CODE(
	    "const scalar dt = $(t) - $(sT_post);\n"
	    "$(postTrace) = $(postTrace) * exp(-dt / $(tauMinus)) + 1.0;\n");

	SET_NEEDS_PRE_SPIKE_TIME(true);
	SET_NEEDS_POST_SPIKE_TIME(true);
};
IMPLEMENT_SNIPPET(SpikePairRuleMultiplicative);

}  // namespace GeNNModels
