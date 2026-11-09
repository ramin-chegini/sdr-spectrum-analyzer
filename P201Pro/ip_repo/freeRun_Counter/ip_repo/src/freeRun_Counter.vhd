library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity freeRun_Counter is
generic
(
    DATA_WIDTH : integer := 64
);

port
(
    ------------------------------------------------------------------
    -- Global Signals
    ------------------------------------------------------------------
    aclk          : in  std_logic;
    aresetn       : in  std_logic;

    ------------------------------------------------------------------
    -- Control
    --
    -- enable = allow counter to generate data
    ------------------------------------------------------------------
    --enable        : in  std_logic;

    ------------------------------------------------------------------
    -- AXI4-Stream Master Interface
    ------------------------------------------------------------------
    m_axis_tdata  : out std_logic_vector(DATA_WIDTH-1 downto 0);
    m_axis_tvalid : out std_logic;
    m_axis_tready : in  std_logic
    --m_axis_tlast  : out std_logic
);

end entity;

architecture rtl of freeRun_Counter is

    ------------------------------------------------------------------
    -- Sample counter
    ------------------------------------------------------------------
    signal counter : unsigned(DATA_WIDTH-1 downto 0);

begin
    ------------------------------------------------------------------
    -- Data
    ------------------------------------------------------------------
    m_axis_tdata <= std_logic_vector(counter);

    ------------------------------------------------------------------
    -- VALID
    --
    -- Counter produces data whenever enable is active.
    ------------------------------------------------------------------
    m_axis_tvalid <= '1';

    ------------------------------------------------------------------
    -- TLAST
    --
    -- Counter does NOT generate packet boundaries.
    -- Capture Controller will generate TLAST.
    ------------------------------------------------------------------
    --m_axis_tlast <= '0';

    ------------------------------------------------------------------
    -- Counter
    ------------------------------------------------------------------
    process(aclk)
    begin

        if rising_edge(aclk) then

            ----------------------------------------------------------
            -- Reset
            ----------------------------------------------------------
            if aresetn = '0' then

                counter <= (others => '0');


            ----------------------------------------------------------
            -- Normal operation
            ----------------------------------------------------------
            else

                ------------------------------------------------------
                -- Advance only after a successful AXI transfer
                --
                -- Transfer occurs when:
                --
                -- TVALID = 1
                -- TREADY = 1
                --
                ------------------------------------------------------
                if m_axis_tready = '1' then

                    counter <= counter + 1;

                end if;

            end if;

        end if;

    end process;

end architecture;