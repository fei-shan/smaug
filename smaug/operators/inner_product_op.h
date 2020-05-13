#ifndef _OPERATORS_INNER_PRODUCT_OP_H_
#define _OPERATORS_INNER_PRODUCT_OP_H_

#include "core/backend.h"
#include "core/operator.h"
#include "core/tensor.h"
#include "core/tensor_utils.h"
#include "core/workspace.h"
#include "operators/common.h"
#include "operators/fused_activation_op.h"

namespace smaug {

template <typename Backend>
class InnerProductOp : public FusedActivationOp {
   public:
    InnerProductOp(const std::string& name, Workspace* workspace)
            : FusedActivationOp(name, OpType::InnerProduct, workspace),
              numOutputs(0), weightsTensorsCreated(false),
              outputTensorsCreated(false), weightsName(name + "/weights"),
              sampling({ NoSampling, 1 }) {
        inputs.resize(kNumInputs, nullptr);
        outputs.resize(kNumOutputs, nullptr);
    }

    void setNumOutputs(int _outputs) { numOutputs = _outputs; }

    void run() override {}
    bool validate() override { return numOutputs > 0 && Operator::validate(); }
    TensorShape inferOutputShape() const {
        const TensorShape& shape = getInput(Inputs)->getShape();
        assert(shape.getLayout() == DataLayout::NC);
        return TensorShape(
                { shape[0], numOutputs }, DataLayout::NC, Backend::Alignment);
    }

    TensorShape inferWeightsShape() const {
        const TensorShape& shape = getInput(Inputs)->getShape();
        assert(shape.getLayout() == DataLayout::NC);
        std::vector<int> outputDims;
        DataLayout outLayout;
        if (Backend::TransposeFCWeights) {
            outputDims = { numOutputs, shape[1] };
            outLayout = DataLayout::NC;
        } else {
            outputDims = { shape[1], numOutputs };
            outLayout = DataLayout::CN;
        }
        return TensorShape(outputDims, outLayout, Backend::Alignment);
    }

    void createWeightsTensors() {
        if (inputs.at(Weights))
            return;
        TensorShape shape = inferWeightsShape();
        Tensor* weights = new Tensor(weightsName, shape);
        workspace->addTensor(weights);
        inputs.at(Weights) = weights;
        weightsTensorsCreated = true;
    }

    void createOutputTensors() {
        if (outputs.at(Outputs))
            return;
        TensorShape shape = inferOutputShape();
        Tensor* output = new Tensor(name, shape);
        workspace->addTensor(output);
        outputs[Outputs] = output;
    }

    void createAllTensors() override {
        createWeightsTensors();
        createOutputTensors();
    }

    int getNumOutputs() const { return numOutputs; }

    void printSummary(std::ostream& out) const override {
        const TensorShape& weightsShape = inputs.at(Weights)->getShape();
        const TensorShape& outputShape = outputs.at(Outputs)->getShape();
        out << this->name << " (InnerProduct)\t\t" << outputShape << "\t\t"
            << weightsShape << "\t\t" << weightsShape.size() << "\n";
    }

    std::vector<TensorBase*> getParameterizableInputs() override {
        return { inputs[Weights] };
    }

    bool isSamplingSupported() const override { return true; }
    void setSamplingInfo(const SamplingInfo& _sampling) override {
        sampling = _sampling;
    }

   public:
    enum { Inputs, Weights, kNumInputs };
    enum { Outputs, kNumOutputs };

   protected:
    int numOutputs;
    bool weightsTensorsCreated;
    bool outputTensorsCreated;
    std::string weightsName;
    SamplingInfo sampling;
};

REGISTER_SPECIAL_OP(InnerProductOp, ReferenceBackend);

}  // namespace smaug

#endif