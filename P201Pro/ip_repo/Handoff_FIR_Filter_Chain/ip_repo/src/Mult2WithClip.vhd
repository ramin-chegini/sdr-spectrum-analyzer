library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use IEEE.STD_LOGIC_UNSIGNED.ALL;
entity Mult2WithClip is
	generic(
		Nb	: natural := 16
	);
	port(
		rstn	: in std_logic;
		Clk	: in std_logic;
		x_1	: in std_logic_vector(Nb  downto 0);	
		x_2	: in std_logic_vector(Nb  downto 0);	
		y_1	: out std_logic_vector(Nb - 1 downto 0);
		y_2	: out std_logic_vector(Nb - 1 downto 0)
	);
end Mult2WithClip;

architecture bhv of Mult2WithClip is





begin
	process(Clk,rstn)
	begin
	    if rstn = '0' then
	       y_1 <= (others => 'Z');
		elsif rising_edge(Clk) then
			if x_1(Nb ) = x_1(Nb - 1) then
				y_1 <= x_1(Nb - 1 downto 0) ;
			else -- Clip positive or negative sides.
				y_1(Nb - 1) <= x_1(Nb);
				y_1(Nb - 2 downto 0) <= x_1(Nb - 2 downto 0);
			end if;
		end if;
	end process;
	process(Clk,rstn)
	begin
	   if rstn = '0' then
            y_2 <= (others => 'Z');
       elsif rising_edge(Clk) then
			if x_2(Nb) = x_2(Nb - 1) then
				y_2 <= x_2(Nb - 1 downto 0) ;
			else -- Clip positive or negative sides.
				y_2(Nb - 1) <= x_2(Nb);
				y_2(Nb - 2 downto 0) <= x_2(Nb - 2 downto 0);
			end if;
		end if;
	end process;	
end bhv;
