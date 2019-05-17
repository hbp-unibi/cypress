/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2018 Christoph Ostrau
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

#ifndef CYPRESS_SYNAPSES_HPP
#define CYPRESS_SYNAPSES_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <cypress/core/data.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/types.hpp>
#include <cypress/util/optional.hpp>

/**
 * Macro used for defining the getters and setters associated with a neuron
 * parameter value.
 */
#define NAMED_PARAMETER(NAME, IDX)            \
	static constexpr size_t idx_##NAME = IDX; \
	auto &NAME(Real x)                        \
	{                                         \
		m_params[IDX] = x;                    \
		return *this;                         \
	}                                         \
	Real NAME() const { return m_params[IDX]; }

namespace cypress {

namespace {
struct SynapseParametersBase {
	const std::string name;
	const std::vector<std::string> parameter_names;
	const std::vector<std::string> parameter_units;
	std::vector<Real> parameter_defaults;
	bool learning;
	SynapseParametersBase(const std::string &name,
	                      std::vector<std::string> parameter_names,
	                      std::vector<std::string> parameter_units,
	                      std::vector<Real> parameter_defaults, bool learning)
	    : name(name),
	      parameter_names(std::move(parameter_names)),
	      parameter_units(std::move(parameter_units)),
	      parameter_defaults(std::move(parameter_defaults)),
	      learning(learning)
	{
	}
};
static const SynapseParametersBase NullSynapse("NULL", {}, {}, {}, false);
}  // namespace
class SynapseBase {
protected:
	std::vector<Real> m_params;
	bool m_learning = false;

	void check_parameter_size()
	{
		if (m_params.size() != parameter_names().size()) {
			throw InvalidParameterArraySize(
			    "Number of parameters in the parameter vector does not match "
			    "the number of parameters of this synapse type");
		}
	}

public:
	SynapseBase(std::initializer_list<Real> parameters) : m_params(parameters)
	{
	}
	explicit SynapseBase(std::vector<Real> parameters)
	    : m_params(std::move(parameters))
	{
	}

	// SynapseBase(SynapseBase &synapse) : m_params(synapse.parameters()) {}

	/**
	 * Name of the neuron type.
	 */
	virtual const std::string name() const { return NullSynapse.name; }

	/**
	 * Name of all neuron parameters.
	 */
	virtual const std::vector<std::string> &parameter_names() const
	{
		return NullSynapse.parameter_names;
	}

	/**
	 * Units of all neuron parameters.
	 */
	virtual const std::vector<std::string> &parameter_units() const
	{
		return NullSynapse.parameter_units;
	}

	/**
	 * Contains default values for the neuron parameters.
	 */
	virtual const std::vector<Real> &parameter_defaults() const
	{
		return NullSynapse.parameter_defaults;
	}
	virtual bool learning() const { return NullSynapse.learning; }

	const std::vector<Real> &parameters() const { return m_params; }
	void parameters(const std::vector<Real> &params)
	{
		m_params = params;
		check_parameter_size();
	}

	/**
	 * Resolves the given parameter name into a parameter index.
	 */
	Optional<size_t> parameter_index(const std::string &name) const
	{
		std::vector<std::string> param_names = parameter_names();
		auto it = std::find(param_names.begin(), param_names.end(), name);
		if (it != param_names.end()) {
			return Optional<size_t>(it - param_names.begin());
		}
		return Optional<size_t>();
	}

	/**
	 * Sets the parameter with the given index to the specified value.
	 *
	 * @param idx is the parameter that should be updated.
	 * @param value is the value the parameter should be set to.
	 */
	void set(size_t idx, Real value) { m_params[idx] = value; }

	/**
	 * Returns a read-only reference at the i-th element in the parameter
	 * vector.
	 */
	Real operator[](size_t i) const { return m_params[i]; }

	/**
	 * Returns an iterator allowing to iterate over the parameter vector.
	 */
	const Real *begin() const { return &m_params[0]; }

	/**
	 * Returns an iterator allowing to iterate over the parameter vector.
	 */
	const Real *end() const { return &m_params.back() + 1; }

	/**
	 * Returns the size of the parameter vector. Aborts if the population is
	 * non-uniform.
	 */
	size_t size() const { return m_params.size(); }

	/**
	 * Create a shared pointer from a reference. Checks the name of the synapse
	 * to create the correct type. This is to avoid creating a copy, which is
	 * then of type SynapseBase and member functions pointing to wrong name,
	 * parameter_names,...
	 * @param synapse Reference to the synapse object to be copied and
	 * administered by a shared pointer
	 * @return Shared pointer to a copy of synapse
	 */
	static std::shared_ptr<SynapseBase> make_shared(SynapseBase &synapse);

	virtual ~SynapseBase() = default;
};

namespace {
static const SynapseParametersBase StaticSynapseParameters(
    "StaticSynapse", {"weight", "delay"}, {"microSiemens/nA", "ms"},
    {0.015, 1.0}, false);
static const SynapseParametersBase SpikePairRuleAdditiveParameters(
    "SpikePairRuleAdditive",
    {"weight", "delay", "tau_plus", "tau_minus", "A_plus", "A_minus", "w_min",
     "w_max"},
    {"microSiemens/nA", "ms", "ms", "ms", "microSiemens/nA", "microSiemens/nA",
     "microSiemens/nA", "microSiemens/nA"},
    {0.015, 1.0, 20.0, 20.0, 0.01, 0.01, 0, 0.1}, true);

static const SynapseParametersBase TsodyksMarkramMechanismParameters(
    "TsodyksMarkramMechanism", {"weight", "delay", "U", "tau_rec", "tau_facil"},
    {"microSiemens/nA", "ms", "", "ms", "ms"}, {0.015, 1.0, 0.0, 100.0, 0.0},
    false);

static const SynapseParametersBase SpikePairRuleMultiplicativeParameters(
    "SpikePairRuleMultiplicative",
    {"weight", "delay", "tau_plus", "tau_minus", "A_plus", "A_minus", "w_min",
     "w_max"},
    {"microSiemens/nA", "ms", "ms", "ms", "microSiemens/nA", "microSiemens/nA",
     "microSiemens/nA", "microSiemens/nA"},
    {0.015, 1.0, 20.0, 20.0, 0.01, 0.01, 0, 0.1}, true);

}  // namespace

class StaticSynapse : public SynapseBase {
public:
	using SynapseBase::SynapseBase;
	StaticSynapse() : SynapseBase(StaticSynapseParameters.parameter_defaults)
	{
		check_parameter_size();
	}
	StaticSynapse(const StaticSynapse &synapse)
	    : SynapseBase(synapse.parameters())
	{
		check_parameter_size();
	}

	const std::string name() const override
	{
		return StaticSynapseParameters.name;
	};
	const std::vector<std::string> &parameter_names() const override
	{
		return StaticSynapseParameters.parameter_names;
	};
	const std::vector<std::string> &parameter_units() const override
	{
		return StaticSynapseParameters.parameter_units;
	};
	const std::vector<Real> &parameter_defaults() const override
	{
		return StaticSynapseParameters.parameter_defaults;
	};
	bool learning() const override { return StaticSynapseParameters.learning; };

	NAMED_PARAMETER(weight, 0);
	NAMED_PARAMETER(delay, 1);
};

class SpikePairRuleAdditive final : public SynapseBase {
public:
	using SynapseBase::SynapseBase;
	SpikePairRuleAdditive()
	    : SynapseBase(SpikePairRuleAdditiveParameters.parameter_defaults)
	{
		check_parameter_size();
	}
	SpikePairRuleAdditive(const StaticSynapse &synapse)
	    : SynapseBase(synapse.parameters())
	{
		check_parameter_size();
	}
	const std::string name() const override
	{
		return SpikePairRuleAdditiveParameters.name;
	};
	const std::vector<std::string> &parameter_names() const override
	{
		return SpikePairRuleAdditiveParameters.parameter_names;
	};
	const std::vector<std::string> &parameter_units() const override
	{
		return SpikePairRuleAdditiveParameters.parameter_units;
	};
	const std::vector<Real> &parameter_defaults() const override
	{
		return SpikePairRuleAdditiveParameters.parameter_defaults;
	};
	bool learning() const override
	{
		return SpikePairRuleAdditiveParameters.learning;
	};
	NAMED_PARAMETER(weight, 0);
	NAMED_PARAMETER(delay, 1);
	NAMED_PARAMETER(tau_plus, 2);
	NAMED_PARAMETER(tau_minus, 3);
	NAMED_PARAMETER(A_plus, 4);
	NAMED_PARAMETER(A_minus, 5);
	NAMED_PARAMETER(w_min, 6);
	NAMED_PARAMETER(w_max, 7);
};

class SpikePairRuleMultiplicative final : public SynapseBase {
public:
	using SynapseBase::SynapseBase;
	SpikePairRuleMultiplicative()
	    : SynapseBase(SpikePairRuleMultiplicativeParameters.parameter_defaults)
	{
		check_parameter_size();
	}
	SpikePairRuleMultiplicative(const StaticSynapse &synapse)
	    : SynapseBase(synapse.parameters())
	{
		check_parameter_size();
	}
	const std::string name() const override
	{
		return SpikePairRuleMultiplicativeParameters.name;
	};
	const std::vector<std::string> &parameter_names() const override
	{
		return SpikePairRuleMultiplicativeParameters.parameter_names;
	};
	const std::vector<std::string> &parameter_units() const override
	{
		return SpikePairRuleMultiplicativeParameters.parameter_units;
	};
	const std::vector<Real> &parameter_defaults() const override
	{
		return SpikePairRuleMultiplicativeParameters.parameter_defaults;
	};
	bool learning() const override
	{
		return SpikePairRuleMultiplicativeParameters.learning;
	};
	NAMED_PARAMETER(weight, 0);
	NAMED_PARAMETER(delay, 1);
	NAMED_PARAMETER(tau_plus, 2);
	NAMED_PARAMETER(tau_minus, 3);
	NAMED_PARAMETER(A_plus, 4);
	NAMED_PARAMETER(A_minus, 5);
	NAMED_PARAMETER(w_min, 6);
	NAMED_PARAMETER(w_max, 7);
};

class TsodyksMarkramMechanism final : public SynapseBase {
public:
	using SynapseBase::SynapseBase;
	TsodyksMarkramMechanism()
	    : SynapseBase(TsodyksMarkramMechanismParameters.parameter_defaults)
	{
		check_parameter_size();
	}
	TsodyksMarkramMechanism(const StaticSynapse &synapse)
	    : SynapseBase(synapse.parameters())
	{
		check_parameter_size();
	}
	const std::string name() const override
	{
		return TsodyksMarkramMechanismParameters.name;
	};
	const std::vector<std::string> &parameter_names() const override
	{
		return TsodyksMarkramMechanismParameters.parameter_names;
	};
	const std::vector<std::string> &parameter_units() const override
	{
		return TsodyksMarkramMechanismParameters.parameter_units;
	};
	const std::vector<Real> &parameter_defaults() const override
	{
		return TsodyksMarkramMechanismParameters.parameter_defaults;
	};
	bool learning() const override
	{
		return TsodyksMarkramMechanismParameters.learning;
	};
	NAMED_PARAMETER(weight, 0);
	NAMED_PARAMETER(delay, 1);
	NAMED_PARAMETER(U, 2);
	NAMED_PARAMETER(tau_rec, 3);
	NAMED_PARAMETER(tau_facil, 4);
};

}  // namespace cypress

#undef NAMED_PARAMETER
#endif  // CYPRESS_SYNAPSES_HPP
