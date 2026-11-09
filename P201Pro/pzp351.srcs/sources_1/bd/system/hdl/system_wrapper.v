//Copyright 1986-2019 Xilinx, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2019.2 (lin64) Build 2700185 Thu Oct 24 18:45:48 MDT 2019
//Date        : Sun Sep 20 18:29:08 2026
//Host        : ubuntu running 64-bit Ubuntu 18.04.2 LTS
//Command     : generate_target system_wrapper.bd
//Design      : system_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module system_wrapper
   (ddr_addr,
    ddr_ba,
    ddr_cas_n,
    ddr_ck_n,
    ddr_ck_p,
    ddr_cke,
    ddr_cs_n,
    ddr_dm,
    ddr_dq,
    ddr_dqs_n,
    ddr_dqs_p,
    ddr_odt,
    ddr_ras_n,
    ddr_reset_n,
    ddr_we_n,
    enable,
    fixed_io_ddr_vrn,
    fixed_io_ddr_vrp,
    fixed_io_mio,
    fixed_io_ps_clk,
    fixed_io_ps_porb,
    fixed_io_ps_srstb,
    gpio_i,
    gpio_o,
    gpio_t,
    led_trf_rx1,
    led_trf_rx2,
    led_trf_tx1,
    led_trf_tx2,
    mdio_port_0_mdc,
    mdio_port_0_mdio_io,
    rgmii_port_0_rd,
    rgmii_port_0_rx_ctl,
    rgmii_port_0_rxc,
    rgmii_port_0_td,
    rgmii_port_0_tx_ctl,
    rgmii_port_0_txc,
    rx_clk_in,
    rx_data_in,
    rx_frame_in,
    spi0_clk_i,
    spi0_clk_o,
    spi0_csn_0_o,
    spi0_csn_1_o,
    spi0_csn_2_o,
    spi0_csn_i,
    spi0_sdi_i,
    spi0_sdo_i,
    spi0_sdo_o,
    tx_clk_out,
    tx_data_out,
    tx_frame_out,
    txnrx,
    up_enable,
    up_txnrx);
  inout [14:0]ddr_addr;
  inout [2:0]ddr_ba;
  inout ddr_cas_n;
  inout ddr_ck_n;
  inout ddr_ck_p;
  inout ddr_cke;
  inout ddr_cs_n;
  inout [3:0]ddr_dm;
  inout [31:0]ddr_dq;
  inout [3:0]ddr_dqs_n;
  inout [3:0]ddr_dqs_p;
  inout ddr_odt;
  inout ddr_ras_n;
  inout ddr_reset_n;
  inout ddr_we_n;
  output enable;
  inout fixed_io_ddr_vrn;
  inout fixed_io_ddr_vrp;
  inout [53:0]fixed_io_mio;
  inout fixed_io_ps_clk;
  inout fixed_io_ps_porb;
  inout fixed_io_ps_srstb;
  input [16:0]gpio_i;
  output [16:0]gpio_o;
  output [16:0]gpio_t;
  output [0:0]led_trf_rx1;
  output [0:0]led_trf_rx2;
  output [0:0]led_trf_tx1;
  output [0:0]led_trf_tx2;
  output mdio_port_0_mdc;
  inout mdio_port_0_mdio_io;
  input [3:0]rgmii_port_0_rd;
  input rgmii_port_0_rx_ctl;
  input rgmii_port_0_rxc;
  output [3:0]rgmii_port_0_td;
  output rgmii_port_0_tx_ctl;
  output rgmii_port_0_txc;
  input rx_clk_in;
  input [11:0]rx_data_in;
  input rx_frame_in;
  input spi0_clk_i;
  output spi0_clk_o;
  output spi0_csn_0_o;
  output spi0_csn_1_o;
  output spi0_csn_2_o;
  input spi0_csn_i;
  input spi0_sdi_i;
  input spi0_sdo_i;
  output spi0_sdo_o;
  output tx_clk_out;
  output [11:0]tx_data_out;
  output tx_frame_out;
  output txnrx;
  input up_enable;
  input up_txnrx;

  wire [14:0]ddr_addr;
  wire [2:0]ddr_ba;
  wire ddr_cas_n;
  wire ddr_ck_n;
  wire ddr_ck_p;
  wire ddr_cke;
  wire ddr_cs_n;
  wire [3:0]ddr_dm;
  wire [31:0]ddr_dq;
  wire [3:0]ddr_dqs_n;
  wire [3:0]ddr_dqs_p;
  wire ddr_odt;
  wire ddr_ras_n;
  wire ddr_reset_n;
  wire ddr_we_n;
  wire enable;
  wire fixed_io_ddr_vrn;
  wire fixed_io_ddr_vrp;
  wire [53:0]fixed_io_mio;
  wire fixed_io_ps_clk;
  wire fixed_io_ps_porb;
  wire fixed_io_ps_srstb;
  wire [16:0]gpio_i;
  wire [16:0]gpio_o;
  wire [16:0]gpio_t;
  wire [0:0]led_trf_rx1;
  wire [0:0]led_trf_rx2;
  wire [0:0]led_trf_tx1;
  wire [0:0]led_trf_tx2;
  wire mdio_port_0_mdc;
  wire mdio_port_0_mdio_i;
  wire mdio_port_0_mdio_io;
  wire mdio_port_0_mdio_o;
  wire mdio_port_0_mdio_t;
  wire [3:0]rgmii_port_0_rd;
  wire rgmii_port_0_rx_ctl;
  wire rgmii_port_0_rxc;
  wire [3:0]rgmii_port_0_td;
  wire rgmii_port_0_tx_ctl;
  wire rgmii_port_0_txc;
  wire rx_clk_in;
  wire [11:0]rx_data_in;
  wire rx_frame_in;
  wire spi0_clk_i;
  wire spi0_clk_o;
  wire spi0_csn_0_o;
  wire spi0_csn_1_o;
  wire spi0_csn_2_o;
  wire spi0_csn_i;
  wire spi0_sdi_i;
  wire spi0_sdo_i;
  wire spi0_sdo_o;
  wire tx_clk_out;
  wire [11:0]tx_data_out;
  wire tx_frame_out;
  wire txnrx;
  wire up_enable;
  wire up_txnrx;

  IOBUF mdio_port_0_mdio_iobuf
       (.I(mdio_port_0_mdio_o),
        .IO(mdio_port_0_mdio_io),
        .O(mdio_port_0_mdio_i),
        .T(mdio_port_0_mdio_t));
  system system_i
       (.ddr_addr(ddr_addr),
        .ddr_ba(ddr_ba),
        .ddr_cas_n(ddr_cas_n),
        .ddr_ck_n(ddr_ck_n),
        .ddr_ck_p(ddr_ck_p),
        .ddr_cke(ddr_cke),
        .ddr_cs_n(ddr_cs_n),
        .ddr_dm(ddr_dm),
        .ddr_dq(ddr_dq),
        .ddr_dqs_n(ddr_dqs_n),
        .ddr_dqs_p(ddr_dqs_p),
        .ddr_odt(ddr_odt),
        .ddr_ras_n(ddr_ras_n),
        .ddr_reset_n(ddr_reset_n),
        .ddr_we_n(ddr_we_n),
        .enable(enable),
        .fixed_io_ddr_vrn(fixed_io_ddr_vrn),
        .fixed_io_ddr_vrp(fixed_io_ddr_vrp),
        .fixed_io_mio(fixed_io_mio),
        .fixed_io_ps_clk(fixed_io_ps_clk),
        .fixed_io_ps_porb(fixed_io_ps_porb),
        .fixed_io_ps_srstb(fixed_io_ps_srstb),
        .gpio_i(gpio_i),
        .gpio_o(gpio_o),
        .gpio_t(gpio_t),
        .led_trf_rx1(led_trf_rx1),
        .led_trf_rx2(led_trf_rx2),
        .led_trf_tx1(led_trf_tx1),
        .led_trf_tx2(led_trf_tx2),
        .mdio_port_0_mdc(mdio_port_0_mdc),
        .mdio_port_0_mdio_i(mdio_port_0_mdio_i),
        .mdio_port_0_mdio_o(mdio_port_0_mdio_o),
        .mdio_port_0_mdio_t(mdio_port_0_mdio_t),
        .rgmii_port_0_rd(rgmii_port_0_rd),
        .rgmii_port_0_rx_ctl(rgmii_port_0_rx_ctl),
        .rgmii_port_0_rxc(rgmii_port_0_rxc),
        .rgmii_port_0_td(rgmii_port_0_td),
        .rgmii_port_0_tx_ctl(rgmii_port_0_tx_ctl),
        .rgmii_port_0_txc(rgmii_port_0_txc),
        .rx_clk_in(rx_clk_in),
        .rx_data_in(rx_data_in),
        .rx_frame_in(rx_frame_in),
        .spi0_clk_i(spi0_clk_i),
        .spi0_clk_o(spi0_clk_o),
        .spi0_csn_0_o(spi0_csn_0_o),
        .spi0_csn_1_o(spi0_csn_1_o),
        .spi0_csn_2_o(spi0_csn_2_o),
        .spi0_csn_i(spi0_csn_i),
        .spi0_sdi_i(spi0_sdi_i),
        .spi0_sdo_i(spi0_sdo_i),
        .spi0_sdo_o(spi0_sdo_o),
        .tx_clk_out(tx_clk_out),
        .tx_data_out(tx_data_out),
        .tx_frame_out(tx_frame_out),
        .txnrx(txnrx),
        .up_enable(up_enable),
        .up_txnrx(up_txnrx));
endmodule
