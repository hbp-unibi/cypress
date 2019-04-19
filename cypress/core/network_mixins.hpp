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
 * @file network_traits.hpp
 *
 * Header containing certain mixin classes used in various network classes to
 * provide a consistent interface.
 *
 * @author Andreas Stöckel
 */

#pragma once

#ifndef CYPRESS_CORE_NETWORK_MIXINS_HPP
#define CYPRESS_CORE_NETWORK_MIXINS_HPP

#include <algorithm>
#include <memory>
#include <iterator>

#include <cypress/core/connector.hpp>
#include <cypress/core/exceptions.hpp>
#include <cypress/core/network_base.hpp>
#include <cypress/core/neurons.hpp>
#include <cypress/core/types.hpp>
#include <cypress/util/comperator.hpp>

namespace cypress {
/**
 * Mixin used by PopulationBase and Population to provide the functions that
 * are expected by a population class.
 */
template <typename Impl, typename Accessor>
class PopulationMixin {
private:
	Impl &impl() { return reinterpret_cast<Impl &>(*this); }
	const Impl &impl() const { return reinterpret_cast<const Impl &>(*this); }

	NetworkBase network() const { return Accessor::network(impl()); }
	PopulationIndex pid() const { return Accessor::pid(impl()); }
	std::shared_ptr<PopulationData> data() const
	{
		return network().population_data(pid());
	}

public:
	/**
	 * Returns the name of the population.
	 */
	const std::string &name() const { return data()->name(); }

	/**
	 * Sets the name of the population and returns a reference at the population
	 * object.
	 */
	Impl &name(const std::string &name)
	{
		data()->name(name);
		return impl();
	}

	/**
	 * Returns the number of neurons in the population.
	 */
	size_t size() const { return data()->size(); }

	/**
	 * Returns true if the population possesses homogeneous neuron parameters.
	 */
	bool homogeneous_parameters() const
	{
		return data()->homogeneous_parameters();
	}

	/**
	 * Returns true if the population possesses homogeneous record flags.
	 */
	bool homogeneous_record() const { return data()->homogeneous_record(); }

	/**
	 * Returns true if the population possesses homogeneous data.
	 */
	bool homogeneous_data() const { return data()->homogeneous_data(); }
};

template <typename Impl>
class NeuronMixin {
private:
	Impl &impl() { return reinterpret_cast<Impl &>(*this); }
	const Impl &impl() const { return reinterpret_cast<const Impl &>(*this); }

public:
	Impl *operator->() { return &impl(); }
	const Impl *operator->() const { return &impl(); }
	Impl &operator*() { return impl(); }
	const Impl &operator*() const { return impl(); }
};

/**
 * Mixin which allows to connect one iterable element with another.
 */
template <typename Impl, typename Accessor>
class ConnectableMixin {
private:
	Impl &impl() { return reinterpret_cast<Impl &>(*this); }
	const Impl &impl() const { return reinterpret_cast<const Impl &>(*this); }

	auto network() const { return Accessor::network(impl()); }
	PopulationIndex pid() const { return Accessor::pid(impl()); }
	NeuronIndex i0() const { return Accessor::begin(impl()); }
	NeuronIndex i1() const { return Accessor::end(impl()); }

public:
	/**
	 * Connects all neurons in this Population, PopulationView or Neuron with
	 * a range of neurons indicated by the given iterators.
	 *
	 * @param tar_begin is an iterator pointing at the first neuron in the
	 * target
	 * population that should be connected.
	 * @param tar_end is an iterator pointing at the last neuron in the
	 * target population that should be connected.
	 * @param connector is the object establishing the actual connections.
	 * @return a reference at this object for function chaining.
	 */
	template <typename TargetIterator>
	Impl &connect_to(TargetIterator tar_begin, TargetIterator tar_end,
	                 std::unique_ptr<Connector> connector, const char *name = "")
	{
		network().connect(pid(), i0(), i1(), tar_begin->pid(), tar_begin->nid(),
		                  tar_end->nid(), std::move(connector), name);
		return impl();
	}

	/**
	 * Connects all neurons in this Population, PopulationView or Neuron with
	 * the given target, which may be a Population, PopulationView or a single
	 * neuron.
	 *
	 * @param tar is the object this object should be connected with.
	 * @param connector is the object establishing the actual connections.
	 * @return a reference at this object for function chaining.
	 */
	template <typename Target>
	Impl &connect_to(const Target &tar, std::unique_ptr<Connector> connector, const char *name = "")
	{
		return connect_to(tar.begin(), tar.end(), std::move(connector), name);
	}
};

/**
 * Mixin which allows to set and access the parameters and signals of a
 * Population, PopulationView or Neuron.
 */
template <typename Impl, typename Accessor, typename Params, typename Signals>
class DataMixin {
private:
	Impl &impl() { return reinterpret_cast<Impl &>(*this); }
	const Impl &impl() const { return reinterpret_cast<const Impl &>(*this); }

	auto network() const { return Accessor::network(impl()); }
	PopulationIndex pid() const { return Accessor::pid(impl()); }
	std::shared_ptr<PopulationData> data() const
	{
		return network().population_data(pid());
	}
	NeuronIndex i0() const { return Accessor::begin(impl()); }
	NeuronIndex i1() const { return Accessor::end(impl()); }

public:
	template <typename Other>
	bool operator==(const Other &o) const
	{
		return Comperator<int32_t>::equals(pid(), o.pid())(
		    i0(), o.begin()->nid())(i1(), o.end()->nid())();
	}

	template <typename Other>
	bool operator!=(const Other &o) const
	{
		return Comperator<int32_t>::inequal(pid(), o.pid())(
		    i0(), o.begin()->nid())(i1(), o.end()->nid())();
	}

	template <typename Other>
	bool operator<(const Other &o) const
	{
		return Comperator<int32_t>::smaller(pid(), o.pid())(
		    i0(), o.begin()->nid())(i1(), o.end()->nid())();
	}

	template <typename Other>
	bool operator<=(const Other &o) const
	{
		return Comperator<int32_t>::smaller_equals(pid(), o.pid())(
		    i0(), o.begin()->nid())(i1(), o.end()->nid())();
	}

	template <typename Other>
	bool operator>(const Other &o) const
	{
		return Comperator<int32_t>::larger(pid(), o.pid())(
		    i0(), o.begin()->nid())(i1(), o.end()->nid())();
	}

	template <typename Other>
	bool operator>=(const Other &o) const
	{
		return Comperator<int32_t>::larger_equals(pid(), o.pid())(
		    i0(), o.begin()->nid())(i1(), o.end()->nid())();
	}

	/**
	 * Returns the type of the population. All neurons share the same type.
	 */
	const NeuronType &type() const { return *(data()->type()); }

	/**
	 * Returns an object which can be used for reading the parameters of the
	 * neurons represented by this object. Note that read access may be
	 * constrained when the parameters in the underlying neurons do not possess
	 * the same parameters.
	 */
	const Params parameters() const { return Params(data(), i0(), i1()); }

	/**
	 * Returns an object which can be used for setting the parameters of the
	 * neurons. Note that read access may be constrained when the parameters in
	 * the underlying neurons do not possess the same parameters.
	 */
	Params parameters() { return Params(data(), i0(), i1()); }

	/**
	 * Returns an object which can be used for reading the record flags and the
	 * recorded data.
	 */
	const Signals signals() const { return Signals(data(), i0(), i1()); }

	/**
	 * Returns an object which can be used for reading and writing the record
	 * flags and the recorded data.
	 */
	Signals signals() { return Signals(data(), i0(), i1()); }
};

/**
 * Mixin used by the PopulationBase, PopulationViewBase, NeuronBase, Population,
 * PopulationView and Neuron class to allow to provide a view (a subrange) of
 * neurons in the object.
 */
template <typename Impl, typename View, typename Accessor>
class ViewableMixin {
private:
	Impl &impl() { return reinterpret_cast<Impl &>(*this); }
	const Impl &impl() const { return reinterpret_cast<const Impl &>(*this); }

	auto network() const { return Accessor::network(impl()); }
	PopulationIndex pid() const { return Accessor::pid(impl()); }
	NeuronIndex i0() const { return Accessor::begin(impl()); }
	NeuronIndex i1() const { return Accessor::end(impl()); }

#ifndef NDEBUG
	/**
	 * Makes sure the given begin and end indices are inside the valid range.
	 */
	void check_range(NeuronIndex nid0, NeuronIndex nid1) const
	{
		if (nid0 < i0() || nid1 > i1()) {
			throw std::out_of_range(
			    "Range must be a subset of the source population range.");
		}
	}
#else
	void check_range(NeuronIndex, NeuronIndex) const {}
#endif

public:
	/**
	 * Returns a constant View object representing a range of neurons relative
	 * to this object.
	 *
	 * @param begin is the index of the first neuron in the resulting view,
	 * relative to the first neuron in this object.
	 * @param end is the index of the last-plus-one neuron in the resulting
	 * view, relative to the first neuron in this object.
	 * @return a new view representing only the given range of neurons.
	 */
	const View range(NeuronIndex begin, NeuronIndex end) const
	{
		check_range(i0() + begin, i0() + end);
		return View(network(), pid(), i0() + begin, i0() + end);
	}

	/**
	 * Returns a View object representing a range of neurons relative to this
	 * object.
	 *
	 * @param begin is the index of the first neuron in the resulting view,
	 * relative to the first neuron in this object.
	 * @param end is the index of the last-plus-one neuron in the resulting
	 * view, relative to the first neuron in this object.
	 * @return a new view representing only the given range of neurons.
	 */
	View range(NeuronIndex begin, NeuronIndex end)
	{
		check_range(i0() + begin, i0() + end);
		return View(network(), pid(), i0() + begin, i0() + end);
	}

	/**
	 * Returns a constant View object representing a range of neurons relative
	 * to this object.
	 *
	 * @param begin is the index of the first neuron in the resulting view,
	 * relative to the first neuron in this object.
	 * @param end is the index of the last-plus-one neuron in the resulting
	 * view, relative to the first neuron in this object.
	 * @return a new view representing only the given range of neurons.
	 */
	const View operator()(NeuronIndex begin, NeuronIndex end) const
	{
		return range(begin, end);
	}

	/**
	 * Returns a View object representing a range of neurons relative to this
	 * object.
	 *
	 * @param begin is the index of the first neuron in the resulting view,
	 * relative to the first neuron in this object.
	 * @param end is the index of the last-plus-one neuron in the resulting
	 * view, relative to the first neuron in this object.
	 * @return a new view representing only the given range of neurons.
	 */
	View operator()(NeuronIndex begin, NeuronIndex end)
	{
		return range(begin, end);
	}
};

/**
 * Mixin used by the PopulationBase, PopulationViewBase, NeuronBase, Population,
 * PopulationView and Neuron class to allow to provide exactly the same
 * interface for iterating.
 */
template <typename Impl, typename Value, typename Accessor>
class IterableMixin {
public:
	/**
	 * Generic class implementing the various iterator types exposed by the
	 * Population class. Note that comparison operators only compare the neuron
	 * index and do not check for equivalence of the population object.
	 * Comparing iterators from different population objects is nonsense.
	 *
	 * @tparam Dir is the direction into which the iterator advances.
	 * @tparam Const if true, the iterator allows no write access to the
	 * underlying population. If false, the corresponding access operators are
	 * enabled.
	 */
	template <int Dir, bool Const>
	class iterator_ : public std::iterator<std::random_access_iterator_tag,
	                                       Value, NeuronIndex, Value, Value> {
	private:
		/**
		 * Shorthand for the own type.
		 */
		using Self = iterator_<Dir, Const>;

		/**
		 * Pointer at the backing population object. May either be a const
		 * pointer (if the Const flag is true) or a normal pointer (if the Const
		 * flag is false).
		 */
		using ImplPtr =
		    typename std::conditional<Const, Impl const *, Impl *>::type;

		/**
		 * Pointer at the parent class.
		 */
		ImplPtr m_impl;

		/**
		 * Current neuron index the iterator points at.
		 */
		NeuronIndex m_idx;

	public:
		/**
		 * Constructor, creates an iterator pointing at the neuron with the
		 * index idx in the given population.
		 *
		 * @param impl is a pointer at the underlying class.
		 * @param idx is the index of the neuron.
		 */
		iterator_(ImplPtr impl, NeuronIndex idx) : m_impl(impl), m_idx(idx) {}

		/**
		 * Allow implicit conversion to const-iterators.
		 */
		operator iterator_<Dir, true>() const
		{
			return iterator_<Dir, true>(m_impl, m_idx);
		}

		bool operator==(const Self &o) const { return m_idx == o.m_idx; }

		bool operator!=(const Self &o) const { return m_idx != o.m_idx; }

		bool operator<(const Self &o) const
		{
			return m_idx * Dir < o.m_idx * Dir;
		}

		bool operator>(const Self &o) const
		{
			return m_idx * Dir > o.m_idx * Dir;
		}

		bool operator<=(const Self &o) const
		{
			return m_idx * Dir <= o.m_idx * Dir;
		}

		bool operator>=(const Self &o) const
		{
			return m_idx * Dir >= o.m_idx * Dir;
		}

		Self operator+(NeuronIndex b) { return Self(m_impl, m_idx + b * Dir); }

		Self operator-(NeuronIndex b) { return Self(m_impl, m_idx - b * Dir); }

		NeuronIndex operator-(const Self &o) const
		{
			return (m_idx - o.m_idx) * Dir;
		}

		Self &operator++()
		{
			m_idx += Dir;
			return *this;
		}
		Self operator++(int)
		{
			auto cpy = Self(m_impl, m_idx);
			m_idx += Dir;
			return cpy;
		}

		Self &operator--()
		{
			m_idx -= Dir;
			return *this;
		}

		Self operator--(int)
		{
			auto cpy = Self(m_impl, m_idx);
			m_idx -= Dir;
			return cpy;
		}

		Self &operator+=(NeuronIndex n)
		{
			m_idx += n * Dir;
			return *this;
		}

		Self &operator-=(NeuronIndex n)
		{
			m_idx -= n * Dir;
			return *this;
		}

		template <typename U = Value,
		          typename = typename std::enable_if<!Const, U>::type>
		Value operator*()
		{
			return Value(*m_impl, m_idx);
		}

		template <typename U = Value,
		          typename = typename std::enable_if<!Const, U>::type>
		Value operator->()
		{
			return Value(*m_impl, m_idx);
		}

		template <typename U = Value,
		          typename = typename std::enable_if<!Const, U>::type>
		Value operator[](NeuronIndex n)
		{
			return Value(*m_impl, m_idx + n * Dir);
		}

		Value operator*() const { return Value(*m_impl, m_idx); }

		Value operator->() const { return Value(*m_impl, m_idx); }

		Value operator[](NeuronIndex n) const
		{
			return Value(*m_impl, m_idx + n * Dir);
		}
	};

private:
	Impl *impl() { return reinterpret_cast<Impl *>(this); }
	const Impl *impl() const { return reinterpret_cast<const Impl *>(this); }

	NeuronIndex i0() const { return Accessor::begin(*impl()); }
	NeuronIndex i1() const { return Accessor::end(*impl()); }
	NeuronIndex ri0() const { return Accessor::end(*impl()) - 1; }
	NeuronIndex ri1() const { return Accessor::begin(*impl()) - 1; }

#ifndef NDEBUG
	/**
	 * Makes sure the given begin and end indices are inside the valid range.
	 */
	void check_range(NeuronIndex nid0, NeuronIndex nid1) const
	{
		if (nid0 < i0() || nid1 > i1()) {
			throw std::out_of_range(
			    "Range must be a subset of the source population range.");
		}
	}
#else
	void check_range(NeuronIndex, NeuronIndex) const {}
#endif

public:
	using iterator = iterator_<1, false>;
	using const_iterator = iterator_<1, true>;
	using reverse_iterator = iterator_<-1, false>;
	using const_reverse_iterator = iterator_<-1, true>;

	/**
	 * Returns a reference at the i-th neuron in the Population or
	 * PopulationView. For neurons, only the index zero is allowed, providing
	 * access at the neuron itself.
	 */
	Value operator[](NeuronIndex i) { return Value(*impl(), i0() + i); }

	/**
	 * Returns a const-reference at the i-th neuron. For neurons, only the index
	 * zero is allowed, providing access at the neuron itself.
	 */
	const Value operator[](NeuronIndex i) const
	{
		return Value(*impl(), i0() + i);
	}

	/**
	 * Returns a reference at the i-th neuron in the Population or
	 * PopulationView. For neurons, only the index zero is allowed, providing
	 * access at the neuron itself.
	 */
	Value operator()(NeuronIndex i) { return Value(*impl(), i0() + i); }

	/**
	 * Returns a const-reference at the i-th neuron. For neurons, only the index
	 * zero is allowed, providing access at the neuron itself.
	 */
	const Value operator()(NeuronIndex i) const
	{
		return Value(*impl(), i0() + i);
	}

	/**
	 * Returns the size of the population or population view. For single neurons
	 * the size is always one.
	 */
	size_t size() { return i1() - i0(); }

	/**
	 * Returns an iterator at the first neuron in the population. For neurons,
	 * returns an iterator pointing at the neuron itself.
	 */
	iterator begin() { return iterator(impl(), i0()); }

	/**
	 * Returns an iterator at the last-plus-one neuron in the population.
	 */
	iterator end() { return iterator(impl(), i1()); }

	reverse_iterator rbegin() { return reverse_iterator(impl(), ri0()); }

	reverse_iterator rend() { return reverse_iterator(impl(), ri1()); }

	const_iterator begin() const { return cbegin(); }

	const_iterator end() const { return cend(); }

	const_reverse_iterator rbegin() const { return crbegin(); }

	const_reverse_iterator rend() const { return crend(); }

	const_iterator cbegin() const { return const_iterator(impl(), i0()); }

	const_iterator cend() const { return const_iterator(impl(), i1()); }

	const_reverse_iterator crbegin() const
	{
		return const_reverse_iterator(impl(), ri0());
	}

	const_reverse_iterator crend() const
	{
		return const_reverse_iterator(impl(), ri1());
	}
};
}

#endif /* CYPRESS_CORE_NETWORK_MIXINS_HPP */
