// ============================================================================
// Pooling Layer Module - Average Pooling with ADD and MUL operations
// MMIO Addresses: 
//   ADD: 0xC400_0018 (A), 0xC400_001C (B & trigger), 0xC400_0020 (read result)
//   MUL: 0xC400_0024 (A), 0xC400_0028 (B & trigger), 0xC400_002C (read result)
// ============================================================================
module pooling_layer
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
// Registers for Pooling ADD: A + B
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
// Input Logic - Pooling ADD/MUL inputs
// ============================================================================
always @(posedge clk_i) begin
    if (rst_i) begin
        pool_add_data_valid <= 0;
        pool_add_feed_dataA <= 0;
        pool_add_feed_dataB <= 0;
        
        pool_mul_data_valid <= 0;
        pool_mul_feed_dataA <= 0;
        pool_mul_feed_dataB <= 0;
    end
    else if (en_i) begin
        // Pooling ADD inputs (0x18, 0x1C)
        if (we_i && addr_i == 32'hC400_0018) begin
            pool_add_feed_dataA <= data_i;
        end
        else if (we_i && addr_i == 32'hC400_001C) begin
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
        
        else if (!we_i) begin
            pool_add_data_valid <= 0;
            pool_mul_data_valid <= 0;
        end
    end
    else begin
        if (pool_add_data_valid) pool_add_data_valid <= 0;
        if (pool_mul_data_valid) pool_mul_data_valid <= 0;
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
        // Capture results when FP IPs output valid
        if (pool_add_result_valid) begin
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
    end
end

// Ready signal: always ready for pooling layer
assign ready_o = 1'b1;

// ============================================================================
// Floating-Point ADD IP for Pooling: computes A + B
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
