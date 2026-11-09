----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 05/29/2022 05:12:14 PM
-- Design Name: 
-- Module Name: AGC_V2 - Behavioral
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

entity AGC_V2 is
    generic( 
             Stereo : boolean  := true;
             Width : integer := 24;
             Avg_Depth : integer := 10);
    Port ( clk      : in STD_LOGIC;
           rst      : in STD_LOGIC;
           Iin      : in STD_LOGIC_VECTOR (Width-1 downto 0);
           Qin      : in STD_LOGIC_VECTOR (Width-1 downto 0);
           Val_in   : in STD_LOGIC;
           MGC_on   : in STD_LOGIC;
           Coef     : in STD_LOGIC_VECTOR (7 downto 0);
           Ref      : in STD_LOGIC_VECTOR (Width-1 downto 0);
           MG_value : in STD_LOGIC_VECTOR (31 downto 0);
           Iout     : out STD_LOGIC_VECTOR (Width-1 downto 0);
           Qout     : out STD_LOGIC_VECTOR (Width-1 downto 0);
           Valout   : out STD_LOGIC);
end AGC_V2;

architecture Behavioral of AGC_V2 is

COMPONENT FM_AGC_Mult
  PORT (
    CLK : IN STD_LOGIC;
    A : IN STD_LOGIC_VECTOR(23 DOWNTO 0);
    B : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
    CE : IN STD_LOGIC;
    P : OUT STD_LOGIC_VECTOR(23 DOWNTO 0)
  );
END COMPONENT;

signal Gain :STD_LOGIC_VECTOR (63 downto 0);
signal Gain_unsigned :STD_LOGIC_VECTOR (63 downto 0);
signal Iin_SCALE,Qin_SCALE :STD_LOGIC_VECTOR (23 downto 0);
signal AVG :STD_LOGIC_VECTOR (Width-1 downto 0);
signal V_estimated :STD_LOGIC_VECTOR (Width-1 downto 0);
signal Iin_d1,Iin_d2,Iin_d3,Iin_d4,Iin_d5 :STD_LOGIC_VECTOR (Width-1 downto 0);
signal Iout_reg,Qout_reg :STD_LOGIC_VECTOR (23 downto 0);
signal overflow_cnt :STD_LOGIC_VECTOR (15 downto 0);
signal V_val,V_val_out,V_val_out_d :STD_LOGIC;
signal AddSub :STD_LOGIC;
signal overflow,Val_in_d1,Val_in_d2,Val_in_d3,Val_in_d4,Val_in_d5 :STD_LOGIC;
attribute mark_debug : string;
attribute keep : string;
attribute mark_debug of overflow,Val_in_d1,Iout_reg,Iin_d1,Iin_d2,Iin_d3,Iin_d4,Iin_d5 : signal is "true";


begin

Iout <= Iout_reg(Iout'LEFT DOWNTO 0);
Qout <= Qout_reg(Qout'LEFT DOWNTO 0);

input_voltage_estimation :entity work.Vest --2delay
    generic map( Width => Width,Stereo => Stereo)
    Port map(   clk => clk,
                rst => rst or overflow,
               
                Iin => Iout_reg(Width-1 downto 0),
                Qin => Qout_reg(Width-1 downto 0),
                Valin => Val_in_d5,
                
                Vout => V_estimated,
                Valout => V_val
           );
input_voltage_average :entity work.EMA --3delay
    generic map( Width => Width,
                 Avg_Depth => Avg_Depth)
    Port map( clk => clk,
           RST => rst or overflow,
           Vin => V_estimated,
           Valin => V_val,
           Valout => V_val_out,
           AVG => AVG
           );
--------------------------------------comparetor-------------------------------------
process(CLK,Rst)
begin
	if (Rst='1' or overflow='1') then
        AddSub <= '0';
        V_val_out_d <= '0';
	elsif rising_edge(CLK) then
	    V_val_out_d <= V_val_out;
	    if V_val_out = '1' then 
            if (AVG) >= (Ref) then --1delay
                AddSub <= '0';
            else
                AddSub <= '1';
            end if;
         end if;
	end if;
end process;
------------------------------------------------------------------------------------------
Gain_est :entity work.GainACC --1delay
    Port map( clk => clk,
           rst => rst or overflow,
           Coef => Coef,
           AddSub => AddSub,
           Val => V_val_out_d,
--           gain_val => gain_val,
           Gain => Gain
           );
           Gain_unsigned <=  x"000" & MG_value & x"00000" when MGC_on = '1' else Gain   ;
------------------------------------------------------------------------------------------
FM_AGC_Mult_I : FM_AGC_Mult
  PORT MAP (
    CLK => CLK,
    A   => Iin,
    B   => Gain_unsigned,
    CE  => '1',
    P   => Iout_reg
  );
  
FM_AGC_Mult_Q : FM_AGC_Mult
  PORT MAP (
    CLK => CLK,
    A   => Qin,
    B   => Gain_unsigned,
    CE  => '1',
    P   => Qout_reg
  );

  Valout <= V_val;
------------------------------------------------------------------------------------------

process(CLK,Rst)
begin
	if (Rst='1') then
        Iin_d1 <= (others => '0');
        Iin_d2 <= (others => '0');
        Iin_d3 <= (others => '0');
        Iin_d4 <= (others => '0');
        Iin_d5 <= (others => '0');
        overflow <= '0';   
        Val_in_d1 <= '0';   
        Val_in_d2 <= '0';   
        Val_in_d3 <= '0';   
        Val_in_d4 <= '0';   
        Val_in_d5 <= '0';   
        overflow_cnt <= (others => '0');
	elsif rising_edge(CLK) then
        Iin_d1 <= Iin;
        Iin_d2 <= Iin_d1;
        Iin_d3 <= Iin_d2;
        Iin_d4 <= Iin_d3;
        Iin_d5 <= Iin_d4;
        Val_in_d1 <= Val_in;
        Val_in_d2 <= Val_in_d1;
        Val_in_d3 <= Val_in_d2;
        Val_in_d4 <= Val_in_d3;
        Val_in_d5 <= Val_in_d4;
        if overflow = '0' and Iin_d5(Iin_d5'left) /= Iout_reg(Iout_reg'left) and Val_in_d1 = '1' then
            overflow <= '1'; 
             overflow_cnt <= (others => '0');
        elsif overflow = '1' and  Iin_d5(Iin_d5'left) = Iout_reg(Iout_reg'left) and Val_in_d1 = '1' then
            if  overflow_cnt > 511 then
                overflow <= '0'; 
            else
                overflow_cnt <= overflow_cnt + 1;
            end if;   
        end if; 
	end if;
end process;



end Behavioral;
