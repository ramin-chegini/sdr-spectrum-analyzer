library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity counter is
port
(
    clk     : in  std_logic;
    rst     : in  std_logic;

    limit   : in  std_logic_vector(31 downto 0);

    led     : out std_logic
);
end entity;

architecture rtl of counter is

signal counter : unsigned(31 downto 0) := (others => '0');
signal led_reg : std_logic := '0';

begin

process(clk)
begin
    if rising_edge(clk) then

        if rst = '1' then

            counter <= (others => '0');
            led_reg <= '0';

        elsif counter >= unsigned(limit) then

            counter <= (others => '0');
            led_reg <= not led_reg;

        else

            counter <= counter + 1;

        end if;

    end if;
end process;


led <= led_reg;

end rtl;