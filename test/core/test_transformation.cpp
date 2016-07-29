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

#include "gtest/gtest.h"

#include <cypress/core/transformation.hpp>
#include <cypress/core/network_base.hpp>

namespace cypress {
namespace {

class TransformationType1 : public Transformation {
protected:
	NetworkBase do_transform(const NetworkBase &src) override { return src; }

public:
	std::string id() const override { return "t1"; }

	virtual ~TransformationType1(){};
};

class TransformationType2 : public TransformationType1 {
public:
	std::string id() const override { return "t2"; }

	TransformationProperties properties() const override
	{
		return TransformationProperties{200, false};
	}

	virtual ~TransformationType2(){};
};

class TransformationType3 : public TransformationType1 {
public:
	std::string id() const override { return "t3"; }

	TransformationProperties properties() const override
	{
		return TransformationProperties{100, true};
	}

	virtual ~TransformationType3(){};
};

class TestNeuronType final
    : public NeuronTypeBase<NullNeuronParameters, NullNeuronSignals> {
public:
	TestNeuronType()
	    : NeuronTypeBase("TestNeuronType", {}, {}, {}, {}, {}, false, false)
	{
	}
};

const TestNeuronType nt1;
const TestNeuronType nt2;
const TestNeuronType nt3;
const TestNeuronType nt4;

TransformationCtor ctor1 = [] {
	return std::make_unique<TransformationType1>();
};

TransformationCtor ctor2 = [] {
	return std::make_unique<TransformationType2>();
};

TransformationCtor ctor3 = [] {
	return std::make_unique<TransformationType3>();
};

static auto descr(TransformationCtor ctor, const NeuronType *src,
                  const NeuronType *tar)
{
	return std::tuple<TransformationCtor, const NeuronType *,
	                  const NeuronType *>(ctor, src, tar);
}
}

TEST(transformations, construct_neuron_type_transformation_chain_simple)
{
	const std::unordered_set<const NeuronType *> unsupported_types{&nt1};
	const std::unordered_set<const NeuronType *> supported_types{&nt2};
	const std::vector<std::tuple<TransformationCtor, const NeuronType *,
	                             const NeuronType *>> transformations{
	    descr(ctor1, &nt1, &nt2)};

	auto res = Transformations::construct_neuron_type_transformation_chain(
	    unsupported_types, supported_types, transformations, false);

	ASSERT_EQ(1U, res.size());
	EXPECT_EQ("t1", res[0]()->id());
}

TEST(transformations, construct_neuron_type_transformation_chain_do_not_use_lossy)
{
	const std::unordered_set<const NeuronType *> unsupported_types{&nt1};
	const std::unordered_set<const NeuronType *> supported_types{&nt2};
	const std::vector<std::tuple<TransformationCtor, const NeuronType *,
	                             const NeuronType *>> transformations{
	    descr(ctor3, &nt1, &nt2)};

	ASSERT_THROW(
	    Transformations::construct_neuron_type_transformation_chain(
	        unsupported_types, supported_types, transformations, false),
	    NotSupportedException);
}

TEST(transformations, construct_neuron_type_transformation_chain_use_lossy)
{
	const std::unordered_set<const NeuronType *> unsupported_types{&nt1};
	const std::unordered_set<const NeuronType *> supported_types{&nt2};
	const std::vector<std::tuple<TransformationCtor, const NeuronType *,
	                             const NeuronType *>> transformations{
	    descr(ctor3, &nt1, &nt2)};

	auto res = Transformations::construct_neuron_type_transformation_chain(
	    unsupported_types, supported_types, transformations, true);

	ASSERT_EQ(1U, res.size());
	EXPECT_EQ("t3", res[0]()->id());
}

TEST(transformations, construct_neuron_type_transformation_linear)
{
	const std::unordered_set<const NeuronType *> unsupported_types{&nt1};
	const std::unordered_set<const NeuronType *> supported_types{&nt4};
	const std::vector<std::tuple<TransformationCtor, const NeuronType *,
	                             const NeuronType *>> transformations{
	    descr(ctor1, &nt1, &nt2),
	    descr(ctor2, &nt2, &nt3),
	    descr(ctor1, &nt3, &nt4)};

	auto res = Transformations::construct_neuron_type_transformation_chain(
	    unsupported_types, supported_types, transformations, true);

	ASSERT_EQ(3U, res.size());
	EXPECT_EQ("t1", res[0]()->id());
	EXPECT_EQ("t2", res[1]()->id());
	EXPECT_EQ("t1", res[2]()->id());
}

}
