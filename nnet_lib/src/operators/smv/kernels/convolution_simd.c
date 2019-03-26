#include <stdbool.h>
#include <stdio.h>

#include "operators/common.h"

#define NUM_PE_INSTS 8
#define NUM_MACC_INSTS 4

#ifdef __cplusplus
extern "C" {
#endif

// Perform a 3D convolution with one kernel on an image, with reduction in NHWC
// format. This is the vectorized implementation.
//
// Args:
//   a: 3D array, indexed as [row][col][channel].
//   kernels: A 3D kernel, indexed as [row][col][channel].
//   curr_layer: Layer (or partial layer) configuration.
//   kern_start: If the kernel array contains weights for multiple output
//      feature maps, start from this one.
//   result: a 3D array indexed as [channel][row][col].
//
// Returns:
//   The reduced 3D convolution values in result in NCHW format.
void smv_conv3d_f32_nhwc_same_padding_vec_fxp(float* inputs,
                                              float* weights,
                                              float* results,
                                              int inputs_dims[4],
                                              int weights_dims[4],
                                              int results_dims[4],
                                              int inputs_pad,
                                              int weights_pad,
                                              int results_pad,
                                              int row_stride,
                                              int col_stride,
                                              int ofmap_start,
                                              int ifmap_start,
                                              bool accumulate) {
    int result_rows = results_dims[1];
    int result_cols = results_dims[2];
    int result_height = results_dims[3];

    int k_rows = weights_dims[1];
    int k_cols = weights_dims[2];
    int k_height = weights_dims[3];
    int k_pad = weights_pad;

    int a_rows = inputs_dims[1];
    int a_cols = inputs_dims[2];
    int a_height = inputs_dims[3];
    int a_pad = inputs_pad;

    int total_row_pad = k_rows - 1;
    int total_col_pad = k_cols - 1;
    int left_pad = k_rows / 2;
    int right_pad = total_col_pad - left_pad;
    int top_pad = k_cols / 2;
    int bottom_pad = total_row_pad - top_pad;

    int end_row = a_rows + top_pad + bottom_pad - k_rows + 1;
    int end_col = a_cols + left_pad + right_pad - k_cols + 1;

    int valid_row_end = a_rows - 1;
    int valid_col_end = a_cols - 1;

    int in_row, in_col;
    const int pe_depth = VECTOR_SIZE * NUM_MACC_INSTS;
    // If we have less than four channels, don't run the extra ones.
    const int kEffNumPeInsts = min2(result_height - ofmap_start, NUM_PE_INSTS);
    const v8fp_t zero = { 0, 0, 0, 0, 0, 0, 0, 0 };

    // Kernels and input are in NHWC.
    VEC_ARRAY_4D(v8fp_t, _kernels, weights, k_rows, k_cols, k_height + k_pad);
    // TODO: Support input batches.
    VEC_ARRAY_3D(v8fp_t, _a, inputs, a_cols, a_height + a_pad);
    // Results in NHWC.
    VEC_ARRAY_3D(
            v8fp_t, _result, results, result_cols, result_height + results_pad);
    int num_chan_blocks = (k_height - 1) / pe_depth;

    k_col:
    for (int kern_row = 0; kern_row < k_rows; kern_row++) {  // Kernel rows
        k_row:
        for (int kern_col = 0; kern_col < k_cols; kern_col++) {  // Kernel cols
            // This loops over all the input channels in groups of VECTOR_SIZE
            // * NUM_MACC_INSTS.
            pe_iteration:
            for (int ifmap_iters = 0; ifmap_iters < num_chan_blocks + 1;
                 ifmap_iters++) {
                bool start_from_zero = (!accumulate && kern_row == 0 &&
                                        kern_col == 0 && ifmap_iters == 0);
                int ifmap_offset = (ifmap_iters * pe_depth) / VECTOR_SIZE;
                int out_i = 0;  // The result row.

                int max_ch_grp = NUM_MACC_INSTS;
                // This is just computing the remaining groups of channels on
                // the last iteration.
                if (ifmap_iters == num_chan_blocks) {
                    max_ch_grp = FRAC_CEIL(
                            (k_height - ifmap_iters * pe_depth), VECTOR_SIZE);
                }

                // Load in all the weights at once before beginning the input
                // loop.
                v8fp_t kernel_reg[NUM_PE_INSTS][NUM_MACC_INSTS] = {
                    { zero }, { zero }, { zero }, { zero },
                    { zero }, { zero }, { zero }, { zero }
                };
                const v8fp_t zero = (v8fp_t){ 0, 0, 0, 0, 0, 0, 0, 0 };
                load_kern_pe:
                for (int pe_id = 0; pe_id < kEffNumPeInsts; pe_id++) {
                    load_kern_mu:
                    for (int macc_idx = 0; macc_idx < NUM_MACC_INSTS;
                         macc_idx++) {
                        kernel_reg[pe_id][macc_idx] =
                                (macc_idx >= max_ch_grp)
                                        ? zero
                                        : _kernels[ofmap_start + pe_id][kern_row]
                                                  [kern_col]
                                                  [ifmap_offset + macc_idx];
                    }
                }

                conv3d_row:
                for (int out_row = 0; out_row < end_row; out_row += row_stride) {
                    int out_j = 0;  // The result col.

                    // We buffer 8 (i.e., the number of PEs) partial sums into
                    // a vector register.
                    v8fp_t results_buffer;

                    conv3d_col:
                    for (int out_col = 0; out_col < end_col;
                         out_col += col_stride) {
                        // Local Regs. These should always be sized the same (so
                        // NUM_PE_INSTS, rather than kNumEffPeInsts).
                        v8fp_t product_reg[NUM_PE_INSTS][NUM_MACC_INSTS];
                        v8fp_t act_reg[NUM_MACC_INSTS];
                        results_buffer =
                                start_from_zero
                                        ? zero
                                        : _result[out_i][out_j][ofmap_start];
                        in_row = out_row - top_pad + kern_row;
                        in_col = out_col - left_pad + kern_col;
                        bool in_padding_row =
                                in_row < 0 || in_row > valid_row_end;
                        bool in_padding_col =
                                in_col < 0 || in_col > valid_col_end;

                        // Load in the activations first, then broadcast them
                        // to all the PEs.
                        load_act_mu:
                        for (int macc_idx = 0; macc_idx < NUM_MACC_INSTS;
                             macc_idx++) {
                            bool is_padding = in_padding_row ||
                                              in_padding_col ||
                                              macc_idx >= max_ch_grp;
                            act_reg[macc_idx] =
                                    (is_padding)
                                            ? zero
                                            : _a[in_row][in_col]
                                                [ifmap_offset + macc_idx];
                        }

                        pe_groups:
                        for (int pe_id = 0; pe_id < kEffNumPeInsts; pe_id++) {
                            mu_groups:
                            for (int macc_idx = 0; macc_idx < NUM_MACC_INSTS;
                                 macc_idx++) {
                                product_reg[pe_id][macc_idx] =
                                        kernel_reg[pe_id][macc_idx] *
                                        act_reg[macc_idx];
                            }
                            v8fp_t accum_vec_reg = zero;
                            reduction_1:
                            for (int macc_idx = 0; macc_idx < NUM_MACC_INSTS;
                                 macc_idx++) {
                                accum_vec_reg += product_reg[pe_id][macc_idx];
                            }

                            float accum_reg = 0;
                            reduction_2:
                            for (int vec_i = 0; vec_i < VECTOR_SIZE; vec_i++) {
                                accum_reg += accum_vec_reg[vec_i];
                            }
                            results_buffer[pe_id] += accum_reg;
                        }

                        // Write the results back to scratchpad.
                        _result[out_i][out_j][ofmap_start] = results_buffer;
                        out_j++;
                    }
                    out_i++;
                    out_j = 0;
                }
            }
        }
    }
}

#ifdef __cplusplus
}  // extern "C"
#endif
