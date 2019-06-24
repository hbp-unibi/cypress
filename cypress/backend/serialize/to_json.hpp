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

#include <cypress/core/backend.hpp>
#include <cypress/core/network.hpp>
#include <cypress/core/network_base_objects.hpp>
#include <cypress/util/json.hpp>

#include <thread>

namespace cypress {

/**
 * The Backend class is an abstract base class which provides the facilities
 * for network execution.
 */
class ToJson : public Backend {
protected:
	std::string m_simulator;
	Json m_setup;
	bool m_save_json = false;
	void do_run(NetworkBase &network, Real duration) const override;
    std::string m_path, m_json_path;

    Json output_json(NetworkBase &network, Real duration) const;
	void read_json(Json& result, NetworkBase &network) const;
public:
	/**
	 * Constructor of the ToJson backend. If setup sets "save_json" : true, all
	 * simulation related data will be written to HDD
	 *
	 * @param simulator is the name of the simulator backend to be used
	 * @param setup contains additional setup information that should be passed
	 * to the backend.
	 */
	ToJson(const std::string &simulator, const Json &setup = Json());

	/**
	 * Destructor of the ToJson class.
	 */
	~ToJson() override;

	std::unordered_set<const NeuronType *> supported_neuron_types()
	    const override;

	/**
	 * Returns the canonical name of the backend.
	 */
	std::string name() const override;

	/**
	 * @brief returns a vector encoding recording flags for a single recording
	 * signal and inhomogeneous record flags
	 *
	 * @param pop Source Population
	 * @param index Recording signal index
	 * @return std::vector< bool > vector encoding record flags
	 */
	static std::vector<bool> inhom_rec_single(const PopulationBase &pop,
	                                          size_t index);

	/**
	 * @brief returns a json encoding all records for a single population.
	 * Json[<signal>] will return a vector of bool flags encoding, wheter
	 * <signal> will be recorded for that specific neuron
	 *
	 * @param pop source population object
	 * @return cypress::Json json object containing above information
	 */
	static Json inhom_rec_to_json(const PopulationBase &pop);

	/**
	 * @brief Converts the information of a ConnectionDescriptor to a JSON
	 * object. List connectors and dynamic connection strategies that cannot be
	 * reproduced by a new simulation will be expanded to list connections
	 *
	 * @param conn source connections description
	 * @return cypress::Json json object containing above information
	 */
	static Json connector_to_json(const ConnectionDescriptor &conn);

	/**
	 * @brief Creates a Json containing information about a population
	 *
	 * @param pop source population
	 * @return cypress::Json target JSon
	 */
	static Json pop_to_json(const PopulationBase &pop);

	/**
	 * @brief Takes a Json object created by pop_to_json and adds information
	 * about record flags, assumes record flags are homogeneous
	 *
	 * @param pop source population object
	 * @param json target json created by ToJson::pop_to_json
	 */
	static void hom_rec_to_json(const PopulationBase &pop, Json &json);

	/**
	 * @brief Creates an array of populations in return json, which contains all
	 * information of all populations, including record flags and parameters
	 *
	 * @param pops vector of population objects, typically netw.populations()
	 * @return cypress::Json Json object containing all information
	 */
	static Json pop_vec_to_json(const std::vector<PopulationBase> &pops);

	/**
	 * @brief Converts all recorded data of a population to json
	 *
	 * @param pop source population for recorded data
	 * @return cypress::Json target Json containing recorded data
	 */
	static Json recs_to_json(const PopulationBase &pop);

	/**
	 * @brief Reads recorded signal from a json object, puts data into network
	 * object
	 *
	 * @param pop_data source simulation data, should be on level of a single
	 * recording
	 * @param netw target network
	 */
	static void read_recordings_from_json(const Json &pop_data,
	                                      NetworkBase &netw);

	/**
	 * @brief Uses data in JSon to create populations and set record flags
	 *
	 * @param pop_json contains population data
	 * @param netw network object in which the population will be created
	 */
	static void create_pop_from_json(const Json &pop_json, Network &netw);

	/**
	 * @brief Create the synapse for a connection dependent on the name.
	 *
	 * @param name name of the synapse model
	 * @param parameters Synapse Parameters, should be in the right order
	 * @return std::shared_ptr< cypress::SynapseBase > pointer to synapse object
	 */
	static std::shared_ptr<SynapseBase> get_synapse(
	    const std::string &name, const std::vector<Real> &parameters);

	/**
	 * @brief Uses the information in JSON to create a single Connection.
	 *
	 * @param con_json JSON object containing the Connection data
	 * @param netw target network of the connection
	 */
	static void create_conn_from_json(const Json &con_json, Network &netw);

	/**
	 * @brief Uses the data in a JSON object to create a full network
	 *
	 * @param path path of the JSON file
	 * @return cypress::NetworkBase the newly constructed network
	 */
	static NetworkBase network_from_json(std::string path);

	static void learned_weights_from_json(const Json &json, NetworkBase netw);
};

/**
 * @brief Automatic conversion of a Network object to JSON. Now you can uses
 * Json(network) as you would do with STL containers
 *
 * @param j target JSon object
 * @param netw source network object
 */
void to_json(Json &j, const Network &netw);

/**
 * @brief With the method you can just use Network netw = json.get<Network>() as
 * you can do with STL containers.
 *
 * @param j source json object
 * @param netw target network, in which the network will be constructed. NOTE:
 * network will not be deleted!
 */
void from_json(const Json &j, Network &netw);

/**
 * @brief Read in NetworkRuntime from json: json["sim"] == runtime.sim;
 *
 * @param json source json
 * @param runtime target runtime.
 */
void from_json(const Json &json, NetworkRuntime &runtime);

/**
 * @brief NetworkRuntime to json conversion: json["sim"] == runtime.sim;
 *
 * @param json target json object
 * @param runtime source NetworkRuntime object
 */
void to_json(Json &json, const NetworkRuntime &runtime);

template <typename T>
/**
 * @brief Automatic conversion of the internal matrix type to json. Use
 * Json(mat).
 *
 * @param T Template parameter of the matrix
 * @param j target json
 * @param mat source matrix
 */
void to_json(Json &j, const Matrix<T> &mat)
{
	/*if (mat.cols() == 1) {
	    std::vector<Real> rows(mat.rows());
	    for (size_t k = 0; k < mat.rows(); k++) {
	        rows[k] = mat(k, 0);
	    }
	    j.push_back(rows);
	}
	else {*/
	for (size_t i = 0; i < mat.rows(); i++) {
		std::vector<Real> rows(mat.cols());
		for (size_t k = 0; k < mat.cols(); k++) {
			rows[k] = mat(i, k);
		}
		j.push_back(rows);
	}
	//}

	// Maybe transpose // TODO
	// std::cout << j<<std::endl;
}

template <typename T>
/**
 * @brief Conversion from json to internal matrix type. Use json.get<Matrix<T>>
 * as you would do with a STL container.
 *
 * @param T matrix type
 * @param j source json
 * @param mat target matrix
 */
void from_json(const Json &j, Matrix<T> &mat)
{
	size_t rows = j.size();
	if (rows == 0) {
		return;
	}
	size_t cols = j[0].size();
	mat = Matrix<T>(rows, cols);
	for (size_t i = 0; i < rows; i++) {
		for (size_t k = 0; k < cols; k++) {
			mat(i, k) = T(j[i][k]);
		}
	}
}

}  // namespace cypress
