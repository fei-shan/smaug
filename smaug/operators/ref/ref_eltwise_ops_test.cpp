#include "catch.hpp"
#include "core/backend.h"
#include "core/tensor.h"
#include "core/smaug_test.h"
#include "operators/eltwise_add_op.h"
#include "operators/eltwise_mul_op.h"
#include "operators/relu_op.h"
#include "operators/elu_op.h"
#include "operators/sigmoid_op.h"
#include "operators/tanh_op.h"

using namespace smaug;

TEST_CASE_METHOD(SmaugTest, "Reference eltwise operators", "[refop]") {
    TensorShape inputShape({ 1, 13 }, DataLayout::NC);
    Tensor* input0 = new Tensor("input0", inputShape);
    input0->allocateStorage<float>();
    input0->fillData<float>({ -1, -2, -3, 4, 5, 6, 7, 8, 9, -10, 11, -12, 13 });
    workspace()->addTensor(input0);

    SECTION("Element-wise add operator") {
        auto addOp = new EltwiseAddOp<ReferenceBackend>("add", workspace());
        Tensor* input1 = new Tensor("input1", inputShape);
        input1->allocateStorage<float>();
        input1->fillData<float>(
                { -2, -3, -4, 5, 6, 7, 8, 9, 10, 11, -12, 13, -14 });
        workspace()->addTensor(input1);
        addOp->setInput(input0, 0);
        addOp->setInput(input1, 1);
        addOp->createAllTensors();
        allocateAllTensors<float>(addOp);
        addOp->run();
        std::vector<float> expectedValues{ -3, -5, -7, 9,  11, 13, 15,
                                           17, 19, 1,  -1, 1,  -1 };
        auto outputsTensor = addOp->getOutput(0);
        verifyOutputs(outputsTensor, expectedValues);
    }

    SECTION("Element-wise mul operator") {
        auto mulOp = new EltwiseMulOp<ReferenceBackend>("mul", workspace());
        Tensor* input1 = new Tensor("input1", inputShape);
        input1->allocateStorage<float>();
        input1->fillData<float>(
                { -2, -3, -4, 5, 6, 7, 8, 9, 10, 11, -12, 13, -14 });
        workspace()->addTensor(input1);
        mulOp->setInput(input0, 0);
        mulOp->setInput(input1, 1);
        mulOp->createAllTensors();
        allocateAllTensors<float>(mulOp);
        mulOp->run();
        std::vector<float> expectedValues{ 2, 6, 12, 20, 30, 42, 56,
                                           72, 90, -110, -132, -156, -182 };
        auto outputsTensor = mulOp->getOutput(0);
        verifyOutputs(outputsTensor, expectedValues);
    }

    SECTION("RELU") {
        auto reluOp = new ReluOp<ReferenceBackend>("relu", workspace());
        reluOp->setInput(input0, 0);

        SECTION("Slope 0") {
            reluOp->createAllTensors();
            allocateAllTensors<float>(reluOp);
            reluOp->run();
            std::vector<float> expectedValues{ 0, 0, 0, 4, 5, 6, 7,
                                               8, 9, 0, 11, 0, 13 };
            auto outputsTensor = reluOp->getOutput(0);
            verifyOutputs(outputsTensor, expectedValues);
        }

        SECTION("Slope 0.1") {
            reluOp->setSlope(0.1);
            reluOp->createAllTensors();
            allocateAllTensors<float>(reluOp);
            reluOp->run();
            std::vector<float> expectedValues{
                -0.1, -0.2, -0.3, 4, 5, 6, 7, 8, 9, -1, 11, -1.2, 13
            };
            auto outputsTensor = reluOp->getOutput(0);
            verifyOutputs(outputsTensor, expectedValues);
        }
    }

    SECTION("ELU") {
        auto eluOp = new EluOp<ReferenceBackend>("elu", workspace(), 0.1);
        eluOp->setInput(input0, 0);
        eluOp->createAllTensors();
        allocateAllTensors<float>(eluOp);
        eluOp->run();
        std::vector<float> expectedValues{
            -0.063212, -0.086466, -0.0950213, 4,  5,           6, 7,
            8,         9,         -0.099995,  11, -0.09999939, 13
        };
        auto outputsTensor = eluOp->getOutput(0);
        verifyOutputs(outputsTensor, expectedValues);
    }

    SECTION("SELU") {
        auto seluOp = new SeluOp<ReferenceBackend>("selu", workspace());
        seluOp->setInput(input0, 0);
        seluOp->createAllTensors();
        allocateAllTensors<float>(seluOp);
        seluOp->run();
        std::vector<float> expectedValues{
            -1.111354, -1.520198, -1.6706,   4.2028,  5.2535,    6.3042, 7.3549,
            8.4056,    9.4563,    -1.758056, 11.5577, -1.758126, 13.6591
        };
        auto outputsTensor = seluOp->getOutput(0);
        verifyOutputs(outputsTensor, expectedValues);
    }
}

TEST_CASE_METHOD(SmaugTest, "Reference saturating nonlinearities", "[refop]") {
    TensorShape inputShape({ 1, 11 }, DataLayout::NC);
    Tensor* input0 = new Tensor("input0", inputShape);
    input0->allocateStorage<float>();
    input0->fillData<float>(
            { -1, -0.8, -0.6, -0.4, -0.2, 0, 0.2, 0.4, 0.6, 0.8, 1 });
    workspace()->addTensor(input0);
    SECTION("Sigmoid") {
        auto sigmoidOp = new SigmoidOp<ReferenceBackend>("sigmoid", workspace());
        sigmoidOp->setInput(input0, 0);
        sigmoidOp->createAllTensors();
        allocateAllTensors<float>(sigmoidOp);
        sigmoidOp->run();

        std::vector<float> expectedValues{ 0.2689414,  0.3100255, 0.354344,
                                           0.40131234, 0.4501660, 0.5,
                                           0.549834,   0.5986876, 0.6456563,
                                           0.6899744,  0.7310586 };
        auto outputsTensor = sigmoidOp->getOutput(0);
        verifyOutputs(outputsTensor, expectedValues);
    }

    SECTION("Tanh") {
        auto tanhOp = new TanhOp<ReferenceBackend>("tanh", workspace());
        tanhOp->setInput(input0, 0);
        tanhOp->createAllTensors();
        allocateAllTensors<float>(tanhOp);
        tanhOp->run();

        std::vector<float> expectedValues{ -0.761594, -0.6640367,   -0.5370496,
                                           -0.379949, -0.1973753, 0,
                                           0.1973753, 0.379949,   0.5370496,
                                           0.6640367, 0.761594 };
        auto outputsTensor = tanhOp->getOutput(0);
        verifyOutputs(outputsTensor, expectedValues);
    }

    SECTION("Hard Tanh") {
        auto hardTanhOp = new HardTanhOp<ReferenceBackend>(
                "hardTanh", workspace(), -0.5, 0.5);
        hardTanhOp->setInput(input0, 0);
        hardTanhOp->createAllTensors();
        allocateAllTensors<float>(hardTanhOp);
        hardTanhOp->run();

        std::vector<float> expectedValues{ -0.5, -0.5, -0.5, -0.4, -0.2, 0,
                                           0.2,  0.4,  0.5,  0.5,  0.5 };
        auto outputsTensor = hardTanhOp->getOutput(0);
        verifyOutputs(outputsTensor, expectedValues);
    }
}