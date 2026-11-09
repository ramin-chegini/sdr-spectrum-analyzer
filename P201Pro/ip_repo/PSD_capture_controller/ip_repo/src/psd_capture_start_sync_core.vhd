library ieee;
use ieee.std_logic_1164.all;

entity psd_capture_start_sync_core is
    Port (
        clk        : in  std_logic;
        reset      : in  std_logic;
        lock_async : in  std_logic;
        lock_sync  : out std_logic
    );
end psd_capture_start_sync_core;

architecture Behavioral of psd_capture_start_sync_core is

    signal trig_meta : std_logic := '0';
    signal trig_sync : std_logic := '0';

begin

    process(clk)
    begin
        if rising_edge(clk) then

            if reset = '0' then
                trig_meta <= '0';
                trig_sync <= '0';
            else
                trig_meta <= lock_async;
                trig_sync <= trig_meta;
            end if;

        end if;
    end process;

    lock_sync <= trig_sync;

end Behavioral;