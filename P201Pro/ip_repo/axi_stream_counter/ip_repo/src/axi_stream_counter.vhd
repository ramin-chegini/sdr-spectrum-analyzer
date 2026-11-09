----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 08/05/2026 10:16:31 PM
-- Design Name: 
-- Module Name: axi_stream_counter - Behavioral
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
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity axi_stream_counter is
    generic
    (
        DATA_WIDTH  : integer := 64;
        PACKET_SIZE : integer := 1024
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
        -- enable = start packet
        -- A pulse on enable starts one packet.
        ------------------------------------------------------------------
        enable        : in  std_logic;

        ------------------------------------------------------------------
        -- AXI4-Stream Master Interface
        ------------------------------------------------------------------
        m_axis_tdata  : out std_logic_vector(DATA_WIDTH-1 downto 0);
        m_axis_tvalid : out std_logic;
        m_axis_tready : in  std_logic;
        m_axis_tlast  : out std_logic
    );

end entity;


architecture rtl of axi_stream_counter is

    ----------------------------------------------------------------------
    -- Sample counter
    --
    -- Data sequence:
    -- 0 ... 1023
    ----------------------------------------------------------------------
    signal counter : unsigned(DATA_WIDTH-1 downto 0);

    ----------------------------------------------------------------------
    -- Packet sample counter
    --
    -- Counts samples within the current packet:
    -- 0 ... 1023
    ----------------------------------------------------------------------
    signal packet_cnt : unsigned(15 downto 0);

    ----------------------------------------------------------------------
    -- Packet active flag
    --
    -- '1' = packet transmission is active
    -- '0' = waiting for the next packet to start
    ----------------------------------------------------------------------
    signal active : std_logic;

begin

    ----------------------------------------------------------------------
    -- AXI4-Stream DATA
    ----------------------------------------------------------------------
    m_axis_tdata <= std_logic_vector(counter);


    ----------------------------------------------------------------------
    -- AXI4-Stream VALID
    --
    -- Data is valid only while a packet is active.
    ----------------------------------------------------------------------
    m_axis_tvalid <= active;


    ----------------------------------------------------------------------
    -- AXI4-Stream TLAST
    --
    -- Sample 1023 is the last sample of the packet.
    ----------------------------------------------------------------------
    m_axis_tlast <= '1'
        when (
            active = '1' and
            packet_cnt = PACKET_SIZE - 1
        )
        else '0';


    ----------------------------------------------------------------------
    -- Counter / Packet Controller
    ----------------------------------------------------------------------
    process(aclk)
    begin

        if rising_edge(aclk) then

            ----------------------------------------------------------------
            -- RESET
            ----------------------------------------------------------------
            if aresetn = '0' then

                counter    <= (others => '0');
                packet_cnt <= (others => '0');
                active     <= '0';


            else

                ----------------------------------------------------------------
                -- IDLE
                --
                -- Wait for the packet start command.
                ----------------------------------------------------------------
                if active = '0' then

                    if enable = '1' then

                        ----------------------------------------------------
                        -- Start a new packet
                        ----------------------------------------------------
                        active     <= '1';
                        counter    <= (others => '0');
                        packet_cnt <= (others => '0');

                    end if;


                ----------------------------------------------------------------
                -- ACTIVE
                --
                -- The packet is currently being transmitted.
                ----------------------------------------------------------------
                elsif m_axis_tready = '1' then

                    ------------------------------------------------------------
                    -- Last sample
                    --
                    -- In this cycle:
                    --
                    -- packet_cnt = 1023
                    -- VALID      = 1
                    -- READY      = 1
                    -- TLAST      = 1
                    --
                    -- Therefore sample 1023 is transferred.
                    ------------------------------------------------------------
                    if packet_cnt = PACKET_SIZE - 1 then

                        --------------------------------------------------------
                        -- Packet is complete.
                        --
                        -- From the next clock cycle:
                        --
                        -- VALID = 0
                        --
                        -- Therefore samples:
                        --
                        -- 1024
                        -- 1025
                        -- 1026
                        -- 1027
                        --
                        -- will NOT be transmitted.
                        --------------------------------------------------------
                        active <= '0';


                    else

                        --------------------------------------------------------
                        -- Normal sample
                        --------------------------------------------------------
                        counter    <= counter + 1;
                        packet_cnt <= packet_cnt + 1;

                    end if;

                end if;

            end if;

        end if;

    end process;

end architecture;