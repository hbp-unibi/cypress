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

#include <cassert>

#include <cypress/core/data.hpp>
#include <cypress/core/exceptions.hpp>

namespace cypress {

/*
 * Class PopulationData
 */

/**
 * Returns a const-reference at the data for the neurons nid0 to nid1.
 */
template <typename T>
static const typename T::value_type &read(const T &data, NeuronIndex nid0,
                                          NeuronIndex nid1)
{
	// Reading data although none has been written
	assert(data.size() > 0);

	// Read access is only possible if a) the underlying population data is
	// homogeneous, or b) the view only represents a single neuron
	if (data.size() == 1) {
		return data[0];
	}
	else if (nid1 - nid0 == 1) {
		return data[nid0];
	}
	else {
		// Probably the data actually is homogeneous, although stored in
		// multiple data elements. In this case, return a reference at
		// the first element of the range.
		auto it0 = data.begin() + nid0;
		auto it1 = data.begin() + nid1;
		if (std::find_if(it0, it1, [it0](const typename T::value_type &val) {
			    return val != *it0;
			}) == it1) {
			return *it0;
		}
	}
	throw HomogeneousPopulationRequiredException();
}

/**
 * Returns a reference at the data-element accessed
 */
template <typename T>
static IterableRange<typename T::iterator> write(T &data, NeuronIndex nid0,
                                                 NeuronIndex nid1, ssize_t size,
                                                 bool partial)
{
	// Check whether this is an homogeneous write access. In case the
	// underlying population already is homogeneous or a non-partial write
	// is performed, only the first storage element needs to be accessed.
	if ((nid1 - nid0 == size) && (!partial || data.size() <= 1)) {
		data.resize(1);
		return make_iterable_range(data.begin(), data.begin() + 1);
	}

	// If the data is currently homogeneous, heterogenise it
	const size_t old_size = data.size();
	if (old_size <= 1) {
		data.resize(size);
		if (old_size == 1) {
			std::fill(data.begin() + 1, data.end(), data[0]);
		}
	}
	return make_iterable_range(data.begin() + nid0, data.begin() + nid1);
}

template <typename T>
static size_t get_size(T &data, NeuronIndex nid0, NeuronIndex nid1)
{
	// Reading data although none has been written
	assert(data.size() > 0);

	if (data.size() == 1) {
		return data[0].size();
	}
	else if (nid0 - nid1 == 0) {
		return data[nid0].size();
	}
	else {
		// Make sure the data size actually is homogeneous
		auto it0 = data.begin() + nid0;
		auto it1 = data.begin() + nid1;
		if (std::find_if(it0, it1, [it0](const typename T::value_type &val) {
			    return val.size() != it0->size();
			}) == it1) {
			return it0->size();
		}
	}
	throw HomogeneousPopulationRequiredException();
}

size_t PopulationData::get_parameters_size(NeuronIndex nid0,
                                           NeuronIndex nid1) const
{
	return get_size(m_parameters, nid0, nid1);
}

size_t PopulationData::get_record_size(NeuronIndex nid0, NeuronIndex nid1) const
{
	return get_size(m_record, nid0, nid1);
}

size_t PopulationData::get_data_size(NeuronIndex nid0, NeuronIndex nid1) const
{
	return get_size(m_data, nid0, nid1);
}

const PopulationData::ParameterType &PopulationData::read_parameters(
    NeuronIndex nid0, NeuronIndex nid1) const
{
	return read(m_parameters, nid0, nid1);
}

const PopulationData::RecordType &PopulationData::read_record(
    NeuronIndex nid0, NeuronIndex nid1) const
{
	return read(m_record, nid0, nid1);
}

const PopulationData::DataType &PopulationData::read_data(
    NeuronIndex nid0, NeuronIndex nid1) const
{
	return read(m_data, nid0, nid1);
}

IterableRange<std::vector<PopulationData::ParameterType>::iterator>
PopulationData::write_parameters(NeuronIndex nid0, NeuronIndex nid1,
                                 bool partial)
{
	return write(m_parameters, nid0, nid1, m_size, partial);
}

IterableRange<std::vector<PopulationData::RecordType>::iterator>
PopulationData::write_record(NeuronIndex nid0, NeuronIndex nid1, bool partial)
{
	return write(m_record, nid0, nid1, m_size, partial);
}

IterableRange<std::vector<PopulationData::DataType>::iterator>
PopulationData::write_data(NeuronIndex nid0, NeuronIndex nid1, bool partial)
{
	return write(m_data, nid0, nid1, m_size, partial);
}

/*
 * Class PopulationDataView
 */

/**
 * Copies the data of one PopulationDataView into another.
 *
 * @param data_src is the list containing the source data vector.
 * @param nid0_src is the first index in the source data vector that should be
 * copied.
 * @param nid1_src is the last-plus-one index in the source data vector that
 * should be copied.
 * @param data_tar is the list to which the data should be written.
 * @param nid0_tar is the first index in the target data vector to which content
 * should be written.
 * @param nid1_tar is the last-plus-one index in the target data vector to which
 * content should be written.
 */
template <typename T>
void copy(const T &data_src, NeuronIndex nid0_src, NeuronIndex nid1_src,
          T &data_tar, NeuronIndex nid0_tar, NeuronIndex nid1_tar,
          size_t tar_size)
{
	// Do nothing if the data source vector is empty
	if (data_src.size() == 0) {
		return;
	}

	// Information about the source and target views
	const bool src_homogeneous =
	    data_src.size() == 1 || (nid1_src - nid0_src == 1);
	const size_t src_first = data_src.size() == 1 ? 0 : nid0_src;
	const bool tar_homogeneous =
	    data_tar.size() <= 1 || (nid1_tar - nid0_tar == 1);
	const size_t tar_first = data_tar.size() <= 1 ? 0 : nid0_tar;

	// Simplest case -- source and target are homogeneous, just copy one entry
	if (src_homogeneous) {
		if (tar_homogeneous) {
			if (data_tar.size() == 0) {
				data_tar.resize(1);
			}
			data_tar[tar_first] = data_src[src_first];
		}
		else {
			for (NeuronIndex nid = nid0_tar; nid < nid1_tar; nid++) {
				data_tar[nid] = data_src[src_first];
			}
		}
	}
	else {
		// Make sure both target and source represent the same number of
		// neurons.
		if (nid1_src - nid0_src != nid1_tar - nid0_tar) {
			// Probably the source population is homogeneous
			auto it0 = data_src.begin() + nid0_src;
			auto it1 = data_src.begin() + nid1_src;
			if (std::find_if(it0, it1,
			                 [it0](const typename T::value_type &val) {
				                 return val != *it0;
				             }) == it1) {
				// Call "copy" again, this time only copy from the first neuron
				// in the source, since we have checked for homogenity
				copy(data_src, nid0_src, nid0_src + 1, data_tar, nid0_tar,
				     nid1_tar, tar_size);
				return;
			}

			throw InvalidParameterArraySize(
			    "Target and source must both represent the same number of "
			    "neurons.");
		}

		// Heterogenise the target data
		if (tar_homogeneous) {
			const size_t old_size = data_tar.size();
			data_tar.resize(tar_size);
			if (old_size == 1) {
				std::fill(data_tar.begin() + 1, data_tar.end(), data_tar[0]);
			}
		}

		// Copy the data from the first to the second parameter vector
		for (NeuronIndex i = 0; i < nid1_src - nid0_src; i++) {
			data_tar[nid0_tar + i] = data_src[nid0_src + i];
		}
	}
}

PopulationDataView::PopulationDataView(const PopulationDataView &other)
{
	// Create an own population data instance, normalise the neuron ids
	m_data = std::make_shared<PopulationData>();
	m_nid0 = 0;
	m_nid1 = other.m_nid1 - other.m_nid0;
	m_own_parameters = other.m_own_parameters;
	m_own_record = other.m_own_record;
	m_own_data = other.m_own_data;

	// Use the assignment operator to copy the data from the other instance into
	// this one
	*this = other;
}

PopulationDataView &PopulationDataView::operator=(
    const PopulationDataView &other)
{
	const size_t size = m_nid1 - m_nid0;
	if (m_own_parameters && other.m_own_parameters) {
		copy(other.m_data->parameters(), other.m_nid0, other.m_nid1,
			 m_data->parameters(), m_nid0, m_nid1, size);
	}
	if (m_own_record && other.m_own_record) {
	copy(other.m_data->record(), other.m_nid0, other.m_nid1, m_data->record(),
	     m_nid0, m_nid1, size);
	}
	if (m_own_data && other.m_own_data) {
		copy(other.m_data->data(), other.m_nid0, other.m_nid1, m_data->data(),
	     m_nid0, m_nid1, size);
	}
	return *this;
}
}
