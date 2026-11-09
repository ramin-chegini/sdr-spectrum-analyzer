library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


entity Demod_Sample_Buffer is
    Port (
        ----------------------------------------------------------------
        -- System Signals
        ----------------------------------------------------------------
        clk         : in  std_logic;
        rst         : in  std_logic;

        ----------------------------------------------------------------
        -- Data from Demodulator
        ----------------------------------------------------------------
        data_in     : in  std_logic_vector(15 downto 0);
        data_valid  : in  std_logic;

        ----------------------------------------------------------------
        -- Test Mode
        ----------------------------------------------------------------
        test_mode   : in  std_logic;

        ----------------------------------------------------------------
        -- One complete 512-sample bank is ready
        ----------------------------------------------------------------
        data_val    : out std_logic;

        ----------------------------------------------------------------
        -- Consumer is reading current bank
        ----------------------------------------------------------------
        mem_busy    : in  std_logic;

        ----------------------------------------------------------------
        -- RAM Read Interface
        --
        -- Consumer still sees only address 0..511.
        -- Bank selection is done internally.
        ----------------------------------------------------------------
        rd_addr     : in  std_logic_vector(8 downto 0);
        rd_data     : out std_logic_vector(15 downto 0)
    );

end Demod_Sample_Buffer;


architecture Behavioral of Demod_Sample_Buffer is


    --------------------------------------------------------------------
    -- Block RAM
    --
    -- NEW SIZE:
    --
    --     1024 x 16 bit
    --
    -- Bank 0:
    --     address 0..511
    --
    -- Bank 1:
    --     address 512..1023
    --------------------------------------------------------------------
    component blk_mem_Demod_Samples
        port (
            clka  : in  std_logic;
            wea   : in  std_logic_vector(0 downto 0);
            addra : in  std_logic_vector(9 downto 0);
            dina  : in  std_logic_vector(15 downto 0);

            clkb  : in  std_logic;
            addrb : in  std_logic_vector(9 downto 0);
            doutb : out std_logic_vector(15 downto 0)
        );
    end component;
    
    COMPONENT ila_dpram_chk

        PORT (
            clk : IN STD_LOGIC;
            probe0 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe1 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe2 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe3 : IN STD_LOGIC_VECTOR(8 DOWNTO 0); 
            probe4 : IN STD_LOGIC_VECTOR(15 DOWNTO 0); 
            probe5 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
            probe6 : IN STD_LOGIC_VECTOR(8 DOWNTO 0);
            probe7 : IN STD_LOGIC_VECTOR(15 DOWNTO 0)
        );
    END COMPONENT  ;    

    --------------------------------------------------------------------
    -- Local address inside each 512-sample bank
    --------------------------------------------------------------------
    signal wr_addr : unsigned(8 downto 0);


    --------------------------------------------------------------------
    -- Ping-Pong Bank Select
    --
    -- '0' -> addresses   0 .. 511
    -- '1' -> addresses 512 .. 1023
    --------------------------------------------------------------------
    signal write_bank : std_logic;
    signal read_bank  : std_logic;


    --------------------------------------------------------------------
    -- Bank Full Flags
    --------------------------------------------------------------------
    signal bank_full : std_logic_vector(1 downto 0);


    --------------------------------------------------------------------
    -- Consumer is actively reading one bank
    --------------------------------------------------------------------
    signal read_active : std_logic;


    --------------------------------------------------------------------
    -- Data Ready
    --------------------------------------------------------------------
    signal data_val_int : std_logic;


    --------------------------------------------------------------------
    -- BRAM Addresses
    --------------------------------------------------------------------
    signal ram_wr_addr : std_logic_vector(9 downto 0);
    signal ram_rd_addr : std_logic_vector(9 downto 0);


    --------------------------------------------------------------------
    -- RAM Write Enable
    --------------------------------------------------------------------
    signal wea_int : std_logic_vector(0 downto 0);


    --------------------------------------------------------------------
    -- RAM Data
    --------------------------------------------------------------------
    signal ram_dout : std_logic_vector(15 downto 0);

    signal write_data : std_logic_vector(15 downto 0);
    signal test_data  : std_logic_vector(15 downto 0);


    --------------------------------------------------------------------
    -- Current Write Bank Status
    --------------------------------------------------------------------
    signal current_write_bank_full : std_logic;


begin


    --------------------------------------------------------------------
    -- BRAM Address Generation
    --
    -- MSB selects Bank.
    --------------------------------------------------------------------
    ram_wr_addr <=
        write_bank & std_logic_vector(wr_addr);

    ram_rd_addr <=
        read_bank & rd_addr;


    --------------------------------------------------------------------
    -- Block RAM
    --------------------------------------------------------------------
    Inst_blk_mem_Demod_Samples : blk_mem_Demod_Samples
        port map (

            ------------------------------------------------------------
            -- Port A : WRITE
            ------------------------------------------------------------
            clka  => clk,
            wea   => wea_int,
            addra => ram_wr_addr,
            dina  => write_data,

            ------------------------------------------------------------
            -- Port B : READ
            ------------------------------------------------------------
            clkb  => clk,
            addrb => ram_rd_addr,
            doutb => ram_dout
        );



    Inst_ila_dpram_chk : ila_dpram_chk
        PORT MAP (
            clk => clk,
            probe0(0) => data_valid, 
            probe1(0) => data_val_int, 
            probe2(0) => mem_busy, 
            probe3 => std_logic_vector(wr_addr), 
            probe4 => write_data, 
            probe5 => wea_int, 
            probe6 => rd_addr,
            probe7 => ram_dout
        );

    --------------------------------------------------------------------
    -- Test Pattern
    --
    -- Each bank contains:
    --
    -- A000
    -- A001
    -- ...
    -- A1FF
    --------------------------------------------------------------------
    test_data <=
        std_logic_vector(
            to_unsigned(16#A000#, 16) +
            resize(wr_addr, 16)
        );


    --------------------------------------------------------------------
    -- Input Data Selection
    --------------------------------------------------------------------
    write_data <=
        test_data
        when test_mode = '1'
        else data_in;


    --------------------------------------------------------------------
    -- Byte Swap
    --
    -- Keep your existing FPGA -> Software byte order.
    --------------------------------------------------------------------
    rd_data <=
        ram_dout(7 downto 0) &
        ram_dout(15 downto 8);


    --------------------------------------------------------------------
    -- Data Ready Output
    --------------------------------------------------------------------
    data_val <= data_val_int;


    --------------------------------------------------------------------
    -- Is the bank currently selected for WRITE already full?
    --------------------------------------------------------------------
    current_write_bank_full <=
        bank_full(0)
        when write_bank = '0'
        else
        bank_full(1);


    --------------------------------------------------------------------
    -- WRITE ENABLE
    --
    -- IMPORTANT:
    --
    -- mem_busy is NOT used here anymore.
    --
    -- While one bank is being read, the other bank continues
    -- receiving Demodulator samples.
    --------------------------------------------------------------------
    wea_int(0) <= '1'
        when
        (
            data_valid = '1' and
            current_write_bank_full = '0'
        )
        else '0';


    --------------------------------------------------------------------
    -- Main Ping-Pong Controller
    --------------------------------------------------------------------
    process(clk, rst)

    begin

        if rst = '1' then

            ------------------------------------------------------------
            -- Start writing Bank 0
            ------------------------------------------------------------
            write_bank <= '0';

            wr_addr <= (others => '0');


            ------------------------------------------------------------
            -- Default Read Bank
            ------------------------------------------------------------
            read_bank <= '0';


            ------------------------------------------------------------
            -- Both banks are initially empty
            ------------------------------------------------------------
            bank_full <= (others => '0');


            ------------------------------------------------------------
            -- No read operation
            ------------------------------------------------------------
            read_active <= '0';

            data_val_int <= '0';


        elsif rising_edge(clk) then


            ----------------------------------------------------------------
            -- ============================================================
            -- READ-SIDE CONTROL
            -- ============================================================
            ----------------------------------------------------------------

            ------------------------------------------------------------
            -- Consumer is currently reading one bank
            ------------------------------------------------------------
            if read_active = '1' then

                --------------------------------------------------------
                -- Reading finished
                --------------------------------------------------------
                if mem_busy = '0' then

                    ----------------------------------------------------
                    -- Release the bank that was just read
                    ----------------------------------------------------
                    if read_bank = '0' then

                        bank_full(0) <= '0';

                    else

                        bank_full(1) <= '0';

                    end if;

                    read_active <= '0';

                end if;


            else

                --------------------------------------------------------
                -- A bank has already been offered to consumer
                --------------------------------------------------------
                if data_val_int = '1' then

                    ----------------------------------------------------
                    -- Consumer accepted it and started reading
                    ----------------------------------------------------
                    if mem_busy = '1' then

                        data_val_int <= '0';

                        read_active <= '1';

                    end if;


                else

                    ----------------------------------------------------
                    -- Find a complete bank waiting for read
                    ----------------------------------------------------
                    if bank_full(0) = '1' then

                        read_bank <= '0';

                        data_val_int <= '1';


                    elsif bank_full(1) = '1' then

                        read_bank <= '1';

                        data_val_int <= '1';

                    end if;

                end if;

            end if;



            ----------------------------------------------------------------
            -- ============================================================
            -- WRITE-SIDE CONTROL
            -- ============================================================
            --
            -- Completely independent from mem_busy.
            --
            -- As long as the selected write bank is free,
            -- incoming samples are written continuously.
            ----------------------------------------------------------------

            if
            (
                data_valid = '1' and
                current_write_bank_full = '0'
            )
            then

                ------------------------------------------------------------
                -- Current sample is the 512th sample
                ------------------------------------------------------------
                if wr_addr = to_unsigned(511, 9) then


                    --------------------------------------------------------
                    -- Mark current bank FULL
                    --------------------------------------------------------
                    if write_bank = '0' then

                        bank_full(0) <= '1';

                    else

                        bank_full(1) <= '1';

                    end if;


                    --------------------------------------------------------
                    -- Restart local address from zero
                    --------------------------------------------------------
                    wr_addr <= (others => '0');


                    --------------------------------------------------------
                    -- Switch to the other bank
                    --------------------------------------------------------
                    write_bank <= not write_bank;


                else

                    --------------------------------------------------------
                    -- Continue filling current bank
                    --------------------------------------------------------
                    wr_addr <= wr_addr + 1;

                end if;

            end if;


        end if;

    end process;


end Behavioral;
