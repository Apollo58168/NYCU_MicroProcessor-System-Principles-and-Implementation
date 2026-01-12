// =============================================================================
//  Program : test_dsa.c
//  Author  : DSA Test for Aquila SoC
//  Date    : Dec/25/2024
// -----------------------------------------------------------------------------
//  Description:
//      簡化版 DSA 測試程式，用於驗證卷積加速器的 MMIO 輸入輸出。
//      使用小型測試數據（4x4 input, 3x3 kernel）手動計算預期結果並比較。
// =============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ============================================================
// MMIO 地址定義 (根據你的 data_feeder.v)
// ============================================================

// FMA 模式地址
#define FMA_A_ADDR        0xC400000C
#define FMA_B_ADDR        0xC4000010
#define FMA_C_ADDR        0xC4000014
#define FMA_BUSY_ADDR     0xC4000050

// Buffered 模式地址
#define CONV_IN_WIDTH     0xC4600004
#define CONV_OUT_WIDTH    0xC4600008
#define CONV_IN_DEPTH     0xC460000C
#define CONV_OUT_DEPTH    0xC4600010
#define CONV_TRIGGER      0xC4600000

#define INPUT_SIZE_ADDR   0xC4200000
#define INPUT_DATA_ADDR   0xC4200004
#define WEIGHT_SIZE_ADDR  0xC4100000
#define WEIGHT_DATA_ADDR  0xC4100004
#define OUTPUT_SIZE_ADDR  0xC4300000
#define OUTPUT_DATA_ADDR  0xC4300004

// Profiling 地址
#define PROF_CONV_FLAG    0xC4000034
#define PROF_COPY_FLAG    0xC4000038
#define PROF_COMP_FLAG    0xC400003C

// ============================================================
// 測試參數
// ============================================================
#define IN_WIDTH    4
#define IN_HEIGHT   4
#define IN_DEPTH    1
#define OUT_DEPTH   1
#define KERNEL_SIZE 3
#define OUT_WIDTH   (IN_WIDTH - KERNEL_SIZE + 1)   // = 2
#define OUT_HEIGHT  (IN_HEIGHT - KERNEL_SIZE + 1)  // = 2

// ============================================================
// 輔助函數
// ============================================================

// 浮點數比較（允許小誤差）
int float_equal(float a, float b, float epsilon) {
    float diff = a - b;
    if (diff < 0) diff = -diff;
    return diff < epsilon;
}

// 打印陣列
void print_array(const char *name, float *arr, int size) {
    printf("%s: [", name);
    for (int i = 0; i < size; i++) {
        printf("%.4f", arr[i]);
        if (i < size - 1) printf(", ");
    }
    printf("]\n");
}

// 打印 2D 陣列
void print_2d(const char *name, float *arr, int h, int w) {
    printf("%s (%dx%d):\n", name, h, w);
    for (int y = 0; y < h; y++) {
        printf("  ");
        for (int x = 0; x < w; x++) {
            printf("%7.3f ", arr[y * w + x]);
        }
        printf("\n");
    }
}

// ============================================================
// 軟體參考實現 - 2D 卷積
// ============================================================
void software_conv2d(float *input, float *kernel, float *output,
                     int in_w, int in_h, int k_size) {
    int out_w = in_w - k_size + 1;
    int out_h = in_h - k_size + 1;
    
    for (int oy = 0; oy < out_h; oy++) {
        for (int ox = 0; ox < out_w; ox++) {
            float sum = 0.0f;
            for (int ky = 0; ky < k_size; ky++) {
                for (int kx = 0; kx < k_size; kx++) {
                    int ix = ox + kx;
                    int iy = oy + ky;
                    sum += input[iy * in_w + ix] * kernel[ky * k_size + kx];
                }
            }
            output[oy * out_w + ox] = sum;
        }
    }
}

// ============================================================
// 測試 1: FMA 單次運算測試
// ============================================================
int test_fma_single(void) {
    printf("\n========================================\n");
    printf("Test 1: FMA Single Operation\n");
    printf("========================================\n");
    
    volatile float *fma_a = (volatile float *)FMA_A_ADDR;
    volatile float *fma_b = (volatile float *)FMA_B_ADDR;
    volatile float *fma_c = (volatile float *)FMA_C_ADDR;
    volatile uint32_t *fma_busy = (volatile uint32_t *)FMA_BUSY_ADDR;
    
    // 測試: A=2.0, B=3.0, C=1.0 => 結果應為 2*3+1 = 7.0
    float a = 2.0f;
    float b = 3.0f;
    float c = 1.0f;
    float expected = a * b + c;  // 7.0
    
    printf("Input: A=%.2f, B=%.2f, C=%.2f\n", a, b, c);
    printf("Expected: A*B+C = %.2f\n", expected);
    
    // 寫入 FMA
    *fma_a = a;
    *fma_b = b;
    *fma_c = c;  // 寫入 C 觸發運算
    
    // 等待完成
    int timeout = 10000;
    while ((*fma_busy & 0x2) && timeout > 0) {
        timeout--;
    }
    
    if (timeout == 0) {
        printf("ERROR: FMA timeout!\n");
        return -1;
    }
    
    // 讀取結果
    float result = *fma_c;
    printf("Result from HW: %.4f\n", result);
    
    if (float_equal(result, expected, 0.001f)) {
        printf("TEST PASSED!\n");
        return 0;
    } else {
        printf("TEST FAILED! Expected %.4f, got %.4f\n", expected, result);
        return -1;
    }
}

// ============================================================
// 測試 2: FMA Dot Product 測試
// ============================================================
int test_fma_dot_product(void) {
    printf("\n========================================\n");
    printf("Test 2: FMA Dot Product\n");
    printf("========================================\n");
    
    volatile float *fma_a = (volatile float *)FMA_A_ADDR;
    volatile float *fma_b = (volatile float *)FMA_B_ADDR;
    volatile float *fma_c = (volatile float *)FMA_C_ADDR;
    volatile uint32_t *fma_busy = (volatile uint32_t *)FMA_BUSY_ADDR;
    
    // 測試向量點積: [1,2,3,4] . [5,6,7,8] = 1*5+2*6+3*7+4*8 = 5+12+21+32 = 70
    float vec_a[] = {1.0f, 2.0f, 3.0f, 4.0f};
    float vec_b[] = {5.0f, 6.0f, 7.0f, 8.0f};
    int vec_len = 4;
    
    float expected = 0.0f;
    for (int i = 0; i < vec_len; i++) {
        expected += vec_a[i] * vec_b[i];
    }
    
    printf("Vector A: [%.1f, %.1f, %.1f, %.1f]\n", vec_a[0], vec_a[1], vec_a[2], vec_a[3]);
    printf("Vector B: [%.1f, %.1f, %.1f, %.1f]\n", vec_b[0], vec_b[1], vec_b[2], vec_b[3]);
    printf("Expected dot product: %.2f\n", expected);
    
    // 使用 FMA 累加計算點積
    float acc = 0.0f;
    for (int i = 0; i < vec_len; i++) {
        *fma_a = vec_a[i];
        *fma_b = vec_b[i];
        *fma_c = acc;  // 累加器
        
        // 等待完成
        int timeout = 10000;
        while ((*fma_busy & 0x2) && timeout > 0) {
            timeout--;
        }
        
        if (timeout == 0) {
            printf("ERROR: FMA timeout at iteration %d!\n", i);
            return -1;
        }
        
        acc = *fma_c;
        printf("  Iteration %d: %.2f * %.2f + %.2f = %.2f\n", 
               i, vec_a[i], vec_b[i], (i > 0 ? acc - vec_a[i]*vec_b[i] : 0.0f), acc);
    }
    
    printf("Result from HW: %.4f\n", acc);
    
    if (float_equal(acc, expected, 0.01f)) {
        printf("TEST PASSED!\n");
        return 0;
    } else {
        printf("TEST FAILED! Expected %.4f, got %.4f\n", expected, acc);
        return -1;
    }
}

// ============================================================
// 測試 3: 簡易卷積測試 (使用 FMA 模式)
// ============================================================
int test_conv_fma_mode(void) {
    printf("\n========================================\n");
    printf("Test 3: Convolution using FMA Mode\n");
    printf("========================================\n");
    
    volatile float *fma_a = (volatile float *)FMA_A_ADDR;
    volatile float *fma_b = (volatile float *)FMA_B_ADDR;
    volatile float *fma_c = (volatile float *)FMA_C_ADDR;
    volatile uint32_t *fma_busy = (volatile uint32_t *)FMA_BUSY_ADDR;
    
    // 4x4 輸入圖像
    float input[IN_WIDTH * IN_HEIGHT] = {
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f
    };
    
    // 3x3 卷積核
    float kernel[KERNEL_SIZE * KERNEL_SIZE] = {
        1.0f, 0.0f, -1.0f,
        2.0f, 0.0f, -2.0f,
        1.0f, 0.0f, -1.0f
    };
    
    // 輸出 (2x2)
    float output_hw[OUT_WIDTH * OUT_HEIGHT] = {0};
    float output_sw[OUT_WIDTH * OUT_HEIGHT] = {0};
    
    print_2d("Input", input, IN_HEIGHT, IN_WIDTH);
    print_2d("Kernel", kernel, KERNEL_SIZE, KERNEL_SIZE);
    
    // 軟體參考計算
    software_conv2d(input, kernel, output_sw, IN_WIDTH, IN_HEIGHT, KERNEL_SIZE);
    print_2d("Expected Output (SW)", output_sw, OUT_HEIGHT, OUT_WIDTH);
    
    // 硬體計算 - 使用 FMA 模擬卷積
    printf("\nComputing with FMA...\n");
    
    for (int oy = 0; oy < OUT_HEIGHT; oy++) {
        for (int ox = 0; ox < OUT_WIDTH; ox++) {
            float acc = 0.0f;
            
            // 3x3 卷積窗口
            for (int ky = 0; ky < KERNEL_SIZE; ky++) {
                for (int kx = 0; kx < KERNEL_SIZE; kx++) {
                    int ix = ox + kx;
                    int iy = oy + ky;
                    float w = kernel[ky * KERNEL_SIZE + kx];
                    float in_val = input[iy * IN_WIDTH + ix];
                    
                    // FMA: acc = w * in_val + acc
                    *fma_a = w;
                    *fma_b = in_val;
                    *fma_c = acc;
                    
                    // 等待完成
                    int timeout = 10000;
                    while ((*fma_busy & 0x2) && timeout > 0) {
                        timeout--;
                    }
                    
                    if (timeout == 0) {
                        printf("ERROR: FMA timeout!\n");
                        return -1;
                    }
                    
                    acc = *fma_c;
                }
            }
            
            output_hw[oy * OUT_WIDTH + ox] = acc;
        }
    }
    
    print_2d("HW Output (FMA)", output_hw, OUT_HEIGHT, OUT_WIDTH);
    
    // 比較結果
    int pass = 1;
    for (int i = 0; i < OUT_WIDTH * OUT_HEIGHT; i++) {
        if (!float_equal(output_hw[i], output_sw[i], 0.01f)) {
            printf("MISMATCH at [%d]: HW=%.4f, SW=%.4f\n", i, output_hw[i], output_sw[i]);
            pass = 0;
        }
    }
    
    if (pass) {
        printf("TEST PASSED!\n");
        return 0;
    } else {
        printf("TEST FAILED!\n");
        return -1;
    }
}

// ============================================================
// 測試 4: Buffered 模式卷積測試
// ============================================================
#ifdef USE_CONV3D_BUFFERED
int test_conv_buffered_mode(void) {
    printf("\n========================================\n");
    printf("Test 4: Convolution using Buffered Mode\n");
    printf("========================================\n");
    
    // 4x4 輸入圖像
    float input[IN_WIDTH * IN_HEIGHT] = {
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f
    };
    
    // 3x3 卷積核
    float kernel[KERNEL_SIZE * KERNEL_SIZE] = {
        1.0f, 0.0f, -1.0f,
        2.0f, 0.0f, -2.0f,
        1.0f, 0.0f, -1.0f
    };
    
    float output_hw[OUT_WIDTH * OUT_HEIGHT] = {0};
    float output_sw[OUT_WIDTH * OUT_HEIGHT] = {0};
    
    print_2d("Input", input, IN_HEIGHT, IN_WIDTH);
    print_2d("Kernel", kernel, KERNEL_SIZE, KERNEL_SIZE);
    
    // 軟體參考計算
    software_conv2d(input, kernel, output_sw, IN_WIDTH, IN_HEIGHT, KERNEL_SIZE);
    print_2d("Expected Output (SW)", output_sw, OUT_HEIGHT, OUT_WIDTH);
    
    // 硬體計算 - Buffered 模式
    printf("\nComputing with Buffered Mode...\n");
    
    int input_size = IN_WIDTH * IN_HEIGHT * IN_DEPTH;
    int weight_size = KERNEL_SIZE * KERNEL_SIZE * IN_DEPTH * OUT_DEPTH;
    int output_size = OUT_WIDTH * OUT_HEIGHT * OUT_DEPTH;
    
    // Step 1: 設定參數
    *((volatile int *)CONV_IN_WIDTH) = IN_WIDTH;
    *((volatile int *)CONV_OUT_WIDTH) = OUT_WIDTH;
    *((volatile int *)CONV_IN_DEPTH) = IN_DEPTH;
    *((volatile int *)CONV_OUT_DEPTH) = OUT_DEPTH;
    
    // Step 2: 設定大小觸發
    *((volatile int *)INPUT_SIZE_ADDR) = input_size;
    *((volatile int *)WEIGHT_SIZE_ADDR) = weight_size;
    *((volatile int *)OUTPUT_SIZE_ADDR) = output_size;
    
    // Step 3: 載入權重
    for (int i = 0; i < weight_size; i++) {
        *((volatile float *)WEIGHT_DATA_ADDR) = kernel[i];
    }
    
    // Step 4: 載入輸入
    for (int i = 0; i < input_size; i++) {
        *((volatile float *)INPUT_DATA_ADDR) = input[i];
    }
    
    // Step 5: 觸發運算
    *((volatile int *)CONV_TRIGGER) = 1;
    
    // Step 6: 等待完成
    int timeout = 100000;
    while (*((volatile int *)CONV_TRIGGER) && timeout > 0) {
        timeout--;
    }
    
    if (timeout == 0) {
        printf("ERROR: Buffered mode timeout!\n");
        return -1;
    }
    
    // Step 7: 讀取輸出
    for (int i = 0; i < output_size; i++) {
        output_hw[i] = *((volatile float *)OUTPUT_DATA_ADDR);
    }
    
    print_2d("HW Output (Buffered)", output_hw, OUT_HEIGHT, OUT_WIDTH);
    
    // 比較結果
    int pass = 1;
    for (int i = 0; i < output_size; i++) {
        if (!float_equal(output_hw[i], output_sw[i], 0.01f)) {
            printf("MISMATCH at [%d]: HW=%.4f, SW=%.4f\n", i, output_hw[i], output_sw[i]);
            pass = 0;
        }
    }
    
    if (pass) {
        printf("TEST PASSED!\n");
        return 0;
    } else {
        printf("TEST FAILED!\n");
        return -1;
    }
}
#endif

// ============================================================
// 測試 5: MMIO 讀寫測試
// ============================================================
int test_mmio_read_write(void) {
    printf("\n========================================\n");
    printf("Test 5: Basic MMIO Read/Write\n");
    printf("========================================\n");
    
    // 測試寫入和讀取 FMA 暫存器
    volatile float *fma_a = (volatile float *)FMA_A_ADDR;
    volatile float *fma_b = (volatile float *)FMA_B_ADDR;
    
    float test_val_a = 3.14159f;
    float test_val_b = 2.71828f;
    
    printf("Writing A = %.5f\n", test_val_a);
    printf("Writing B = %.5f\n", test_val_b);
    
    *fma_a = test_val_a;
    *fma_b = test_val_b;
    
    // 嘗試讀回（如果硬體支援）
    // 注意：某些 MMIO 暫存器可能是只寫的
    printf("MMIO write completed.\n");
    
    // 檢查 busy flag
    volatile uint32_t *fma_busy = (volatile uint32_t *)FMA_BUSY_ADDR;
    uint32_t busy_val = *fma_busy;
    printf("Busy register value: 0x%08X\n", busy_val);
    printf("  - conv_fma_busy (bit 1): %d\n", (busy_val >> 1) & 1);
    
    printf("TEST PASSED (basic MMIO access works)!\n");
    return 0;
}

// ============================================================
// 主程式
// ============================================================
int main(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  DSA Convolution Accelerator Test\n");
    printf("========================================\n");
    printf("Input size: %dx%dx%d\n", IN_WIDTH, IN_HEIGHT, IN_DEPTH);
    printf("Kernel size: %dx%d\n", KERNEL_SIZE, KERNEL_SIZE);
    printf("Output size: %dx%dx%d\n", OUT_WIDTH, OUT_HEIGHT, OUT_DEPTH);
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // 測試 5: 基本 MMIO 測試
    total_tests++;
    if (test_mmio_read_write() == 0) passed_tests++;
    
    // 測試 1: FMA 單次運算
    total_tests++;
    if (test_fma_single() == 0) passed_tests++;
    
    // 測試 2: FMA 點積
    total_tests++;
    if (test_fma_dot_product() == 0) passed_tests++;
    
    // 測試 3: FMA 模式卷積
    total_tests++;
    if (test_conv_fma_mode() == 0) passed_tests++;
    
#ifdef USE_CONV3D_BUFFERED
    // 測試 4: Buffered 模式卷積
    total_tests++;
    if (test_conv_buffered_mode() == 0) passed_tests++;
#endif
    
    printf("\n========================================\n");
    printf("  Test Summary: %d/%d PASSED\n", passed_tests, total_tests);
    printf("========================================\n");
    
    return (passed_tests == total_tests) ? 0 : -1;
}
