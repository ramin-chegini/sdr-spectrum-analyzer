----------------------------------------------------------------------------------
-- Company: Sazgan Ertebat
-- Engineer: Ramin Chegini
-- 
-- Create Date: 12/05/2023 17:00:25 PM
-- Design Name: 
-- Module Name: SERFFT2K_Maxhold - Behavioral
-- Project Name: 
-- Target Devices: 
-- Tool Versions: 
-- Description: 
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.std_logic_arith.all ;
use ieee.std_logic_unsigned.all ;
use ieee.std_logic_misc.all ;
use ieee.numeric_std.all;

entity SERFFT2K_Maxhold is
    generic (
                PSDIdx_WIDTH    : integer    := 11;
                DPRam_DATA_WIDTH: integer    := 11
            );
    Port ( 
            Wr_clk                      : in STD_LOGIC;
            Rd_clk                      : in STD_LOGIC;
            rst                         : in STD_LOGIC;
            IQDataVal                   : in STD_LOGIC;
            ADCDataIn_I                 : in STD_LOGIC_VECTOR (15 downto 0);
            ADCDataIn_Q                 : in STD_LOGIC_VECTOR (15 downto 0);
            Start_NB_FFT                : in STD_LOGIC;
            MemBusy                     : in STD_LOGIC;
            NbPSD_Data_Ready            : out STD_LOGIC;
            NbPSD_DataValid             : out STD_LOGIC;
--            NbFFT_Data_Valid            : out STD_LOGIC;
            Real_Time_MaxHold           : in  STD_LOGIC;
            Real_Time_MaxHold_Dly       : in  STD_LOGIC_VECTOR(31 downto 0);
            NBPSD_DPram_rd_addr         : in  STD_LOGIC_VECTOR (DPRam_DATA_WIDTH-1 downto 0);
--            NBPSD_DPram_rd_addr_Out     : out STD_LOGIC_VECTOR (DPRam_DATA_WIDTH-1 downto 0);
            NBPSD_DPram_rd_Data         : out STD_LOGIC_VECTOR(31 DOWNTO 0)
--            oNBcordic_fft_index         : out std_logic_vector(PSDIdx_WIDTH-1 downto 0);
--            oNBcordic_dout_tdata        : out std_logic_vector(31 downto 0);
--            NB_FFT_DataOut_Real         : out std_logic_vector(31 downto 0);        
--            NB_FFT_DataOut_Image        : out std_logic_vector(31 downto 0)     
       );
end SERFFT2K_Maxhold;

architecture Behavioral of SERFFT2K_Maxhold is
--define components below:
COMPONENT NB_xfft_2048
  PORT  (
            aclk                        : IN STD_LOGIC;
            aresetn                     : IN STD_LOGIC;
            s_axis_config_tdata         : IN STD_LOGIC_VECTOR(7 DOWNTO 0);
            s_axis_config_tvalid        : IN STD_LOGIC;
            s_axis_config_tready        : OUT STD_LOGIC;
            s_axis_data_tdata           : IN STD_LOGIC_VECTOR(63 DOWNTO 0);
            s_axis_data_tvalid          : IN STD_LOGIC;
            s_axis_data_tready          : OUT STD_LOGIC;
            s_axis_data_tlast           : IN  STD_LOGIC;
            m_axis_data_tdata           : OUT STD_LOGIC_VECTOR(95 DOWNTO 0);
            m_axis_data_tuser           : OUT STD_LOGIC_VECTOR(15 DOWNTO 0);
            m_axis_data_tvalid          : OUT STD_LOGIC;
            m_axis_data_tready          : IN STD_LOGIC;
            m_axis_data_tlast           : OUT STD_LOGIC;
            event_frame_started         : OUT STD_LOGIC;
            event_tlast_unexpected      : OUT STD_LOGIC;
            event_tlast_missing         : OUT STD_LOGIC;
            event_status_channel_halt   : OUT STD_LOGIC;
            event_data_in_channel_halt  : OUT STD_LOGIC;
            event_data_out_channel_halt : OUT STD_LOGIC
        );
END COMPONENT;
--
COMPONENT Cordic_NB_2048_Monitoring
  PORT (
            aclk                    : IN STD_LOGIC;
            aresetn                 : IN STD_LOGIC;
            s_axis_cartesian_tvalid : IN STD_LOGIC;
            s_axis_cartesian_tdata  : IN STD_LOGIC_VECTOR(95 DOWNTO 0);
            m_axis_dout_tvalid      : OUT STD_LOGIC;
            m_axis_dout_tdata       : OUT STD_LOGIC_VECTOR(63 DOWNTO 0)
        );
END COMPONENT;

COMPONENT NB_fft_2048_amp_vs_index_bram
  PORT (
            clka    : IN STD_LOGIC;
            wea     : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
            addra   : IN STD_LOGIC_VECTOR(10 DOWNTO 0);
            dina    : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
            clkb    : IN STD_LOGIC;
            addrb   : IN STD_LOGIC_VECTOR(10 DOWNTO 0);
            doutb   : OUT STD_LOGIC_VECTOR(31 DOWNTO 0)
        );
END COMPONENT;
--
COMPONENT Rom_Hanning2048_Coe
  PORT  (
            a       : IN STD_LOGIC_VECTOR(10 DOWNTO 0);
            clk     : IN STD_LOGIC;
            qspo    : OUT STD_LOGIC_VECTOR(15 DOWNTO 0)
        );
END COMPONENT;
--
COMPONENT mult_win_16bit
  PORT  (
            CLK : IN STD_LOGIC;
            A   : IN STD_LOGIC_VECTOR(15 DOWNTO 0);
            B   : IN STD_LOGIC_VECTOR(15 DOWNTO 0);
            CE  : IN STD_LOGIC;
            P   : OUT STD_LOGIC_VECTOR(31 DOWNTO 0)
        );
END COMPONENT;

COMPONENT FFT_2048_IndexFIFO
  PORT  (
            clk         : IN STD_LOGIC;
            srst         : IN STD_LOGIC;
            din         : IN STD_LOGIC_VECTOR(10 DOWNTO 0);
            wr_en       : IN STD_LOGIC;
            rd_en       : IN STD_LOGIC;
            dout        : OUT STD_LOGIC_VECTOR(10 DOWNTO 0);
            full        : OUT STD_LOGIC;
            empty       : OUT STD_LOGIC           
        );
END COMPONENT;

COMPONENT RealTime_PSDMaxHold_Fifo_2K
  PORT  (
            clk         : IN STD_LOGIC;
            rst         : IN STD_LOGIC;
            din         : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
            wr_en       : IN STD_LOGIC;
            rd_en       : IN STD_LOGIC;
            dout        : OUT STD_LOGIC_VECTOR(31 DOWNTO 0);
            full        : OUT STD_LOGIC;
            empty       : OUT STD_LOGIC            
        );
END COMPONENT;

--COMPONENT ila_NbFFT_Maxhold_Inspectors
--PORT (
--        clk    : IN STD_LOGIC;
--        probe0 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
--        probe1 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--        probe2 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--        probe3 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
--        probe4 : IN STD_LOGIC_VECTOR(31 DOWNTO 0); 
--        probe5 : IN STD_LOGIC_VECTOR(8 DOWNTO 0); 
--        probe6 : IN STD_LOGIC_VECTOR(8 DOWNTO 0); 
--        probe7 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--        probe8 : IN STD_LOGIC_VECTOR(3 DOWNTO 0);
--        probe9 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
--        probe10 : IN STD_LOGIC_VECTOR(15 DOWNTO 0);
--        probe11 : IN STD_LOGIC_VECTOR(0 DOWNTO 0)
--);
--END COMPONENT  ;

--COMPONENT ila_NbFFT_Inspectors

--PORT (
--	clk : IN STD_LOGIC;
--	probe0 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--	probe1 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--	probe2 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
--	probe3 : IN STD_LOGIC_VECTOR(3 DOWNTO 0);
--    probe4 : IN STD_LOGIC_VECTOR(0 DOWNTO 0); 
--	probe5 : IN STD_LOGIC_VECTOR(11 DOWNTO 0);
--	probe6 : IN STD_LOGIC_VECTOR(31 DOWNTO 0);
--	probe7 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
--	probe8 : IN STD_LOGIC_VECTOR(0 DOWNTO 0);
--	probe9 : IN STD_LOGIC_VECTOR(15 DOWNTO 0)
--);
--END COMPONENT  ;

--
--define signals below:
signal rstN                             : std_logic;
signal FFT_Maxhold_last_shot            : std_logic:='0';
signal NbFFT_Maxhold_Cordic_DataIn_Val  : std_logic;
signal PSDMaxHold_Fifo_Rst              : std_logic;
signal PSDMaxHold_Fifo_Rst_flg          : std_logic;
signal PSDMaxHold_Fifo_Rst_flg_B        : std_logic;
signal PSDMaxHold_Fifo_Rst_Cntr         : std_logic_vector(3 downto 0);
signal Start_NbFFT_MaxHold              : std_logic;
signal Start_NbFFT_MaxHold_D            : std_logic;
signal sStart_NB_FFT                    : std_logic;
signal Start_NB_FFT_B                   : std_logic;
signal Start_NB_FFT_D                   : std_logic;
signal Rst_FFT_MaxHold                  : std_logic;
signal NbFFT_MaxHold_Dvalid             : std_logic;
--signal NbFFT_MaxHold_tlast              : std_logic;
--signal NbFFT_MaxHold_tlast_B            : std_logic;
signal NbFFT_MaxHold_Dvalid_B           : std_logic;
signal IQDataVal_Prev                   : std_logic;     
signal IQDataVal_Prev2                  : std_logic;     
signal IQDataVal_Maxhold_Prev2          : std_logic;     
signal NBFFT_DataVal                    : std_logic:='0';     
--signal sNBdelayCntr                     : std_logic_vector(11 downto 0);
signal sNBFFTdelayCntr                  : std_logic_vector(15 downto 0);
signal Real_Time_MaxHold_Dly_Cntr       : std_logic_vector(31 downto 0);
signal NbFFT_Maxhold_Cntr               : std_logic_vector(31 downto 0);
signal Real_Time_MaxHold_Dly_B          : std_logic_vector(31 downto 0);
--fft signal definitions
signal NBfft_s_axis_config_tdata        : std_logic_vector(7 downto 0);
signal NBfft_s_axis_config_tvalid       : std_logic;
signal NBfft_s_axis_config_tready       : std_logic;
signal NBfft_s_axis_data_tdata          : std_logic_vector(63 downto 0);
signal NBfft_s_axis_data_tready         : std_logic;
signal NBfft_m_axis_data_tdata          : std_logic_vector(95 downto 0);
signal NBfft_m_axis_data_tuser          : std_logic_vector(15 downto 0);
signal NBfft_m_axis_data_tvalid         : std_logic;
signal NBfft_m_axis_data_tready         : std_logic;
signal NBfft_m_axis_data_tlast          : std_logic;
signal NBfft_event_frame_started        : std_logic;
signal NBfft_event_tlast_unexpected     : std_logic;
signal NBfft_event_tlast_missing        : std_logic;
signal NBfft_event_status_channel_halt  : std_logic;
signal NBfft_event_data_in_channel_halt : std_logic;
signal NBfft_event_data_out_channel_halt: std_logic;
--cordic
signal NBcordic_dout_valid              : std_logic;
signal NbPSD_Maxhold_Fifo_RdData        : std_logic;
signal NbPSD_Fifo_Read_Data             : std_logic;
signal NBcordic_dout_valid_Prev         : std_logic;
signal NBcordic_Maxhold_dout_valid_Prev : std_logic;
signal DPram_Wr_DataIn                  : std_logic_vector(31 downto 0);
signal DPram_WrEn                       : std_logic_vector(0 downto 0);
signal NbPSD_Cordic_Dout                : std_logic_vector(63 downto 0);
signal NbPSD_Fifo_MaxHold_Dout          : std_logic_vector(31 downto 0);
signal NbPSD_Fifo_MaxHold_Din           : std_logic_vector(31 downto 0);
signal NbPSD_Fifo_Index                 : std_logic_vector(10 downto 0);
--signal NbPSD_Fifo_MaxHold_Index         : std_logic_vector(8 downto 0);
signal DPram_WrEn_NbPSD_MaxHold         : std_logic_vector(0 downto 0);
signal NBPSD_DPram_rd_Data_B            : std_logic_vector(31 DOWNTO 0);
-------------------------------------
signal NBfft_s_axis_data_tvalid         : std_logic;
signal NBfft_s_axis_data_tvalid_B       : std_logic;
signal NBfft_Data_tlast                 : std_logic;
signal sNbPSD_Data_Ready                : std_logic;
signal sNbPSD_Maxhold_Data_Ready        : std_logic;
signal NbPSD_Data_Ready_B               : std_logic;
signal NbPSD_DataValid_B                : std_logic;
signal NbPSD_Maxhold_Data_Ready_B       : std_logic;
signal NbPSD_Maxhold_DataValid_B        : std_logic;
signal sNbPSD_DataValid                 : std_logic;
signal sNbPSD_Maxhold_DataValid         : std_logic;
signal sromIQdouta                      : std_logic_vector(15 downto 0);
signal sADCDataIn_I                     : std_logic_vector(31 downto 0);
signal sADCDataIn_Q                     : std_logic_vector(31 downto 0);
signal sRom_Address                     : std_logic_vector(10 downto 0);
----------------------------------- ila signal ------------------
signal Nb_Maxhold_State                 : std_logic_vector(3 downto 0);
signal Nb_Maxhold_State_B               : std_logic_vector(3 downto 0);

signal Nb_State                 : std_logic_vector(3 downto 0);
signal Nb_State_B               : std_logic_vector(3 downto 0);
--signal IQDataVal_Ila               : std_logic;

TYPE NB_Module_StateType is (
                                ST0,
                                ST1,
                                ST2,
                                ST3,
                                ST4,
                                ST5,
                                ST6,
                                ST7
                            );					
signal NB_Module_State,NB_Module_Next_State: NB_Module_StateType;

TYPE NB_Maxhold_Module_StateType is (
                                ST0,
                                ST1,
                                ST2,
                                ST3,
                                ST4,
                                ST5,
                                ST6
                                    );					
signal NB_Maxhold_Module_State,NB_Maxhold_Module_Next_State: NB_Maxhold_Module_StateType;

begin
-------------------------------------------------------------
--Combinational assignment of I/O to/from signals:
sStart_NB_FFT       <= Start_NB_FFT   when Real_Time_MaxHold ='0'  else 
                       '0';
                       
Start_NbFFT_MaxHold <= Start_NB_FFT   when Real_Time_MaxHold ='1'  else 
                       '0';                       
                         
NbPSD_Data_Ready    <= sNbPSD_Data_Ready  when Real_Time_MaxHold ='0'  else 
                       sNbPSD_Maxhold_Data_Ready;
                         
NbPSD_DataValid     <= sNbPSD_DataValid   when Real_Time_MaxHold ='0'  else 
                       sNbPSD_Maxhold_DataValid;
                                                                                                     
Rom_Hanning2048_Coe_Inst : Rom_Hanning2048_Coe
  PORT MAP  (
                clk    => Wr_clk,
                a      => sRom_Address,
                qspo   => sromIQdouta
            );
  
mult_win_InstI : mult_win_16bit
  PORT MAP  (
                CLK => Wr_clk,
                A   => ADCDataIn_I,
                B   => sromIQdouta,
                CE  => IQDataVal,
                P   => sADCDataIn_I
            );
mult_win_InstQ : mult_win_16bit
  PORT MAP  (
                CLK => Wr_clk,
                A   => ADCDataIn_Q,
                B   => sromIQdouta,
                CE  => IQDataVal,
                P   => sADCDataIn_Q
            );
------------------------------------------------------------------------------------------------------------------------------
process (Wr_clk,rst,MemBusy,Start_NB_FFT) 
begin
    if (rst = '1' or MemBusy = '1') then
        sRom_Address       <=(others=>'0');
        Start_NB_FFT_B     <='0';
        IQDataVal_Prev     <='0';
    else
        if(rising_edge(Wr_clk)) then 
            IQDataVal_Prev            <= IQDataVal; 
            Start_NB_FFT_B            <= Start_NB_FFT; 
            if (Start_NB_FFT_B = '1') then
                if(IQDataVal_Prev = '0' and IQDataVal = '1') then 
                    sRom_Address  <= sRom_Address + 1;
                end if;
            end if;
        end if;
    end if;
end process;  
------------------------------------------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------------------------------------------
------------------------------------------------------------------------------------------------------------------------------                                                                 
MGMT_NB_FFT_SYNC_PROC : process (Wr_clk,rst,MemBusy,IQDataVal,sStart_NB_FFT)  
begin
    if (rst = '1' or MemBusy = '1') then
        NB_Module_State               <= ST0;
        sNBFFTdelayCntr               <=(others=>'0');
        Nb_State                      <=(others=>'0');
        IQDataVal_Prev2               <= '0';
        NBcordic_dout_valid_Prev      <= '0';
        sNbPSD_Data_Ready             <= '0';
        sNbPSD_DataValid              <= '0';
        NBfft_s_axis_data_tvalid      <= '0';
        Start_NB_FFT_D                <= '0';        
    else
        if(rising_edge(Wr_clk)) then  
            NB_Module_State             <= NB_Module_Next_State;
            IQDataVal_Prev2             <= IQDataVal;
            Start_NB_FFT_D              <= sStart_NB_FFT;
            sNbPSD_Data_Ready           <= NbPSD_Data_Ready_B;
            sNbPSD_DataValid            <= NbPSD_DataValid_B;            
            NBcordic_dout_valid_Prev    <= NBcordic_dout_valid;    
            NBfft_s_axis_data_tvalid    <= NBfft_s_axis_data_tvalid_B; 
            Nb_State                    <= Nb_State_B;    
            if (NB_Module_State = ST0) then
                sNBFFTdelayCntr    <= (others=>'0');
            elsif(NB_Module_State = ST2) then
                if(IQDataVal_Prev2 = '0' and IQDataVal = '1') then
                    sNBFFTdelayCntr    <= x"0001"; 
                end if;
            elsif(NB_Module_State = ST3) then
                if(IQDataVal_Prev2 = '0' and IQDataVal = '1') then
                    sNBFFTdelayCntr    <= sNBFFTdelayCntr + 1; 
                end if;              
            end if;  
        end if;
    end if; 
end process;  

------------------------------------------------------------------------------------------------------------------------------
MGMT_NB_FFT_OUTPUT_DECODE : process (NB_Module_State)
begin
    case (NB_Module_State) is 	 
        when ST0 => 
            NBfft_s_axis_data_tvalid_B  <= '0';
            NbPSD_Data_Ready_B          <= '0';
            NbPSD_DataValid_B           <= '0';
            Nb_State_B          <= x"0";                 
        when ST1 =>
            NBfft_s_axis_data_tvalid_B  <= '0';
            NbPSD_Data_Ready_B          <= '0';
            NbPSD_DataValid_B           <= '0'; 
            Nb_State_B          <= x"1";                       
        when ST2 =>
            NBfft_s_axis_data_tvalid_B  <= '0';
            NbPSD_Data_Ready_B          <= '0';
            NbPSD_DataValid_B           <= '0';
            Nb_State_B          <= x"2";                         
        when ST3 =>
            NBfft_s_axis_data_tvalid_B  <= '1';
            NbPSD_Data_Ready_B          <= '0';
            NbPSD_DataValid_B           <= '0'; 
            Nb_State_B          <= x"3";                        
        when ST4 =>
            NBfft_s_axis_data_tvalid_B  <= '0';
            NbPSD_Data_Ready_B          <= '0';
            NbPSD_DataValid_B           <= '0';
            Nb_State_B          <= x"4";                        
        when ST5 =>
            NBfft_s_axis_data_tvalid_B  <= '0';
            NbPSD_Data_Ready_B          <= '0';
            NbPSD_DataValid_B           <= '0'; 
            Nb_State_B          <= x"5";                       
        when ST6 =>
            NBfft_s_axis_data_tvalid_B  <= '0';
            NbPSD_Data_Ready_B          <= '1';
            NbPSD_DataValid_B           <= '0';
            Nb_State_B          <= x"6";                        
        when ST7 =>
            NBfft_s_axis_data_tvalid_B  <= '0';
            NbPSD_Data_Ready_B          <= '1';
            NbPSD_DataValid_B           <= '1'; 
            Nb_State_B          <= x"7";                       
    end case; 
end process;
------------------------------------------------------------------------------------------------------------------------------
MGMT_NB_FFT_NEXT_STATE_DECODE : process (NB_Module_State,Start_NB_FFT_D,sNBFFTdelayCntr,IQDataVal,IQDataVal_Prev2,NBcordic_dout_valid,NBcordic_dout_valid_Prev)
begin
 
	NB_Module_Next_State <= NB_Module_State; 
	 
	case (NB_Module_State) is 
        when ST0 =>
            if (Start_NB_FFT_D = '1') then 
                NB_Module_Next_State    <= ST1; 
            end if;         
		when ST1 =>
            if(IQDataVal_Prev2 = '0' and IQDataVal = '1') then
                NB_Module_Next_State    <= ST2; 
            end if;
		when ST2 =>
            if(IQDataVal_Prev2 = '0' and IQDataVal = '1') then
                NB_Module_Next_State    <= ST3; 
            end if;              		
		when ST3 =>
            if(IQDataVal_Prev2 = '0' and IQDataVal = '1') then
                if(sNBFFTdelayCntr = x"800") then
                    NB_Module_Next_State    <= ST4;
                end if;
            end if;
		when ST4 =>
            NB_Module_Next_State <= ST5;
		when ST5 =>
            if(NBcordic_dout_valid_Prev = '1' and NBcordic_dout_valid = '0') then
                NB_Module_Next_State <= ST6; 
            end if;
		when ST6 =>
            NB_Module_Next_State <= ST7;
		when ST7 =>
            NB_Module_Next_State <= ST7;                                         
	end case; 
end process;

NBFFT_DataVal <= (NBfft_s_axis_data_tvalid and IQDataVal) when Real_Time_MaxHold ='0'  else 
                 (NbFFT_MaxHold_Dvalid and IQDataVal and not(FFT_Maxhold_last_shot)); 
-----------------------------------------------------------------------------------------------------------------------------
---------------------------------------maxhold--------------------------------------------------------------------------------------
MGMT_NB_FFT_MaxHold_SYNC_PROC : process (Wr_clk,rst,MemBusy,IQDataVal,Start_NbFFT_MaxHold,Real_Time_MaxHold_Dly)  
begin
    if (rst = '1' or MemBusy = '1') then
        NB_Maxhold_Module_State             <= ST0;
        Start_NbFFT_MaxHold_D               <= '0';        
        IQDataVal_Maxhold_Prev2             <= '0';        
        sNbPSD_Maxhold_Data_Ready           <= '0';        
        sNbPSD_Maxhold_DataValid            <= '0';        
        NBcordic_Maxhold_dout_valid_Prev    <= '0';        
        NbFFT_MaxHold_Dvalid                <= '0';        
        Real_Time_MaxHold_Dly_Cntr          <= (others=>'0');        
        Real_Time_MaxHold_Dly_B             <= (others=>'0');        
--        DPram_WrEn_NbPSD_MaxHold            <= (others=>'0'); 
        PSDMaxHold_Fifo_Rst_Cntr            <= (others=>'0'); 
        NbFFT_Maxhold_Cntr                  <= (others=>'0'); 
        Nb_Maxhold_State                    <= (others=>'0'); 
        NbPSD_Maxhold_Fifo_RdData           <= '0';       
        PSDMaxHold_Fifo_Rst_flg             <= '0'; 
        FFT_Maxhold_last_shot               <= '0';      
    else
        if(rising_edge(Wr_clk)) then  
            NB_Maxhold_Module_State           <= NB_Maxhold_Module_Next_State;
            IQDataVal_Maxhold_Prev2           <= IQDataVal;
            Start_NbFFT_MaxHold_D             <= Start_NbFFT_MaxHold;
            sNbPSD_Maxhold_Data_Ready         <= NbPSD_Maxhold_Data_Ready_B;
            sNbPSD_Maxhold_DataValid          <= NbPSD_Maxhold_DataValid_B;            
            NbFFT_MaxHold_Dvalid              <= NbFFT_MaxHold_Dvalid_B;    
            Real_Time_MaxHold_Dly_B           <= Real_Time_MaxHold_Dly; 
            PSDMaxHold_Fifo_Rst_flg           <= PSDMaxHold_Fifo_Rst_flg_B; 
            Nb_Maxhold_State                  <= Nb_Maxhold_State_B; 
               
            if(NB_Maxhold_Module_State = ST3) then
                if(NBcordic_dout_valid = '1') then
                    if(NbPSD_Fifo_Index = x"7FF") then
                        NbPSD_Maxhold_Fifo_RdData   <= '1';
                        Real_Time_MaxHold_Dly_Cntr  <= Real_Time_MaxHold_Dly_Cntr + 1; 
                    end if;
--                    if(Real_Time_MaxHold_Dly_Cntr = Real_Time_MaxHold_Dly_B - 1) then
--                        if(NbPSD_Fifo_Index = x"000") then
--                            DPram_WrEn_NbPSD_MaxHold <= conv_std_logic_vector(1,1);--(arg,size)
--                        elsif(NbPSD_Fifo_Index = x"FFF") then
--                            DPram_WrEn_NbPSD_MaxHold <= conv_std_logic_vector(0,1);
--                        end if;                                           
--                    end if;                                           
                end if;
                if(NBfft_m_axis_data_tvalid = '1') then
                    if(NBfft_m_axis_data_tuser(11 downto 0) = x"7FF") then
                        NbFFT_Maxhold_Cntr  <= NbFFT_Maxhold_Cntr + '1';
                        if(NbFFT_Maxhold_Cntr = Real_Time_MaxHold_Dly_B - 1) then
                            FFT_Maxhold_last_shot  <= '1';
                        end if;    
                    end if; 
                end if; 
            elsif(NB_Maxhold_Module_State = ST4) then
                PSDMaxHold_Fifo_Rst_Cntr <= PSDMaxHold_Fifo_Rst_Cntr+1;                                                           
            end if;  
        end if;
    end if; 
end process;  
--------------------------------------------------------------------------------------------------------------------------------
MGMT_NB_FFT_MaxHold_OUTPUT_DECODE : process (NB_Maxhold_Module_State)
begin
    case (NB_Maxhold_Module_State) is 	 
        when ST0 => 
            NbFFT_MaxHold_Dvalid_B      <= '0';
            NbPSD_Maxhold_Data_Ready_B  <= '0';
            NbPSD_Maxhold_DataValid_B   <= '0';            
            PSDMaxHold_Fifo_Rst_flg_B   <= '0';
            Nb_Maxhold_State_B          <= x"0";            
        when ST1 =>
            NbFFT_MaxHold_Dvalid_B      <= '0';
            NbPSD_Maxhold_Data_Ready_B  <= '0';
            NbPSD_Maxhold_DataValid_B   <= '0';                
            PSDMaxHold_Fifo_Rst_flg_B   <= '0'; 
            Nb_Maxhold_State_B          <= x"1";                                
        when ST2 =>
            NbFFT_MaxHold_Dvalid_B      <= '0';
            NbPSD_Maxhold_Data_Ready_B  <= '0';
            NbPSD_Maxhold_DataValid_B   <= '0';                 
            PSDMaxHold_Fifo_Rst_flg_B   <= '0';
            Nb_Maxhold_State_B          <= x"2";                                 
        when ST3 =>
            NbFFT_MaxHold_Dvalid_B      <= '1';
            NbPSD_Maxhold_Data_Ready_B  <= '0';
            NbPSD_Maxhold_DataValid_B   <= '0';                              
            PSDMaxHold_Fifo_Rst_flg_B   <= '0';
            Nb_Maxhold_State_B          <= x"3";                                              
        when ST4 =>
            NbFFT_MaxHold_Dvalid_B      <= '0';
            NbPSD_Maxhold_Data_Ready_B  <= '0';
            NbPSD_Maxhold_DataValid_B   <= '0';                
            PSDMaxHold_Fifo_Rst_flg_B   <= '1';
            Nb_Maxhold_State_B          <= x"4";                                
        when ST5 =>
            NbFFT_MaxHold_Dvalid_B      <= '0';
            NbPSD_Maxhold_Data_Ready_B  <= '1';
            NbPSD_Maxhold_DataValid_B   <= '0';
            PSDMaxHold_Fifo_Rst_flg_B   <= '0';
            Nb_Maxhold_State_B          <= x"5";                
        when ST6 =>
            NbFFT_MaxHold_Dvalid_B      <= '0';
            NbPSD_Maxhold_Data_Ready_B  <= '1';
            NbPSD_Maxhold_DataValid_B   <= '1'; 
            PSDMaxHold_Fifo_Rst_flg_B   <= '0';
            Nb_Maxhold_State_B          <= x"6";                                                                                         
    end case; 
end process;
--------------------------------------------------------------------------------------------------------------------------------
MGMT_NB_FFT_MaxHold_NEXT_STATE_DECODE : process (NB_Maxhold_Module_State,Real_Time_MaxHold_Dly_B,NBcordic_dout_valid,PSDMaxHold_Fifo_Rst_Cntr,Start_NbFFT_MaxHold_D,Real_Time_MaxHold_Dly_Cntr,IQDataVal,IQDataVal_Maxhold_Prev2,NbPSD_Fifo_Index)
begin
 
	NB_Maxhold_Module_Next_State <= NB_Maxhold_Module_State; 
	 
	case (NB_Maxhold_Module_State) is 
        when ST0 =>
            if (Start_NbFFT_MaxHold_D = '1') then 
                NB_Maxhold_Module_Next_State    <= ST1; 
            end if;         
		when ST1 =>
            if(IQDataVal_Maxhold_Prev2 = '0' and IQDataVal = '1') then
                NB_Maxhold_Module_Next_State    <= ST2; 
            end if;
		when ST2 =>
            if(IQDataVal_Maxhold_Prev2 = '0' and IQDataVal = '1') then
                NB_Maxhold_Module_Next_State    <= ST3; 
            end if;              		
		when ST3 =>
            if(NBcordic_dout_valid = '1') then
                if(Real_Time_MaxHold_Dly_Cntr = Real_Time_MaxHold_Dly_B - 1) then
                    if(NbPSD_Fifo_Index = x"7FF") then
                        NB_Maxhold_Module_Next_State    <= ST4;	
                    end if;   
                end if;                                             
            end if;                                             
		when ST4 =>
            if(PSDMaxHold_Fifo_Rst_Cntr = x"5") then
                NB_Maxhold_Module_Next_State <= ST5;
            end if;
		when ST5 =>
            NB_Maxhold_Module_Next_State <= ST6; 
		when ST6 =>
            NB_Maxhold_Module_Next_State <= ST6;                                                               
	end case; 
end process;          
-----------------------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------------------------
--process (Rd_clk,rst) 
--begin
--    if (rst = '1') then
--        NBPSD_DPram_rd_addr_Out      <= (others=>'0');
--    else
--        if(rising_edge(Rd_clk)) then  
--            NBPSD_DPram_rd_addr_Out <= NBPSD_DPram_rd_addr;
--        end if;   
--	end if; 
--end process;

NBfft_s_axis_data_tdata(63 downto 32)   <= sADCDataIn_Q;
NBfft_s_axis_data_tdata(31 downto 0)    <= sADCDataIn_I;

NBFFT_Init : process(NBfft_s_axis_config_tready,NBfft_s_axis_data_tready)
begin
    if(rising_edge(NBfft_s_axis_config_tready)) then
        NBfft_s_axis_config_tdata  <= x"01";   -- x"01" = forward FFT, x"00" = inverse FFT (IFFT)
        NBfft_s_axis_config_tvalid <= '1';
    end if;
end process;
------------------------------------------------------------------------------------------------------------------------------                      
NB_fft_module : NB_xfft_2048
  PORT MAP (
    aclk                        => Wr_clk,
    aresetn                     => rstN,
    s_axis_config_tdata         => NBfft_s_axis_config_tdata,
    s_axis_config_tvalid        => NBfft_s_axis_config_tvalid,
    s_axis_config_tready        => NBfft_s_axis_config_tready,
    s_axis_data_tdata           => NBfft_s_axis_data_tdata,
    s_axis_data_tvalid          => NBFFT_DataVal,
    s_axis_data_tready          => NBfft_s_axis_data_tready,
    s_axis_data_tlast           => '0',
    m_axis_data_tdata           => NBfft_m_axis_data_tdata,
    m_axis_data_tuser           => NBfft_m_axis_data_tuser,
    m_axis_data_tvalid          => NBfft_m_axis_data_tvalid,
    m_axis_data_tready          => '1',
    m_axis_data_tlast           => NBfft_m_axis_data_tlast,
    event_frame_started         => NBfft_event_frame_started,
    event_tlast_unexpected      => NBfft_event_tlast_unexpected,
    event_tlast_missing         => NBfft_event_tlast_missing,
    event_status_channel_halt   => NBfft_event_status_channel_halt,
    event_data_in_channel_halt  => NBfft_event_data_in_channel_halt,
    event_data_out_channel_halt => NBfft_event_data_out_channel_halt
  );
 ------------------------------------------------------------------------------------------------------------------------------                          
NbPSD_Fifo_MaxHold_Din <= NbPSD_Cordic_Dout(31 downto 0) when Real_Time_MaxHold_Dly_Cntr = x"00000000" else
                           NbPSD_Cordic_Dout(31 downto 0) when (Real_Time_MaxHold_Dly_Cntr /= x"00000000") and (NbPSD_Cordic_Dout(31 downto 0) > NbPSD_Fifo_MaxHold_Dout) else
                           NbPSD_Fifo_MaxHold_Dout;                            
                                                   
NbPSD_Fifo_Read_Data <= NBcordic_dout_valid and NbPSD_Maxhold_Fifo_RdData;
 
PSDMaxHold_Fifo_Rst  <= PSDMaxHold_Fifo_Rst_flg or rst;
 
rstN  <= (not rst) when Real_Time_MaxHold = '0'  else 
         (not PSDMaxHold_Fifo_Rst_flg) and (not rst);

  
RealTime_PSDMaxHold_Fifo_2K_Inst : RealTime_PSDMaxHold_Fifo_2K
  PORT MAP (
            clk         => Wr_clk,
            rst         => PSDMaxHold_Fifo_Rst,
            din         => NbPSD_Fifo_MaxHold_Din,
            wr_en       => NBcordic_dout_valid,
            rd_en       => NbPSD_Fifo_Read_Data,
            dout        => NbPSD_Fifo_MaxHold_Dout,
            full        => open,
            empty       => open
            ); 
 ------------------------------------------------------------------------------------------------------------------------------
FFT_2048_IndexFIFO_Inst : FFT_2048_IndexFIFO
  PORT MAP (
            clk         => Wr_clk,
            srst        => rst,
            din         => NBfft_m_axis_data_tuser(10 downto 0),
            wr_en       => NbFFT_Maxhold_Cordic_DataIn_Val,
            rd_en       => NBcordic_dout_valid,
            dout        => NbPSD_Fifo_Index,
            full        => open,
            empty       => open
            );
 --------------------------------------------------------------
 NbFFT_Maxhold_Cordic_DataIn_Val  <= NBfft_m_axis_data_tvalid when Real_Time_MaxHold ='0'  else 
                                     NBfft_m_axis_data_tvalid and (not FFT_Maxhold_last_shot);
  Cordic_NB_2048_Monitoring_Inst : Cordic_NB_2048_Monitoring
  PORT MAP (
            aclk                    => Wr_clk,
            aresetn                 => rstN,
            s_axis_cartesian_tvalid => NbFFT_Maxhold_Cordic_DataIn_Val,
            s_axis_cartesian_tdata  => NBfft_m_axis_data_tdata,
            m_axis_dout_tvalid      => NBcordic_dout_valid,
            m_axis_dout_tdata       => NbPSD_Cordic_Dout
            );
--------------------------------------------------------------
--NbFFT_Data_Valid        <= NBfft_m_axis_data_tvalid;
--NB_FFT_DataOut_Real     <= NBfft_m_axis_data_tdata(34 downto 3);      
--NB_FFT_DataOut_Image    <= NBfft_m_axis_data_tdata(74 downto 43);
--oNBcordic_fft_index     <= NbPSD_Fifo_Index;
--oNBcordic_dout_tdata    <= NbPSD_Cordic_Dout(31 downto 0);
NBPSD_DPram_rd_Data     <= NBPSD_DPram_rd_Data_B;
--NBPSD_DPram_rd_Data     <= NBPSD_DPram_rd_Data_B(31 downto 0)&NBPSD_DPram_rd_Data_B(63 downto 32);

DPram_Wr_DataIn         <= NbPSD_Cordic_Dout(31 downto 0)   when Real_Time_MaxHold ='0'  else 
                           NbPSD_Fifo_MaxHold_Dout;
                                           
DPram_WrEn              <= conv_std_logic_vector(NBcordic_dout_valid,1) when Real_Time_MaxHold ='0' else
                           DPram_WrEn_NbPSD_MaxHold and conv_std_logic_vector(NBcordic_dout_valid,1);                           

DPram_WrEn_NbPSD_MaxHold   <=  conv_std_logic_vector(1,1) when (NBcordic_dout_valid = '1') and (Real_Time_MaxHold_Dly_Cntr = Real_Time_MaxHold_Dly_B - 1) and (NbPSD_Fifo_Index >= x"000") and (NbPSD_Fifo_Index <= x"7FF") else
                               conv_std_logic_vector(0,1);

NB_fft_2048_result_saver_bram : NB_fft_2048_amp_vs_index_bram
  PORT MAP (
            clka    => Wr_clk,
            wea     => DPram_WrEn,
            addra   => NbPSD_Fifo_Index,
            dina    => DPram_Wr_DataIn,
            clkb    => Rd_clk,
            addrb   => NBPSD_DPram_rd_addr,
            doutb   => NBPSD_DPram_rd_Data_B
            );
            
--ila_NbFFT_Maxhold_Inspectors_Inst : ila_NbFFT_Maxhold_Inspectors
--PORT MAP (
--            clk    => Wr_clk,
--            probe0 => NbPSD_Fifo_MaxHold_Din, 
--            probe1 => conv_std_logic_vector(NBcordic_dout_valid,1), 
--            probe2 => conv_std_logic_vector(NbPSD_Fifo_Read_Data,1), 
--            probe3 => NbPSD_Fifo_MaxHold_Dout, 
--            probe4 => NbPSD_Cordic_Dout(31 downto 0), 
--            probe5 => NbPSD_Fifo_Index, 
--            probe6 => NBfft_m_axis_data_tuser(9 downto 0), 
--            probe7 => DPram_WrEn, 
--            probe8 => Nb_Maxhold_State,
--            probe9 => conv_std_logic_vector(NBfft_m_axis_data_tvalid,1),
--            probe10 => Real_Time_MaxHold_Dly_Cntr,
--            probe11 => conv_std_logic_vector(sNbPSD_DataValid,1)
--);       
--IQDataVal_Ila <= IQDataVal;
--ila_NbFFT_Inspectors_Inst : ila_NbFFT_Inspectors
--PORT MAP (
--	clk => Wr_clk,
--	probe0 => conv_std_logic_vector(IQDataVal_Ila,1), 
--	probe1 => conv_std_logic_vector(Start_NB_FFT_D,1), 
--	probe2 => conv_std_logic_vector(NBcordic_dout_valid,1),
--	probe3 => Nb_State,
--	probe4 => conv_std_logic_vector(sNbPSD_DataValid,1), 
--	probe5 => NbPSD_Fifo_Index,
--	probe6 => NbPSD_Cordic_Dout(31 downto 0),
--	probe7 => conv_std_logic_vector(NBfft_m_axis_data_tvalid,1),
--	probe8 => conv_std_logic_vector(NBFFT_DataVal,1),
--	probe9 => sNBFFTdelayCntr
--);     
                             
end Behavioral;
