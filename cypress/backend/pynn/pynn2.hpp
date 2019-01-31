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

#ifndef CYPRESS_BACKEND_PYNN2_HPP
#define CYPRESS_BACKEND_PYNN2_HPP

// Include first to avoid "_POSIX_C_SOURCE redefined" warning
#include <pybind11/pybind11.h>

#include <pybind11/embed.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <cypress/backend/pynn/pynn.hpp>
#include <cypress/core/backend.hpp>
#include <cypress/core/network_base_objects.hpp>
#include <cypress/util/json.hpp>

namespace cypress {
namespace py = pybind11;

/**
 * The Python interpreter is not completely isolated: importing numpy in 2
 * consecutive runs will lead to a Segmentation fault (compare
 * https://github.com/numpy/numpy/issues/3837). Thus, all simulations have to be
 * executed in a single interpreter. This class makes sure that the python
 * interpreter is started and is kept alive, as long as on instance of the
 * object exists. When the last instance is destroyed, the interpreter will be
 * safely shutdown before reaching the end of main, preventing a segfault at the
 * end of the application.
 *
 * Note: Sub-interpreters and also this approach in general are not thread-safe.
 * Instead of parallelizing simulations, you should use several threads in e.g.
 * NEST (call pynn.nest='{"threads": 8}'), or combine several networks into one.
 *
 */
class PythonInstance {
private:
	PythonInstance();

public:
	PythonInstance(PythonInstance const &) = delete;
	void operator=(PythonInstance const &) = delete;
	static PythonInstance &instance()
	{
		static PythonInstance instance;
		return instance;
	}

private:
	~PythonInstance();
};

/**
 * The Backend class is an abstract base class which provides the facilities
 * for network execution.
 */
class PyNN_ : public PyNN {
private:
	void do_run(NetworkBase &network, Real duration) const override;

public:

	/**
	 * Constructor of the PyNN backend. Throws an exception if the given PyNN
	 * backend does not exist.
	 *
	 * @param simulator is the name of the simulator backend to be used by PyNN.
	 * Use the static backends method to list available backends.
	 * @param setup contains additional setup information that should be passed
	 * to the backend.
	 */
	PyNN_(const std::string &simulator, const Json &setup = Json());

	/**
	 * Destructor of the PyNN class.
	 */
	~PyNN_() override;

	/**
	 * Returns the PyNN version, 8 represents version 0.8.x
	 *
	 * @return integer representing the pyNN version
	 */
	static int get_pynn_version();

	/**
	 * Returns the neo version, 5 represents version 0.5.x
	 *
	 * @return integer representing the neo version
	 */
	static int get_neo_version();

	/**
	 * Converting a Json object to a py::dict. Make sure that the python
	 * interpreter is already started before calling this function
	 *
	 * @param json The json object which will be converted
	 * @return python dictionary
	 */
	static py::dict json_to_dict(Json json);

	/**
	 * Given a NeuronType, this function returns the name of the PyNN
	 * neuron_type
	 *
	 * @param neuron_type Reference to the NeuronType of the population
	 * @return PyNN class name of the neuron type
	 */
	static std::string get_neuron_class(const NeuronType &neuron_type);

	/**
	 * Intitialize the logger, still TODO
	 *
	 */
	static void init_logger();

	/**
	 * Creates a PyNN source population
	 *
	 * @param pop The cypress population
	 * @param pynn the PyNN instance
	 * @return handler for the PyNN population
	 */
	static py::object create_source_population(const PopulationBase &pop,
	                                           py::module &pynn);
	/**
	 * Creates a PyNN population with homogeneous parameters, inhomogeneous
	 * parameters can be set with set_inhomogeneous_parameters afterwards.
	 *
	 * @param pop The cypress population
	 * @param pynn the PyNN instance
	 * @return handler for the PyNN population
	 */
	static py::object create_homogeneous_pop(const PopulationBase &pop,
	                                         py::module &pynn,
	                                         bool &init_available);

	/**
	 * Sets parameters of an existing population
	 *
	 * @param pop Source cypress population
	 * @param pypop Target PyNN population
	 * @param init_available flag for initializing the population
	 */
	static void set_inhomogeneous_parameters(const PopulationBase &pop,
	                                         py::object &pypop,
	                                         bool init_available = false);

	/**
	 * Set recording for a full population
	 *
	 * @param pop Source cypress population
	 * @param pypop Target PyNN population
	 */
	static void set_homogeneous_rec(const PopulationBase &pop,
	                                py::object &pypop);

	/**
	 * Set mixed recordings for a population
	 *
	 * @param pop Source cypress population
	 * @param pypop Target PyNN population
	 */
	static void set_inhomogenous_rec(const PopulationBase &pop,
	                                 py::object &pypop, py::module &pynn);

	/**
	 * Get a handler for a part of a population
	 *
	 * @param pynn handler for the PyNN instance
	 * @param py_pop PyNN population
	 * @param c_pop Cypress population
	 * @param start id of the first neuron
	 * @param end id of the last neuron
	 * @return handler for PopulationView object
	 */
	static py::object get_pop_view(const py::module &pynn,
	                               const py::object &py_pop,
	                               const PopulationBase &c_pop,
	                               const size_t &start, const size_t &end);

	/**
	 * Creates a PyNN 0.7 Connector
	 *
	 * @param connector_name name of the pyNN connector
	 * @param group_conn Group connection description
	 * @param pynn Reference to the pynn module
	 * @return py::object reference to the connector
	 */
	static py::object get_connector7(const ConnectionDescriptor &conn,
	                                 const py::module &pynn);

	/**
	 * Creates a PyNN 0.9 Connector
	 *
	 * @param connector_name name of the pyNN connector
	 * @param pynn Reference to the pynn module
	 * @param additional_parameter Additional parameter
	 * needed by the connector
	 * @param allow_self_connections flag to allow self connections if source
	 * and target populations are the same
	 * @return py::object reference to the connector
	 */
	static py::object get_connector(const std::string &connector_name,
	                                const py::module &pynn,
	                                const Real &additional_parameter,
	                                const bool allow_self_connections);

	/**
	 * Connect based on an existing PyNN connector
	 *
	 * @param populations vector of cypress populations
	 * @param pypopulations vector of PyNN populations
	 * @param conn Cypress connection descriptor
	 * @param pynn Handler for PyNN module
	 * @param timestep timestep of the simulator
	 * @return Handler for the connector
	 */
	static py::object group_connect(
	    const std::vector<PopulationBase> &populations,
	    const std::vector<py::object> &pypopulations,
	    const ConnectionDescriptor &conn, const py::module &pynn,
	    const Real timestep = 0.0);

	/**
	 * Connect based on an existing PyNN connector
	 *
	 * @param populations vector of cypress populations
	 * @param pypopulations vector of PyNN populations
	 * @param conn Cypress connection descriptor
	 * @param pynn Handler for PyNN module
	 * @return Handler for the connector
	 */
	static py::object group_connect7(
	    const std::vector<PopulationBase> &populations,
	    const std::vector<py::object> &pypopulations,
	    const ConnectionDescriptor &conn, const py::module &pynn);

	/**
	 * Creates a PyNN FromList Connection
	 *
	 * @param pypopulations list of python populations
	 * @param conn ConnectionDescriptor of the List connection
	 * @param pynn Handler for PyNN python module
	 * @param timestep Timestep of the simulator, default 0
	 * @return tuple of the excitatory,inhibitory connection. List is separated
	 * for compatibility reasons
	 */
	static std::tuple<py::object, py::object> list_connect(
	    const std::vector<py::object> &pypopulations,
	    const ConnectionDescriptor conn, const py::module &pynn,
	    const Real timestep = 0.0);

	/**
	 * Creates a PyNN6/7 FromList Connection
	 *
	 * @param pypopulations list of python populations
	 * @param conn ConnectionDescriptor of the List connection
	 * @param pynn Handler for PyNN python module
	 * @return tuple of the excitatory,inhibitory connection. List is separated
	 * for compatibility reasons
	 */
	static std::tuple<py::object, py::object> list_connect7(
	    const std::vector<py::object> &pypopulations,
	    const ConnectionDescriptor conn, const py::module &pynn);

	template <typename T>
	/**
	 * Given a numpy object, this method creates the (transposed) C++ matrix
	 * without creating a copy. The python dtype is checked and compared to
	 * @param T. Matrix sizes are caught in Python, because the py::buffer_info
	 * seems to be misleading in some cases.
	 *
	 * @param T Type of the matrix
	 * @param object handler for numpy array
	 * @param transposed matrix dimensions are swapped (hack for NEO)
	 * @return C++ matrix of the numpy array
	 */
	static Matrix<T> matrix_from_numpy(const py::object &object,
	                                   bool transposed = false);

	/**
	 * Fetch all data (spikes, traces) from nest. This is faster than using NEO.
	 *
	 * @param populations Cypress populations
	 * @param pypopulations PyNN populations
	 */
	static void fetch_data_nest(const std::vector<PopulationBase> &populations,
	                            const std::vector<py::object> &pypopulations);

	/**
	 * Fetch all data (spikes, traces) from spiNNaker. This is faster than using
	 * NEO.
	 *
	 * @param populations Cypress populations
	 * @param pypopulations PyNN populations
	 */
	static void fetch_data_spinnaker(
	    const std::vector<PopulationBase> &populations,
	    const std::vector<py::object> &pypopulations);

	/**
	 * Fetch all data (spikes, traces) from NEO. This is a general method
	 *
	 * @param populations Cypress populations
	 * @param pypopulations PyNN populations
	 */
	static void fetch_data_neo(const std::vector<PopulationBase> &populations,
	                           const std::vector<py::object> &pypopulations);

	// ______________ Spikey related part _______________________

	/**
	 * Special run function for the Spikey system
	 *
	 * @param source The Network description
	 * @param duration Simulation duration
	 * @param pynn Module pointer to PyNN.hardware.spikey
	 */
	static void spikey_run(NetworkBase &source, Real duration, py::module &pynn,
	                       py::dict dict);

	/**
	 * Creates a PyNN source population for Spikey
	 *
	 * @param pop The cypress population
	 * @param pynn the PyNN instance
	 * @return handler for the PyNN population
	 */
	static py::object spikey_create_source_population(const PopulationBase &pop,
	                                                  py::module &pynn);

	/**
	 * Creates a PyNN population with homogeneous parameters for Spikey,
	 * inhomogeneous parameters can be set with set_inhomogeneous_parameters
	 * afterwards.
	 *
	 * @param pop The cypress population
	 * @param pynn the PyNN instance
	 * @return handler for the PyNN population
	 */
	static py::object spikey_create_homogeneous_pop(const PopulationBase &pop,
	                                                py::module &pynn);

	/**
	 * Sets parameters of an existing population for spikey
	 *
	 * @param pop Source cypress population
	 * @param pypop Target PyNN population
	 * @param init_available flag for initializing the population
	 */
	static void spikey_set_homogeneous_rec(const PopulationBase &pop,
	                                       py::object &pypop, py::module &pynn);

	/**
	 * Set recording for a full population for spikey
	 *
	 * @param pop Source cypress population
	 * @param pypop Target PyNN population
	 */
	static void spikey_set_inhomogeneous_rec(const PopulationBase &pop,
	                                         py::object &pypop,
	                                         py::module pynn);

	/**
	 * Set mixed recordings for a population for the Spikey system
	 *
	 * @param pop Source cypress population
	 * @param pypop Target PyNN population
	 */
	static void spikey_set_inhomogeneous_parameters(const PopulationBase &pop,
	                                                py::object &pypop);

	/**
	 * Fetches spikes and saves them to pop
	 *
	 * @param pop cypress population
	 * @param pypop PyNN population
	 */

	static void spikey_get_spikes(PopulationBase pop, py::object &pypop);
	/**
	 * Fetch membrane voltage for a neuron in Spikey
	 *
	 * @param neuron The cypress neuron recording the membrane
	 * @param pynn pointer to PyNN module
	 */
	static void spikey_get_voltage(NeuronBase neuron, py::module &pynn);
};
}  // namespace cypress

#endif /* CYPRESS_BACKEND_PYNN_HPP */
