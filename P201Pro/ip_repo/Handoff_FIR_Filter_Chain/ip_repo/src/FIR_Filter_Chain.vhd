----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 08/31/2026 11:22:27 AM
-- Design Name: 
-- Module Name: FIR_Filter_Chain - Behavioral
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

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity FIR_Filter_Chain is
	generic(
		Nq			: natural := 16;
		Na			: natural := 24;
		Nb			: natural := 24
	);

	port(
		clk			: in std_logic;
		rstn    	: in std_logic;
	
		Din_I		: in std_logic_vector (Nq - 1 downto 0);
		Din_Q		: in std_logic_vector (Nq - 1 downto 0);
		BandWidth_Select: in std_logic_vector (3 downto 0);
		IQ_Data_Val : in std_logic;
	    I_DataOut	: out std_logic_vector(Nb-1 downto 0);
	    Q_DataOut	: out std_logic_vector(Nb-1 downto 0);
	    IQ_Val_128Kbit: out std_logic;
	    IQ_Val_Out: out std_logic


	);
end FIR_Filter_Chain;

architecture Behavioral of FIR_Filter_Chain is

signal fir_s_axis_data_tready :std_logic;
signal s_axis_data_tdata :  STD_LOGIC_VECTOR(31 DOWNTO 0);
signal s_axis_data_tdata_200KHz :  STD_LOGIC_VECTOR(31 DOWNTO 0);
signal s_axis_data_tdata_150KHz :  STD_LOGIC_VECTOR(47 DOWNTO 0);
signal s_axis_data_tdata_60KHz :  STD_LOGIC_VECTOR(47 DOWNTO 0);
signal s_axis_data_tdata_25KHz :  STD_LOGIC_VECTOR(47 DOWNTO 0);
signal s_axis_data_tdata_30KHz :  STD_LOGIC_VECTOR(47 DOWNTO 0);
signal s_axis_data_tdata_15KHz :  STD_LOGIC_VECTOR(47 DOWNTO 0);
signal s_axis_data_tdata_12KHz :  STD_LOGIC_VECTOR(47 DOWNTO 0);
signal s_axis_data_tdata_7_5KHz :  STD_LOGIC_VECTOR(47 DOWNTO 0);
signal s_axis_data_tdata_5KHz :  STD_LOGIC_VECTOR(47 DOWNTO 0);
signal s_axis_data_tdata_3KHz :  STD_LOGIC_VECTOR(47 DOWNTO 0);
signal s_axis_data_tdata_1_2KHz :  STD_LOGIC_VECTOR(47 DOWNTO 0);
signal s_axis_data_tdata_600Hz :  STD_LOGIC_VECTOR(47 DOWNTO 0);

signal Decimator_Filter_m_axis_data_tvalid :  STD_LOGIC;
signal Filt200KHz_m_axis_data_tvalid :  STD_LOGIC;
signal Filt150KHz_m_axis_data_tvalid :  STD_LOGIC;
signal Filt60KHz_m_axis_data_tvalid :  STD_LOGIC;
signal Filt25KHz_m_axis_data_tvalid :  STD_LOGIC;
signal Filt30KHz_m_axis_data_tvalid :  STD_LOGIC;
signal Filt15KHz_m_axis_data_tvalid :  STD_LOGIC;
signal Filt12KHz_m_axis_data_tvalid :  STD_LOGIC;
signal Filt7_5KHz_m_axis_data_tvalid :  STD_LOGIC;
signal Filt5KHz_m_axis_data_tvalid :  STD_LOGIC;
signal Filt3KHz_m_axis_data_tvalid :  STD_LOGIC;
signal Filt1_2KHz_m_axis_data_tvalid :  STD_LOGIC;
signal Filt600Hz_m_axis_data_tvalid :  STD_LOGIC;
signal Decimator_Filter_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);

signal Filt200KHz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);
signal Filt150KHz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);
signal Filt60KHz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);
signal Filt25KHz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);
signal Filt30KHz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);
signal Filt15KHz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);
signal Filt12KHz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);
signal Filt7_5KHz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);
signal Filt5KHz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);
signal Filt3KHz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);
signal Filt1_2KHz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);
signal Filt600Hz_m_axis_data_tdata :  STD_LOGIC_VECTOR(63 DOWNTO 0);

signal sI_Dec		: std_logic_vector(Nb downto 0);
signal sI_200KHz		: std_logic_vector(Nb downto 0);
signal sI_150KHz		: std_logic_vector(Nb downto 0);
signal sI_60KHz		: std_logic_vector(Nb downto 0);
signal sI_30KHz		: std_logic_vector(Nb downto 0);
signal sI_25KHz		: std_logic_vector(Nb downto 0);
signal sI_15KHz		: std_logic_vector(Nb downto 0);
signal sI_12KHz		: std_logic_vector(Nb downto 0);
signal sI_7_5KHz		: std_logic_vector(Nb downto 0);
signal sI_5KHz		: std_logic_vector(Nb downto 0);
signal sI_3KHz		: std_logic_vector(Nb downto 0);
signal sI_1_2KHz		: std_logic_vector(Nb downto 0);
signal sI_600Hz		: std_logic_vector(Nb downto 0);

signal I_DecOut	: std_logic_vector(Nb-1 downto 0);
signal I_200KHzOut	: std_logic_vector(Nb-1 downto 0);
signal I_150KHzOut	: std_logic_vector(Nb-1 downto 0);
signal I_60KHzOut	: std_logic_vector(Nb-1 downto 0);
--signal I_25KHzOut	: std_logic_vector(Nb-1 downto 0);
signal I_30KHzOut	: std_logic_vector(Nb-1 downto 0);
signal I_15KHzOut	: std_logic_vector(Nb-1 downto 0);
signal I_12KHzOut	: std_logic_vector(Nb-1 downto 0);
signal I_7_5KHzOut	: std_logic_vector(Nb-1 downto 0);
signal I_5KHzOut	: std_logic_vector(Nb-1 downto 0);
signal I_3KHzOut	: std_logic_vector(Nb-1 downto 0);
signal I_1_2KHzOut	: std_logic_vector(Nb-1 downto 0);
signal I_600HzOut	: std_logic_vector(Nb-1 downto 0);

signal sQ_Dec		: std_logic_vector(Nb downto 0);
signal sQ_200KHz		: std_logic_vector(Nb downto 0);
signal sQ_150KHz		: std_logic_vector(Nb downto 0);
signal sQ_60KHz		: std_logic_vector(Nb downto 0);
signal sQ_25KHz		: std_logic_vector(Nb downto 0);
signal sQ_30KHz		: std_logic_vector(Nb downto 0);
signal sQ_15KHz		: std_logic_vector(Nb downto 0);
signal sQ_12KHz		: std_logic_vector(Nb downto 0);
signal sQ_7_5KHz		: std_logic_vector(Nb downto 0);
signal sQ_5KHz		: std_logic_vector(Nb downto 0);
signal sQ_3KHz		: std_logic_vector(Nb downto 0);
signal sQ_1_2KHz		: std_logic_vector(Nb downto 0);
signal sQ_600Hz		: std_logic_vector(Nb downto 0);

signal Q_DecOut	: std_logic_vector(Nb-1 downto 0);
signal Q_200KHzOut	: std_logic_vector(Nb-1 downto 0);
signal Q_150KHzOut	: std_logic_vector(Nb-1 downto 0);
signal Q_60KHzOut	: std_logic_vector(Nb-1 downto 0);
--signal Q_25KHzOut	: std_logic_vector(Nb-1 downto 0);
signal Q_30KHzOut	: std_logic_vector(Nb-1 downto 0);
signal Q_15KHzOut	: std_logic_vector(Nb-1 downto 0);
signal Q_12KHzOut	: std_logic_vector(Nb-1 downto 0);
signal Q_7_5KHzOut	: std_logic_vector(Nb-1 downto 0);
signal Q_5KHzOut	: std_logic_vector(Nb-1 downto 0);
signal Q_3KHzOut	: std_logic_vector(Nb-1 downto 0);
signal Q_1_2KHzOut	: std_logic_vector(Nb-1 downto 0);
signal Q_600HzOut	: std_logic_vector(Nb-1 downto 0);

signal IQ_VAL_Decimator	: std_logic;
signal IQ_VAL_200KHz	: std_logic;
signal IQ_VAL_150KHz	: std_logic;
signal IQ_VAL_60KHz	: std_logic;
--signal IQ_VAL_25KHz	: std_logic;
signal IQ_VAL_30KHz	: std_logic;
signal IQ_VAL_15KHz	: std_logic;
signal IQ_VAL_12KHz	: std_logic;
signal IQ_VAL_7_5KHz	: std_logic;
signal IQ_VAL_5KHz	: std_logic;
signal IQ_VAL_3KHz	: std_logic;
signal IQ_VAL_1_2KHz	: std_logic;
signal IQ_VAL_600Hz	: std_logic;



--COMPONENT fir_dec2_stage0
--  PORT (
--    aresetn : IN STD_LOGIC;
--    aclk : IN STD_LOGIC;
--    s_axis_data_tvalid : IN STD_LOGIC;
--    s_axis_data_tready : OUT STD_LOGIC;
--    s_axis_data_tdata : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
--    m_axis_data_tvalid : OUT STD_LOGIC;
--    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
--  );
--END COMPONENT;

COMPONENT fir_compiler_200KHz
  PORT (
    aresetn : IN STD_LOGIC;
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

COMPONENT fir_compiler_150KHz
  PORT (
    aresetn : IN STD_LOGIC;
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

COMPONENT fir_compiler_60KHz
  PORT (
    aresetn : IN STD_LOGIC;
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

COMPONENT fir_compiler_30KHz
  PORT (
    aresetn : IN STD_LOGIC;
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

COMPONENT fir_compiler_15KHz
  PORT (
    aresetn : IN STD_LOGIC;
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

COMPONENT fir_compiler_12KHz
  PORT (
    aresetn : IN STD_LOGIC;
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

COMPONENT fir_compiler_7_5KHz
  PORT (
    aresetn : IN STD_LOGIC;
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

COMPONENT fir_compiler_5KHz
  PORT (
    aresetn : IN STD_LOGIC;
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

COMPONENT fir_compiler_3KHz
  PORT (
    aresetn : IN STD_LOGIC;
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

COMPONENT fir_compiler_1_2KHz
  PORT (
    aresetn : IN STD_LOGIC;
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;

COMPONENT fir_compiler_600Hz
  PORT (
    aresetn : IN STD_LOGIC;
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
  );
END COMPONENT;


begin

I_DecOut <= std_logic_vector(resize(signed(Din_I), I_DecOut'length));
Q_DecOut <= std_logic_vector(resize(signed(Din_Q), Q_DecOut'length));
IQ_VAL_Decimator <= IQ_Data_Val;
---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

--    s_axis_data_tdata <= Din_Q & Din_I;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 2
    -- Fs_in  = 2.083340 MHz
    -- Fs_out = 1.04167MSPS
    --------------------------------------------------------------------

--Inst_fir_dec2_stage0 : fir_dec2_stage0 
--  PORT MAP (
--    aresetn             => rstn,
--    aclk                => clk,
--    s_axis_data_tvalid  => IQ_Data_Val,
--    s_axis_data_tready  => fir_s_axis_data_tready,
--    s_axis_data_tdata   => s_axis_data_tdata,
--    m_axis_data_tvalid  => Decimator_Filter_m_axis_data_tvalid,
--    m_axis_data_tdata   => Decimator_Filter_m_axis_data_tdata
--  );
  
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

--    sI_Dec <= Decimator_Filter_m_axis_data_tdata(24 downto 0);

--    sQ_Dec <= Decimator_Filter_m_axis_data_tdata(56 downto 32);

--    IQ_VAL_Decimator <= Decimator_Filter_m_axis_data_tvalid;
    
    
--Mult2WithClipDecimator_I: entity work.Mult2WithClip
--    generic map(Nb    => Nb)
--    port map(
--        clk => clk,
--        rstn => rstn,
--        x_1    => sI_Dec,
--        x_2    => sQ_Dec,
--        y_1    => I_DecOut,
--        y_2    => Q_DecOut
--    ); 
    
    
    
---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

    s_axis_data_tdata_200KHz <= Din_Q & Din_I;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 4
    -- Fs_in  = 1.04167 MHz
    -- Fs_out = 260.4175KHZ
    --------------------------------------------------------------------
        
Inst_fir_compiler_200KHz : fir_compiler_200KHz 
  PORT MAP (
    aresetn             => rstn,
    aclk                => clk,
--    s_axis_data_tvalid  => '1',
    s_axis_data_tvalid  => IQ_Data_Val,
    s_axis_data_tready  => open,
    s_axis_data_tdata   => s_axis_data_tdata_200KHz,
    m_axis_data_tvalid  => Filt200KHz_m_axis_data_tvalid,
    m_axis_data_tdata   => Filt200KHz_m_axis_data_tdata
  );    
    
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

    sI_200KHz <= Filt200KHz_m_axis_data_tdata(24 downto 0);

    sQ_200KHz <= Filt200KHz_m_axis_data_tdata(56 downto 32);

    IQ_VAL_200KHz <= Filt200KHz_m_axis_data_tvalid;
    
          
 Mult2WithClip200KHz_I: entity work.Mult2WithClip
    generic map(Nb    => Nb)
    port map(
        clk => clk,
        rstn => rstn,
        x_1    => sI_200KHz,
        x_2    => sQ_200KHz,
        y_1    => I_200KHzOut,
        y_2    => Q_200KHzOut
    );   
    
    ---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

    s_axis_data_tdata_150KHz <= Q_200KHzOut & I_200KHzOut;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 2
    -- Fs_in  = 260.4175 KHz
    -- Fs_out = 260.4175 KHz
    --------------------------------------------------------------------
        
Inst_fir_compiler_150KHz : fir_compiler_150KHz 
  PORT MAP (
    aresetn             => rstn,
    aclk                => clk,
    s_axis_data_tvalid  => IQ_VAL_200KHz,
    s_axis_data_tready  => open,
    s_axis_data_tdata   => s_axis_data_tdata_150KHz,
    m_axis_data_tvalid  => Filt150KHz_m_axis_data_tvalid,
    m_axis_data_tdata   => Filt150KHz_m_axis_data_tdata
  );    
    
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

    sI_150KHz <= Filt150KHz_m_axis_data_tdata(24 downto 0);

    sQ_150KHz <= Filt150KHz_m_axis_data_tdata(56 downto 32);

    IQ_VAL_150KHz <= Filt150KHz_m_axis_data_tvalid;
    
          
 Mult2WithClip150KHz_I: entity work.Mult2WithClip
    generic map(Nb    => Nb)
    port map(
        clk => clk,
        rstn => rstn,
        x_1    => sI_150KHz,
        x_2    => sQ_150KHz,
        y_1    => I_150KHzOut,
        y_2    => Q_150KHzOut
    );     
    
    
    ---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

    s_axis_data_tdata_60KHz <= Q_200KHzOut & I_200KHzOut;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 2
    -- Fs_in  = 260.4175KHz
    -- Fs_out = 130.20875KSPS
    --------------------------------------------------------------------
        
Inst_fir_compiler_60KHz : fir_compiler_60KHz 
  PORT MAP (
    aresetn             => rstn,
    aclk                => clk,
    s_axis_data_tvalid  => IQ_VAL_200KHz,
    s_axis_data_tready  => open,
    s_axis_data_tdata   => s_axis_data_tdata_60KHz,
    m_axis_data_tvalid  => Filt60KHz_m_axis_data_tvalid,
    m_axis_data_tdata   => Filt60KHz_m_axis_data_tdata
  );    
    
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

    sI_60KHz <= Filt60KHz_m_axis_data_tdata(24 downto 0);

    sQ_60KHz <= Filt60KHz_m_axis_data_tdata(56 downto 32);

    IQ_VAL_60KHz   <= Filt60KHz_m_axis_data_tvalid;
    IQ_Val_128Kbit <= Filt60KHz_m_axis_data_tvalid;
    
          
 Mult2WithClip60KHz_I: entity work.Mult2WithClip
    generic map(Nb    => Nb)
    port map(
        clk => clk,
        rstn => rstn,
        x_1    => sI_60KHz,
        x_2    => sQ_60KHz,
        y_1    => I_60KHzOut,
        y_2    => Q_60KHzOut
    ); 
    
    
    ---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

    s_axis_data_tdata_30KHz <= Q_60KHzOut & I_60KHzOut;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 2
    -- Fs_in  = 130.20875KHz
    -- Fs_out = 65.104375KSPS
    --------------------------------------------------------------------
        
Inst_fir_compiler_30KHz : fir_compiler_30KHz 
  PORT MAP (
    aresetn             => rstn,
    aclk                => clk,
    s_axis_data_tvalid  => IQ_VAL_60KHz,
    s_axis_data_tready  => open,
    s_axis_data_tdata   => s_axis_data_tdata_30KHz,
    m_axis_data_tvalid  => Filt30KHz_m_axis_data_tvalid,
    m_axis_data_tdata   => Filt30KHz_m_axis_data_tdata
  );    
    
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

    sI_30KHz <= Filt30KHz_m_axis_data_tdata(24 downto 0);

    sQ_30KHz <= Filt30KHz_m_axis_data_tdata(56 downto 32);

    IQ_VAL_30KHz <= Filt30KHz_m_axis_data_tvalid;

    
          
 Mult2WithClip30KHz_I: entity work.Mult2WithClip
    generic map(Nb    => Nb)
    port map(
        clk => clk,
        rstn => rstn,
        x_1    => sI_30KHz,
        x_2    => sQ_30KHz,
        y_1    => I_30KHzOut,
        y_2    => Q_30KHzOut
    ); 
    
    
    ---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

    s_axis_data_tdata_15KHz <= Q_30KHzOut & I_30KHzOut;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 2
    -- Fs_in  = 65.104375KHz
    -- Fs_out = 32.5521875KSPS
    --------------------------------------------------------------------
        
Inst_fir_compiler_15KHz : fir_compiler_15KHz 
  PORT MAP (
    aresetn             => rstn,
    aclk                => clk,
    s_axis_data_tvalid  => IQ_VAL_30KHz,
    s_axis_data_tready  => open,
    s_axis_data_tdata   => s_axis_data_tdata_15KHz,
    m_axis_data_tvalid  => Filt15KHz_m_axis_data_tvalid,
    m_axis_data_tdata   => Filt15KHz_m_axis_data_tdata
  );    
    
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

    sI_15KHz <= Filt15KHz_m_axis_data_tdata(24 downto 0);

    sQ_15KHz <= Filt15KHz_m_axis_data_tdata(56 downto 32);

    IQ_VAL_15KHz <= Filt15KHz_m_axis_data_tvalid;
    
          
 Mult2WithClip15KHz_I: entity work.Mult2WithClip
    generic map(Nb    => Nb)
    port map(
        clk => clk,
        rstn => rstn,
        x_1    => sI_15KHz,
        x_2    => sQ_15KHz,
        y_1    => I_15KHzOut,
        y_2    => Q_15KHzOut
    );   
    
    ---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

    s_axis_data_tdata_12KHz <= Q_30KHzOut & I_30KHzOut;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 2
    -- Fs_in  = 65.104375KHz
    -- Fs_out = 32.5521875KSPS
    --------------------------------------------------------------------
        
Inst_fir_compiler_12KHz : fir_compiler_12KHz 
  PORT MAP (
    aresetn             => rstn,
    aclk                => clk,
    s_axis_data_tvalid  => IQ_VAL_30KHz,
    s_axis_data_tready  => open,
    s_axis_data_tdata   => s_axis_data_tdata_12KHz,
    m_axis_data_tvalid  => Filt12KHz_m_axis_data_tvalid,
    m_axis_data_tdata   => Filt12KHz_m_axis_data_tdata
  );    
    
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

    sI_12KHz <= Filt12KHz_m_axis_data_tdata(24 downto 0);

    sQ_12KHz <= Filt12KHz_m_axis_data_tdata(56 downto 32);

    IQ_VAL_12KHz <= Filt12KHz_m_axis_data_tvalid;
    
          
 Mult2WithClip12KHz_I: entity work.Mult2WithClip
    generic map(Nb    => Nb)
    port map(
        clk => clk,
        rstn => rstn,
        x_1    => sI_12KHz,
        x_2    => sQ_12KHz,
        y_1    => I_12KHzOut,
        y_2    => Q_12KHzOut
    );
    
    ---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

    s_axis_data_tdata_7_5KHz <= Q_12KHzOut & I_12KHzOut;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 2
    -- Fs_in  = 32.5521875KHz
    -- Fs_out = 16.27609375KSPS
    --------------------------------------------------------------------
        
Inst_fir_compiler_7_5KHz : fir_compiler_7_5KHz 
  PORT MAP(
  
    aresetn             => rstn,
    aclk                => clk,
    s_axis_data_tvalid  => IQ_VAL_12KHz,
    s_axis_data_tready  => open,
    s_axis_data_tdata   => s_axis_data_tdata_7_5KHz,
    m_axis_data_tvalid  => Filt7_5KHz_m_axis_data_tvalid,
    m_axis_data_tdata   => Filt7_5KHz_m_axis_data_tdata
  );    
    
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

    sI_7_5KHz <= Filt7_5KHz_m_axis_data_tdata(24 downto 0);

    sQ_7_5KHz <= Filt7_5KHz_m_axis_data_tdata(56 downto 32);

    IQ_VAL_7_5KHz <= Filt7_5KHz_m_axis_data_tvalid;
    
          
 Mult2WithClip7_5KHz_I: entity work.Mult2WithClip
    generic map(Nb    => Nb)
    port map(
        clk => clk,
        rstn => rstn,
        x_1    => sI_7_5KHz,
        x_2    => sQ_7_5KHz,
        y_1    => I_7_5KHzOut,
        y_2    => Q_7_5KHzOut
    );   
    
    
    
    ---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

    s_axis_data_tdata_5KHz <= Q_7_5KHzOut & I_7_5KHzOut;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 2
    -- Fs_in  = 16.27609375KHz
    -- Fs_out = 8.138046875KSPS
    --------------------------------------------------------------------
        
Inst_fir_compiler_5KHz : fir_compiler_5KHz 
  PORT MAP(
  
    aresetn             => rstn,
    aclk                => clk,
    s_axis_data_tvalid  => IQ_VAL_7_5KHz,
    s_axis_data_tready  => open,
    s_axis_data_tdata   => s_axis_data_tdata_5KHz,
    m_axis_data_tvalid  => Filt5KHz_m_axis_data_tvalid,
    m_axis_data_tdata   => Filt5KHz_m_axis_data_tdata
  );    
    
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

    sI_5KHz <= Filt5KHz_m_axis_data_tdata(24 downto 0);

    sQ_5KHz <= Filt5KHz_m_axis_data_tdata(56 downto 32);

    IQ_VAL_5KHz <= Filt5KHz_m_axis_data_tvalid;
    
          
 Mult2WithClip5KHz_I: entity work.Mult2WithClip
    generic map(Nb    => Nb)
    port map(
        clk => clk,
        rstn => rstn,
        x_1    => sI_5KHz,
        x_2    => sQ_5KHz,
        y_1    => I_5KHzOut,
        y_2    => Q_5KHzOut
    ); 
    
    ---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

    s_axis_data_tdata_3KHz <= Q_5KHzOut & I_5KHzOut;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 2
    -- Fs_in  = 8.138046875KHz
    -- Fs_out = 4.0690234375KSPS
    --------------------------------------------------------------------
        
Inst_fir_compiler_3KHz : fir_compiler_3KHz 
  PORT MAP(
  
    aresetn             => rstn,
    aclk                => clk,
    s_axis_data_tvalid  => IQ_VAL_5KHz,
    s_axis_data_tready  => open,
    s_axis_data_tdata   => s_axis_data_tdata_3KHz,
    m_axis_data_tvalid  => Filt3KHz_m_axis_data_tvalid,
    m_axis_data_tdata   => Filt3KHz_m_axis_data_tdata
  );    
    
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

    sI_3KHz <= Filt3KHz_m_axis_data_tdata(24 downto 0);

    sQ_3KHz <= Filt3KHz_m_axis_data_tdata(56 downto 32);

    IQ_VAL_3KHz <= Filt3KHz_m_axis_data_tvalid;
    
          
 Mult2WithClip3KHz_I: entity work.Mult2WithClip
    generic map(Nb    => Nb)
    port map(
        clk => clk,
        rstn => rstn,
        x_1    => sI_3KHz,
        x_2    => sQ_3KHz,
        y_1    => I_3KHzOut,
        y_2    => Q_3KHzOut
    );  
    
    
    ---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

    s_axis_data_tdata_1_2KHz <= Q_3KHzOut & I_3KHzOut;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 2
    -- Fs_in  = 4.0690234375KHz
    -- Fs_out = 2.03451171875KSPS
    --------------------------------------------------------------------
        
Inst_fir_compiler_1_2KHz : fir_compiler_1_2KHz 
  PORT MAP(
  
    aresetn             => rstn,
    aclk                => clk,
    s_axis_data_tvalid  => IQ_VAL_3KHz,
    s_axis_data_tready  => open,
    s_axis_data_tdata   => s_axis_data_tdata_1_2KHz,
    m_axis_data_tvalid  => Filt1_2KHz_m_axis_data_tvalid,
    m_axis_data_tdata   => Filt1_2KHz_m_axis_data_tdata
  );    
    
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

    sI_1_2KHz <= Filt1_2KHz_m_axis_data_tdata(24 downto 0);

    sQ_1_2KHz <= Filt1_2KHz_m_axis_data_tdata(56 downto 32);

    IQ_VAL_1_2KHz <= Filt1_2KHz_m_axis_data_tvalid;
    
          
 Mult2WithClip1_2KHz_I: entity work.Mult2WithClip
    generic map(Nb    => Nb)
    port map(
        clk => clk,
        rstn => rstn,
        x_1    => sI_1_2KHz,
        x_2    => sQ_1_2KHz,
        y_1    => I_1_2KHzOut,
        y_2    => Q_1_2KHzOut
    );   
    
    
    ---------------------------------------------------------------
    -- AXI4-Stream input mapping
    --
    -- PATH_1 [31:16] = Q
    -- PATH_0 [15:0]  = I
    --------------------------------------------------------------------

    s_axis_data_tdata_600Hz <= Q_1_2KHzOut & I_1_2KHzOut;

    --------------------------------------------------------------------
    -- FIR Compiler
    -- Decimation by 2
    -- Fs_in  = 2.03451171875KHz
    -- Fs_out = 1.017255859375KSPS
    --------------------------------------------------------------------
        
Inst_fir_compiler_600Hz : fir_compiler_600Hz 
  PORT MAP(
  
    aresetn             => rstn,
    aclk                => clk,
    s_axis_data_tvalid  => IQ_VAL_1_2KHz,
    s_axis_data_tready  => open,
    s_axis_data_tdata   => s_axis_data_tdata_600Hz,
    m_axis_data_tvalid  => Filt600Hz_m_axis_data_tvalid,
    m_axis_data_tdata   => Filt600Hz_m_axis_data_tdata
  );    
    
    --------------------------------------------------------------------
    -- AXI4-Stream output mapping
    --
    -- PATH_0 [24:0] = I
    -- PATH_1 [56:32] = Q
    --------------------------------------------------------------------

    sI_600Hz <= Filt600Hz_m_axis_data_tdata(24 downto 0);

    sQ_600Hz <= Filt600Hz_m_axis_data_tdata(56 downto 32);

    IQ_VAL_600Hz <= Filt600Hz_m_axis_data_tvalid;
    
          
 Mult2WithClip600Hz_I: entity work.Mult2WithClip
    generic map(Nb    => Nb)
    port map(
        clk => clk,
        rstn => rstn,
        x_1    => sI_600Hz,
        x_2    => sQ_600Hz,
        y_1    => I_600HzOut,
        y_2    => Q_600HzOut
    );                         
    
    
         
I_DataOut <= I_DecOut     when BandWidth_Select = X"0"  else   -- Without FIR Filter, in fact depends on AD9361 Aliasing Filter
             I_200KHzOut  when BandWidth_Select = X"1"  else   -- BW= 200KHz
             I_150KHzOut  when BandWidth_Select = X"2"  else   -- BW= 150KHz
             I_60KHzOut   when BandWidth_Select = X"3"  else 
             I_30KHzOut   when BandWidth_Select = X"4"  else 
             I_15KHzOut   when BandWidth_Select = X"5"  else 
             I_12KHzOut   when BandWidth_Select = X"6"  else 
             I_7_5KHzOut  when BandWidth_Select = X"7"  else                      
             I_5KHzOut    when BandWidth_Select = X"8"  else                      
             I_3KHzOut    when BandWidth_Select = X"9"  else                      
             I_1_2KHzOut  when BandWidth_Select = X"A"  else                      
             I_600HzOut   when BandWidth_Select = X"B"  else                      
             I_DecOut ;
             
Q_DataOut <= Q_DecOut     when BandWidth_Select = X"0"  else  -- Without FIR Filter, in fact depends on AD9361 Aliasing Filter
             Q_200KHzOut  when BandWidth_Select = X"1"  else 
             Q_150KHzOut  when BandWidth_Select = X"2"  else 
             Q_60KHzOut   when BandWidth_Select = X"3"  else 
             Q_30KHzOut   when BandWidth_Select = X"4"  else 
             Q_15KHzOut   when BandWidth_Select = X"5"  else 
             Q_12KHzOut   when BandWidth_Select = X"6"  else 
             Q_7_5KHzOut  when BandWidth_Select = X"7"  else                      
             Q_5KHzOut    when BandWidth_Select = X"8"  else                      
             Q_3KHzOut    when BandWidth_Select = X"9"  else                      
             Q_1_2KHzOut  when BandWidth_Select = X"A"  else                      
             Q_600HzOut   when BandWidth_Select = X"B"  else                      
             Q_DecOut ;
             
IQ_Val_Out<= IQ_VAL_Decimator   when BandWidth_Select = X"0"  else 
             IQ_VAL_200KHz      when BandWidth_Select = X"1"  else 
             IQ_VAL_150KHz      when BandWidth_Select = X"2"  else 
             IQ_VAL_60KHz       when BandWidth_Select = X"3"  else 
             IQ_VAL_30KHz       when BandWidth_Select = X"4"  else 
             IQ_VAL_15KHz       when BandWidth_Select = X"5"  else 
             IQ_VAL_12KHz       when BandWidth_Select = X"6"  else 
             IQ_VAL_7_5KHz      when BandWidth_Select = X"7"  else                      
             IQ_VAL_5KHz        when BandWidth_Select = X"8"  else                                      
             IQ_VAL_3KHz        when BandWidth_Select = X"9"  else                      
             IQ_VAL_1_2KHz      when BandWidth_Select = X"A"  else                      
             IQ_VAL_600Hz       when BandWidth_Select = X"B"  else                      
             IQ_VAL_Decimator;                                                 
           
end Behavioral;
