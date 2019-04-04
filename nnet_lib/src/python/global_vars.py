"""Here we define global variables.

Currently it contains:
  1) A global active graph
  2) Alignment information for various backends.
  3) Input/output layouts for operators of various backends.
"""

from types_pb2 import *
from datatypes import LayoutSet,OperatorLayouts

# This keeps track of the current active graph. Currently we only support
# one active graph, we can change that if later we need multiple graphs.
active_graph = None

def get_graph():
  """Obtain the current active graph."""
  return active_graph

def set_graph(graph):
  """Set the active graph."""
  global active_graph
  active_graph = graph

def clear_graph():
  """Clear the active graph.

  This will be used when the graph context is cleaned up.
  """
  global active_graph
  active_graph = None

# Alignment information for various backends.
backend_alignment = {"Reference": 0, "SMV": 8}

# Input/output layouts for various backends.
backend_layouts = {
    "Reference": {
        Convolution3d: OperatorLayouts(NCHW, NCHW),
        ConvolutionDepthwise: OperatorLayouts(NCHW, NCHW),
        MaxPooling: OperatorLayouts(NCHW, NCHW),
        AveragePooling: OperatorLayouts(NCHW, NCHW),
        InnerProduct: OperatorLayouts(NC, NC),
        BatchNorm: OperatorLayouts(NCHW, NCHW),
        Data: OperatorLayouts(X, X),
        ReLU: OperatorLayouts(X, X),
        LReLU: OperatorLayouts(X, X),
        ELU: OperatorLayouts(X, X),
        SELU: OperatorLayouts(X, X),
        Tanh: OperatorLayouts(X, X),
        HardTanh: OperatorLayouts(X, X),
        Sigmoid: OperatorLayouts(X, X),
        Softmax: OperatorLayouts(X, X),
        EltwiseAdd: OperatorLayouts(X, X)
    },
    "SMV": {
        Convolution3d: OperatorLayouts(NHWC, NHWC),
        ConvolutionDepthwise: OperatorLayouts(NHWC, NHWC),
        MaxPooling: OperatorLayouts(NHWC, NHWC),
        AveragePooling: OperatorLayouts(NHWC, NHWC),
        InnerProduct: OperatorLayouts(NC, NC),
        BatchNorm: OperatorLayouts(NCHW, NCHW),
        Data: OperatorLayouts(X, X),
        ReLU: OperatorLayouts(X, X),
        LReLU: OperatorLayouts(X, X),
        ELU: OperatorLayouts(X, X),
        SELU: OperatorLayouts(X, X),
        Tanh: OperatorLayouts(X, X),
        HardTanh: OperatorLayouts(X, X),
        Sigmoid: OperatorLayouts(X, X),
        Softmax: OperatorLayouts(X, X),
        EltwiseAdd: OperatorLayouts(X, X)
    }
}