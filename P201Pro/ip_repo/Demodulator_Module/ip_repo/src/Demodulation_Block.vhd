----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date: 11/26/2023 12:39:05 PM
-- Design Name: 
-- Module Name: FM_Block - Behavioral
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
use ieee.std_logic_1164.all ;
use ieee.std_logic_arith.all ;
use ieee.std_logic_unsigned.all ;
use ieee.std_logic_misc.all ;
library unisim;
use unisim.vcomponents.all;

entity DeModulation_Block is
    Port ( rst              : in STD_LOGIC;
           clk              : in STD_LOGIC;
           AGC_rst          : in STD_LOGIC;
           -----------------Data -----------------------------
           MOD_en           : in STD_LOGIC;
           DataVal_128kbit  : in STD_LOGIC;
           Data_En          : in STD_LOGIC;
           Data_I_Sig       : in STD_LOGIC_vector(23 downto 0);
           Data_Q_Sig       : in STD_LOGIC_vector(23 downto 0);
           Mod_Sel          : in STD_LOGIC_vector(7 downto 0);
           AMP_OK           : in STD_LOGIC;
           squelch          : in STD_LOGIC;
           --------------AGC Ctrl------------------------------
           MGC2_on          : in std_logic;
           AGC1_on          : in std_logic;
           AGC2_on          : in std_logic;
           MG_value         : in std_logic_vector(31 downto 0);
           a_coef1          : in STD_LOGIC_VECTOR(7 DOWNTO 0);
           ref1             : in STD_LOGIC_VECTOR(23 DOWNTO 0);
           a_coef2          : in STD_LOGIC_VECTOR(7 DOWNTO 0);
           ref2             : in STD_LOGIC_VECTOR(23 DOWNTO 0);             
           ----------------PLL signal--------------------------
           PLL_rst_INT      : out STD_LOGIC;    
           PLL_rst_INT2     : out STD_LOGIC;
           ----------------------------------------------------
           Dmode_Data_val   : out STD_LOGIC;
           Dmode_Data       : out STD_LOGIC_VECTOR(15 DOWNTO 0)
           
           );
end DeModulation_Block;

architecture Behavioral of DeModulation_Block is
   COMPONENT Mode_fifo
  PORT (
    clk : IN STD_LOGIC;
    din : IN STD_LOGIC_VECTOR(15 DOWNTO 0);
    wr_en : IN STD_LOGIC;
    rd_en : IN STD_LOGIC;
    dout : OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
    full : OUT STD_LOGIC;
    almost_full : OUT STD_LOGIC;
    empty : OUT STD_LOGIC;
    rd_data_count : OUT STD_LOGIC_VECTOR(11 DOWNTO 0);
    prog_full : OUT STD_LOGIC
  );
END COMPONENT;	
COMPONENT fir_compiler_1
  PORT (
    aclk : IN STD_LOGIC;
    s_axis_data_tvalid : IN STD_LOGIC;
    s_axis_data_tready : OUT STD_LOGIC;
    s_axis_data_tdata : IN STD_LOGIC_VECTOR(23 DOWNTO 0);
    m_axis_data_tvalid : OUT STD_LOGIC;
    m_axis_data_tdata : OUT STD_LOGIC_VECTOR(23 DOWNTO 0)
  );
END COMPONENT;   
   
    signal message_out , Data_AGC, Message_AGC      :  std_logic_vector(23 downto 0);
    signal PLL_EN           : std_logic;
    signal FM_vol           : std_logic_vector(23 downto 0);
    signal IoutSig_AGC,QoutSig_AGC   :STD_LOGIC_VECTOR(23 DOWNTO 0);  
    signal Sig_AGC_val,Mod_val   :STD_LOGIC;
    signal I_Mod,Q_Mod  :STD_LOGIC_VECTOR(23 DOWNTO 0);
    signal FM_out,AM_out,PM_out ,FM_dout,PM_DC_Removed ,Message_data  ,PhaseOfset ,Data_Mod       :  std_logic_vector(23 downto 0);
    signal FM_dout_en  ,FM_Prog_full ,FM_rd_en,FM_empty,FM_val,PM_val ,AM_val ,Message_val ,Message_AGC_val ,message_out_val,Data_mode_val           :  std_logic;
--    signal Dmode_Data_val : STD_LOGIC;   
    signal ModeFifo_rd_en : STD_LOGIC;   
    signal ModeFifo_dout:  STD_LOGIC_VECTOR(7 DOWNTO 0);    
--    signal Dmode_Data:  STD_LOGIC_VECTOR(15 DOWNTO 0);    
    signal ModeFifo_rd_data_count:  STD_LOGIC_VECTOR(11 DOWNTO 0);    
    signal ModeFifo_empty : STD_LOGIC;   
    signal ModeFifo_prog_full: STD_LOGIC;    
    signal ModeFifo_rd_en_d: STD_LOGIC;    
    signal Signal_detected: STD_LOGIC; 
    signal count       : std_logic_vector(31 downto 0);
 	signal FM_FilterType_TMP		: std_logic_vector(3 downto 0);
	signal FM_FilterType		: std_logic_vector(3 downto 0);  
	signal Counter_arr          :  std_logic_vector(63 downto 0);
	signal filterType_reg_TMP		: std_logic_vector(7 downto 0);
	signal filterType_reg		: std_logic_vector(7 downto 0);

begin
----------------------------------------------------------------------------------------------
----------------------------------------------------------------------------------------------
AgcI_TopM_Inst : entity work.AGC_V2
  generic map(Stereo => TRUE,Width => 24,Avg_Depth => 9)
  PORT MAP (
   clk      => clk,
   rst      => rst,
   
   Iin      => Data_I_Sig,
   Qin      => Data_Q_Sig,
   Val_in   => Data_En,
   
   MGC_on   => '0',
   MG_value => MG_value,
   Coef     => a_coef1,
   Ref      => Ref1,
   
   Iout     => IoutSig_AGC,
   Qout     => QoutSig_AGC,
   Valout   => Sig_AGC_val
   );

   I_Mod     <= Data_I_Sig when AGC1_on='0'  else IoutSig_AGC  ;
   Q_Mod     <= Data_Q_Sig when AGC1_on='0'  else QoutSig_AGC  ;
   Mod_val   <= Data_En    when AGC1_on='0'  else Sig_AGC_val ; 


FM_Demodulator_inst: entity work.FM_Demodulator
 Port map( 
          CLK_102_4M       		=> clk,
		  RST         			=> Rst,
		  Mod_Sel 		  	   	=> Mod_Sel,
		  		
		  I_in 		  	   		=> I_Mod,
		  Q_in    		   		=> Q_Mod,
		  IQ_Val 				=> Mod_val,
		  
		  PhaseOfset   		   	=> PhaseOfset,
		  AM_val   		   		=> AM_val,
		  AM_out   		   		=> AM_out,
		  FM_val   		   		=> FM_val,
		  FM_out   		   		=> FM_out,
		  PM_val   		   		=> PM_val,
		  PM_out   		        => PM_out);

Agc_voice : entity work.AGC_V2
  generic map( Stereo => FALSE,width => 24,Avg_Depth=>6)
  PORT MAP (
        clk      => clk,
        rst      => rst,
        
        Iin      => Data_Mod,
        Qin      => (others => '0'),
        Val_in   => Data_mode_val,
        
        Coef     => a_coef2,
        Ref      => Ref2,
        MGC_on   => MGC2_on,
        MG_value => MG_value,
                
        Iout     => Data_AGC,
        Qout     => open,
        Valout   => open
   );
   
   Message_AGC <=    Data_AGC  when (AGC2_on='1' OR MGC2_on='1') else Data_Mod ;
       
FM_Filter :fir_compiler_1
  PORT MAP (
    aclk => clk,
    s_axis_data_tvalid => DataVal_128kbit,
    s_axis_data_tready => open,
    s_axis_data_tdata => Message_AGC,
    
    m_axis_data_tvalid => message_out_val,
    m_axis_data_tdata  => message_out
  );
	
-- Mode_fifo_inst : Mode_fifo
--      PORT MAP (
--        clk             => clk,
--        din             => Dmode_Data,
--        wr_en           => Dmode_Data_val,
        
--        rd_en           => ModeFifo_rd_en,
--        dout            => ModeFifo_dout,
--        full            => open,
--        almost_full     => open,-- almost_full,
--        rd_data_count   => open,-- ModeFifo_rd_data_count,
--        empty           => ModeFifo_empty,
--        prog_full       => ModeFifo_prog_full
--      );  

    Mode_fifo_CTRL:PROCESS (clk,Rst )
	    BEGIN
			IF (Rst='1')THEN
                Dmode_Data_val   <= '0';
                Dmode_Data       <= (others => '0');
                Data_Mod       <= (others => '0');
                ModeFifo_rd_en_d <=  '0';
                Data_mode_val <=  '0';
                count <= (others => '0');
                ModeFifo_rd_data_count <= (others => '0');
                FM_FilterType_TMP <= (others => '0');
                Counter_arr <= (others => '0');
                filterType_reg_TMP <= (others => '0');
                PLL_rst_INT <= '1';
                PLL_rst_INT2 <= '1';
			ELSIF (RISING_EDGE(clk)) THEN
			  FM_FilterType_TMP <= FM_FilterType;
			  if FM_FilterType_TMP /= FM_FilterType then
			     PLL_rst_INT <= '1';
			  else
			     PLL_rst_INT <= '0';
			  end if;
			  filterType_reg_TMP <= filterType_reg;
			  if filterType_reg_TMP /= filterType_reg then
			     PLL_rst_INT2 <= '1';
			  else
			     PLL_rst_INT2 <= '0';
			  end if;
			   -------------------------------------------------------------
			   -------------------------------------------------------------
			   -------------------------------------------------------------
			    if (AMP_OK = '1'  ) then---and PSD_Index(0) = 0
                    Signal_detected <= '1';
                    count <= (others => '0');
                else
                    if count <= X"30d4000" then
                        count <= count + 1;
                    else
                        Signal_detected <= '0';
                    end if;    
                end if;
			    if MOD_en = '0' then
                    Dmode_Data_val   <= '0' ; 
			    elsIF squelch = '0'  then
                    Dmode_Data_val   <= message_out_val ;
                ELSIF squelch = '1' and Signal_detected = '1'  then
                    Dmode_Data_val   <= message_out_val ;
   
                ELSIF squelch = '1' and Signal_detected = '0' then
                    Dmode_Data_val   <= '0' ;    
                END IF;
--                IF Dmode_Data_val = '1' then
--                    Counter_arr <= Counter_arr + '1';
--                END IF;
--                Dmode_Data <= Counter_arr(15 DOWNTO 0); 
			   Dmode_Data    <= message_out(23)& message_out(20 downto 6);  
			   -------------------------------------------------------------
			   -------------------------------------------------------------
			   -------------------------------------------------------------
                if Mod_sel = x"00" then 
                    Data_Mod         <= AM_out ;
                    Data_mode_val    <= AM_val ;
                elsif Mod_sel = x"02" then 
                    Data_Mod         <= PM_out ;
                    Data_mode_val    <= PM_val ;
                else 
                    Data_Mod         <= FM_out ;
                    Data_mode_val    <= FM_val ;
                end if;
               -------------------------------------------------------------
			   -------------------------------------------------------------
			   ------------------------------------------------------------- 	
--			   if    (ModeFifo_prog_full = '1' and ModeFifo_rd_en = '0') then
--			         ModeFifo_rd_en <= '1';
--			         ModeFifo_rd_data_count <= (others => '0');
--			   elsif (ModeFifo_rd_en='1' and  ModeFifo_rd_data_count < x"03ff") then
--			         ModeFifo_rd_data_count <=  ModeFifo_rd_data_count + '1';
--			   else 
--			         ModeFifo_rd_en <= '0';
--			         ModeFifo_rd_data_count <= (others => '0');
--			   end if;		
--			   ModeFifo_rd_en_d <= ModeFifo_rd_en ;
			   ------------------------------------------------------------- 			
			   ------------------------------------------------------------- 			
			   ------------------------------------------------------------- 			
			end if;
	end process;
	
--	Dmode_Data_out_val <= ModeFifo_rd_en_d;
--	Dmode_Data_out <= ModeFifo_dout;
	
end Behavioral;	