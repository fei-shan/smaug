import numpy as np
from warnings import warn

from smaug.core.types_pb2 import *
from smaug.core.node_pb2 import *
from smaug.python.global_vars import *
from smaug.python.tensor import Tensor
from smaug.python.ops.common import *
from smaug.python.ops.activation_ops import *

def to_padding_type(padding):
  if padding == "same":
    return SamePadding
  elif padding == "valid":
    return ValidPadding
  else:
    return UnknownPadding

def convolution(
    input_tensor, filter_tensor, stride, padding, activation=None,
    activation_params=None, name="conv"):
  def compute_output_dim(input_dim, weight_dim, stride, padding):
    pad = 0
    if to_padding_type(padding) == SamePadding:
      pad = weight_dim - 1
    return (input_dim - weight_dim + pad) // stride + 1

  input_tensor, filter_tensor = check_and_add_layout_transform(
      name=name, op=Convolution3d, input_tensors=[input_tensor, filter_tensor])

  row_idx = 2 if input_tensor.shape.layout == NCHW else 1
  col_idx = 3 if input_tensor.shape.layout == NCHW else 2
  chan_idx = 1 if input_tensor.shape.layout == NCHW else 3
  assert input_tensor.dims(chan_idx) == filter_tensor.dims(chan_idx), (
      "The weights must have the same number of channels as the inputs.")
  output_rows = compute_output_dim(input_tensor.shape.dims[row_idx],
                                   filter_tensor.shape.dims[row_idx], stride[0],
                                   padding)
  output_cols = compute_output_dim(input_tensor.shape.dims[col_idx],
                                   filter_tensor.shape.dims[col_idx], stride[1],
                                   padding)
  output_layout = input_tensor.shape.layout
  if output_layout == NCHW:
    output_tensor_dims = [
        input_tensor.shape.dims[0], filter_tensor.shape.dims[0], output_rows,
        output_cols
    ]
  elif output_layout == NHWC:
    output_tensor_dims = [
        input_tensor.shape.dims[0], output_rows, output_cols,
        filter_tensor.shape.dims[0]
    ]
  else:
    assert False, "Unsupported output layout!"
  params = Params()
  params.conv_params.padding = to_padding_type(padding)
  params.conv_params.stride.extend(stride)
  if activation != None:
    params.act_params.activation = activation
    set_activation_params(activation, params.act_params, activation_params)

  return add_node(
      name=name, op=Convolution3d, input_tensors=[input_tensor, filter_tensor],
      output_tensors_dims=[output_tensor_dims],
      output_tensor_layout=output_layout, params=params)[0]

def batch_norm(
    input_tensor, mean_tensor, var_tensor, gamma_tensor, beta_tensor,
    activation=None, activation_params=None, name="batch_norm"):
  assert (len(mean_tensor.shape.dims) == 2 and len(var_tensor.shape.dims) == 2
          and len(gamma_tensor.shape.dims) == 2
          and len(beta_tensor.shape.dims) == 2)
  # If the batch norm is after a FC layer, then the input/output tensors should
  # be in NC. Otherwise, the batch norm is after a convolution layer, and we
  # check backend_layouts for expected input/output layouts and do layout
  # transformation if needed.
  post_fc = False
  if len(input_tensor.shape.dims) == 2:
    post_fc = True

  if not post_fc:
    input_tensor = check_and_add_layout_transform(
        name=name, op=BatchNorm, input_tensors=[input_tensor])[0]

  output_layout = UnknownLayout
  output_layout = NC if post_fc else input_tensor.shape.layout
  params = Params()
  if activation != None:
    params.act_params.activation = activation
    set_activation_params(activation, params.act_params, activation_params)
  return add_node(
      name=name, op=BatchNorm, input_tensors=[
          input_tensor, mean_tensor, var_tensor, gamma_tensor, beta_tensor
      ], output_tensors_dims=[input_tensor.shape.dims],
      output_tensor_layout=output_layout, params=params)[0]

def max_pool(input_tensor, pool_size, stride, name="max_pool"):
  def compute_output_dim(input_dim, pool_size, stride):
    return (input_dim - pool_size) // stride + 1

  input_tensor = check_and_add_layout_transform(
      name=name, op=MaxPooling, input_tensors=[input_tensor])[0]

  row_idx = 2 if input_tensor.shape.layout == NCHW else 1
  col_idx = 3 if input_tensor.shape.layout == NCHW else 2
  output_rows = compute_output_dim(input_tensor.shape.dims[row_idx],
                                   pool_size[0], stride[0])
  output_cols = compute_output_dim(input_tensor.shape.dims[col_idx],
                                   pool_size[1], stride[1])
  output_layout = input_tensor.shape.layout
  if output_layout == NCHW:
    output_tensor_dims = [
        input_tensor.shape.dims[0], input_tensor.shape.dims[1], output_rows,
        output_cols
    ]
  else:
    output_tensor_dims = [
        input_tensor.shape.dims[0], output_rows, output_cols,
        input_tensor.shape.dims[3]
    ]
  params = Params()
  params.pool_params.stride.extend(stride)
  params.pool_params.pool_size.extend(pool_size)
  return add_node(
      name=name, op=MaxPooling, input_tensors=[input_tensor],
      output_tensors_dims=[output_tensor_dims],
      output_tensor_layout=output_layout, params=params)[0]

def mat_mul(
    input_tensor, weight_tensor, activation=None, activation_params=None,
    name="mat_mul"):
  input_tensor, weight_tensor = check_and_add_layout_transform(
      name=name, op=InnerProduct, input_tensors=[input_tensor, weight_tensor])

  weight_layout = weight_tensor.shape.layout
  actIdx = 1 if weight_layout == NC else 0
  neuronIdx = 0 if weight_layout == NC else 1
  assert (len(input_tensor.shape.dims) == 2
          and len(weight_tensor.shape.dims) == 2
          and input_tensor.shape.dims[1] == weight_tensor.shape.dims[actIdx])
  output_tensor_dims = [
      input_tensor.shape.dims[0], weight_tensor.shape.dims[neuronIdx]
  ]
  params = Params()
  if activation != None:
    params.act_params.activation = activation
    set_activation_params(activation, params.act_params, activation_params)
  return add_node(
      name=name, op=InnerProduct, input_tensors=[input_tensor, weight_tensor],
      output_tensors_dims=[output_tensor_dims], output_tensor_layout=NC,
      params=params)[0]