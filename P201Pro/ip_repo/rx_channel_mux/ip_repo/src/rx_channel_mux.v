`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 08/19/2026 11:44:49 AM
// Design Name: 
// Module Name: rx_channel_mux
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

module rx_channel_mux (
    input  wire        rx_sel,       // 0 = RX1, 1 = RX2

    input  wire [15:0] rx1_i,
    input  wire [15:0] rx1_q,

    input  wire [15:0] rx2_i,
    input  wire [15:0] rx2_q,
    
    input  wire        rx1_En_i,
    input  wire        rx1_En_q,
    
    input  wire        rx2_En_i,
    input  wire        rx2_En_q,
    

    output wire [15:0] out_i,
    output wire [15:0] out_q,
    output wire        rx_En_i,
    output wire        rx_En_q
);

    assign out_i = (rx_sel == 1'b0) ? rx1_i : rx2_i;
    assign out_q = (rx_sel == 1'b0) ? rx1_q : rx2_q;
    
    assign rx_En_i = (rx_sel == 1'b0) ? rx1_En_i : rx2_En_i;
    assign rx_En_q = (rx_sel == 1'b0) ? rx1_En_q : rx2_En_q;

endmodule
