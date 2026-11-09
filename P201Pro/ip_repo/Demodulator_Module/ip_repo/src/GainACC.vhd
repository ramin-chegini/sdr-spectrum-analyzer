----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 05/29/2022 04:09:44 PM
-- Design Name: 
-- Module Name: GainACC - Behavioral
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

entity GainACC is
    Port ( clk : in STD_LOGIC;
           rst : in STD_LOGIC;
           Coef : in STD_LOGIC_VECTOR (7 downto 0);
           AddSub : in STD_LOGIC;
           Val : in STD_LOGIC;
           Gain : out STD_LOGIC_VECTOR (63 downto 0));
end GainACC;

architecture Behavioral of GainACC is
signal Gain_reg : STD_LOGIC_VECTOR (71 downto 0);
signal E : STD_LOGIC_VECTOR (71 downto 0);
signal Coef_notzero : STD_LOGIC_VECTOR (7 downto 0);
signal LSBNO : integer range  0 to 65535 := 0;

begin

Gain <= Gain_reg(63 downto 0);
Coef_notzero <= "00000001" when Coef = x"00" else Coef;
LSBNO <= CONV_INTEGER (Coef) when Coef/=0 else 4;
process(CLK,Rst)
begin
	if (Rst='1') then
        Gain_reg <= x"000000000100000000";
        E <= (others => '0');
	elsif rising_edge(CLK) then
	    if Val = '1' then 
	       E <= ext(Gain_reg(71 downto LSBNO) , 72);
            if AddSub = '1' then
                if Gain_reg >= x"0FFFFFFFFFFFF0000" then
                    Gain_reg <= Gain_reg ;
                 else
                    Gain_reg <= Gain_reg  + E;--Coef_notzero *  X"0000000000000ffF"  ;
                 end if ;
            else
                if (Gain_reg) > (x"000000000000FFFF") then
                    Gain_reg <= Gain_reg   - E;--Coef_notzero * X"0000000000000ffF"  ;
                else
                    Gain_reg <= Gain_reg ;
                END IF;
            end if;
         end if;
	end if;
end process;


end Behavioral;
