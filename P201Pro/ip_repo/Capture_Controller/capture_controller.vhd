library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


entity capture_controller is

generic
(
    DATA_WIDTH  : integer := 64;
    SAMPLE_COUNT : integer := 1024
);


port
(
    clk           : in  std_logic;
    rstn          : in  std_logic;


    ----------------------------------------------------
    -- Capture command
    ----------------------------------------------------
    capture_start : in std_logic;


    ----------------------------------------------------
    -- Input stream from ADC/source
    ----------------------------------------------------
    s_axis_tdata  : in  std_logic_vector(DATA_WIDTH-1 downto 0);
    s_axis_tvalid : in  std_logic;
    s_axis_tready : out std_logic;


    ----------------------------------------------------
    -- Output stream to FIFO/DMA
    ----------------------------------------------------
    m_axis_tdata  : out std_logic_vector(DATA_WIDTH-1 downto 0);
    m_axis_tvalid : out std_logic;
    m_axis_tready : in  std_logic;
    m_axis_tlast  : out std_logic;


    ----------------------------------------------------
    -- Status
    ----------------------------------------------------
    capture_done  : out std_logic

);

end entity;


architecture rtl of capture_controller is


signal active : std_logic;
signal capture_start_Prev : std_logic;

signal sample_counter : unsigned(15 downto 0);


begin


--------------------------------------------------------
-- Stream forwarding
--------------------------------------------------------

m_axis_tdata <= s_axis_tdata;

--------------------------------------------------------
-- Only forward data during capture
--------------------------------------------------------

m_axis_tvalid <= s_axis_tvalid and active;

--------------------------------------------------------
-- Back pressure to source
--------------------------------------------------------

s_axis_tready <= m_axis_tready and active ;

--------------------------------------------------------
-- TLAST generation
--------------------------------------------------------
m_axis_tlast <= '1'
    when (
        s_axis_tvalid = '1' and active = '1' and
        sample_counter = SAMPLE_COUNT-1
    )
    else '0';

--------------------------------------------------------
-- Capture controller
--------------------------------------------------------

process(clk)
begin

if rising_edge(clk) then

    if rstn='0' then

        active             <= '0';
        capture_start_Prev <= '0';
        sample_counter     <= (others=>'0');
        capture_done       <= '0';

    else

        capture_done <= '0';
        capture_start_Prev <= capture_start ; 

        if (capture_start_Prev = '0' and capture_start='1') then

            active <= '1';
            sample_counter <= (others=>'0');


        elsif active='1' then


            if s_axis_tvalid='1' and  m_axis_tready='1' then

                if sample_counter = SAMPLE_COUNT-1 then

                    active <= '0';
                    capture_done <= '1';
                    sample_counter <= (others=>'0');


                else

                    sample_counter <= sample_counter + 1;

                end if;


            end if;

        end if;

    end if;

end if;

end process;


end architecture;