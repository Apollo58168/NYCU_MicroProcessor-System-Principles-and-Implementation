/*
 * conv3d_dsa_driver_simple.h
 * 
 * Simplified driver for 3D convolution DSA using FMA-only approach.
 * Software controls all loops, hardware only accelerates multiply-add.
 * 
 * MMIO Register Map (Base address: 0xC4000000):
 *   0x00: Operand A (write)
 *   0x04: Operand B (write - triggers computation)
 *   0x08: Result (read - blocks until ready)
 *   0x0C: Reset accelerator (write 1 to reset)
 */

#ifndef CONV3D_DSA_DRIVER_H
#define CONV3D_DSA_DRIVER_H

#include <stdint.h>

// DSA MMIO base address
#define DSA_BASE_ADDR 0xC4000000

// MMIO register offsets
#define DSA_REG_A         0x00
#define DSA_REG_B         0x04
#define DSA_REG_RESULT    0x08
#define DSA_REG_RESET     0x0C

// Helper macros for MMIO access
#define DSA_WRITE(offset, value) \
    (*((volatile uint32_t*)(DSA_BASE_ADDR + (offset))) = (value))

#define DSA_READ(offset) \
    (*((volatile uint32_t*)(DSA_BASE_ADDR + (offset))))

/**
 * Reset the DSA accelerator
 */
static inline void dsa_reset(void) {
    DSA_WRITE(DSA_REG_RESET, 1);
}

/**
 * Perform a single FMA operation: result = a * b + previous_accumulator
 * Returns the result as float
 * 
 * IMPORTANT: This function waits for the result to be ready before returning.
 * The hardware automatically uses previous result as accumulator (C).
 * Call dsa_reset() before starting a new accumulation sequence.
 */
static inline float dsa_fma(float a, float b, float accumulator) {
    union {
        float f;
        uint32_t u;
    } ua, ub, ures;
    
    // Convert floats to uint32_t for MMIO
    ua.f = a;
    ub.f = b;
    
    // Write A register
    DSA_WRITE(DSA_REG_A, ua.u);
    
    // Write B and trigger computation
    // Hardware will automatically use previous result as C
    DSA_WRITE(DSA_REG_B, ub.u);
    
    // MUST wait for result - hardware stalls if result not ready
    // The read will block until waiting_for_result becomes false
    ures.u = DSA_READ(DSA_REG_RESULT);
    
    return ures.f;
}
/**
 * Software-controlled 3D convolution using DSA for FMA acceleration
 * 
 * Parameters match the original conv_3d function:
 *   out_channels: Number of output feature maps
 *   in_channels: Number of input feature maps
 *   input: Input data [in_h][in_w][in_channels]
 *   output: Output data [out_h][out_w][out_channels]
 *   kernel: Convolution kernel [kernel_size][kernel_size][in_channels][out_channels]
 *   bias: Bias values [out_channels]
 *   in_h, in_w: Input dimensions
 *   out_h, out_w: Output dimensions
 *   kernel_size: Size of convolution kernel (typically 5)
 */
static inline void dsa_conv3d_execute(
    int out_channels, int in_channels,
    float* input, float* output,
    float* kernel, float* bias,
    int in_h, int in_w,
    int out_h, int out_w,
    int kernel_size)
{
    // Reset DSA before starting
    dsa_reset();
    
    // Loop over all output pixels and channels
    for (int oc = 0; oc < out_channels; oc++) {
        for (int oh = 0; oh < out_h; oh++) {
            for (int ow = 0; ow < out_w; ow++) {
                
                // Start with bias
                float sum = bias[oc];
                
                // Convolve over kernel and input channels
                for (int ic = 0; ic < in_channels; ic++) {
                    for (int kh = 0; kh < kernel_size; kh++) {
                        for (int kw = 0; kw < kernel_size; kw++) {
                            
                            // Input coordinates
                            int ih = oh + kh;
                            int iw = ow + kw;
                            
                            // Get input value: input[ih][iw][ic]
                            int input_idx = (ih * in_w + iw) * in_channels + ic;
                            float input_val = input[input_idx];
                            
                            // Get kernel weight: kernel[kh][kw][ic][oc]
                            int kernel_idx = ((kh * kernel_size + kw) * in_channels + ic) * out_channels + oc;
                            float kernel_val = kernel[kernel_idx];
                            
                            // Use DSA for FMA: sum = input_val * kernel_val + sum
                            sum = dsa_fma(input_val, kernel_val, sum);
                        }
                    }
                }
                
                // Write output: output[oh][ow][oc]
                int output_idx = (oh * out_w + ow) * out_channels + oc;
                output[output_idx] = sum;
            }
        }
    }
}

#endif // CONV3D_DSA_DRIVER_H
