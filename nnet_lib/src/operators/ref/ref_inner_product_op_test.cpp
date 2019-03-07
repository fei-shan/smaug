#include "catch.hpp"
#include "core/backend.h"
#include "core/tensor.h"
#include "core/smaug_test.h"
#include "operators/inner_product_op.h"

using namespace smaug;

TEST_CASE_METHOD(SmaugTest, "Reference inner product operator", "[refop]") {
    auto matMulOp = new InnerProductOp<ReferenceBackend>("matmul", workspace());
    TensorShape inputShape({ 1, 10 }, DataLayout::NC);
    Tensor* input = new Tensor("input", inputShape);
    input->allocateStorage<float>();
    // Input data looks like:
    input->fillData<float>({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });
    workspace()->addTensor(input);
    matMulOp->setInput(input, 0);
    SECTION("10x10, constant weights per neuron") {
        matMulOp->setNumOutputs(10);
        matMulOp->createAllTensors();
        allocateAllTensors<float>(matMulOp);
        auto weightsTensor = matMulOp->getInput(1);
        weightsTensor->fillData<float>({
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
        });

        matMulOp->run();

        // Expected output (with zero padding):
        //
        // (1...10) * 1 = 55
        // (1...10) * 2 = 110
        // (1...10) * 3 = 165
        // (1...10) * 4 = 220
        // (1...10) * 5 = 275
        // (1...10) * 6 = 330
        // (1...10) * 7 = 385
        // (1...10) * 8 = 440
        // (1...10) * 9 = 495
        // (1...10) * 10 = 550
        std::vector<float> expectedValues{ 55,  110, 165, 220, 275,
                                           330, 385, 440, 495, 550 };
        auto outputsTensor = matMulOp->getOutput(0);
        verifyOutputs(outputsTensor, expectedValues);
    }

    SECTION("10x10, distinct weights per neuron") {
        matMulOp->setNumOutputs(10);
        matMulOp->createAllTensors();
        allocateAllTensors<float>(matMulOp);
        auto weightsTensor = matMulOp->getInput(1);
        weightsTensor->fillData<float>({
            1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
            2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
            3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
            4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
            5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
            6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
            8,  9,  10, 11, 12, 13, 14, 15, 16, 17,
            9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
            10, 11, 12, 13, 14, 15, 16, 17, 18, 19
        });

        matMulOp->run();

        // Expected output:
        //
        // 1*1 + 2*2 + 3*3 + ... 10*10 = 385
        // 1*2 + 2*3 + 3*4 + ... 10*11 = 385 + (1+...+10) = 385+55 = 440
        // 1*3 + 2*4 + 3*4 + ... 10*11 = 440 + 55 = 495
        // 1*4 + 2*3 + 3*4 + ... 10*11 = 550
        // 1*5 + 2*3 + 3*4 + ... 10*11 = 605
        // 1*6 + 2*3 + 3*4 + ... 10*11 = 660
        // 1*7 + 2*3 + 3*4 + ... 10*11 = 715
        // 1*8 + 2*3 + 3*4 + ... 10*11 = 770
        // 1*9 + 2*3 + 3*4 + ... 10*11 = 825
        // 1*10 + 2*3 + 3*4 + ... 10*11 = 880
        // 385 440 495 550 605 660 715 770 825 880
        std::vector<float> expectedValues{ 385, 440, 495, 550, 605,
                                           660, 715, 770, 825, 880 };
        auto outputsTensor = matMulOp->getOutput(0);
        verifyOutputs(outputsTensor, expectedValues);
    }
}
