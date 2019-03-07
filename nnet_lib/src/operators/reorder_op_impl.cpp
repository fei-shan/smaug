#include "core/tensor.h"
#include "operators/reorder_op_impl.h"

namespace smaug {

void convertNchwToNhwc(Tensor* input, Tensor* output) {
    DataType datatype = input->getDataType();
    assert(input->ndims() == output->ndims() && input->ndims() == 4);
    switch (datatype) {
        case Float16:
            convertNchwToNhwcImpl<float16>(input, output);
            return;
        case Float32:
            convertNchwToNhwcImpl<float>(input, output);
            return;
        case Float64:
            convertNchwToNhwcImpl<double>(input, output);
            return;
        case Int32:
            convertNchwToNhwcImpl<int>(input, output);
            return;
        case Int64:
            convertNchwToNhwcImpl<int64_t>(input, output);
            return;
        default:
            assert(false && "Unknown data format!");
    }
}

void convertNhwcToNchw(Tensor* input, Tensor* output) {
    DataType datatype = input->getDataType();
    assert(input->ndims() == output->ndims() && input->ndims() == 4);
    switch (datatype) {
        case Float16:
            convertNhwcToNchwImpl<float16>(input, output);
            return;
        case Float32:
            convertNhwcToNchwImpl<float>(input, output);
            return;
        case Float64:
            convertNhwcToNchwImpl<double>(input, output);
            return;
        case Int32:
            convertNhwcToNchwImpl<int>(input, output);
            return;
        case Int64:
            convertNhwcToNchwImpl<int64_t>(input, output);
            return;
        default:
            assert(false && "Unknown data format!");
    }
}

void flatten(Tensor* input, Tensor* output) {
    DataType datatype = input->getDataType();
    assert(input->ndims() == 4 && output->ndims() == 2);
    switch (datatype) {
        case Float16:
            flattenImpl<float16>(input, output);
            return;
        case Float32:
            flattenImpl<float>(input, output);
            return;
        case Float64:
            flattenImpl<double>(input, output);
            return;
        case Int32:
            flattenImpl<int>(input, output);
            return;
        case Int64:
            flattenImpl<int64_t>(input, output);
            return;
        default:
            assert(false && "Unknown data format!");
    }
}

}  // namespace smaug
