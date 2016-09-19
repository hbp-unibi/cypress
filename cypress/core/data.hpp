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

#pragma once

#ifndef CYPRESS_CORE_DATA_HPP
#define CYPRESS_CORE_DATA_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <cypress/core/types.hpp>
#include <cypress/util/matrix.hpp>

namespace cypress {
/*
 * Forward declarations
 */
class NeuronType;

/**
 * Used internally to specify a range between two iterators which itself
 * can be used in a range-based for loop.
 *
 * @tparam iterator is the iterator type that is being used.
 */
template <typename Iterator>
struct IterableRange {
private:
	/**
	 * First element.
	 */
	Iterator m_begin;

	/**
	 * Last-plus-one element.
	 */
	Iterator m_end;

public:
	/**
	 * Constructor -- copies the given iterators into the private class
	 * members. Prefer to use the make_iterable_range method instead.
	 *
	 * @param begin is an iterator pointing at the first element.
	 * @param end is an iterator pointing at the last-plus-one element.
	 */
	IterableRange(Iterator begin, Iterator end) : m_begin(begin), m_end(end) {}

	/**
	 * Returns the begin iterator.
	 */
	Iterator begin() { return m_begin; }

	/**
	 * Returns the end iterator.
	 */
	Iterator end() { return m_end; }
};

/**
 * Allows to create an instance of IterableRange while automatically
 * deducing the iterator type.
 *
 * @param begin is an iterator pointing at the first element.
 * @param end is an iterator pointing at the last-plus-one element.
 */
template <typename Iterator>
IterableRange<Iterator> make_iterable_range(Iterator begin, Iterator end)
{
	return IterableRange<Iterator>(begin, end);
}

/**
 * Stores the actual data that is being accessed by the NeuronParametersBase
 * and NeuronSignalsBase classes via the PopulationDataView class.
 */
class PopulationData {
public:
	/**
	 * Type used to represent the parameters of a single neuron.
	 */
	using ParameterType = std::vector<Real>;

	/**
	 * Type used to represent the record flags of a single neuron.
	 */
	using RecordType = std::vector<uint8_t>;

	/**
	 * Type used to represent the data matrices containing the recorded data for
	 * a single neuron, each vector enetry corresponds to a signal.
	 */
	using DataType = std::vector<std::shared_ptr<Matrix<Real>>>;

private:
	/**
	 * Size of the population.
	 */
	size_t m_size;

	/**
	 * Neuron type used by the population.
	 */
	NeuronType const *m_type;

	/**
	 * Name of the population.
	 */
	std::string m_name;

	/**
	 * Parameters of the individual neurons. If one element is stored in the
	 * vector, this means that the parameters are shared homogeneously between
	 * all neurons of the population. If the size of the parameter vector
	 * corresponds to the size of the population, each neuron possesses an
	 * individual parameter set.
	 */
	std::vector<ParameterType> m_parameters;

	/**
	 * Flags indicating whether a signal should be recorded or not. If one
	 * element is stored in the vector, this means that the record flags are
	 * shared homogeneously between all neurons of the population. If the size
	 * of the vector corresponds to the size of the population, each neuron
	 * individual record flags.
	 */
	std::vector<RecordType> m_record;

	/**
	 * Actually recorded data for each neuron.
	 */
	std::vector<DataType> m_data;

public:
	/**
	 * Constructor of the PopulationData class.
	 */
	PopulationData(
	    size_t size = 1, NeuronType const *type = nullptr,
	    const std::string &name = std::string(),
	    const std::vector<ParameterType> &parameters =
	        std::vector<ParameterType>(),
	    const std::vector<RecordType> &record = std::vector<RecordType>(),
	    const std::vector<DataType> &data = std::vector<DataType>())
	    : m_size(size),
	      m_type(type),
	      m_name(name),
	      m_parameters(parameters),
	      m_record(record),
	      m_data(data)
	{
	}

	/**
	 * Returns the population size.
	 */
	size_t size() const { return m_size; }

	/**
	 * Returns the population type.
	 */
	NeuronType const *type() const { return m_type; }

	/**
	 * Returns a reference at the population name, allowing to set it.
	 */
	std::string &name() { return m_name; }

	/**
	 * Returns a const-reference at the population name.
	 */
	const std::string &name() const { return m_name; }

	/**
	 * Returns a const reference at the internal parameter vector, containing
	 * either exactly one entry or a number of entries corresponding to the size
	 * of the population.
	 */
	const std::vector<ParameterType> &parameters() const
	{
		return m_parameters;
	}

	/**
	 * Returns a reference at the internal parameter vector, possibly containing
	 * the parameters of all neurons.
	 */
	std::vector<ParameterType> &parameters() { return m_parameters; }

	/**
	 * Returns a const reference at the internal record-flag vector, containing
	 * either exactly one entry or a number of entries corresponding to the size
	 * of the population.
	 */
	const std::vector<RecordType> &record() const { return m_record; }

	/**
	 * Returns a reference at the internal record-flag vector, possibly
	 * containing the record flags of all neurons.
	 */
	std::vector<RecordType> &record() { return m_record; }

	/**
	 * Returns a const reference at the internal parameter vector, containing
	 * either exactly one entry or a number of entries corresponding to the size
	 * of the population.
	 */
	const std::vector<DataType> &data() const { return m_data; }

	/**
	 * Returns a reference at the internal parameter vector, possibly containing
	 * the record flags of all neurons.
	 */
	std::vector<DataType> &data() { return m_data; }

	/**
	 * Returns the size of the parameter vector. Throws an exception if the
	 * neurons in the given range do not share parameters of the same size.
	 *
	 * @param nid0 is the index of the first neuron for which the parameter
	 * vector size should be returned.
	 * @param nid1 is the index of the last-plus-one neuron for which the
	 * parameter vector size should be returned.
	 * @return the number of parameters in the parameter array.
	 */
	size_t get_parameters_size(NeuronIndex nid0, NeuronIndex nid1) const;

	/**
	 * Returns the size of the record vector. Throws an exception if the
	 * neurons in the given range do not share parameters of the same size.
	 *
	 * @param nid0 is the index of the first neuron for which the record
	 * vector size should be returned.
	 * @param nid1 is the index of the last-plus-one neuron for which the
	 * record vector size should be returned.
	 * @return the number of entires in the record array.
	 */
	size_t get_record_size(NeuronIndex nid0, NeuronIndex nid1) const;

	/**
	 * Returns the size of the record vector. Throws an exception if the
	 * neurons in the given range do not share parameters of the same size.
	 *
	 * @param nid0 is the index of the first neuron for which the record data
	 * size should be returned.
	 * @param nid1 is the index of the last-plus-one neuron for which the
	 * data vector size should be returned.
	 * @return the number of entires in the data array.
	 */
	size_t get_data_size(NeuronIndex nid0, NeuronIndex nid1) const;

	/**
	 * Returns a const-reference at the parameter vector for the neurons with
	 * the id nid0 to nid1. Throws an exception if the neurons in the given
	 * range do not share the same parameters.
	 *
	 * @param nid0 is the index of the first neuron for which the parameter
	 * vector should be returned.
	 * @param nid1 is the index of the last-plus-one neuron for which the
	 * parameter vector should be returned.
	 * @return the parameters shared by the neurons in the given range.
	 */
	const ParameterType &read_parameters(NeuronIndex nid0,
	                                     NeuronIndex nid1) const;

	/**
	 * Returns a const-reference at the vector specifying whether a signal
	 * should be recorded or not for the neurons with id nid0 to nid1. Throws
	 * an exception if the neurons in this range do not share the same
	 * record flags.
	 *
	 * @param nid0 is the index of the first neuron for which the record flags
	 * should be returned.
	 * @param nid1 is the index of the last-plus-one neuron for which the
	 * record flags should be returned.
	 * @return the record flags shared by the neurons in the given range.
	 */
	const RecordType &read_record(NeuronIndex nid0, NeuronIndex nid1) const;

	/**
	 * Returns a const-reference at the vector containing the references at the
	 * recorded data for the neurons with the nid0 to nid1. Throws an exception
	 * if the neurons in this range do not share the same data.
	 *
	 * @param nid0 is the index of the first neuron for which the record flags
	 * should be returned.
	 * @param nid1 is the index of the last-plus-one neuron for which the
	 * record flags should be returned.
	 * @return the record data shared by the neurons in the given range.
	 */
	const DataType &read_data(NeuronIndex nid0, NeuronIndex nid1) const;

	/**
	 * Returns an iterable range allowing to modify the parameters for the given
	 * range of neurons. Assumes that the data is being set to the
	 * same value for the specified range.
	 *
	 * @param nid0 is the index of the first neuron for which the parameters
	 * should be written.
	 * @param nid1 is the index of the last-plus-one neuron for which the
	 * parameters should be written.
	 * @param partial if true, indicates that the parameters are not entirely
	 * overridden, preventing the method from re-homogenising the data.
	 * @return an iterable object pointing at the parameters for the specified
	 * neuron range.
	 */
	IterableRange<std::vector<ParameterType>::iterator> write_parameters(
	    NeuronIndex nid0, NeuronIndex nid1, bool partial = true);

	/**
	 * Returns an iteratble range allowing to modify the record flags for the
	 * specified range of neurons. Assumes that the data is being set to the
	 * same value for the specified range.
	 *
	 * @param nid0 is the index of the first neuron for which the record flags
	 * should be written.
	 * @param nid1 is the index of the last-plus-one neuron for which the
	 * record flags should be written.
	 * @param partial if true, indicates that the record flags are not entirely
	 * overridden, preventing the method from re-homogenising the data.
	 * @return an iterable object pointing at the record flags for the specified
	 * neuron range.
	 */
	IterableRange<std::vector<RecordType>::iterator> write_record(
	    NeuronIndex nid0, NeuronIndex nid1, bool partial = true);

	/**
	 * Returns an iteratble range allowing to write the recorded data for the
	 * specified range of neurons. Assumes that the data is being set to the
	 * same value for the specified range.
	 *
	 * @param nid0 is the index of the first neuron for which the recorded data
	 * should be written.
	 * @param nid1 is the index of the last-plus-one neuron for which the
	 * recorded data should be written.
	 * @param partial if true, indicates that the record data is not entirely
	 * overridden, preventing the method from re-homogenising the data.
	 * @return an iterable object pointing at the recorded data for the
	 * specified neuron range.
	 */
	IterableRange<std::vector<DataType>::iterator> write_data(
	    NeuronIndex nid0, NeuronIndex nid1, bool partial = true);

	/**
	 * Returns true if all neurons in the population share the same parameters.
	 */
	bool homogeneous_parameters() const { return m_parameters.size() <= 1; }

	/**
	 * Returns true if all neurons in the population share the same record
	 * flags.
	 */
	bool homogeneous_record() const { return m_record.size() <= 1; }

	/**
	 * Returns true if all neurons in the poopulation share the same recorded
	 * data.
	 */
	bool homogeneous_data() const { return m_data.size() <= 1; }
};

/**
 * View into part of the population data, indicated by a neuron start index
 * and a neuron end index.
 */
class PopulationDataView {
private:
	/**
	 * Pointer at the data storage.
	 */
	std::shared_ptr<PopulationData> m_data;

	/**
	 * Index of the first neuron represented by this view.
	 */
	NeuronIndex m_nid0;

	/**
	 * Index of the last-plus-one neuron represented by this view.
	 */
	NeuronIndex m_nid1;

	/**
	 * True if the view covers the "parameters" section.
	 */
	bool m_own_parameters : 1;

	/**
	 * True if the view covers the "record" section.
	 */
	bool m_own_record : 1;

	/**
	 * True if the view covers the "data" section.
	 */
	bool m_own_data : 1;

protected:
	/**
	 * Templated method which allows to construct a PopulationDataView with its
	 * own data from a list of PopulationData objects. This method is used by
	 * the various neuron parameter and signal implementations to be able to
	 * supply a list of parameters/signals to the various constructors.
	 */
	template <typename Seq>
	static typename Seq::value_type from_sequence(Seq list)
	{
		using Impl = typename Seq::value_type;

		// Create containers for the compund object.
		const size_t size = list.size();
		std::vector<PopulationData::ParameterType> parameters(size);
		std::vector<PopulationData::RecordType> record(size);
		std::vector<PopulationData::DataType> data(size);

		// Fill the containers
		size_t idx = 0;
		for (const Impl &i : list) {
			const PopulationData &id = i.population_data();
			if (id.parameters().size() == 1 && i.m_own_parameters) {
				parameters[idx] = id.parameters()[0];
			}
			if (id.record().size() == 1 && i.m_own_record) {
				record[idx] = id.record()[0];
			}
			if (id.data().size() == 1 && i.m_own_data) {
				data[idx] = id.data()[0];
			}
			idx++;
		}
		return Impl(std::make_shared<PopulationData>(
		                size, nullptr, std::string(), parameters, record, data),
		            0, size);
	}

	/**
	 * Returns a const reference at the underlying PopulationData.
	 */
	const PopulationData &population_data() const { return *m_data; }

	/**
	 * Returns a reference at the underlying PopulationData.
	 */
	PopulationData &population_data() { return *m_data; }

	/**
	 * Returns the size of the parameter vector. Throws an exception if the
	 * neurons in the given range do not share parameters of the same size.
	 *
	 * @return the number of parameters in the parameter array.
	 */
	size_t get_parameters_size() const
	{
		return population_data().get_parameters_size(m_nid0, m_nid1);
	}

	/**
	 * Returns the size of the record vector. Throws an exception if the
	 * neurons in the given range do not share parameters of the same size.
	 *
	 * @return the number of entires in the record array.
	 */
	size_t get_record_size() const
	{
		return population_data().get_record_size(m_nid0, m_nid1);
	}

	/**
	 * Returns the size of the record vector. Throws an exception if the
	 * neurons in the given range do not share parameters of the same size.
	 *
	 * @return the number of entires in the data array.
	 */
	size_t get_data_size() const
	{
		return population_data().get_data_size(m_nid0, m_nid1);
	}

	/**
	 * Returns a const-reference at the parameter vector for the neurons
	 * represented by this view. Throws an exception if the neurons in this view
	 * do not share the same parameters.
	 *
	 * @return the parameters shared by the neurons in the given range.
	 */
	const auto &read_parameters() const
	{
		return population_data().read_parameters(m_nid0, m_nid1);
	}

	/**
	 * Returns a const-reference at the vector specifying whether a signal
	 * should be recorded or not for the neurons represented by this view.
	 * Throws an exception if the neurons in this view do not share the same
	 * record flags.
	 *
	 * @return the record flags shared by the neurons in the given range.
	 */
	const auto &read_record() const
	{
		return population_data().read_record(m_nid0, m_nid1);
	}

	/**
	 * Returns a const-reference at the vector containing the recorded data for
	 * the neurons represented by this view. Throws an exception if the neurons
	 * in this view do not share the same record flags.
	 *
	 * @return the record data shared by the neurons in the given range.
	 */
	const auto &read_data() const
	{
		return population_data().read_data(m_nid0, m_nid1);
	}

	/**
	 * Returns an iterable range allowing to modify the parameters for the range
	 * of neurons represented by this view. Assumes that the data is being set
	 * to the same value for the specified range.
	 *
	 * @param partial if true, indicates that the parameters are not entirely
	 * overridden, preventing the method from re-homogenising the data.
	 * @return an iterable object pointing at the parameters for the specified
	 * neuron range.
	 */
	auto write_parameters(bool partial = true)
	{
		return population_data().write_parameters(m_nid0, m_nid1, partial);
	}

	/**
	 * Returns an iteratble range allowing to modify the record flags for the
	 * range of neurons represented by this view. Assumes that the data is being
	 * set to the same value for the specified range.
	 *
	 * @param partial if true, indicates that the record flags are not entirely
	 * overridden, preventing the method from re-homogenising the data.
	 * @return an iterable object pointing at the record flags for the specified
	 * neuron range.
	 */
	auto write_record(bool partial = true)
	{
		return population_data().write_record(m_nid0, m_nid1, partial);
	}

	/**
	 * Returns an iteratble range allowing to write the recorded data for the
	 * range of neurons represented by this vie2. Assumes that the data is being
	 * set to the
	 * same value for the specified range.
	 *
	 * @param partial if true, indicates that the record data is not entirely
	 * overridden, preventing the method from re-homogenising the data.
	 * @return an iterable object pointing at the recorded data for the
	 * specified neuron range.
	 */
	auto write_data(bool partial = true)
	{
		return population_data().write_data(m_nid0, m_nid1, partial);
	}

public:
	/**
	 * Default constructor -- creates a separate instance of the underlying
	 * PopulationData.
	 */
	PopulationDataView()
	    : m_data(std::make_shared<PopulationData>(1)),
	      m_nid0(0),
	      m_nid1(1),
	      m_own_parameters(true),
	      m_own_record(true),
	      m_own_data(true)
	{
	}

	/**
	 * Creates a decoupled copy of the original population data view.
	 *
	 * @param other is the PopulationDataView of which a copy should be created.
	 */
	PopulationDataView(const PopulationDataView &other);

	/**
	 * Move constructor.
	 */
	PopulationDataView(PopulationDataView &&) noexcept = default;

	/**
	 * Constructor which allows the PopulationDataView to act as a view onto
	 * the data stored for a range of neurons in a population. The ownership
	 * flags specify which data sections are updated on assignment.
	 *
	 * @param data is a reference onto the underlying population data store.
	 * @param nid0 is the index of the first neuron represented by this view.
	 * @param nid1 is the index of the last-plus-one neuron represented by this
	 * view.
	 * @param own_parameters if true, the view covers the "parameters" section
	 * of the underlying data.
	 * @param own_record if true, the view covers the "record" section of the
	 * underlying data.
	 * @param own_data if true, the view covers the "data" section of the
	 * underlying data.
	 */
	PopulationDataView(std::shared_ptr<PopulationData> data, NeuronIndex nid0,
	                   NeuronIndex nid1, bool own_parameters = true,
	                   bool own_record = true, bool own_data = true)
	    : m_data(data),
	      m_nid0(nid0),
	      m_nid1(nid1),
	      m_own_parameters(own_parameters),
	      m_own_record(own_record),
	      m_own_data(own_data)
	{
	}

	/**
	 * Assignment operator. Sets the data in this instance to the data given in
	 * the other instance.
	 */
	PopulationDataView &operator=(const PopulationDataView &other);
};
}

#endif /* CYPRESS_CORE_DATA_HPP */
