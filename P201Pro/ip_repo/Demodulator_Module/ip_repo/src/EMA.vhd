----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 05/29/2022 02:12:46 PM
-- Design Name: 
-- Module Name: EMA - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
-- 
-- Dependencies: 
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
-- 
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
USE ieee.std_logic_arith.ALL;
USE ieee.std_logic_signed.all;
Library UNISIM;
use UNISIM.vcomponents.all;

entity EMA is
    generic( Width : integer := 16;
             Avg_Depth : integer := 6);
    Port ( clk : in STD_LOGIC;
           RST : in STD_LOGIC;
           Vin : in STD_LOGIC_VECTOR (Width-1 downto 0);
           Valin : in STD_LOGIC;
           Valout : out STD_LOGIC;
           AVG : out STD_LOGIC_VECTOR (Width-1 downto 0));
end EMA;

architecture Behavioral of EMA is

COMPONENT EMA_fifo
  PORT (
    clk : IN STD_LOGIC;
    srst : IN STD_LOGIC;
    din : IN STD_LOGIC_VECTOR(23 DOWNTO 0);
    wr_en : IN STD_LOGIC;
    rd_en : IN STD_LOGIC;
    dout : OUT STD_LOGIC_VECTOR(23 DOWNTO 0);
    full : OUT STD_LOGIC;
    empty : OUT STD_LOGIC;
    data_count : OUT STD_LOGIC_VECTOR(9 DOWNTO 0)
  );
END COMPONENT;

signal Vacc       : STD_LOGIC_VECTOR(Width - 1 + Avg_Depth  DOWNTO 0);
signal Vtotal       : STD_LOGIC_VECTOR(Width - 1 DOWNTO 0);
signal data_count : STD_LOGIC_VECTOR(9 DOWNTO 0);
signal dout       : STD_LOGIC_VECTOR(23 DOWNTO 0);
signal Vin_fifo       : STD_LOGIC_VECTOR(23 DOWNTO 0);
signal rd_en  ,Valin_d ,rd_en_d   : STD_LOGIC;
signal Depth : integer ;
attribute mark_debug : string;
attribute mark_debug of data_count : signal is "True";
attribute mark_debug of AVG ,Vtotal , Vacc , Depth: signal is "True";
begin

Depth <=    
            1    when Avg_Depth = 0  else
            2    when Avg_Depth = 1  else
            4    when Avg_Depth = 2  else
            8    when Avg_Depth = 3  else
            16   when Avg_Depth = 4  else
            32   when Avg_Depth = 5  else
            64   when Avg_Depth = 6  else
            128  when Avg_Depth = 7  else
            256  when Avg_Depth = 8  else
            512  when Avg_Depth = 9  else
            1024 when Avg_Depth = 10 ;
       

EMA_fifo_inst : EMA_fifo
  PORT MAP (
    clk        => clk,
    srst       => rst,
    din        => Vin_fifo,
    wr_en      => Valin,
    rd_en      => rd_en,
    dout       => dout,
    full       => open,
    empty      => open,
    data_count => data_count
  );
  Vin_fifo <= EXT( Vin , Vin_fifo'LEFT + 1);
process(CLK,Rst)
begin
	if (Rst='1') then
        rd_en <= '0';
        Valin_d <= '0';
        rd_en_d <= '0';
        Vtotal <= (others => '0');
        Vacc <= (others => '0');
	    AVG <= (others => '0');
	elsif rising_edge(CLK) then
	    Valin_d <= Valin;
	    rd_en_d <= rd_en;
        if (data_count >= CONV_STD_LOGIC_VECTOR(Depth-1,10)) then
            rd_en <= Valin;
        else
            rd_en <= '0';
        end if;
               
        if (rd_en = '1' ) then
            Vtotal <= Vin - dout(Width -1 downto 0);
        elsif (data_count >= CONV_STD_LOGIC_VECTOR(Depth-1,10)) then
            Vtotal <= Vin - dout(Width -1 downto 0);
        else
            Vtotal <= Vin ;
        end if;
        
        if (Valin_d = '1' ) then
	       Vacc   <= Vacc + sxt(Vtotal, Avg_Depth + Width)  ;
	    end if;
	    Valout <= rd_en_d;
        AVG <= Vacc(Width - 1 + Avg_Depth downto Avg_Depth);
	end if;
end process;


end Behavioral;
