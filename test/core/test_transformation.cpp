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
private:
	std::string m_id;

protected:
	NetworkBase do_transform(const NetworkBase &src,
	                         TransformationAuxData &) override
	{
		return src;
	}

public:
	TransformationType1(const std::string &id = "t1") : m_id(id) {}

	std::string id() const override { return m_id; }

	virtual ~TransformationType1(){};
};

class TransformationType2 : public TransformationType1 {
public:
	std::string id() const override { return "t2"; }

	TransformationProperties properties() const override
	{
		TransformationProperties res;
		res.cost = 200;
		res.lossy = false;
		return res;
	}

	virtual ~TransformationType2(){};
};

class TransformationType3 : public TransformationType1 {
public:
	std::string id() const override { return "t3"; }

	TransformationProperties properties() const override
	{
		TransformationProperties res;
		res.cost = 100;
		res.lossy = true;
		return res;
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
const TestNeuronType nt5;
const TestNeuronType nt6;
const TestNeuronType nt7;

TransformationCtor ctor1 = [] {
	return std::make_unique<TransformationType1>();
};

TransformationCtor ctor2 = [] {
	return std::make_unique<TransformationType2>();
};

TransformationCtor ctor3 = [] {
	return std::make_unique<TransformationType3>();
};

TransformationCtor ctor4 = [] {
	return std::make_unique<TransformationType1>("t4");
};

TransformationCtor ctor5 = [] {
	return std::make_unique<TransformationType1>("t5");
};

TransformationCtor ctor6 = [] {
	return std::make_unique<TransformationType1>("t6");
};

TransformationCtor ctor7 = [] {
	return std::make_unique<TransformationType1>("t7");
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
	const std::vector<const NeuronType *> unsupported_types{&nt1};
	const std::unordered_set<const NeuronType *> supported_types{&nt2};
	const std::vector<std::tuple<TransformationCtor, const NeuronType *,
	                             const NeuronType *>> transformations{
	    descr(ctor1, &nt1, &nt2)};

	auto res = Transformations::construct_neuron_type_transformation_chain(
	    unsupported_types, supported_types, transformations, false);

	ASSERT_EQ(1U, res.size());
	EXPECT_EQ("t1", res[0]()->id());
}

TEST(transformations,
     construct_neuron_type_transformation_chain_do_not_use_lossy)
{
	const std::vector<const NeuronType *> unsupported_types{&nt1};
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
	const std::vector<const NeuronType *> unsupported_types{&nt1};
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
	const std::vector<const NeuronType *> unsupported_types{&nt1};
	const std::unordered_set<const NeuronType *> supported_types{&nt4};
	const std::vector<std::tuple<TransformationCtor, const NeuronType *,
	                             const NeuronType *>> transformations{
	    descr(ctor1, &nt1, &nt2), descr(ctor2, &nt2, &nt3),
	    descr(ctor1, &nt3, &nt4)};

	auto res = Transformations::construct_neuron_type_transformation_chain(
	    unsupported_types, supported_types, transformations, true);

	ASSERT_EQ(3U, res.size());
	EXPECT_EQ("t1", res[0]()->id());
	EXPECT_EQ("t2", res[1]()->id());
	EXPECT_EQ("t1", res[2]()->id());
}

TEST(transformations, construct_neuron_type_transformation_shortest_path)
{
	const std::vector<const NeuronType *> unsupported_types{&nt1};
	const std::unordered_set<const NeuronType *> supported_types{&nt4};
	const std::vector<std::tuple<TransformationCtor, const NeuronType *,
	                             const NeuronType *>> transformations{
	    descr(ctor2, &nt1, &nt2), descr(ctor2, &nt2, &nt3),
	    descr(ctor2, &nt3, &nt4), descr(ctor3, &nt1, &nt4),
	    descr(ctor1, &nt1, &nt5), descr(ctor1, &nt5, &nt6),
	    descr(ctor1, &nt6, &nt7), descr(ctor2, &nt7, &nt4)};

	auto res = Transformations::construct_neuron_type_transformation_chain(
	    unsupported_types, supported_types, transformations, true);

	ASSERT_EQ(4U, res.size());
	EXPECT_EQ("t1", res[0]()->id());
	EXPECT_EQ("t1", res[1]()->id());
	EXPECT_EQ("t1", res[2]()->id());
	EXPECT_EQ("t2", res[3]()->id());
}

TEST(transformations, construct_neuron_type_transformation_multiple_starts)
{
	const std::vector<const NeuronType *> unsupported_types{&nt1, &nt2, &nt3,
	                                                        &nt4};
	const std::unordered_set<const NeuronType *> supported_types{&nt7};
	const std::vector<std::tuple<TransformationCtor, const NeuronType *,
	                             const NeuronType *>> transformations{
	    descr(ctor1, &nt1, &nt4), descr(ctor2, &nt2, &nt5),
	    descr(ctor4, &nt3, &nt6), descr(ctor5, &nt4, &nt5),
	    descr(ctor6, &nt5, &nt6), descr(ctor7, &nt6, &nt7)};

	auto res = Transformations::construct_neuron_type_transformation_chain(
	    unsupported_types, supported_types, transformations, true);

	ASSERT_EQ(6U, res.size());
	EXPECT_EQ("t4", res[0]()->id());
	EXPECT_EQ("t2", res[1]()->id());
	EXPECT_EQ("t1", res[2]()->id());
	EXPECT_EQ("t5", res[3]()->id());
	EXPECT_EQ("t6", res[4]()->id());
	EXPECT_EQ("t7", res[5]()->id());
}

TEST(transformations, construct_neuron_type_transformation_prefer_non_lossy)
{
	const std::vector<const NeuronType *> unsupported_types{&nt2, &nt1};
	const std::unordered_set<const NeuronType *> supported_types{&nt3};
	const std::vector<std::tuple<TransformationCtor, const NeuronType *,
	                             const NeuronType *>> transformations{
	    descr(ctor3, &nt1, &nt3), descr(ctor2, &nt1, &nt3),
	    descr(ctor3, &nt2, &nt3)};

	auto res = Transformations::construct_neuron_type_transformation_chain(
	    unsupported_types, supported_types, transformations, true);

	ASSERT_EQ(2U, res.size());
	EXPECT_EQ("t2", res[0]()->id());
	EXPECT_EQ("t3", res[1]()->id());
}
}
