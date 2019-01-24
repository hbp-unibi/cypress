
/*
 *  Cypress -- C++ Spiking Neural Network Simulation Framework
 *  Copyright (C) 2018 Christoph Jenzen
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
#include <cypress/cypress.hpp>

#include <cypress/backend/pynn/pynn2.cpp>

#include "gtest/gtest.h"

#include <sstream>

namespace cypress {

TEST(pynn2, normalisation)
{
	EXPECT_EQ("NEST", PyNN_("NEST").simulator());
	EXPECT_EQ("nest", PyNN_("NEST").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.NEST", "pyNN.nest"}),
	          PyNN_("NEST").imports());
	EXPECT_EQ("", PyNN_("NEST").nmpi_platform());

	EXPECT_EQ("pynn.NEST", PyNN_("pynn.NEST").simulator());
	EXPECT_EQ("nest", PyNN_("pynn.NEST").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.NEST", "pyNN.nest"}),
	          PyNN_("pynn.NEST").imports());
	EXPECT_EQ("", PyNN_("pynn.NEST").nmpi_platform());

	EXPECT_EQ("spinnaker", PyNN_("spinnaker").simulator());
	EXPECT_EQ("nmmc1", PyNN_("spinnaker").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.spinnaker", "pyNN.spiNNaker"}),
	          PyNN_("spinnaker").imports());
	EXPECT_EQ("SpiNNaker", PyNN_("spinnaker").nmpi_platform());

	EXPECT_EQ("spiNNaker", PyNN_("spiNNaker").simulator());
	EXPECT_EQ("nmmc1", PyNN_("spiNNaker").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.spiNNaker"}),
	          PyNN_("spiNNaker").imports());
	EXPECT_EQ("SpiNNaker", PyNN_("spiNNaker").nmpi_platform());

	EXPECT_EQ("pyhmf", PyNN_("pyhmf").simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.pyhmf", "pyhmf"}),
	          PyNN_("pyhmf").imports());

	EXPECT_EQ("Spikey", PyNN_("Spikey").simulator());
	EXPECT_EQ("spikey", PyNN_("Spikey").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.Spikey", "pyNN.hardware.spikey"}),
	          PyNN_("Spikey").imports());
	EXPECT_EQ("Spikey", PyNN_("Spikey").nmpi_platform());

	EXPECT_EQ("ess", PyNN_("ess").normalised_simulator());
	EXPECT_EQ(std::vector<std::string>({"pyNN.ess", "pyhmf"}),
	          PyNN_("ess").imports());
	EXPECT_EQ("BrainScaleS-ESS", PyNN_("ess").nmpi_platform());
}

TEST(pynn2, timestep)
{
	EXPECT_FLOAT_EQ(42.0, PyNN_("bla", {{"timestep", 42.0}}).timestep());
	EXPECT_FLOAT_EQ(0.1, PyNN_("nest").timestep());
	EXPECT_FLOAT_EQ(1.0, PyNN_("nmmc1").timestep());
	EXPECT_FLOAT_EQ(0.0, PyNN_("nmpm1").timestep());
	EXPECT_FLOAT_EQ(0.0, PyNN_("spikey").timestep());
}
// auto python_instance = PythonInstance();
std::vector<std::string> all_avail_imports;
std::vector<std::string> simulators;
int version = 0;
int neo_version = 0;

TEST(pynn2, get_import)
{
	for (const std::string sim : PyNN_::simulators()) {
		try {
			auto res = PYNN_UTIL.lookup_simulator(sim);
			auto normalised_simulator = res.first;
			auto imports = res.second;
			all_avail_imports.push_back(PyNN_::get_import(imports, sim));
			simulators.push_back(sim);
		}
		catch (...) {
		};
	}
	if (all_avail_imports.size() == 0) {
		std::cerr << "No imports were found, testing of PyNN backend disabled"
		          << std::endl;
	}
}

TEST(pynn2, get_pynn_version)
{
	if (all_avail_imports.size() != 0) {
		EXPECT_NO_THROW(PyNN_::get_pynn_version());
		version = PyNN_::get_pynn_version();
		EXPECT_TRUE((version == 6) || (version == 8) || (version == 9));

		py::module pynn = py::module::import("pyNN");
		std::string backup = py::cast<std::string>(pynn.attr("__version__"));
		pynn.attr("__version__") = "0.5.2";
		EXPECT_ANY_THROW(PyNN_::get_pynn_version());

		pynn.attr("__version__") = "0.10";
		EXPECT_ANY_THROW(PyNN_::get_pynn_version());

		pynn.attr("__version__") = "1.0";
		EXPECT_ANY_THROW(PyNN_::get_pynn_version());

		pynn.attr("__version__") = "0.0";
		EXPECT_ANY_THROW(PyNN_::get_pynn_version());

		pynn.attr("__version__") = "0.8.2";
		EXPECT_NO_THROW(PyNN_::get_pynn_version());
		EXPECT_EQ(8, PyNN_::get_pynn_version());

		pynn.attr("__version__") = "0.9.5";
		EXPECT_NO_THROW(PyNN_::get_pynn_version());
		EXPECT_EQ(9, PyNN_::get_pynn_version());

		pynn.attr("__version__") = backup;
	}
	else {
		try {
			py::module pynn = py::module::import("pyNN");
			EXPECT_NO_THROW(PyNN_::get_pynn_version());
		}
		catch (...) {
			EXPECT_ANY_THROW(PyNN_::get_pynn_version());
			std::cout << " ... Skipping test" << std::endl;
		}
	}
}

TEST(pynn2, get_neo_version)
{
	if (all_avail_imports.size() != 0 && (version >= 8)) {
		EXPECT_NO_THROW(PyNN_::get_neo_version());
		neo_version = PyNN_::get_neo_version();

		py::module neo = py::module::import("neo");
		std::string backup = py::cast<std::string>(neo.attr("__version__"));
		neo.attr("__version__") = "0.5.2";
		EXPECT_NO_THROW(PyNN_::get_neo_version());
		EXPECT_EQ(5, PyNN_::get_neo_version());

		neo.attr("__version__") = "0.4.1";
		EXPECT_NO_THROW(PyNN_::get_neo_version());
		EXPECT_EQ(4, PyNN_::get_neo_version());

		neo.attr("__version__") = "0.10";
		EXPECT_ANY_THROW(PyNN_::get_neo_version());

		neo.attr("__version__") = "1.0";
		EXPECT_ANY_THROW(PyNN_::get_neo_version());

		neo.attr("__version__") = "0.0";
		EXPECT_ANY_THROW(PyNN_::get_neo_version());

		neo.attr("__version__") = backup;
	}
	else if (all_avail_imports.size() != 0 && (version < 8)) {
		EXPECT_NO_THROW(PyNN_::get_neo_version());
		EXPECT_EQ(0, PyNN_::get_neo_version());
	}
	else {
		EXPECT_ANY_THROW(PyNN_::get_neo_version());
	}
}

TEST(pynn2, json_to_dict)
{
	Json o1{{"key1", {1, 2, 3, 4}},
	        {"key2",
	         {
	             {"key21", 1},
	             {"key22", 2},
	             {"key23", 3},
	         }},
	        {"key3", "Hello World"}};

	Json o2{{"key2",
	         {
	             {"key23", 4},
	             {"key24",
	              {
	                  {"key241", 1},
	                  {"key242", 2},
	              }},
	         }},
	        {"key3", "Hello World2"}};
	py::dict o1_py = PyNN_::json_to_dict(o1);
	py::dict o2_py = PyNN_::json_to_dict(o2);

	auto array = py::list(o1_py["key1"]);
	for (size_t i = 0; i < o1["key1"].size(); i++) {
		EXPECT_EQ(int(o1["key1"][i]), py::cast<int>(array[i]));
	}

	auto dict = py::dict(o1_py["key2"]);
	EXPECT_EQ(int(o1["key2"]["key21"]), py::cast<int>(dict["key21"]));
	EXPECT_EQ(int(o1["key2"]["key22"]), py::cast<int>(dict["key22"]));
	EXPECT_EQ(int(o1["key2"]["key23"]), py::cast<int>(dict["key23"]));

	EXPECT_EQ(o1["key3"], py::cast<std::string>(o1_py["key3"]));
	dict = py::dict(o2_py["key2"]);
	auto dict2 = py::dict(dict["key24"]);
	EXPECT_EQ(int(o2["key2"]["key23"]), py::cast<int>(dict["key23"]));
	EXPECT_EQ(int(o2["key2"]["key24"]["key241"]),
	          py::cast<int>(dict2["key241"]));
	EXPECT_EQ(int(o2["key2"]["key24"]["key242"]),
	          py::cast<int>(dict2["key242"]));
	EXPECT_EQ(o2["key3"], py::cast<std::string>(o2_py["key3"]));
}

TEST(pynn2, get_neuron_class)
{
	EXPECT_EQ("IF_cond_exp",
	          PyNN_::get_neuron_class(cypress::IfCondExp::inst()));
	EXPECT_EQ("IF_facets_hardware1",
	          PyNN_::get_neuron_class(cypress::IfFacetsHardware1::inst()));
	EXPECT_EQ("EIF_cond_exp_isfa_ista",
	          PyNN_::get_neuron_class(cypress::EifCondExpIsfaIsta::inst()));
	EXPECT_EQ("SpikeSourceArray",
	          PyNN_::get_neuron_class(cypress::SpikeSourceArray::inst()));
	EXPECT_ANY_THROW(
	    PyNN_::get_neuron_class(cypress::SpikeSourcePoisson::inst()));

	for (auto import : all_avail_imports) {
		if (import == "pyNN.nest") {
			py::module pynn = py::module::import(import.c_str());
			EXPECT_NO_THROW(pynn.attr(
			    PyNN_::get_neuron_class(cypress::IfCondExp::inst()).c_str())());
			EXPECT_NO_THROW(pynn.attr(
			    PyNN_::get_neuron_class(cypress::IfFacetsHardware1::inst())
			        .c_str())());
			EXPECT_NO_THROW(pynn.attr(
			    PyNN_::get_neuron_class(cypress::EifCondExpIsfaIsta::inst())
			        .c_str())());
			EXPECT_NO_THROW(pynn.attr(
			    PyNN_::get_neuron_class(cypress::SpikeSourceArray::inst())
			        .c_str())());
		}
		else if (import == "pyNN.spiNNaker") {
			py::module pynn = py::module::import(import.c_str());
			EXPECT_NO_THROW(pynn.attr(
			    PyNN_::get_neuron_class(cypress::IfCondExp::inst()).c_str())());
		}
		else if (import == "pyNN.hardware.spikey") {
			py::module pynn = py::module::import(import.c_str());
			EXPECT_NO_THROW(pynn.attr(
			    PyNN_::get_neuron_class(cypress::IfFacetsHardware1::inst())
			        .c_str()));
		}
		// TODO BRAINSCALES
	}
	if (all_avail_imports.size() == 0) {
		std::cout << " ... Skipping test" << std::endl;
	}
}

TEST(pynn2, create_source_population)
{
	if (all_avail_imports.size() == 0) {
		std::cout << " ... Skipping test" << std::endl;
		return;
	}
	Network netw;
	auto pop1 = Population<SpikeSourceArray>(
	    netw, 1, SpikeSourceArrayParameters().spike_times({3, 4, 5}));
	auto pop2 = Population<SpikeSourceArray>(
	    netw, 1, SpikeSourceArrayParameters().spike_times({}));
	auto pop3 = Population<SpikeSourceArray>(
	    netw, 2, SpikeSourceArrayParameters().spike_times({9}));
	auto pop4 = Population<SpikeSourceArray>(
	    netw, 1,
	    SpikeSourceArrayParameters().spike_times({11, 12, 13, 14, 15}));

	auto pop5 = Population<SpikeSourceArray>(
	    netw, 8, SpikeSourceArrayParameters().spike_times({3, 4, 5, 6}));

	auto pop6 = Population<SpikeSourceArray>(
	    netw, 3, SpikeSourceArrayParameters().spike_times());
	pop6[0].parameters().spike_times({1, 2, 3});
	pop6[1].parameters().spike_times({});
	pop6[2].parameters().spike_times({3, 4, 5, 6, 7});

	for (auto import : all_avail_imports) {
		py::module pynn = py::module::import(import.c_str());
		pynn.attr("setup")();
		std::vector<Real> spikes1, spikes2, spikes3, spikes4, spikes50,
		    spikes57, spikes60, spikes61, spikes62;
		if (version > 8 || import == "pyNN.nest") {
			auto pypop1 = PyNN_::create_source_population(pop1, pynn);
			auto pypop2 = PyNN_::create_source_population(pop2, pynn);
			auto pypop3 = PyNN_::create_source_population(pop3, pynn);
			auto pypop4 = PyNN_::create_source_population(pop4, pynn);
			auto pypop5 = PyNN_::create_source_population(pop5, pynn);
			auto pypop6 = PyNN_::create_source_population(pop6, pynn);
			spikes1 = py::cast<std::vector<Real>>(
			    py::list(py::list(pypop1.attr("get")("spike_times"))[0].attr(
			        "value"))[0]);
			spikes2 = py::cast<std::vector<Real>>(
			    py::list(py::list(pypop2.attr("get")("spike_times"))[0].attr(
			        "value"))[0]);
			spikes3 = py::cast<std::vector<Real>>(
			    py::list(py::list(pypop3.attr("get")("spike_times"))[0].attr(
			        "value"))[0]);
			spikes4 = py::cast<std::vector<Real>>(
			    py::list(py::list(pypop4.attr("get")("spike_times"))[0].attr(
			        "value"))[0]);
			spikes50 = py::cast<std::vector<Real>>(
			    py::list(py::list(pypop5.attr("get")("spike_times"))[0].attr(
			        "value"))[0]);
			spikes57 = py::cast<std::vector<Real>>(
			    py::list(py::list(pypop5.attr("get")("spike_times"))[7].attr(
			        "value"))[0]);

			spikes60 = py::cast<std::vector<Real>>(
			    py::list(py::list(pypop6.attr("get")("spike_times"))[0].attr(
			        "value"))[0]);

			spikes61 = py::cast<std::vector<Real>>(
			    py::list(py::list(pypop6.attr("get")("spike_times"))[1].attr(
			        "value"))[0]);

			spikes62 = py::cast<std::vector<Real>>(
			    py::list(py::list(pypop6.attr("get")("spike_times"))[2].attr(
			        "value"))[0]);
		}
		else if (import == "pyNN.spiNNaker") {
			auto pypop1 = PyNN_::create_source_population(pop1, pynn);
			auto pypop2 = PyNN_::create_source_population(pop2, pynn);
			auto pypop3 = PyNN_::create_source_population(pop3, pynn);
			auto pypop4 = PyNN_::create_source_population(pop4, pynn);
			auto pypop5 = PyNN_::create_source_population(pop5, pynn);
			auto pypop6 = PyNN_::create_source_population(pop6, pynn);
			spikes1 = py::cast<std::vector<Real>>(
			    py::list(pypop1.attr("get")("spike_times"))[0]);
			spikes2 = py::cast<std::vector<Real>>(
			    py::list(pypop2.attr("get")("spike_times"))[0]);
			spikes3 = py::cast<std::vector<Real>>(
			    py::list(pypop3.attr("get")("spike_times"))[0]);
			spikes4 = py::cast<std::vector<Real>>(
			    py::list(pypop4.attr("get")("spike_times"))[0]);
			spikes50 = py::cast<std::vector<Real>>(
			    py::list(pypop5.attr("get")("spike_times"))[0]);
			spikes57 = py::cast<std::vector<Real>>(
			    py::list(pypop5.attr("get")("spike_times"))[7]);

			spikes60 = py::cast<std::vector<Real>>(
			    py::list(pypop6.attr("get")("spike_times"))[0]);
			spikes61 = py::cast<std::vector<Real>>(
			    py::list(pypop6.attr("get")("spike_times"))[1]);
			spikes62 = py::cast<std::vector<Real>>(
			    py::list(pypop6.attr("get")("spike_times"))[2]);
		}
		else if (import == "pyNN.hardware.spikey") {
			auto pypop1 = PyNN_::spikey_create_source_population(pop1, pynn);
			auto pypop2 = PyNN_::spikey_create_source_population(pop2, pynn);
			auto pypop3 = PyNN_::spikey_create_source_population(pop3, pynn);
			auto pypop4 = PyNN_::spikey_create_source_population(pop4, pynn);
			auto pypop5 = PyNN_::spikey_create_source_population(pop5, pynn);
			auto pypop6 = PyNN_::spikey_create_source_population(pop6, pynn);
			spikes1 = py::cast<std::vector<Real>>(
			    py::list(pypop1.attr("get")("spike_times"))[0]);
			spikes2 = py::cast<std::vector<Real>>(
			    py::list(pypop2.attr("get")("spike_times"))[0]);
			spikes3 = py::cast<std::vector<Real>>(
			    py::list(pypop3.attr("get")("spike_times"))[0]);
			spikes4 = py::cast<std::vector<Real>>(
			    py::list(pypop4.attr("get")("spike_times"))[0]);
			spikes50 = py::cast<std::vector<Real>>(
			    py::list(pypop5.attr("get")("spike_times"))[0]);
			spikes57 = py::cast<std::vector<Real>>(
			    py::list(pypop5.attr("get")("spike_times"))[7]);

			spikes60 = py::cast<std::vector<Real>>(
			    py::list(pypop6.attr("get")("spike_times"))[0]);
			spikes61 = py::cast<std::vector<Real>>(
			    py::list(pypop6.attr("get")("spike_times"))[1]);
			spikes62 = py::cast<std::vector<Real>>(
			    py::list(pypop6.attr("get")("spike_times"))[2]);
		}
		else {
			std::cout << " ... Skipping test" << std::endl;
			return;
		}

		EXPECT_EQ(pop1[0].parameters().spike_times().size(), spikes1.size());
		for (size_t i = 0; i < spikes1.size(); i++) {
			EXPECT_EQ(pop1[0].parameters().spike_times()[i], spikes1[i]);
		}
		EXPECT_EQ(pop2[0].parameters().spike_times().size(), spikes2.size());
		for (size_t i = 0; i < spikes2.size(); i++) {
			EXPECT_EQ(pop2[0].parameters().spike_times()[i], spikes2[i]);
		}
		EXPECT_EQ(pop3[0].parameters().spike_times().size(), spikes3.size());
		for (size_t i = 0; i < spikes3.size(); i++) {
			EXPECT_EQ(pop3[0].parameters().spike_times()[i], spikes3[i]);
		}
		EXPECT_EQ(pop4[0].parameters().spike_times().size(), spikes4.size());
		for (size_t i = 0; i < spikes4.size(); i++) {
			EXPECT_EQ(pop4[0].parameters().spike_times()[i], spikes4[i]);
		}
		EXPECT_EQ(pop5[0].parameters().spike_times().size(), spikes50.size());
		for (size_t i = 0; i < spikes50.size(); i++) {
			EXPECT_EQ(pop5[0].parameters().spike_times()[i], spikes50[i]);
		}
		EXPECT_EQ(pop5[7].parameters().spike_times().size(), spikes57.size());
		for (size_t i = 0; i < spikes57.size(); i++) {
			EXPECT_EQ(pop5[7].parameters().spike_times()[i], spikes57[i]);
		}

		EXPECT_EQ(pop6[0].parameters().spike_times().size(), spikes60.size());
		for (size_t i = 0; i < spikes60.size(); i++) {
			EXPECT_EQ(pop6[0].parameters().spike_times()[i], spikes60[i]);
		}
		EXPECT_EQ(pop6[1].parameters().spike_times().size(), spikes61.size());
		for (size_t i = 0; i < spikes61.size(); i++) {
			EXPECT_EQ(pop6[1].parameters().spike_times()[i], spikes61[i]);
		}
		EXPECT_EQ(pop6[2].parameters().spike_times().size(), spikes62.size());
		for (size_t i = 0; i < spikes62.size(); i++) {
			EXPECT_EQ(pop6[2].parameters().spike_times()[i], spikes62[i]);
		}
	}
	// TODO SPIKEY + BRAINSCALES
}

void check_pop_parameters_nest(const py::list &pypop, const PopulationBase &pop)
{
	const std::vector<std::string> &parameter_names =
	    pop.type().parameter_names;
	EXPECT_EQ(pop.size(), py::len(pypop));
	for (size_t i = 0; i < parameter_names.size(); i++) {
		for (size_t j = 0; j < py::len(pypop); j++) {
			EXPECT_EQ(
			    pop[j].parameters()[i],
			    py::cast<Real>(pypop[j].attr(parameter_names[i].c_str())));
		}
	}
}
void check_pop_parameters_spinnaker(const py::object &pypop,
                                    const PopulationBase &pop)
{
	const std::vector<std::string> &parameter_names =
	    pop.type().parameter_names;
	EXPECT_EQ(pop.size(), py::cast<size_t>(pypop.attr("size")));
	for (size_t i = 0; i < parameter_names.size(); i++) {
		std::vector<Real> params = py::cast<std::vector<Real>>(
		    pypop.attr("get")(parameter_names[i].c_str()));
		for (size_t j = 0; j < pop.size(); j++) {
			EXPECT_EQ(pop[j].parameters()[i], params[j]);
		}
	}
}

TEST(pynn2, create_homogeneous_pop)
{
	for (auto import : all_avail_imports) {
		py::module pynn = py::module::import(import.c_str());
		pynn.attr("setup")();
		bool temp = false;

		cypress::Network netw;
		auto pop1 = netw.create_population<IfCondExp>(2, IfCondExpParameters());
		auto pop2 = netw.create_population<IfFacetsHardware1>(
		    4, IfFacetsHardware1Parameters());
		auto pop3 = netw.create_population<EifCondExpIsfaIsta>(
		    32, EifCondExpIsfaIstaParameters());

		if (import == "pyNN.nest") {
			py::list pypop =
			    py::list(PyNN_::create_homogeneous_pop(pop1, pynn, temp));
			EXPECT_TRUE(temp);
			check_pop_parameters_nest(pypop, pop1);
			// IfFacetsHardware1 does not know tau_refrac in Nest
			pypop = py::list(PyNN_::create_homogeneous_pop(pop3, pynn, temp));
			EXPECT_TRUE(temp);
			check_pop_parameters_nest(pypop, pop3);
		}
		else if (import == "pyNN.spiNNaker") {
			py::object pypop =
			    py::object(PyNN_::create_homogeneous_pop(pop1, pynn, temp));
			EXPECT_TRUE(temp);
			check_pop_parameters_spinnaker(pypop, pop1);
		}
		else if (import == "pyNN.hardware.spikey") {
			py::object pypop = PyNN_::spikey_create_homogeneous_pop(pop2, pynn);
			check_pop_parameters_spinnaker(pypop, pop2);
		}
		else {
			std::cout << " ... Skipping test" << std::endl;
		}
		// TODO BRAINSCALES
	}
}

TEST(pynn2, set_inhomogeneous_parameters)
{
	for (auto import : all_avail_imports) {
		py::module pynn = py::module::import(import.c_str());
		if (import == "pyNN.hardware.spikey") {
			pynn.attr("end")();
			pynn.attr("setup")("calibIcb"_a = true);
		}
		else {
			pynn.attr("setup")();
		}
		cypress::Network netw;
		auto pop1 = netw.create_population<IfCondExp>(3, IfCondExpParameters());
		pop1[0].parameters().tau_refrac(50);
		pop1[1].parameters().v_rest(-80);
		pop1[2].parameters().v_rest(-80);
		pop1[2].parameters().v_reset(-120);
		if (import == "pyNN.nest") {
			bool temp;
			py::object pypop =
			    py::object(PyNN_::create_homogeneous_pop(pop1, pynn, temp));
			EXPECT_TRUE(temp);
			PyNN_::set_inhomogeneous_parameters(pop1, pypop, temp);
			check_pop_parameters_nest(py::list(pypop), pop1);
		}
		else if (import == "pyNN.spiNNaker") {
			// Not supporterd by spinnaker
			std::cout << " ... Skipping test" << std::endl;
		}
		else if (import == "pyNN.hardware.spikey") {
			auto pop2 = netw.create_population<IfFacetsHardware1>(
			    3, IfFacetsHardware1Parameters());
			pop2[0].parameters().tau_refrac(5);
			pop2[1].parameters().tau_m(10);
			pop2[2].parameters().tau_m(9);
			pop2[2].parameters().tau_refrac(7);
			py::object pypop =
			    py::object(PyNN_::spikey_create_homogeneous_pop(pop2, pynn));
			PyNN_::spikey_set_inhomogeneous_parameters(pop2, pypop);
			check_pop_parameters_spinnaker(pypop, pop2);
		}
		else {
			std::cout << " ... Skipping test" << std::endl;
		}
		// TODO BRAINSCALES
	}
}

TEST(pynn2, set_homogeneous_rec)
{
	for (auto import : all_avail_imports) {
		py::module pynn = py::module::import(import.c_str());
		pynn.attr("setup")();
		bool temp = false;

		cypress::Network netw;
		auto pop1 = netw.create_population<IfCondExp>(2, IfCondExpParameters(),
		                                              IfCondExpSignals()
		                                                  .record_spikes()
		                                                  .record_v()
		                                                  .record_gsyn_exc()
		                                                  .record_gsyn_inh());
		auto pop2 = netw.create_population<IfFacetsHardware1>(
		    4, IfFacetsHardware1Parameters(),
		    IfFacetsHardware1Signals().record_spikes().record_v());
		auto pop3 = netw.create_population<EifCondExpIsfaIsta>(
		    32, EifCondExpIsfaIstaParameters(),
		    EifCondExpIsfaIstaSignals()
		        .record_gsyn_exc()
		        .record_gsyn_inh()
		        .record_spikes()
		        .record_v());
		if (import == "pyNN.nest") {
			py::object pypop = PyNN_::create_homogeneous_pop(pop1, pynn, temp);
			EXPECT_NO_THROW(PyNN_::set_homogeneous_rec(pop1, pypop));

			pypop = PyNN_::create_homogeneous_pop(pop3, pynn, temp);
			EXPECT_NO_THROW(PyNN_::set_homogeneous_rec(pop3, pypop));
		}
		else if (import == "pyNN.spiNNaker") {
			py::object pypop =
			    py::object(PyNN_::create_homogeneous_pop(pop1, pynn, temp));
			EXPECT_NO_THROW(PyNN_::set_homogeneous_rec(pop1, pypop));
		}
		else if (import == "pyNN.hardware.spikey") {
			py::object pypop =
			    py::object(PyNN_::spikey_create_homogeneous_pop(pop2, pynn));
			EXPECT_NO_THROW(
			    PyNN_::spikey_set_homogeneous_rec(pop2, pypop, pynn));
		}
		else {
			std::cout << " ... Skipping test" << std::endl;
		}
		// TODO BRAINSCALES
	}
}

TEST(pynn2, set_inhomogenous_rec)
{
	for (auto import : all_avail_imports) {
		py::module pynn = py::module::import(import.c_str());
		pynn.attr("setup")();
		bool temp = false;

		cypress::Network netw;
		auto pop1 = netw.create_population<IfCondExp>(2, IfCondExpParameters());
		pop1[1]
		    .signals()
		    .record_spikes()
		    .record_v()
		    .record_gsyn_exc()
		    .record_gsyn_inh();
		auto pop2 = netw.create_population<IfFacetsHardware1>(
		    4, IfFacetsHardware1Parameters());
		pop2[0].signals().record_spikes().record_v();

		auto pop3 = netw.create_population<EifCondExpIsfaIsta>(
		    32, EifCondExpIsfaIstaParameters());
		pop3[16]
		    .signals()
		    .record_gsyn_exc()
		    .record_gsyn_inh()
		    .record_spikes()
		    .record_v();

		auto pop4 = netw.create_population<EifCondExpIsfaIsta>(
		    16, EifCondExpIsfaIstaParameters());
		pop4[14].signals().record_gsyn_exc();
		if (import == "pyNN.nest") {
			py::object pypop = PyNN_::create_homogeneous_pop(pop1, pynn, temp);
			EXPECT_NO_THROW(PyNN_::set_inhomogenous_rec(pop1, pypop, pynn));

			pypop = PyNN_::create_homogeneous_pop(pop3, pynn, temp);
			EXPECT_NO_THROW(PyNN_::set_inhomogenous_rec(pop3, pypop, pynn));
			pypop = PyNN_::create_homogeneous_pop(pop4, pynn, temp);
			EXPECT_NO_THROW(PyNN_::set_inhomogenous_rec(pop4, pypop, pynn));
		}
		else if (import == "pyNN.spiNNaker") {
			// Not supporterd by spinnaker
			std::cout << " ... Skipping test" << std::endl;
		}
		else if (import == "pyNN.hardware.spikey") {
			py::object pypop =
			    py::object(PyNN_::spikey_create_homogeneous_pop(pop2, pynn));
			EXPECT_NO_THROW(
			    PyNN_::spikey_set_inhomogeneous_rec(pop2, pypop, pynn));
		}
		else {
			std::cout << " ... Skipping test" << std::endl;
		}
		// TODO BRAINSCALES
	}
}

TEST(pynn2, get_pop_view)
{
	for (auto import : all_avail_imports) {
		py::module pynn = py::module::import(import.c_str());
		pynn.attr("setup")();

		cypress::Network netw;
		auto pop1 =
		    netw.create_population<IfCondExp>(16, IfCondExpParameters());
		auto pop3 = netw.create_population<EifCondExpIsfaIsta>(
		    16, EifCondExpIsfaIstaParameters());
		pop1[6].parameters().tau_refrac(50);
		pop1[12].parameters().v_rest(-80);
		pop1[13].parameters().v_rest(-80);
		pop1[13].parameters().v_reset(-120);
		if (import == "pyNN.nest") {
			bool temp;
			py::object pypop =
			    py::object(PyNN_::create_homogeneous_pop(pop1, pynn, temp));
			PyNN_::set_inhomogeneous_parameters(pop1, pypop, temp);
			py::object popview =
			    PyNN_::get_pop_view(pynn, pypop, pop1, 0, pop1.size());
			EXPECT_EQ(pop1.size(), py::cast<size_t>(popview.attr("size")));

			popview = PyNN_::get_pop_view(pynn, pypop, pop1, 6, pop1.size());
			EXPECT_EQ(50,
			          py::cast<Real>(py::list(popview)[0].attr("tau_refrac")));

			popview = PyNN_::get_pop_view(pynn, pypop, pop1, 12, pop1.size());
			EXPECT_EQ(-80, py::cast<Real>(py::list(popview)[0].attr("v_rest")));
			EXPECT_EQ(-80, py::cast<Real>(py::list(popview)[1].attr("v_rest")));
			EXPECT_EQ(-120,
			          py::cast<Real>(py::list(popview)[1].attr("v_reset")));

			EXPECT_NO_THROW(
			    PyNN_::get_pop_view(pynn, pypop, pop1, 0, pop1.size() + 1));
			EXPECT_NO_THROW(PyNN_::get_pop_view(
			    pynn, pypop, pop1, pop1.size() - 2, pop1.size() + 1));
		}
		else if (import == "pyNN.spiNNaker") {
			// Not supporterd by spinnaker
			std::cout << " ... Skipping test" << std::endl;
		}
		else {
			std::cout << " ... Skipping test" << std::endl;
		}
		// TODO + BRAINSCALES
	}
}

TEST(pynn2, get_connector)
{
	if (version >= 8) {
		for (auto import : all_avail_imports) {
			py::module pynn = py::module::import(import.c_str());
			pynn.attr("setup")();
			EXPECT_NO_THROW(PyNN_::get_connector("AllToAllConnector", pynn, 3));
			EXPECT_NO_THROW(PyNN_::get_connector("OneToOneConnector", pynn, 3));

			if (import == "pyNN.spiNNaker") {
				EXPECT_ANY_THROW(
				    PyNN_::get_connector("FixedProbabilityConnector", pynn, 3));
			}
			else {
				EXPECT_NO_THROW(
				    PyNN_::get_connector("FixedProbabilityConnector", pynn, 3));
			}
			EXPECT_NO_THROW(
			    PyNN_::get_connector("FixedProbabilityConnector", pynn, 0.5));
			EXPECT_NO_THROW(
			    PyNN_::get_connector("FixedNumberPreConnector", pynn, 2));
			EXPECT_NO_THROW(
			    PyNN_::get_connector("FixedNumberPreConnector", pynn, 0.3));
			EXPECT_NO_THROW(
			    PyNN_::get_connector("FixedNumberPostConnector", pynn, 2));
			EXPECT_NO_THROW(
			    PyNN_::get_connector("FixedNumberPostConnector", pynn, 3.8));
			EXPECT_ANY_THROW(PyNN_::get_connector("foo", pynn, 2));
		}
	}
	else {
		std::cout << " ... Skipping test" << std::endl;
	}
}

TEST(pynn2, get_connector7)
{
	if (version == 6) {
		py::module pynn = py::module::import("pyNN.hardware.spikey");
		pynn.attr("setup")();

		GroupConnection conn(0, 1, 0, 16, 0, 16, 0.15, 1, 3, "asd", true);
		EXPECT_NO_THROW(PyNN_::get_connector7("AllToAllConnector", conn, pynn));
		EXPECT_NO_THROW(PyNN_::get_connector7("OneToOneConnector", conn, pynn));

		EXPECT_NO_THROW(
		    PyNN_::get_connector7("FixedNumberPreConnector", conn, pynn));
		EXPECT_NO_THROW(
		    PyNN_::get_connector7("FixedNumberPreConnector", conn, pynn));
		EXPECT_ANY_THROW(
		    PyNN_::get_connector7("FixedProbabilityConnector", conn, pynn));
		conn.additional_parameter = 0.5;
		EXPECT_NO_THROW(
		    PyNN_::get_connector7("FixedProbabilityConnector", conn, pynn));
	}
	else {
		std::cout << " ... Skipping test" << std::endl;
	}
	// TODO BRAINSCALES
}

void check_projection(py::object projection, GroupConnection &conn)
{
	py::list weights = py::list(projection.attr("getWeights")());
	py::list delays = py::list(projection.attr("getDelays")());
	for (auto i : delays) {
		EXPECT_EQ(conn.synapse_parameters[1], py::cast<Real>(i));
	}
	if (conn.synapse_parameters[0] >= 0) {
		if (version > 8) {
			EXPECT_EQ("excitatory",
			          py::cast<std::string>(projection.attr("receptor_type")));
		}
		for (auto i : weights) {
			EXPECT_NEAR(conn.synapse_parameters[0], py::cast<Real>(i), 1e-4);
		}
	}
	else {
		if (version > 8) {
			EXPECT_EQ("inhibitory",
			          py::cast<std::string>(projection.attr("receptor_type")));
		}
		for (auto i : weights) {
			EXPECT_NEAR(-conn.synapse_parameters[0], py::cast<Real>(i), 1e-4);
		}
	}
}

void check_all_to_all_list_projection(py::object projection, Real weight,
                                      Real delay)
{
	//"getWeights"/ "getDelays" only returns a list of set synapses. It is
	// therefor unfeasible to keep track which synapse in python belongs to
	// which  synapse. Thus, this test is limited to all_to_all connections
	py::list weights = py::list(projection.attr("getWeights")());
	py::list delays = py::list(projection.attr("getDelays")());
	for (auto i : delays) {
		EXPECT_EQ(delay, py::cast<Real>(i));
	}
	if (weight >= 0) {
		if (version > 8) {
			EXPECT_EQ("excitatory",
			          py::cast<std::string>(projection.attr("receptor_type")));
		}
		for (auto i : weights) {
			EXPECT_NEAR(weight, py::cast<Real>(i), 1e-4);
		}
	}
	else {

		if (version > 8) {
			EXPECT_EQ("inhibitory",
			          py::cast<std::string>(projection.attr("receptor_type")));
		}
		for (auto i : weights) {
			EXPECT_NEAR(-weight, py::cast<Real>(i), 1e-4);
		}
	}
}

TEST(pynn2, group_connect)
{
	Network netw;
	std::vector<PopulationBase> pops{
	    netw.create_population<IfCondExp>(16, IfCondExpParameters()),
	    netw.create_population<IfCondExp>(16, IfCondExpParameters())};
	if (version > 7) {
		for (auto import : all_avail_imports) {
			py::module pynn = py::module::import(import.c_str());
			pynn.attr("setup")();
			bool temp;
			std::vector<py::object> pypops{
			    PyNN_::create_homogeneous_pop(pops[0], pynn, temp),
			    PyNN_::create_homogeneous_pop(pops[1], pynn, temp)};
			GroupConnection conn(0, 1, 0, 16, 0, 16, 0.15, 1, 3, "asd", true);
			ConnectionDescriptor conn_desc(0, 0, 16, 1, 0, 16,
			                               Connector::all_to_all(0.15, 1));
			EXPECT_NO_THROW(PyNN_::group_connect(
			    pops, pypops, conn_desc, conn, pynn, "AllToAllConnector", 0.1));
			EXPECT_NO_THROW(PyNN_::group_connect(
			    pops, pypops, conn_desc, conn, pynn, "OneToOneConnector", 0.1));
			EXPECT_NO_THROW(
			    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
			                         "FixedNumberPreConnector", 0.1));
			EXPECT_NO_THROW(
			    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
			                         "FixedNumberPostConnector", 0.1));
			conn.additional_parameter = 0.5;
			EXPECT_NO_THROW(
			    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
			                         "FixedProbabilityConnector", 0.1));

			if (import == "pyNN.nest" && version > 8) {
				conn.src1 = 10;
				conn.tar0 = 5;
				conn.additional_parameter = 3;
				EXPECT_NO_THROW(PyNN_::group_connect(pops, pypops, conn_desc,
				                                     conn, pynn,
				                                     "AllToAllConnector", 0.1));
				EXPECT_NO_THROW(PyNN_::group_connect(pops, pypops, conn_desc,
				                                     conn, pynn,
				                                     "OneToOneConnector", 0.1));
				EXPECT_NO_THROW(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "FixedNumberPreConnector", 0.1));
				EXPECT_NO_THROW(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "FixedNumberPostConnector", 0.1));
				conn.additional_parameter = 0.5;
				EXPECT_NO_THROW(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "FixedProbabilityConnector", 0.1));

				check_projection(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "AllToAllConnector", 0.1),
				    conn);
				check_projection(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "OneToOneConnector", 0.1),
				    conn);
				conn.additional_parameter = 3;
				check_projection(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "FixedNumberPreConnector", 0.1),
				    conn);
				check_projection(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "FixedNumberPostConnector", 0.1),
				    conn);
				conn.additional_parameter = 0.5;
				check_projection(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "FixedProbabilityConnector", 0.1),
				    conn);

				conn.synapse_parameters[0] = -0.015;
				check_projection(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "AllToAllConnector", 0.1),
				    conn);
				check_projection(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "OneToOneConnector", 0.1),
				    conn);
				conn.additional_parameter = 3;
				check_projection(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "FixedNumberPreConnector", 0.1),
				    conn);
				check_projection(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "FixedNumberPostConnector", 0.1),
				    conn);
				conn.additional_parameter = 0.5;
				check_projection(
				    PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
				                         "FixedProbabilityConnector", 0.1),
				    conn);
			}
		}
	}
	else {
		std::cout << " ... Skipping test" << std::endl;
	}
}
TEST(pynn2, group_connect7)
{
	Network netw;
	std::vector<PopulationBase> pops{netw.create_population<IfFacetsHardware1>(
	                                     16, IfFacetsHardware1Parameters()),
	                                 netw.create_population<IfFacetsHardware1>(
	                                     16, IfFacetsHardware1Parameters())};
	if (version > 8) {
		std::cout << " ... Skipping test" << std::endl;
		return;
	}
	for (auto import : all_avail_imports) {
		if (import == "pyNN.hardware.spikey") {
			py::module pynn = py::module::import(import.c_str());
			pynn.attr("setup")();
			std::vector<py::object> pypops{
			    PyNN_::spikey_create_homogeneous_pop(pops[0], pynn),
			    PyNN_::spikey_create_homogeneous_pop(pops[1], pynn)};
			GroupConnection conn(0, 1, 0, 16, 0, 16, 0.15, 1, 3, "asd", true);
			EXPECT_NO_THROW(PyNN_::group_connect7(pops, pypops, conn, pynn,
			                                      "AllToAllConnector"));
			EXPECT_NO_THROW(PyNN_::group_connect7(pops, pypops, conn, pynn,
			                                      "OneToOneConnector"));
			EXPECT_NO_THROW(PyNN_::group_connect7(pops, pypops, conn, pynn,
			                                      "FixedNumberPreConnector"));
			EXPECT_NO_THROW(PyNN_::group_connect7(pops, pypops, conn, pynn,
			                                      "FixedNumberPostConnector"));
			conn.additional_parameter = 0.5;
			EXPECT_NO_THROW(PyNN_::group_connect7(pops, pypops, conn, pynn,
			                                      "FixedProbabilityConnector"));

			conn.additional_parameter = 3.0;
			check_projection(PyNN_::group_connect7(pops, pypops, conn, pynn,
			                                       "AllToAllConnector"),
			                 conn);
			check_projection(PyNN_::group_connect7(pops, pypops, conn, pynn,
			                                       "OneToOneConnector"),
			                 conn);
			check_projection(PyNN_::group_connect7(pops, pypops, conn, pynn,
			                                       "FixedNumberPreConnector"),
			                 conn);
			check_projection(PyNN_::group_connect7(pops, pypops, conn, pynn,
			                                       "FixedNumberPostConnector"),
			                 conn);
			conn.additional_parameter = 0.5;
			check_projection(PyNN_::group_connect7(pops, pypops, conn, pynn,
			                                       "FixedProbabilityConnector"),
			                 conn);
		}
		else {
			std::cout << " ... Skipping test" << std::endl;
		}
		// TODO BRAINSCALES
	}
}
TEST(pynn2, list_connect)
{
	Network netw;

	for (auto import : all_avail_imports) {
		std::vector<PopulationBase> pops;
		if (import == "pyNN.hardware.spikey") {
			pops = std::vector<PopulationBase>{
			    netw.create_population<IfFacetsHardware1>(
			        16, IfFacetsHardware1Parameters()),
			    netw.create_population<IfFacetsHardware1>(
			        16, IfFacetsHardware1Parameters())};
		}
		else {
			pops = std::vector<PopulationBase>{
			    netw.create_population<IfCondExp>(16, IfCondExpParameters()),
			    netw.create_population<IfCondExp>(16, IfCondExpParameters())};
		}
		py::module pynn = py::module::import(import.c_str());
		pynn.attr("setup")();
		bool temp;

		std::vector<py::object> pypops;
		if (import == "pyNN.hardware.spikey") {
			pypops = std::vector<py::object>{
			    PyNN_::spikey_create_homogeneous_pop(pops[0], pynn),
			    PyNN_::spikey_create_homogeneous_pop(pops[1], pynn)};
		}
		else {
			pypops = std::vector<py::object>{
			    PyNN_::create_homogeneous_pop(pops[0], pynn, temp),
			    PyNN_::create_homogeneous_pop(pops[1], pynn, temp)};
		}

		// excitatory
		ConnectionDescriptor conn(0, 0, 15, 1, 0, 15,
		                          Connector::all_to_all(0.015, 1));
		std::tuple<py::object, py::object> connection;
		if (import == "pyNN.hardware.spikey") {
			connection = PyNN_::list_connect7(pypops, conn, pynn);
		}
		else {
			connection = PyNN_::list_connect(pypops, conn, pynn, 0.1);
		}
		py::object exc = std::get<0>(connection);
		py::object inh = std::get<1>(connection);
		if (import == "pyNN.spiNNaker") {
			pynn.attr("run")(10);
		}

		if (!(version < 9 && import == "pyNN.nest")) {
			check_all_to_all_list_projection(exc, 0.015, 1.0);
		}

		EXPECT_TRUE(py::object().is(inh));
		EXPECT_FALSE(py::object().is(exc));

		// inhibitory
		conn = ConnectionDescriptor(0, 0, 15, 1, 0, 15,
		                            Connector::all_to_all(-0.015, 1));
		if (import == "pyNN.spiNNaker") {
			pynn.attr("reset")();
		}

		if (import == "pyNN.hardware.spikey") {
			connection = PyNN_::list_connect7(pypops, conn, pynn);
		}
		else {
			connection = PyNN_::list_connect(pypops, conn, pynn, 0.1);
		}
		exc = std::get<0>(connection);
		inh = std::get<1>(connection);

		if (import == "pyNN.spiNNaker") {
			pynn.attr("run")(10);
		}
		if (!(version < 9 && import == "pyNN.nest")) {
			check_all_to_all_list_projection(inh, -0.015, 1.0);
		}
		EXPECT_TRUE(py::object().is(exc));
		EXPECT_FALSE(py::object().is(inh));

		std::vector<LocalConnection> conn_list(
		    {LocalConnection(0, 1, 0.015, 1), LocalConnection(0, 2, 2.0, 3),
		     LocalConnection(0, 3, -0.015, 1), LocalConnection(0, 4, -2.0, 3)});
		conn = ConnectionDescriptor(0, 0, 15, 1, 0, 15,
		                            Connector::from_list(conn_list));

		if (import == "pyNN.spiNNaker") {
			pynn.attr("reset")();
		}
		if (import == "pyNN.hardware.spikey") {
			connection = PyNN_::list_connect7(pypops, conn, pynn);
		}
		else {
			connection = PyNN_::list_connect(pypops, conn, pynn, 0.1);
		}
		exc = std::get<0>(connection);
		inh = std::get<1>(connection);
		EXPECT_FALSE(py::object().is(exc));
		EXPECT_FALSE(py::object().is(inh));

		if (import == "pyNN.spiNNaker") {
			pynn.attr("run")(10);
		}
		if (!(version < 9 && import == "pyNN.nest")) {
			py::list weights = py::list(exc.attr("getWeights")());
			py::list delays = py::list(exc.attr("getDelays")());
			py::list weights_i = py::list(inh.attr("getWeights")());
			py::list delays_i = py::list(inh.attr("getDelays")());

			EXPECT_NEAR(0.015, py::cast<Real>(weights[0]), 1e-4);
			EXPECT_NEAR(2.0, py::cast<Real>(weights[1]), 1e-4);
			EXPECT_NEAR(1.0, py::cast<Real>(delays[0]), 1e-4);
			EXPECT_NEAR(3.0, py::cast<Real>(delays[1]), 1e-4);
			if (version > 8) {
				EXPECT_EQ("excitatory",
				          py::cast<std::string>(exc.attr("receptor_type")));
			}

			EXPECT_NEAR(0.015, py::cast<Real>(weights_i[0]), 1e-4);
			EXPECT_NEAR(2.0, py::cast<Real>(weights_i[1]), 1e-4);
			EXPECT_NEAR(1.0, py::cast<Real>(delays_i[0]), 1e-4);
			EXPECT_NEAR(3.0, py::cast<Real>(delays_i[1]), 1e-4);
			if (version > 8) {
				EXPECT_EQ("inhibitory",
				          py::cast<std::string>(inh.attr("receptor_type")));
			}
		}
		if (import == "pyNN.spiNNaker") {
			pynn.attr("reset")();
			pynn.attr("end")();
		}
	}
}

TEST(pynn2, matrix_from_numpy)
{
	try {
		py::module::import("numpy");
	}
	catch (...) {
		std::cout << "No numpy installed...\n"
		          << " ... Skipping test" << std::endl;
		return;
	}
	py::module numpy = py::module::import("numpy");
	py::list list;
	list.append(0);
	list.append(1);
	list.append(2);
	py::object array =
	    numpy.attr("array")(list, "dtype"_a = numpy.attr("int64"));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int32_t>(array));
	EXPECT_NO_THROW(PyNN_::matrix_from_numpy<int64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<double>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<float>(array));
	auto mat = PyNN_::matrix_from_numpy<int64_t>(array);
	EXPECT_EQ(mat[0], py::cast<int64_t>(list[0]));
	EXPECT_EQ(mat[1], py::cast<int64_t>(list[1]));
	EXPECT_EQ(mat[2], py::cast<int64_t>(list[2]));

	array = numpy.attr("array")(list, "dtype"_a = numpy.attr("int32"));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int16_t>(array));
	EXPECT_NO_THROW(PyNN_::matrix_from_numpy<int32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<double>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<float>(array));
	auto mat1 = PyNN_::matrix_from_numpy<int32_t>(array);
	EXPECT_EQ(mat1[0], py::cast<int32_t>(list[0]));
	EXPECT_EQ(mat1[1], py::cast<int32_t>(list[1]));
	EXPECT_EQ(mat1[2], py::cast<int32_t>(list[2]));

	array = numpy.attr("array")(list, "dtype"_a = numpy.attr("int16"));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int8_t>(array));
	EXPECT_NO_THROW(PyNN_::matrix_from_numpy<int16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<double>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<float>(array));
	auto mat2 = PyNN_::matrix_from_numpy<int16_t>(array);
	EXPECT_EQ(mat2[0], py::cast<int16_t>(list[0]));
	EXPECT_EQ(mat2[1], py::cast<int16_t>(list[1]));
	EXPECT_EQ(mat2[2], py::cast<int16_t>(list[2]));

	array = numpy.attr("array")(list, "dtype"_a = numpy.attr("int8"));
	EXPECT_NO_THROW(PyNN_::matrix_from_numpy<int8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<double>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<float>(array));
	auto mat3 = PyNN_::matrix_from_numpy<int8_t>(array, true);
	EXPECT_EQ(mat3[0], py::cast<int8_t>(list[0]));
	EXPECT_EQ(mat3[1], py::cast<int8_t>(list[1]));
	EXPECT_EQ(mat3[2], py::cast<int8_t>(list[2]));

	array = numpy.attr("array")(list, "dtype"_a = numpy.attr("uint8"));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int64_t>(array));
	EXPECT_NO_THROW(PyNN_::matrix_from_numpy<uint8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<double>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<float>(array));
	auto mat4 = PyNN_::matrix_from_numpy<uint8_t>(array, true);
	EXPECT_EQ(mat4[0], py::cast<uint8_t>(list[0]));
	EXPECT_EQ(mat4[1], py::cast<uint8_t>(list[1]));
	EXPECT_EQ(mat4[2], py::cast<uint8_t>(list[2]));

	array = numpy.attr("array")(list, "dtype"_a = numpy.attr("uint16"));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint8_t>(array));
	EXPECT_NO_THROW(PyNN_::matrix_from_numpy<uint16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<double>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<float>(array));
	auto mat5 = PyNN_::matrix_from_numpy<uint16_t>(array);
	EXPECT_EQ(mat5[0], py::cast<uint16_t>(list[0]));
	EXPECT_EQ(mat5[1], py::cast<uint16_t>(list[1]));
	EXPECT_EQ(mat5[2], py::cast<uint16_t>(list[2]));

	array = numpy.attr("array")(list, "dtype"_a = numpy.attr("uint32"));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint16_t>(array));
	EXPECT_NO_THROW(PyNN_::matrix_from_numpy<uint32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<double>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<float>(array));
	auto mat6 = PyNN_::matrix_from_numpy<uint32_t>(array);
	EXPECT_EQ(mat6[0], py::cast<uint32_t>(list[0]));
	EXPECT_EQ(mat6[1], py::cast<uint32_t>(list[1]));
	EXPECT_EQ(mat6[2], py::cast<uint32_t>(list[2]));

	array = numpy.attr("array")(list, "dtype"_a = numpy.attr("uint64"));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint32_t>(array));
	EXPECT_NO_THROW(PyNN_::matrix_from_numpy<uint64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<double>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<float>(array));
	auto mat7 = PyNN_::matrix_from_numpy<uint64_t>(array);
	EXPECT_EQ(mat7[0], py::cast<uint64_t>(list[0]));
	EXPECT_EQ(mat7[1], py::cast<uint64_t>(list[1]));
	EXPECT_EQ(mat7[2], py::cast<uint64_t>(list[2]));

	array = numpy.attr("array")(list, "dtype"_a = numpy.attr("float64"));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint64_t>(array));
	EXPECT_NO_THROW(PyNN_::matrix_from_numpy<double>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<float>(array));
	auto mat8 = PyNN_::matrix_from_numpy<double>(array);
	EXPECT_NEAR(mat8[0], py::cast<double>(list[0]), 1e-8);
	EXPECT_NEAR(mat8[1], py::cast<double>(list[1]), 1e-8);
	EXPECT_NEAR(mat8[2], py::cast<double>(list[2]), 1e-8);

	array = numpy.attr("array")(list, "dtype"_a = numpy.attr("float32"));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<int64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint8_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint16_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint32_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<uint64_t>(array));
	EXPECT_ANY_THROW(PyNN_::matrix_from_numpy<double>(array));
	EXPECT_NO_THROW(PyNN_::matrix_from_numpy<float>(array));
	auto mat9 = PyNN_::matrix_from_numpy<float>(array);
	EXPECT_NEAR(mat9[0], py::cast<float>(list[0]), 1e-8);
	EXPECT_NEAR(mat9[1], py::cast<float>(list[1]), 1e-8);
	EXPECT_NEAR(mat9[2], py::cast<float>(list[2]), 1e-8);
}

TEST(pynn2, fetch_data_nest)
{
	if (std::find(all_avail_imports.begin(), all_avail_imports.end(),
	              "pyNN.nest") != all_avail_imports.end()) {

		Network netw;
		std::vector<PopulationBase> pops{
		    netw.create_population<SpikeSourceArray>(
		        1, SpikeSourceArrayParameters().spike_times({20, 50})),
		    netw.create_population<IfCondExp>(3,
		                                      IfCondExpParameters().tau_m(1.0),
		                                      IfCondExpSignals()
		                                          .record_v()
		                                          .record_spikes()
		                                          .record_gsyn_exc()
		                                          .record_gsyn_inh())};

		py::module pynn = py::module::import("pyNN.nest");
		pynn.attr("end")();
		pynn.attr("reset")();
		pynn.attr("setup")("timestep"_a = 0.1);
		bool temp;
		std::vector<py::object> pypops{
		    PyNN_::create_source_population(pops[0], pynn),
		    PyNN_::create_homogeneous_pop(pops[1], pynn, temp)};
		PyNN_::set_homogeneous_rec(pops[1], pypops[1]);

		GroupConnection conn(0, 1, 0, 1, 0, 3, 0.5, 1.0, 0, "AllToAllConnector",
		                     true);
		ConnectionDescriptor conn_desc(0, 0, 16, 1, 0, 16,
		                               Connector::all_to_all(0.15, 1));
		PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
		                     "AllToAllConnector", 0.1);

		pynn.attr("run")(100);
		PyNN_::fetch_data_nest(pops, pypops);

		// Test fetching spikes
		EXPECT_NEAR(pops[1][0].signals().data(0)[0], 20, 3);
		EXPECT_NEAR(pops[1][1].signals().data(0)[0], 20, 3);
		EXPECT_NEAR(pops[1][2].signals().data(0)[0], 20, 3);

		EXPECT_NEAR(pops[1][0].signals().data(
		                0)[pops[1][0].signals().data(0).size() - 1],
		            50, 3);
		EXPECT_NEAR(pops[1][1].signals().data(
		                0)[pops[1][1].signals().data(0).size() - 1],
		            50, 3);
		EXPECT_NEAR(pops[1][2].signals().data(
		                0)[pops[1][2].signals().data(0).size() - 1],
		            50, 3);
		// Test fetching voltage
		EXPECT_NEAR(pops[1][0].signals().data(1)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(1)(0, 1),
		            IfCondExpParameters().v_rest(), 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(1)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(1)(0, 1),
		            IfCondExpParameters().v_rest(), 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(1)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(1)(0, 1),
		            IfCondExpParameters().v_rest(), 0.01);

		size_t size = pops[1][2].signals().data(1).rows() - 1;
		EXPECT_NEAR(pops[1][0].signals().data(1)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(1)(size, 1),
		            IfCondExpParameters().v_rest(), 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(1)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(1)(size, 1),
		            IfCondExpParameters().v_rest(), 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(1)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(1)(size, 1),
		            IfCondExpParameters().v_rest(), 0.01);

		// Test fetching gsyn
		EXPECT_NEAR(pops[1][0].signals().data(2)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(2)(0, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(2)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(2)(0, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(2)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(2)(0, 1), 0, 0.01);

		size = pops[1][2].signals().data(2).rows() - 1;
		EXPECT_NEAR(pops[1][0].signals().data(2)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(2)(size, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(2)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(2)(size, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(2)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(2)(size, 1), 0, 0.01);

		EXPECT_NEAR(pops[1][0].signals().data(2)(219, 0), 22, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(2)(219, 1), 0.5, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(2)(219, 0), 22, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(2)(219, 1), 0.5, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(2)(219, 0), 22, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(2)(219, 1), 0.5, 0.01);

		// gsyn inhibitory
		EXPECT_NEAR(pops[1][0].signals().data(3)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(3)(0, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(3)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(3)(0, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(3)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(3)(0, 1), 0, 0.01);

		size = pops[1][2].signals().data(3).rows() - 1;
		EXPECT_NEAR(pops[1][0].signals().data(3)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(3)(size, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(3)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(3)(size, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(3)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(3)(size, 1), 0, 0.01);

		EXPECT_NEAR(pops[1][0].signals().data(3)(219, 0), 22, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(3)(219, 1), 0.0, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(3)(219, 0), 22, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(3)(219, 1), 0.0, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(3)(219, 0), 22, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(3)(219, 1), 0.0, 0.01);
		pynn.attr("reset")();
		pynn.attr("end")();
	}
	else {
		std::cout << " ... Skipping test" << std::endl;
	}
}

TEST(pynn2, fetch_data_spinnaker)
{
	if (std::find(all_avail_imports.begin(), all_avail_imports.end(),
	              "pyNN.spiNNaker") != all_avail_imports.end()) {

		Network netw;
		std::vector<PopulationBase> pops{
		    netw.create_population<SpikeSourceArray>(
		        1, SpikeSourceArrayParameters().spike_times({20, 50})),
		    netw.create_population<IfCondExp>(3,
		                                      IfCondExpParameters().tau_m(1.0),
		                                      IfCondExpSignals()
		                                          .record_v()
		                                          .record_spikes()
		                                          .record_gsyn_exc()
		                                          .record_gsyn_inh())};

		py::module pynn = py::module::import("pyNN.spiNNaker");
		pynn.attr("setup")("timestep"_a = 0.1);
		bool temp;
		std::vector<py::object> pypops{
		    PyNN_::create_source_population(pops[0], pynn),
		    PyNN_::create_homogeneous_pop(pops[1], pynn, temp)};
		PyNN_::set_homogeneous_rec(pops[1], pypops[1]);

		GroupConnection conn(0, 1, 0, 1, 0, 3, 0.5, 1.0, 0, "AllToAllConnector",
		                     true);
		ConnectionDescriptor conn_desc(0, 0, 16, 1, 0, 16,
		                               Connector::all_to_all(0.15, 1));
		PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
		                     "AllToAllConnector", 0.1);

		pynn.attr("run")(100);
		PyNN_::fetch_data_spinnaker(pops, pypops);

		// Test fetching spikes
		EXPECT_NEAR(pops[1][0].signals().data(0)[0], 20, 3);
		EXPECT_NEAR(pops[1][1].signals().data(0)[0], 20, 3);
		EXPECT_NEAR(pops[1][2].signals().data(0)[0], 20, 3);

		EXPECT_NEAR(pops[1][0].signals().data(
		                0)[pops[1][0].signals().data(0).size() - 1],
		            50, 3);
		EXPECT_NEAR(pops[1][1].signals().data(
		                0)[pops[1][1].signals().data(0).size() - 1],
		            50, 3);
		EXPECT_NEAR(pops[1][2].signals().data(
		                0)[pops[1][2].signals().data(0).size() - 1],
		            50, 3);
		// Test fetching voltage
		EXPECT_NEAR(pops[1][0].signals().data(1)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(1)(0, 1),
		            IfCondExpParameters().v_rest(), 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(1)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(1)(0, 1),
		            IfCondExpParameters().v_rest(), 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(1)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(1)(0, 1),
		            IfCondExpParameters().v_rest(), 0.01);

		size_t size = pops[1][2].signals().data(1).rows() - 1;
		EXPECT_NEAR(pops[1][0].signals().data(1)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(1)(size, 1),
		            IfCondExpParameters().v_rest(), 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(1)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(1)(size, 1),
		            IfCondExpParameters().v_rest(), 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(1)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(1)(size, 1),
		            IfCondExpParameters().v_rest(), 0.01);

		// Test fetching gsyn
		EXPECT_NEAR(pops[1][0].signals().data(2)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(2)(0, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(2)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(2)(0, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(2)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(2)(0, 1), 0, 0.01);

		size = pops[1][2].signals().data(2).rows() - 1;
		EXPECT_NEAR(pops[1][0].signals().data(2)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(2)(size, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(2)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(2)(size, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(2)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(2)(size, 1), 0, 0.01);

		EXPECT_NEAR(pops[1][0].signals().data(2)(210, 0), 21, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(2)(210, 1), 0.5, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(2)(210, 0), 21, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(2)(210, 1), 0.5, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(2)(210, 0), 21, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(2)(210, 1), 0.5, 0.01);

		// gsyn inhibitory
		EXPECT_NEAR(pops[1][0].signals().data(3)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(3)(0, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(3)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(3)(0, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(3)(0, 0), 0.1, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(3)(0, 1), 0, 0.01);

		size = pops[1][2].signals().data(3).rows() - 1;
		EXPECT_NEAR(pops[1][0].signals().data(3)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(3)(size, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(3)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(3)(size, 1), 0, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(3)(size, 0), 100, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(3)(size, 1), 0, 0.01);

		EXPECT_NEAR(pops[1][0].signals().data(3)(210, 0), 21, 0.2);
		EXPECT_NEAR(pops[1][0].signals().data(3)(210, 1), 0.0, 0.01);
		EXPECT_NEAR(pops[1][1].signals().data(3)(210, 0), 21, 0.2);
		EXPECT_NEAR(pops[1][1].signals().data(3)(210, 1), 0.0, 0.01);
		EXPECT_NEAR(pops[1][2].signals().data(3)(210, 0), 21, 0.2);
		EXPECT_NEAR(pops[1][2].signals().data(3)(210, 1), 0.0, 0.01);
		pynn.attr("reset")();
		pynn.attr("end")();
	}
	else {
		std::cout << " ... Skipping test" << std::endl;
	}
}

TEST(pynn2, fetch_data_neo)
{
	if (neo_version >= 4) {
		if (std::find(all_avail_imports.begin(), all_avail_imports.end(),
		              "pyNN.nest") != all_avail_imports.end()) {

			Network netw;
			std::vector<PopulationBase> pops{
			    netw.create_population<SpikeSourceArray>(
			        1, SpikeSourceArrayParameters().spike_times({20, 50})),
			    netw.create_population<IfCondExp>(
			        3, IfCondExpParameters().tau_m(1.0),
			        IfCondExpSignals()
			            .record_v()
			            .record_spikes()
			            .record_gsyn_exc()
			            .record_gsyn_inh())};

			py::module pynn = py::module::import("pyNN.nest");
			pynn.attr("end")();
			pynn.attr("reset")();
			pynn.attr("setup")("timestep"_a = 0.1);
			bool temp;
			std::vector<py::object> pypops{
			    PyNN_::create_source_population(pops[0], pynn),
			    PyNN_::create_homogeneous_pop(pops[1], pynn, temp)};
			PyNN_::set_homogeneous_rec(pops[1], pypops[1]);
			PyNN_::set_homogeneous_rec(pops[0], pypops[0]);

			GroupConnection conn(0, 1, 0, 1, 0, 3, 0.5, 1.0, 0,
			                     "AllToAllConnector", true);
			ConnectionDescriptor conn_desc(0, 0, 16, 1, 0, 16,
			                               Connector::all_to_all(0.15, 1));
			PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
			                     "AllToAllConnector", 0.1);

			pynn.attr("run")(100);
			PyNN_::fetch_data_neo(pops, pypops);

			// Test fetching spikes
			EXPECT_NEAR(pops[1][0].signals().data(0)[0], 20, 3);
			EXPECT_NEAR(pops[1][1].signals().data(0)[0], 20, 3);
			EXPECT_NEAR(pops[1][2].signals().data(0)[0], 20, 3);

			EXPECT_NEAR(pops[1][0].signals().data(
			                0)[pops[1][0].signals().data(0).size() - 1],
			            50, 3);
			EXPECT_NEAR(pops[1][1].signals().data(
			                0)[pops[1][1].signals().data(0).size() - 1],
			            50, 3);
			EXPECT_NEAR(pops[1][2].signals().data(
			                0)[pops[1][2].signals().data(0).size() - 1],
			            50, 3);
			// Test fetching voltage
			size_t size = pops[1][2].signals().data(1).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);

			size = pops[1][2].signals().data(1).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);

			// Test fetching gsyn
			EXPECT_NEAR(pops[1][0].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(0, 1), 0, 0.01);

			size = pops[1][2].signals().data(2).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(size, 1), 0, 0.01);

			EXPECT_NEAR(pops[1][0].signals().data(2)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(219, 1), 0.5, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(219, 1), 0.5, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(219, 1), 0.5, 0.01);

			// gsyn inhibitory
			EXPECT_NEAR(pops[1][0].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(0, 1), 0, 0.01);

			size = pops[1][2].signals().data(3).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(size, 1), 0, 0.01);

			EXPECT_NEAR(pops[1][0].signals().data(3)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(219, 1), 0.0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(219, 1), 0.0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(219, 1), 0.0, 0.01);
		}
		else if (std::find(all_avail_imports.begin(), all_avail_imports.end(),
		                   "pyNN.spiNNaker") != all_avail_imports.end()) {

			Network netw;
			std::vector<PopulationBase> pops{
			    netw.create_population<SpikeSourceArray>(
			        1, SpikeSourceArrayParameters().spike_times({20, 50})),
			    netw.create_population<IfCondExp>(
			        3, IfCondExpParameters().tau_m(1.0),
			        IfCondExpSignals()
			            .record_v()
			            .record_spikes()
			            .record_gsyn_exc()
			            .record_gsyn_inh())};

			py::module pynn = py::module::import("pyNN.spiNNaker");
			pynn.attr("setup")("timestep"_a = 0.1);
			pynn.attr("reset")();
			pynn.attr("end")();
			pynn.attr("setup")("timestep"_a = 0.1);
			bool temp;
			std::vector<py::object> pypops{
			    PyNN_::create_source_population(pops[0], pynn),
			    PyNN_::create_homogeneous_pop(pops[1], pynn, temp)};
			PyNN_::set_homogeneous_rec(pops[1], pypops[1]);
			PyNN_::set_homogeneous_rec(pops[0], pypops[0]);

			GroupConnection conn(0, 1, 0, 1, 0, 3, 0.5, 1.0, 0,
			                     "AllToAllConnector", true);
			ConnectionDescriptor conn_desc(0, 0, 16, 1, 0, 16,
			                               Connector::all_to_all(0.15, 1));
			PyNN_::group_connect(pops, pypops, conn_desc, conn, pynn,
			                     "AllToAllConnector", 0.1);

			pynn.attr("run")(100);
			PyNN_::fetch_data_neo(pops, pypops);

			// Test fetching spikes
			EXPECT_NEAR(pops[1][0].signals().data(0)[0], 20, 3);
			EXPECT_NEAR(pops[1][1].signals().data(0)[0], 20, 3);
			EXPECT_NEAR(pops[1][2].signals().data(0)[0], 20, 3);

			EXPECT_NEAR(pops[1][0].signals().data(
			                0)[pops[1][0].signals().data(0).size() - 1],
			            50, 3);
			EXPECT_NEAR(pops[1][1].signals().data(
			                0)[pops[1][1].signals().data(0).size() - 1],
			            50, 3);
			EXPECT_NEAR(pops[1][2].signals().data(
			                0)[pops[1][2].signals().data(0).size() - 1],
			            50, 3);
			// Test fetching voltage
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);

			size_t size = pops[1][2].signals().data(1).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);

			// Test fetching gsyn
			EXPECT_NEAR(pops[1][0].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(0, 1), 0, 0.01);

			size = pops[1][2].signals().data(2).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(size, 1), 0, 0.01);

			EXPECT_NEAR(pops[1][0].signals().data(2)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(210, 1), 0.5, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(210, 1), 0.5, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(210, 1), 0.5, 0.01);

			// gsyn inhibitory
			EXPECT_NEAR(pops[1][0].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(0, 1), 0, 0.01);

			size = pops[1][2].signals().data(3).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(size, 1), 0, 0.01);

			EXPECT_NEAR(pops[1][0].signals().data(3)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(210, 1), 0.0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(210, 1), 0.0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(210, 1), 0.0, 0.01);
			pynn.attr("reset")();
			pynn.attr("end")();
		}
		else {
			std::cout << " ... Skipping test" << std::endl;
		}
	}
	else {
		std::cout << " ... Skipping test" << std::endl;
	}
}

TEST(pynn2, fetch_data_spikey)
{

	for (auto import : all_avail_imports) {
		std::vector<PopulationBase> pops;
		if (import == "pyNN.hardware.spikey") {
			Network netw;
			std::vector<PopulationBase> pops{
			    netw.create_population<SpikeSourceArray>(
			        10, SpikeSourceArrayParameters().spike_times({20, 50})),
			    netw.create_population<IfFacetsHardware1>(
			        2,
			        IfFacetsHardware1Parameters()
			            .tau_m(1.0)
			            .v_thresh(-55.0)
			            .v_rest(-60.0)
			            .v_reset(-70.0),
			        IfFacetsHardware1Signals().record_v().record_spikes())};

			py::module pynn = py::module::import("pyNN.hardware.spikey");
			pynn.attr("setup")();
			std::vector<py::object> pypops{
			    PyNN_::spikey_create_source_population(pops[0], pynn),
			    PyNN_::spikey_create_homogeneous_pop(pops[1], pynn)};
			PyNN_::spikey_set_homogeneous_rec(pops[1], pypops[1], pynn);

			GroupConnection conn(0, 1, 0, 10, 0, 1, 0.5, 1.0, 0,
			                     "AllToAllConnector", true);
			PyNN_::group_connect7(pops, pypops, conn, pynn,
			                      "AllToAllConnector");

			pynn.attr("run")(100);

			PyNN_::spikey_get_spikes(pops[1], pypops[1]);
			PyNN_::spikey_get_voltage(pops[1][0], pynn);

			// Test fetching spikes
			EXPECT_NEAR(pops[1][0].signals().data(0)[0], 20, 5);

			EXPECT_NEAR(pops[1][0].signals().data(
			                0)[pops[1][0].signals().data(0).size() - 1],
			            50, 25);

			// Test fetching spikes
			EXPECT_NEAR(pops[1][1].signals().data(0)[0], 20, 5);

			EXPECT_NEAR(pops[1][1].signals().data(
			                0)[pops[1][1].signals().data(0).size() - 1],
			            50, 20);

			// Test fetching voltage
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 1), -60, 5);

			size_t size = pops[1][0].signals().data(1).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 1), -60, 5);
		}
	}
}

TEST(pynn2, pynn2)
{
	for (auto sim : simulators) {
		Network netw;
		std::vector<PopulationBase> pops{
		    netw.create_population<SpikeSourceArray>(
		        1, SpikeSourceArrayParameters().spike_times({20, 50})),
		    netw.create_population<IfCondExp>(3,
		                                      IfCondExpParameters().tau_m(1.0),
		                                      IfCondExpSignals()
		                                          .record_v()
		                                          .record_spikes()
		                                          .record_gsyn_exc()
		                                          .record_gsyn_inh())};
		std::cout << "test" << std::endl;
		netw.add_connection(pops[0], pops[1], Connector::all_to_all(0.5, 1.0));
		std::cout << "test" << std::endl;
		if (sim == "nest") {
			std::cout << "run" << std::endl;
			netw.run("pynn." + sim + "={\"timestep\" : 0.1}", 100);
			std::cout << "test" << std::endl;
			// Test fetching spikes
			EXPECT_NEAR(pops[1][0].signals().data(0)[0], 20, 3);
			EXPECT_NEAR(pops[1][1].signals().data(0)[0], 20, 3);
			EXPECT_NEAR(pops[1][2].signals().data(0)[0], 20, 3);

			EXPECT_NEAR(pops[1][0].signals().data(
			                0)[pops[1][0].signals().data(0).size() - 1],
			            50, 3);
			EXPECT_NEAR(pops[1][1].signals().data(
			                0)[pops[1][1].signals().data(0).size() - 1],
			            50, 3);
			EXPECT_NEAR(pops[1][2].signals().data(
			                0)[pops[1][2].signals().data(0).size() - 1],
			            50, 3);
			// Test fetching voltage
			size_t size = pops[1][2].signals().data(1).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);

			size = pops[1][2].signals().data(1).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);

			// Test fetching gsyn
			EXPECT_NEAR(pops[1][0].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(0, 1), 0, 0.01);

			size = pops[1][2].signals().data(2).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(size, 1), 0, 0.01);

			EXPECT_NEAR(pops[1][0].signals().data(2)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(219, 1), 0.5, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(219, 1), 0.5, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(219, 1), 0.5, 0.01);

			// gsyn inhibitory
			EXPECT_NEAR(pops[1][0].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(0, 1), 0, 0.01);

			size = pops[1][2].signals().data(3).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(size, 1), 0, 0.01);

			EXPECT_NEAR(pops[1][0].signals().data(3)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(219, 1), 0.0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(219, 1), 0.0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(219, 0), 22, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(219, 1), 0.0, 0.01);
		}
		else if (sim == "nmmc1") {
			netw.run(sim + "={\"timestep\" : 0.1}", 100);
			// Test fetching spikes
			EXPECT_NEAR(pops[1][0].signals().data(0)[0], 20, 3);
			EXPECT_NEAR(pops[1][1].signals().data(0)[0], 20, 3);
			EXPECT_NEAR(pops[1][2].signals().data(0)[0], 20, 3);

			EXPECT_NEAR(pops[1][0].signals().data(
			                0)[pops[1][0].signals().data(0).size() - 1],
			            50, 3);
			EXPECT_NEAR(pops[1][1].signals().data(
			                0)[pops[1][1].signals().data(0).size() - 1],
			            50, 3);
			EXPECT_NEAR(pops[1][2].signals().data(
			                0)[pops[1][2].signals().data(0).size() - 1],
			            50, 3);
			// Test fetching voltage
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 0.01);

			size_t size = pops[1][2].signals().data(1).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 0.01);

			// Test fetching gsyn
			EXPECT_NEAR(pops[1][0].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(0, 1), 0, 0.01);

			size = pops[1][2].signals().data(2).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(size, 1), 0, 0.01);

			EXPECT_NEAR(pops[1][0].signals().data(2)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(2)(210, 1), 0.5, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(2)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(2)(210, 1), 0.5, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(2)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(2)(210, 1), 0.5, 0.01);

			// gsyn inhibitory
			EXPECT_NEAR(pops[1][0].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(0, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(0, 1), 0, 0.01);

			size = pops[1][2].signals().data(3).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(size, 1), 0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(size, 1), 0, 0.01);

			EXPECT_NEAR(pops[1][0].signals().data(3)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(3)(210, 1), 0.0, 0.01);
			EXPECT_NEAR(pops[1][1].signals().data(3)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][1].signals().data(3)(210, 1), 0.0, 0.01);
			EXPECT_NEAR(pops[1][2].signals().data(3)(210, 0), 21, 0.2);
			EXPECT_NEAR(pops[1][2].signals().data(3)(210, 1), 0.0, 0.01);
		}
		else if (sim == "spikey") {
			Network netw2;
			std::vector<PopulationBase> pops{
			    netw2.create_population<SpikeSourceArray>(
			        10, SpikeSourceArrayParameters().spike_times({20, 50})),
			    netw2.create_population<IfCondExp>(
			        3, IfCondExpParameters().tau_m(1.0),
			        IfCondExpSignals().record_v().record_spikes())};
			netw2.add_connection(pops[0], pops[1],
			                     Connector::all_to_all(0.5, 1.0));

			netw2.run(sim, 100);
			// Test fetching spikes
			EXPECT_NEAR(pops[1][0].signals().data(0)[0], 20, 5);
			EXPECT_NEAR(pops[1][1].signals().data(0)[0], 20, 5);
			EXPECT_NEAR(pops[1][2].signals().data(0)[0], 20, 5);

			EXPECT_NEAR(pops[1][0].signals().data(
			                0)[pops[1][0].signals().data(0).size() - 1],
			            50, 25);
			EXPECT_NEAR(pops[1][1].signals().data(
			                0)[pops[1][1].signals().data(0).size() - 1],
			            50, 25);
			EXPECT_NEAR(pops[1][2].signals().data(
			                0)[pops[1][2].signals().data(0).size() - 1],
			            50, 25);

			// Test fetching voltage
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 0), 0.1, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(0, 1),
			            IfCondExpParameters().v_rest(), 5);

			size_t size = pops[1][0].signals().data(1).rows() - 1;
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 0), 100, 0.2);
			EXPECT_NEAR(pops[1][0].signals().data(1)(size, 1),
			            IfCondExpParameters().v_rest(), 5);
		}
		else {
			// TODO
			std::cout << " ... Skipping test" << std::endl;
			// netw.run("pynn." + sim, 100);
		}
	}
}

}  // namespace cypress

int main(int argc, char **argv)
{
	// Start the python interpreter
	int temp = 0;
	{
		::testing::InitGoogleTest(&argc, argv);
		temp = RUN_ALL_TESTS();
	}
	return temp;
}
