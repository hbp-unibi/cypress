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
 * @file network_base_objects.hpp
 *
 * Header containing the declarations of the basic neuron and population
 * classes. The classes used in this header are generic in a sense
 * that they can represent neurons and populations of any type. If possible do
 * not use the types in this header directly, but use the templated versions
 * in network.hpp.
 *
 * @author Andreas Stöckel
 */

#pragma once

#ifndef CYPRESS_CORE_NETWORK_BASE_OBJECTS_HPP
#define CYPRESS_CORE_NETWORK_BASE_OBJECTS_HPP

#include <cypress/core/network_base.hpp>
#include <cypress/core/network_mixins.hpp>

namespace cypress {

/*
 * Forward declarations
 */
class PopulationBase;
class PopulationViewBase;
class NeuronBase;
class Accessor;

/**
 * The PopulationBase class represents a generic neuron population of neurons
 * of any type. If possible, prefer the usage of Population<T>.
 */
class PopulationBase
    : public IterableMixin<PopulationBase, NeuronBase, Accessor>,
      public ViewableMixin<PopulationBase, PopulationViewBase, Accessor>,
      public ParametersMixin<PopulationBase, Accessor, NeuronParametersBase>,
      public PopulationMixin<PopulationBase, Accessor, NeuronParametersBase>,
      public ConnectableMixin<PopulationBase, Accessor> {
private:
	NetworkBase m_network;
	PopulationIndex m_pid;

public:
	using PopulationMixin::size;
	using PopulationMixin::type;
	using PopulationMixin::parameters;
	using ParametersMixin::parameters;
	using IterableMixin::operator();
	using ViewableMixin::operator();

	/**
	 * Creates a population as a handle pointing at an already existing
	 * population with the given population index.
	 *
	 * @param network is the network in which the population is located.
	 * @arapm pid is the population index of the already existing population
	 * in the referenced network.
	 */
	PopulationBase(const NetworkBase &network, PopulationIndex pid)
	    : m_network(network), m_pid(pid)
	{
	}

	/**
	 * Returns a handle pointing at the network this population is located in.
	 *
	 * @return a NetworkBase object pointing at the network this neuron is
	 * located in.
	 */
	NetworkBase network() const { return m_network; }

	/**
	 * Returns the population index of this population.
	 *
	 * @return the population index of this population.
	 */
	PopulationIndex pid() const { return m_pid; }
};

/**
 * The PopulationViewBase class represents a subset of neurons in a
 * PopulationView.
 */
class PopulationViewBase
    : public IterableMixin<PopulationViewBase, NeuronBase, Accessor>,
      public ViewableMixin<PopulationViewBase, PopulationViewBase, Accessor>,
      public ParametersMixin<PopulationViewBase, Accessor, NeuronParametersBase>,
      public ConnectableMixin<PopulationViewBase, Accessor> {
private:
	NetworkBase m_network;
	PopulationIndex m_pid;
	NeuronIndex m_nid0;
	NeuronIndex m_nid1;

public:
	using IterableMixin::operator();
	using ViewableMixin::operator();

	/**
	 * Creates a population view as a handle pointing at an already existing
	 * population with the given population index and the specified range of
	 * neuron indices.
	 *
	 * @param network is the network in which the population is located.
	 * @param pid is the population index of the already existing population
	 * in the referenced network.
	 * @param nid0 is the index of the first neuron in the population that
	 * can be accessed in this view.
	 * @param nid1 is the index of the last-plus-one neuron in the population
	 * that can be accessed in this view.
	 */
	PopulationViewBase(const NetworkBase &network, PopulationIndex pid,
	                   NeuronIndex nid0, NeuronIndex nid1)
	    : m_network(network), m_pid(pid), m_nid0(nid0), m_nid1(nid1)
	{
	}

	/**
	 * Returns a handle pointing at the network this population is located in.
	 *
	 * @return a NetworkBase object pointing at the network this neuron is
	 * located in.
	 */
	NetworkBase network() const { return m_network; }

	/**
	 * Returns the population index of this population.
	 *
	 * @return the population index of this population.
	 */
	PopulationIndex pid() const { return m_pid; }

	/**
	 * Returns the index of the first neuron belonging to this PopulationView.
	 */
	NeuronIndex nid_begin() const { return m_nid0; }

	/**
	 * Returns the index of the last-plus-one neuron belonging to this
	 * PopulationView.
	 */
	NeuronIndex nid_end() const { return m_nid1; }
};

/**
 * The NeuronBase class represents a generic neuron of any type. If possible,
 * prefer the usage of Neuron<T>.
 */
class NeuronBase
    : public IterableMixin<NeuronBase, NeuronBase, Accessor>,
      public ViewableMixin<NeuronBase, PopulationViewBase, Accessor>,
      public ParametersMixin<NeuronBase, Accessor, NeuronParametersBase>,
      public NeuronMixin<NeuronBase, Accessor, NeuronParametersBase>,
      public ConnectableMixin<NeuronBase, Accessor> {
private:
	NetworkBase m_network;
	PopulationIndex m_pid;
	NeuronIndex m_nid;

public:
	using NeuronMixin::parameters;
	using NeuronMixin::type;
	using ParametersMixin::parameters;
	using IterableMixin::operator();
	using ViewableMixin::operator();

	/**
	 * Creates a NeuronBase object pointing at the nid-th neuron in the given
	 * neuron population.
	 *
	 * @param paraneter is the neuron population or population view the neuron#
	 * is located in.
	 * @param nid is the absolute index of the neuron in its population.
	 */
	template<typename Parent>
	NeuronBase(const Parent &parent, NeuronIndex nid)
	    : m_network(parent.network()), m_pid(parent.pid()), m_nid(nid)
	{
	}

	/**
	 * Returns the type of the neuron.
	 */
	const NeuronType &type() const;

	/**
	 * Returns a reference at the network instance the neuron is part of.
	 */
	NetworkBase network() const { return NetworkBase(m_network); }

	/**
	 * Returns a handle pointing at the population this neuron is located in.
	 *
	 * @return a PopulationBase object pointing at the population this neuron
	 * is located in.
	 */
	PopulationBase population() const
	{
		return PopulationBase(m_network, m_pid);
	}

	/**
	 * Returns the population index of the population this neuron belongs to.
	 */
	PopulationIndex pid() const { return m_pid; }

	/**
	 * Returns the index of this neuron within its population. Note that this
	 * index is never relative to the start index of a population view but
	 * always relative to the population itself.
	 */
	NeuronIndex nid() const { return m_nid; }
};

/**
 * The Accessor class provides access to certain members of the network objects
 * to the various mixin classes. This perculiar construction is used, since some
 * objects do have special implementations of these functions -- for example,
 * the first neuron index in a population is always zero. With this construct,
 * the compile can directly take these special cases into account when
 * instantiating the mixins.
 */
struct Accessor {
	static NeuronIndex begin(const PopulationBase &) { return 0; }
	static NeuronIndex end(const PopulationBase &pop) { return pop.size(); }
	static PopulationIndex pid(const PopulationBase &pop) { return pop.pid(); }
	static NetworkBase network(const PopulationBase &pop)
	{
		return pop.network();
	}

	static NeuronIndex begin(const PopulationViewBase &view)
	{
		return view.nid_begin();
	}
	static NeuronIndex end(const PopulationViewBase &view)
	{
		return view.nid_end();
	}
	static PopulationIndex pid(const PopulationViewBase &view)
	{
		return view.pid();
	}
	static NetworkBase network(const PopulationViewBase &view)
	{
		return view.network();
	}

	static NeuronIndex begin(const NeuronBase &neuron) { return neuron.nid(); }
	static NeuronIndex end(const NeuronBase &neuron)
	{
		return neuron.nid() + 1;
	}
	static PopulationIndex pid(const NeuronBase &neuron)
	{
		return neuron.pid();
	}
	static NetworkBase network(const NeuronBase &neuron)
	{
		return neuron.network();
	}
};
}

#endif /* CYPRESS_CORE_NETWORK_BASE_OBJECTS_HPP */
