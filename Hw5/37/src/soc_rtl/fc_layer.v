// ============================================================================
// FC Layer Module - Fully Connected Layer with FMA operation
// MMIO Addresses: 0xC400_0000 (A), 0xC400_0004 (B), 0xC400_0008 (C & trigger/read)
// ============================================================================
module fc_layer
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
// Registers for FC FMA (fully connected): A*B + C
// ============================================================================
reg [31:0] fc_fma_feed_dataA;
reg [31:0] fc_fma_feed_dataB;
reg [31:0] fc_fma_feed_dataC;
reg [31:0] fc_fma_result_reg;

reg fc_fma_data_valid;
wire fc_fma_result_valid;
wire [31:0] fc_fma_result_data;

// ============================================================================
// Input Logic - FC FMA inputs
// 0x00 = A, 0x04 = B, 0x08 write = C and trigger
// ============================================================================
always @(posedge clk_i) begin
    if (rst_i) begin
        fc_fma_data_valid <= 0;
        fc_fma_feed_dataA <= 0;
        fc_fma_feed_dataB <= 0;
        fc_fma_feed_dataC <= 0;
    end
    else if (en_i) begin
        if (we_i && addr_i == 32'hC400_0000) begin
            fc_fma_feed_dataA <= data_i;
        end
        else if (we_i && addr_i == 32'hC400_0004) begin
            fc_fma_feed_dataB <= data_i;
        end
        else if (we_i && addr_i == 32'hC400_0008) begin
            // Write to 0x08 provides C value and triggers FMA
            fc_fma_feed_dataC <= data_i;
            fc_fma_data_valid <= 1;
        end
        else if (!we_i) begin
            fc_fma_data_valid <= 0;
        end
    end
    else begin
        if (fc_fma_data_valid) fc_fma_data_valid <= 0;
    end
end

// ============================================================================
// Output Logic - Capture result and read
// ============================================================================
always @(posedge clk_i) begin
    if (rst_i) begin
        fc_fma_result_reg <= 0;
        data_o <= 0;
    end
    else begin
        // Capture result when FP IP outputs valid
        if (fc_fma_result_valid) begin
            fc_fma_result_reg <= fc_fma_result_data;
        end

        // Read FC FMA result at 0x08
        if (en_i && !we_i && addr_i == 32'hC400_0008) begin
            data_o <= fc_fma_result_reg;
        end
    end
end

// Ready signal: always ready for FC layer
assign ready_o = 1'b1;

// ============================================================================
// Floating-Point FMA IP for FC layer: computes A * B + C
// ============================================================================
floating_point_0 ip_fc_fma (
    .aclk(clk_i),
    
    .s_axis_a_tvalid(fc_fma_data_valid),
    .s_axis_a_tdata(fc_fma_feed_dataA),
    
    .s_axis_b_tvalid(fc_fma_data_valid),
    .s_axis_b_tdata(fc_fma_feed_dataB),
    
    .s_axis_c_tvalid(fc_fma_data_valid),
    .s_axis_c_tdata(fc_fma_feed_dataC),
    
    .m_axis_result_tvalid(fc_fma_result_valid),
    .m_axis_result_tdata(fc_fma_result_data)
);

endmodule
