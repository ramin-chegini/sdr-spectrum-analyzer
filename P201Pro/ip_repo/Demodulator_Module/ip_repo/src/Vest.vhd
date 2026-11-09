----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 05/29/2022 01:07:36 PM
-- Design Name: 
-- Module Name: Vest - Behavioral
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
--USE ieee.std_logic_UNsigned.all;
USE ieee.std_logic_signed.all;
--use ieee.numeric_std.all;
Library UNISIM;
use UNISIM.vcomponents.all;

entity Vest is
    generic( Stereo : boolean  := true;Width : integer := 24);
    Port ( clk : in STD_LOGIC;
           rst : in STD_LOGIC;
           Iin : in STD_LOGIC_VECTOR (Width-1 downto 0);
           Qin : in STD_LOGIC_VECTOR (Width-1 downto 0);
           Valin : in STD_LOGIC;
           Valout : out STD_LOGIC;
           Vout : out STD_LOGIC_VECTOR (Width-1 downto 0)
           );
end Vest;

architecture Behavioral of Vest is

signal Iabs,Qabs : STD_LOGIC_VECTOR (Width-1 downto 0);
signal  Val_d : STD_LOGIC;
attribute mark_debug : string;
attribute mark_debug of Iabs, Qabs : signal is "True";

begin

process(CLK,Rst)
begin
		if (Rst='1') then
            Iabs <= (others => '0');
            Val_d <= '0';
		elsif rising_edge(CLK) then
		      Val_d <= Valin;
		      if Valin ='1' then
                if (Iin(Width-1) = '0') then
                    Iabs <= Iin;
                else
                    Iabs <= NOT(Iin) + '1';
                end if;
              end if;
		end if;
end process;
process(CLK,Rst)
begin
		if (Rst='1') then
            Qabs <= (others => '0');
		elsif rising_edge(CLK) then
		      if Valin ='1' then
                if (Qin(Width-1) = '0') then
                    Qabs <= Qin;
                else
                    Qabs <= NOT(Qin) + '1';
                end if;
              end if;
		end if;
end process;
stereo_AGC:if Stereo = true generate 
begin
    process(CLK,Rst)
    begin
            if (Rst='1') then
                Vout <= (others => '0');
                Valout <= '0';
            elsif rising_edge(CLK) then
                if Val_d ='1' then
                    Vout <= Iabs + Qabs;
                end if;
                Valout <= Val_d;
            end if;
    end process;
end generate;
mono_AGC:if Stereo = FALSE generate 
begin
    process(CLK,Rst)
    begin
            if (Rst='1') then
                Vout <= (others => '0');
                Valout <= '0';
            elsif rising_edge(CLK) then
                if Val_d ='1' then
                    Vout <= Iabs;
                end if;
                Valout <= Val_d;
            end if;
    end process;
end generate;
end Behavioral;
