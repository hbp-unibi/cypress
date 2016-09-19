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
 * @file transformation.hpp
 *
 * Contains the general transformation interface -- transformations are
 * responsible for expressing a certain aspect of a network graph in another
 * way. For example, the IfFacetsHardware1 neuron type is a subset of the
 * IfCondExp neuron type, yet not supported by backends other than Spikey. A
 * corresponding transformation takes care of transforming this neuron type into
 * the IfCondExp neuron type for simulators which do not support it.
 *
 * If transformations change the network topology, they must be capable of
 * writing recorded data back into the original network graph.
 *
 * @author Andreas Stöckel
 */

#ifndef CYPRESS_CORE_TRANSFORMATION_HPP
#define CYPRESS_CORE_TRANSFORMATION_HPP

#include <functional>
#include <memory>
#include <typeinfo>
#include <unordered_set>
#include <vector>

#include <cypress/core/types.hpp>

namespace cypress {

// Forward declarations
class NeuronType;
class NetworkBase;
class Backend;

template <typename T>
class Population;

/**
 * Structure describing the
 */
struct TransformationProperties {
	/**
	 * A non-zero cost value which can be used to indicate the cost associated
	 * with executing this transformation.
	 */
	size_t cost = 100;

	/**
	 * If true indicates that the transformation cannot be transformed without
	 * loosing informatoion. Usage of these transformations can be disabled
	 * globally.
	 */
	bool lossy = false;
};

/**
 * Auxiliary data which may be affected by the transformation.
 */
struct TransformationAuxData {
	/**
	 * Contains the network simulation duration. Certain transformations may
	 * choose to extend the actual simulation duration.
	 */
	Real duration;
};

/**
 * Virtual base class from which all Transformations should be derived. The
 * transformation class is as general as possible, just taking a NetworkBase
 * as input and returning a new NetworkBase instance.
 */
class Transformation {
protected:
	/**
	 * Function which should be overriden by child classes. This function should
	 * perform the actual transformation an remember which changes were done.
	 *
	 * @param src is the network which should be modified.
	 * @return is a new Network instance which contains the transformed data.
	 */
	virtual NetworkBase do_transform(const NetworkBase &src,
	                                 TransformationAuxData &aux) = 0;

	/**
	 * Function called after the network was executed. This function should copy
	 * recorded data from the actually executed network to the target network.
	 * This function must be overriden if the network topology (including the
	 * neuron type) was changed by the transformation. Otherwise a default
	 * implementation is used.
	 */
	virtual void do_copy_results(const NetworkBase &src,
	                             NetworkBase &tar) const;

public:
	/**
	 * Returns a structure describing the transformation, e.g. whether it is
	 * lossy or the priority with which it should be applied.
	 *
	 * @return an instance of Transformation properties which contains
	 * information about this particular transformation.
	 */
	virtual TransformationProperties properties() const
	{
		return TransformationProperties{};
	}

	/**
	 * Returns an id which can for example be used to explicitly disable certain
	 * transformations.
	 *
	 * @return a unique ID describing this transformation.
	 */
	virtual std::string id() const = 0;

	/**
	 * Performs the transformation on the given network instance.
	 *
	 * @param src is the network which should be modified.
	 * @param aux is some auxiliary data which may be modified by the
	 * transformation.
	 * @return is a new Network instance which contains the transformed data.
	 */
	NetworkBase transform(const NetworkBase &src, TransformationAuxData &aux);

	/**
	 * Copys the results from a transformed network to the original network.
	 */
	void copy_results(const NetworkBase &src, NetworkBase &tar) const;

	virtual ~Transformation(){};
};

/**
 * Constructor function type which constructs a new Transformation instance.
 */
using TransformationCtor = std::function<std::unique_ptr<Transformation>()>;

/**
 * Test function type which is used to test whether a certain transformation
 * should be applied to a Network/Backend combination.
 */
using TransformationTest =
    std::function<bool(const Backend &, const NetworkBase &)>;

/**
 * Stub type which is used to allow the registration of new transformations as
 * part of a static value initialization.
 */
using RegisteredTransformation = size_t;

/**
 * The Transformations class is responsible for holding a list of currently
 * supported transformations and to apply transformations in such a way that the
 * network can be executed on the given backend.
 */
class Transformations {
public:
	/**
	 * Function used to find a series of transfomations which unsupported neuron
	 * types into supported neuron types. Throws an exception if no path is
	 * found.
	 */
	static std::vector<TransformationCtor>
	construct_neuron_type_transformation_chain(
	    const std::vector<const NeuronType *> &unsupported_types,
	    const std::unordered_set<const NeuronType *> &supported_types,
	    const std::vector<std::tuple<TransformationCtor, const NeuronType *,
	                                 const NeuronType *>> &transformations,
	    bool use_lossy);

	/**
	 * Method which transforms the given network in such a way, that it can be
	 * executed by the given backend.
	 *
	 * @param network is the network instance that is the subject of the
	 * transformation.
	 * @param backend is the backend on which the network should be executed.
	 * @param aux is auxiliary data that should be passed to the run() method
	 * of the backend, such as the network execution time.
	 * @param disabled_trafo_ids is a set of transformation identifiers
	 * specifying transformations which should not be executed.
	 * @param use_lossy if true allows the use of so called "lossy"
	 * transformations. These transformations may result in a network which is
	 * not equivalent to the network the user described.
	 */
	static void run(const Backend &backend, NetworkBase network,
	                const TransformationAuxData &aux,
	                std::unordered_set<std::string> disabled_trafo_ids =
	                    std::unordered_set<std::string>(),
	                bool use_lossy = true);

	/**
	 * Registers a general transformation which should be executed if an
	 * arbitrary test condition regarding the backend and the network is
	 * fulfilled.
	 *
	 * @param ctor is the constructor that should be called in order to create a
	 * new instance of the transformation in question.
	 * @param test is a callback function which is called whenever the
	 * Transformations::execute() function is called. The test function must
	 * check whether it is relevant for the given backend and network in
	 * question. If yes, it should return true and false if the transformation
	 * should not be used.
	 * @return RegisteredTransformation, an integer specifying how many
	 * transformations have already been registered. This is mostly done to
	 * allow register transformations as part of a value initialization once
	 * the program is loaded.
	 */
	static RegisteredTransformation register_general_transformation(
	    TransformationCtor ctor, TransformationTest test);

	/**
	 * Registers a new transformation which transforms from one neuron type to
	 * another neuron type.
	 *
	 * @param ctor is the constructor that should be called in order to create a
	 * new instance of the transformation in question.
	 * @param src is the source neuron type the transformation allows to
	 * transform form.
	 * @param tar is the target neuron type the transformation transforms to.
	 */
	static RegisteredTransformation register_neuron_type_transformation(
	    TransformationCtor ctor, const NeuronType &src, const NeuronType &tar);

	template <typename SourceType, typename TargetType>
	static RegisteredTransformation register_neuron_type_transformation(
	    TransformationCtor ctor)
	{
		return register_neuron_type_transformation(ctor, SourceType::inst(),
		                                           TargetType::inst());
	}
};
}

#endif /* CYPRESS_CORE_TRANSFORMATION_HPP */
