
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

#include "gtest/gtest.h"

#include <sstream>

#include <cypress/cypress.hpp>

#include <cypress/backend/pynn/pynn2.cpp>
#include <cypress/backend/pynn/pynn2.hpp>

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
		EXPECT_TRUE((version == 8) || (version == 9));

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
	else {
		try {
			py::module neo = py::module::import("neo");
			EXPECT_NO_THROW(PyNN_::get_neo_version());
		}
		catch (...) {
			EXPECT_ANY_THROW(PyNN_::get_neo_version());
			std::cout << " ... Skipping test" << std::endl;
		}
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
		// TODO SPIKEY + BRAINSCALES
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
		auto pypop1 = PyNN_::create_source_population(pop1, pynn);
		auto pypop2 = PyNN_::create_source_population(pop2, pynn);
		auto pypop3 = PyNN_::create_source_population(pop3, pynn);
		auto pypop4 = PyNN_::create_source_population(pop4, pynn);
		auto pypop5 = PyNN_::create_source_population(pop5, pynn);
		auto pypop6 = PyNN_::create_source_population(pop6, pynn);
		std::vector<Real> spikes1, spikes2, spikes3, spikes4, spikes50,
		    spikes57, spikes60, spikes61, spikes62;
		if (version > 8) {
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
		else {
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
			/*pypop = py::list(PyNN_::create_homogeneous_pop(pop2, pynn, temp));
			EXPECT_TRUE(temp);
			check_pop_parameters_nest(
			    pypop, pop2);*/
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
		else {
			std::cout << " ... Skipping test" << std::endl;
		}
		// TODO SPIKEY + BRAINSCALES
	}
}

TEST(pynn2, set_inhomogeneous_parameters)
{
	for (auto import : all_avail_imports) {
		py::module pynn = py::module::import(import.c_str());
		pynn.attr("setup")();
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
		else {
			std::cout << " ... Skipping test" << std::endl;
		}
		// TODO SPIKEY + BRAINSCALES
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
		else {
			std::cout << " ... Skipping test" << std::endl;
		}
		// TODO SPIKEY + BRAINSCALES
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
		if (import == "pyNN.nest") {
			py::object pypop = PyNN_::create_homogeneous_pop(pop1, pynn, temp);
			EXPECT_NO_THROW(PyNN_::set_inhomogenous_rec(pop1, pypop, pynn));

			pypop = PyNN_::create_homogeneous_pop(pop3, pynn, temp);
			EXPECT_NO_THROW(PyNN_::set_inhomogenous_rec(pop3, pypop, pynn));
		}
		else if (import == "pyNN.spiNNaker") {
			// Not supporterd by spinnaker
			std::cout << " ... Skipping test" << std::endl;
		}
		else {
			std::cout << " ... Skipping test" << std::endl;
		}
		// TODO SPIKEY + BRAINSCALES
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
		// TODO SPIKEY + BRAINSCALES
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
	// TODO SPIKEY + BRAINSCALES
}



}  // namespace cypress

int main(int argc, char **argv)
{
	// Start the python interpreter
	auto python = cypress::PythonInstance();
	::testing::InitGoogleTest(&argc, argv);
	auto temp = RUN_ALL_TESTS();
	return temp;
}
