// ============================================================================
// Pooling Layer Module - Average Pooling with ADD, MUL, and Accumulator
// MMIO Addresses: 
//   ADD: 0xC400_0018 (A), 0xC400_001C (B & trigger), 0xC400_0020 (read result)
//   MUL: 0xC400_0024 (A), 0xC400_0028 (B & trigger), 0xC400_002C (read result)
//   ACCUM: 0xC400_0050 (reset & set count), 0xC400_0054 (write value), 0xC400_0058 (read result)
//   Note: Accumulator reuses the ADD IP to save resources
// ============================================================================
module pool_layer
#( parameter XLEN = 32 )
(
    input                   clk_i,
    input                   rst_i,

    // MMIO interface
    input                   en_i,
    input                   we_i,
    input [XLEN-1 : 0]      addr_i,
    input [XLEN-1 : 0]      data_i,
    output reg [XLEN-1 : 0] data_o,
    output                  ready_o
);

// ============================================================================
// Registers for Pooling ADD: A + B (shared with accumulator)
// ============================================================================
reg [31:0] pool_add_feed_dataA;
reg [31:0] pool_add_feed_dataB;
reg [31:0] pool_add_result_reg;

reg pool_add_data_valid;
wire pool_add_result_valid;
wire [31:0] pool_add_result_data;

// ============================================================================
// Registers for Pooling MUL: A * B
// ============================================================================
reg [31:0] pool_mul_feed_dataA;
reg [31:0] pool_mul_feed_dataB;
reg [31:0] pool_mul_result_reg;

reg pool_mul_data_valid;
wire pool_mul_result_valid;
wire [31:0] pool_mul_result_data;

// ============================================================================
// Registers for Hardware Accumulator (reuses ADD IP)
// ============================================================================
reg [31:0] accum_value;            // Current accumulated value
reg [31:0] accum_result_reg;       // Final result
reg [3:0]  accum_count;            // Number of values to accumulate
reg [3:0]  accum_received;         // Number of values received
reg        accum_busy;             // Accumulator is processing
reg        accum_first;            // First value flag (no addition needed)
reg        accum_pending;          // Waiting for ADD result

// ============================================================================
// Input Logic - Pooling ADD/MUL/ACCUM inputs
// ============================================================================
always @(posedge clk_i) begin
    if (rst_i) begin
        pool_add_data_valid <= 0;
        pool_add_feed_dataA <= 0;
        pool_add_feed_dataB <= 0;
        
        pool_mul_data_valid <= 0;
        pool_mul_feed_dataA <= 0;
        pool_mul_feed_dataB <= 0;
        
        accum_count <= 0;
        accum_received <= 0;
        accum_value <= 0;
        accum_busy <= 0;
        accum_first <= 1;
        accum_pending <= 0;
        accum_result_reg <= 0;
    end
    else begin
        // Clear valid signals after one cycle
        if (pool_add_data_valid) pool_add_data_valid <= 0;
        if (pool_mul_data_valid) pool_mul_data_valid <= 0;
        
        // Handle accumulator ADD result
        if (accum_pending && pool_add_result_valid) begin
            accum_value <= pool_add_result_data;
            accum_pending <= 0;
            if (accum_received == accum_count) begin
                accum_result_reg <= pool_add_result_data;
                accum_busy <= 0;
            end
        end
        
        if (en_i) begin
            // Pooling ADD inputs (0x18, 0x1C) - only when not in accum mode
            if (we_i && addr_i == 32'hC400_0018 && !accum_busy) begin
                pool_add_feed_dataA <= data_i;
            end
            else if (we_i && addr_i == 32'hC400_001C && !accum_busy) begin
                pool_add_feed_dataB <= data_i;
                pool_add_data_valid <= 1;
            end
            
            // Pooling MUL inputs (0x24, 0x28)
            else if (we_i && addr_i == 32'hC400_0024) begin
                pool_mul_feed_dataA <= data_i;
            end
            else if (we_i && addr_i == 32'hC400_0028) begin
                pool_mul_feed_dataB <= data_i;
                pool_mul_data_valid <= 1;
            end
            
            // Accumulator: Reset and set count (0x50)
            else if (we_i && addr_i == 32'hC400_0050) begin
                accum_count <= data_i[3:0];  // Max 15 values
                accum_received <= 0;
                accum_value <= 0;
                accum_result_reg <= 0;
                accum_busy <= 1;
                accum_first <= 1;
                accum_pending <= 0;
            end
            
            // Accumulator: Write value to accumulate (0x54)
            else if (we_i && addr_i == 32'hC400_0054 && accum_busy && !accum_pending) begin
                if (accum_first) begin
                    // First value: store directly, no addition
                    accum_value <= data_i;
                    accum_first <= 0;
                    accum_received <= accum_received + 1;
                    // Check if only 1 value
                    if (accum_received + 1 == accum_count) begin
                        accum_result_reg <= data_i;
                        accum_busy <= 0;
                    end
                end
                else begin
                    // Subsequent values: use ADD IP
                    pool_add_feed_dataA <= accum_value;
                    pool_add_feed_dataB <= data_i;
                    pool_add_data_valid <= 1;
                    accum_pending <= 1;
                    accum_received <= accum_received + 1;
                end
            end
        end
    end
end

// ============================================================================
// Output Logic - Capture results and read
// ============================================================================
always @(posedge clk_i) begin
    if (rst_i) begin
        pool_add_result_reg <= 0;
        pool_mul_result_reg <= 0;
        data_o <= 0;
    end
    else begin
        // Capture ADD result (only for non-accum mode)
        if (pool_add_result_valid && !accum_pending) begin
            pool_add_result_reg <= pool_add_result_data;
        end
        if (pool_mul_result_valid) begin
            pool_mul_result_reg <= pool_mul_result_data;
        end

        // Read Pooling ADD result at 0x20
        if (en_i && !we_i && addr_i == 32'hC400_0020) begin
            data_o <= pool_add_result_reg;
        end
        
        // Read Pooling MUL result at 0x2C
        else if (en_i && !we_i && addr_i == 32'hC400_002C) begin
            data_o <= pool_mul_result_reg;
        end
        
        // Read Accumulator result at 0x58
        else if (en_i && !we_i && addr_i == 32'hC400_0058) begin
            data_o <= accum_result_reg;
        end
    end
end

// Ready signal: block when accumulator is waiting for ADD result
assign ready_o = !accum_pending;

// ============================================================================
// Floating-Point ADD IP for Pooling (shared with accumulator)
// ============================================================================
floating_point_add ip_pool_add (
    .aclk(clk_i),
    
    .s_axis_a_tvalid(pool_add_data_valid),
    .s_axis_a_tdata(pool_add_feed_dataA),
    
    .s_axis_b_tvalid(pool_add_data_valid),
    .s_axis_b_tdata(pool_add_feed_dataB),
    
    .m_axis_result_tvalid(pool_add_result_valid),
    .m_axis_result_tdata(pool_add_result_data)
);

// ============================================================================
// Floating-Point MUL IP for Pooling: computes A * B
// ============================================================================
floating_point_mul ip_pool_mul (
    .aclk(clk_i),
    
    .s_axis_a_tvalid(pool_mul_data_valid),
    .s_axis_a_tdata(pool_mul_feed_dataA),
    
    .s_axis_b_tvalid(pool_mul_data_valid),
    .s_axis_b_tdata(pool_mul_feed_dataB),
    
    .m_axis_result_tvalid(pool_mul_result_valid),
    .m_axis_result_tdata(pool_mul_result_data)
);

endmodule


