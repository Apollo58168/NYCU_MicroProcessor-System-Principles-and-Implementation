`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2025/12/19 10:16:55
// Design Name: 
// Module Name: dotprod
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
//  
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module dotprod(
    input  wire        clk,
    input  wire        rst,
    input  wire        en,           // Enable (strobe)
    input  wire        we,           // Write enable
    input  wire [31:0] addr,         // Address bus
    input  wire [31:0] wdata,        // Write data
    output reg  [31:0] rdata,        // Read data
    output reg         ready         // Ready signal for handshake
);

    reg signed [7:0] A[0:7];
    reg signed [7:0] B[0:7]; 
    reg signed [31:0] c;
    reg [31:0] trig;
    reg signed [31:0] prod;           // Minimum value
    reg signed [31:0] tmpt1;           // Minimum value
    reg signed [31:0] tmpt2;           // Minimum value
    reg signed [31:0] tmpt3;           // Minimum value
    reg signed [31:0] tmpt4;           // Minimum value
    reg signed [31:0] tmpt5;           // Minimum value
    reg signed [31:0] tmpt6;           // Minimum value
    reg signed [31:0] tmpt7;           // Minimum value
    reg signed [31:0] tmpt8;           // Minimum value

    integer i;
    integer j;


    always @(posedge clk) begin
        if (rst)
            ready <= 1'b0;
        else if (en)
            ready <= 1'b1;
        else
            ready <= 1'b0;
    end

    always @(posedge clk) begin
        if (rst) begin
            // Reset all registers to 0
            prod <= 32'b0;
        end else begin
            tmpt1 <= A[0] * B[0];
            tmpt2 <= A[1] * B[1];
            tmpt3 <= A[2] * B[2];
            tmpt4 <= A[3] * B[3];
            tmpt5 <= A[4] * B[4];
            tmpt6 <= A[5] * B[5];
            tmpt7 <= A[6] * B[6];
            tmpt8 <= A[7] * B[7];
            prod <= tmpt1 + tmpt2 + tmpt3 + tmpt4 + tmpt5 + tmpt6 + tmpt7 + tmpt8;
        end 
    end

    always @(posedge clk) begin
        if (rst) begin
            // Reset all registers to 0
            trig <= 32'b0;
            c    <= 32'b0;
            for (i = 0; i < 8; i = i + 1) begin
                A[i] <= 8'b0;
                B[i] <= 8'b0;
            end
        end else begin
            // Handle writes when write enable is asserted
            if (en && we) begin
            case (addr)
                32'hC2000000: A[0] <= wdata[7:0];
                32'hC2000001: A[1] <= wdata[15:8];
                32'hC2000002: A[2] <= wdata[23:16];
                32'hC2000003: A[3] <= wdata[31:24];
                32'hC2000004: A[4] <= wdata[7:0];
                32'hC2000005: A[5] <= wdata[15:8];
                32'hC2000006: A[6] <= wdata[23:16];
                32'hC2000007: A[7] <= wdata[31:24];
                32'hC2000010: B[0] <= wdata[7:0];
                32'hC2000011: B[1] <= wdata[15:8];
                32'hC2000012: B[2] <= wdata[23:16];
                32'hC2000013: B[3] <= wdata[31:24];
                32'hC2000014: B[4] <= wdata[7:0];
                32'hC2000015: B[5] <= wdata[15:8];
                32'hC2000016: B[6] <= wdata[23:16];
                32'hC2000017: B[7] <= wdata[31:24];
                32'hC2000030: trig <= wdata;
                default: ;
            endcase
        end
            // Compute min/max when triggered - use combinational results
            if (trig != 32'b0) begin
                // min <= min_comb;
                // max <= max_comb;
                c    <= prod;
                trig <= 32'b0;
            end
        end
    end

    // Read logic - combinational
    always @(*) begin
        case (addr)
            32'hC2000000: rdata[7:0] = A[0];
            32'hC2000001: rdata[15:8] = A[1];
            32'hC2000002: rdata[23:16] = A[2];
            32'hC2000003: rdata[31:24] = A[3];
            32'hC2000004: rdata[7:0] = A[4];
            32'hC2000005: rdata[15:8] = A[5];
            32'hC2000006: rdata[23:16] = A[6];
            32'hC2000007: rdata[31:24] = A[7];
            32'hC2000010: rdata[7:0] = B[0];
            32'hC2000011: rdata[15:8] = B[1];
            32'hC2000012: rdata[23:16] = B[2];
            32'hC2000013: rdata[31:24] = B[3];
            32'hC2000014: rdata[7:0] = B[4];
            32'hC2000015: rdata[15:8] = B[5];
            32'hC2000016: rdata[23:16] = B[6];
            32'hC2000017: rdata[31:24] = B[7];
            32'hC2000020: rdata = c; 
            32'hC2000030: rdata = trig;
            default:      rdata = 32'b0;
        endcase
    end


endmodule