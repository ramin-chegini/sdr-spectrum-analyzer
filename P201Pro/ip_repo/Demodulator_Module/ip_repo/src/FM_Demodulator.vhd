----------------------------------------------------------------------------------
-- Company: 			Sazgan Ertebat
-- Engineer: 			Masoud Maryami
-- 
-- Create Date:    15:38:16 06/06/2015 
-- Design Name: 	  	
-- Module Name:    FM_Demodulator - Behavioral 
-- Project Name: 	 CPU_IF_ver2
-- Target Devices: 

----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
USE ieee.std_logic_arith.ALL;
--USE ieee.std_logic_UNsigned.all;
USE ieee.std_logic_signed.all;
--use ieee.numeric_std.all;
Library UNISIM;
use UNISIM.vcomponents.all;

entity FM_Demodulator is
 generic ( Na  : integer := 24;Nb  : integer := 24) ;
 Port ( CLK_102_4M        		: in    std_logic; --- 1.024  Mhz	
		  RST         			: in    std_logic;
		  Mod_Sel 		  	   	: in    std_logic_vector (7 downto 0);
		  I_in 		  	   		: in    std_logic_vector (Na-1 downto 0);
		  Q_in    		   		: in    std_logic_vector (Na-1 downto 0);
		  IQ_Val 				: in    std_logic;
		  PhaseOfset   		    : out   std_logic_vector (Nb-1 downto 0);
		  PhaseOfset_val   		    : inout   std_logic;
		  
		  AM_out   		   		: out   std_logic_vector (Nb-1 downto 0);
		  AM_val   		        : out   std_logic;
		  FM_out   		   		: INout   std_logic_vector (Nb-1 downto 0);
		  FM_val   		        : inout   std_logic;
		  PM_val   		        : out   std_logic;
		  PM_out   		: out   std_logic_vector (Nb-1 downto 0));
end FM_Demodulator;

 architecture Behavioral of FM_Demodulator is

COMPONENT Phase_detector
  PORT (
    CLK : IN STD_LOGIC;
    A : IN STD_LOGIC_VECTOR(23 DOWNTO 0);
    B : IN STD_LOGIC_VECTOR(23 DOWNTO 0);
    CE : IN STD_LOGIC;
    P : OUT STD_LOGIC_VECTOR(47 DOWNTO 0)
  );
END COMPONENT;
COMPONENT addsub_FM
  PORT (
    A : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    B : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    CLK : IN STD_LOGIC;
    ADD : IN STD_LOGIC;
    CE : IN STD_LOGIC;
    S : OUT STD_LOGIC_VECTOR(47 DOWNTO 0)
  );
END COMPONENT;
COMPONENT mult_IQ_FM
  PORT (
    CLK : IN STD_LOGIC;
    A : IN STD_LOGIC_VECTOR(23 DOWNTO 0);
    B : IN STD_LOGIC_VECTOR(23 DOWNTO 0);
    CE : IN STD_LOGIC;
    P : OUT STD_LOGIC_VECTOR(47 DOWNTO 0)
  );
END COMPONENT;
COMPONENT AM_Cordic
  PORT (
    aclk : IN STD_LOGIC;
    s_axis_cartesian_tvalid : IN STD_LOGIC;
    s_axis_cartesian_tdata : IN STD_LOGIC_VECTOR(47 DOWNTO 0);
    m_axis_dout_tvalid : OUT STD_LOGIC;
    m_axis_dout_tdata : OUT STD_LOGIC_VECTOR(23 DOWNTO 0)
  );
END COMPONENT;
--constant Nb			: integer := 24;
signal   		 CORDIC_RDY			 			   						   			: std_logic;
signal   		 AM_Data_val			 			   						   			: std_logic;
signal   		 CORDIC_PHASE_OUT,CORDIC_PHASE_OUT_d1,PHASE_BUF,CORDIC_AMP_OUT,AM_Data					: std_logic_vector(Na-1 downto 0);
signal   		 Data_dif,Input_magnitude,FM_out_buf,AM_DC_Removed		: std_logic_vector(Na-1 downto 0);
signal   		 m_axis_dout_tdata,Phase_detector_data	        														: std_logic_vector(23 downto 0);        		         	
signal   		 PM_OUT_scaled,PM_OUT_VAR			 								: std_logic_vector(Nb-1 downto 0);
signal   		 LAST_DC,AM_LAST_DC																		: std_logic_vector(Nb-1+10 downto 0):=(others=>'0');
signal   		 data_out,Phase_DC								: std_logic_vector(Nb-1 downto 0);
signal   		 I_in_d1,Q_in_d1								: std_logic_vector(Na-1 downto 0);
signal   		 IxId,QxQd,QxId,IxQd,Sin,Cos,QxQ,IxI,IQ_acc								: std_logic_vector(2*Na-1 downto 0);
signal   		 data_out_val							: std_logic;
signal   		 s_axis_cartesian_tdata					: std_logic_vector(2*Na-1 downto 0);

attribute keep : string;
attribute mark_debug :string;
attribute keep of  Phase_DC  : signal is "TRUE";
attribute mark_debug of  Phase_DC  : signal is "TRUE";
        		               												
begin
s_axis_cartesian_tdata <= Q_in & I_in;
ATAN_CORDIC_INSTANCE : entity work.ArcTAN_CORDIC_2_1
	port map (
    aclk => CLK_102_4M,
    s_axis_cartesian_tvalid => IQ_Val,
    s_axis_cartesian_tdata => s_axis_cartesian_tdata,
    m_axis_dout_tvalid => CORDIC_RDY,
    m_axis_dout_tdata => CORDIC_PHASE_OUT
  );
-----------------------------------------------------------------------------------------------------------------------
------------------------------------------------- FM Demodulator		--------------------------------------------------
-----------------------------------------------------------------------------------------------------------------------
process(CLK_102_4M,RST)
begin
if (RST='1') then
    PHASE_BUF <= (others => '0');
    Data_dif <= (others => '0');
    FM_out <= (others => '0');
elsif(rising_edge(CLK_102_4M)) then
	if(CORDIC_RDY='1') then
	   PHASE_BUF<=CORDIC_PHASE_OUT;
	   Data_dif<=CORDIC_PHASE_OUT  - PHASE_BUF;	
	   FM_out(Nb - 4 downto 0) 			 <= Data_dif(Nb - 4 downto 0);
       if   (Data_dif(Nb - 1 downto Nb - 3) = "001") then                                            
              FM_out(Nb - 1 downto Nb - 3) 		 <= "111" ;
       elsif (Data_dif(Nb - 1 downto Nb - 3) = "110") then
              FM_out(Nb - 1 downto Nb - 3) 		 <= "000" ;
       else
              FM_out(Nb - 1 downto Nb - 3) 		 <= Data_dif(Nb - 1 downto Nb - 3);
       end if;                                       
--      FM_out_buf(Nb - 4 downto 0) 		 <= Data_dif(Nb - 4 downto 0);
--      FM_out_buf(Nb - 1 downto Nb - 3)  <= "111" when Data_dif(Nb - 1 downto Nb - 3) = "001" else -- If +1.0 <= UPhi < +2.0 then FM_out = Data_dif - 2.0
--                                                     "000" when Data_dif(Nb - 1 downto Nb - 3) = "110" else -- If -2.0 <= UPhi < -1.0 then FM_out = Data_dif + 2.0
--                                                     Data_dif(Nb - 1 downto Nb - 3); 
	end if;	
	FM_val <= CORDIC_RDY;									
 end if;				
end process;
--process(CLK_102_4M,rst)
--begin
--	if rst='1'  then
--        I_in_d1 <= (others => '0');
--        Q_in_d1 <= (others => '0');
----        FM_out <= (others => '0');
----        FM_val <= '0';
--	elsif rising_edge(CLK_102_4M) then
--	   if IQ_Val='1' then
--            I_in_d1 <= I_in;
--            Q_in_d1 <= Q_in;
--	   end if;
----	   if CORDIC_RDY ='1' then
----	       FM_out  <= m_axis_dout_tdata - x"0C00";
----	   end if;
----	   FM_val <= CORDIC_RDY;
--	end if;
--end process;
--mult_IQ_FM_1 : mult_IQ_FM
--  PORT MAP (
--    CLK => CLK_102_4M,
--    A => I_in,
--    B => I_in_d1 ,
--    CE => '1',
--    P => IxId
--  );
--mult_IQ_FM_2 : mult_IQ_FM
--  PORT MAP (
--    CLK => CLK_102_4M,
--    A => I_in_d1 ,
--    B => Q_in ,
--    CE => '1',
--    P => QxId
--  );
--mult_IQ_FM_3 : mult_IQ_FM
--  PORT MAP (
--    CLK => CLK_102_4M,
--    A => I_in ,
--    B => Q_in_d1 ,
--    CE => '1',
--    P => IxQd
--  );
--mult_IQ_FM_4 : mult_IQ_FM
--  PORT MAP (
--    CLK => CLK_102_4M,
--    A => Q_in_d1  ,
--    B => Q_in ,
--    CE => '1',
--    P => QxQd
--  );

--addsub_FM_sin : addsub_FM
--  PORT MAP (
--    A => QxId,
--    B => IxQd,
--    CLK => CLK_102_4M,
--    ADD => '0',
--    CE => '1',
--    S => Sin
--  );
--addsub_FM_cos : addsub_FM
--  PORT MAP (
--    A => IxId,
--    B => QxQd,
--    CLK => CLK_102_4M,
--    ADD => '1',
--    CE => '1',
--    S => Cos
--  );

--s_axis_cartesian_tdata <= cos & Sin;

----s_axis_cartesian_tdata <= Q_in & I_in;
--ATAN_CORDIC_INSTANCE : entity work.ATAN_CORDIC
--	port map (
--    aclk => CLK_102_4M,
--    s_axis_cartesian_tvalid => IQ_Val,
--    s_axis_cartesian_tdata => s_axis_cartesian_tdata,
--    m_axis_dout_tvalid => CORDIC_RDY,
--    m_axis_dout_tdata => m_axis_dout_tdata
--  );
process(CLK_102_4M,rst)
	variable   MULTIPLIER_OUT :STD_LOGIC_VECTOR(Nb-1+10 downto 0):=(others=>'0');
	variable   SUBTRACTOR_OUT :STD_LOGIC_VECTOR(Nb-1 downto 0):=(others=>'0');
	variable   cntavg	      : integer range 0 to 400 := 0;
begin
	if rst='1'  then
--	   MULTIPLIER_OUT := (others=>'0');
--	   SUBTRACTOR_OUT := (others=>'0');
--	   cntavg         := 0;
--	   LAST_DC 		  <= (others=>'0');
--       PM_out         <= (others => '0');
       PM_val         <= '0';	   
	elsif rising_edge(CLK_102_4M) then
--	   if (CORDIC_RDY = '1') then
--            SUBTRACTOR_OUT := CORDIC_PHASE_OUT - LAST_DC( Nb-1+10 downto 10);
--            MULTIPLIER_OUT := SXT(SUBTRACTOR_OUT, Nb+10);
--            LAST_DC <= MULTIPLIER_OUT + LAST_DC;
--            if cntavg >= 37 then
----                FM_out 		<= m_axis_dout_tdata;
--                PM_out 		<= SUBTRACTOR_OUT;
--            else
--                cntavg := cntavg + 1;
--                PM_out 		<=  (others =>'0');
--            end if;
--		end if;
	   PM_val <= CORDIC_RDY;
	end if;
end process;

PM_DC_remover : entity work.DC_Removal_v3
    PORT MAP (
              Clk           => CLK_102_4M,
              rst           => rst,
              Data_val      => CORDIC_RDY,
              x             => CORDIC_PHASE_OUT,
              y             => PM_out
    );
--------------------------------------------------------------------------------------------------------		
--------------------------------------------------------------------------------------------------------		
--------------------------------------------------------------------------------------------------------		
--------------------------------------------------------------------------------------------------------		
process(CLK_102_4M,RST)
begin
if (RST='1') then
--    AM_Data  <= (others => '0');
elsif(rising_edge(CLK_102_4M)) then
	if(IQ_Val='1') then
         IQ_acc <=  IxI + QxQ ;     
	end if;			
end if;			
end process ;			
mult_I_AM : mult_IQ_FM
  PORT MAP (
    CLK => CLK_102_4M,
    A => I_in,
    B => I_in ,
    CE => IQ_Val,
    P => IxI
  );		
mult_Q_AM : mult_IQ_FM
  PORT MAP (
    CLK => CLK_102_4M,
    A => Q_in,
    B => Q_in ,
    CE => IQ_Val,
    P => QxQ
  );		
AM_Cordic_inst : AM_Cordic
  PORT MAP (
    aclk => CLK_102_4M,
    s_axis_cartesian_tvalid => IQ_Val,
    s_axis_cartesian_tdata => IQ_acc,
    m_axis_dout_tvalid => AM_Data_val ,
    m_axis_dout_tdata => AM_Data
  );		
--Phase_detector_inst : Phase_detector
--  PORT MAP (
--    CLK => CLK_102_4M,
--    A => I_in,
--    B => Q_in,
--    CE => IQ_Val,
--    P => Phase_detector_data
--  );		
process(CLK_102_4M,rst)
	variable   MULTIPLIER_OUT :STD_LOGIC_VECTOR(Na-1+10 downto 0):=(others=>'0');
	variable   SUBTRACTOR_OUT :STD_LOGIC_VECTOR(Na-1 downto 0):=(others=>'0');
	variable   cntavg	      : integer range 0 to 400 := 0;
begin
	if rst='1'  then
	   MULTIPLIER_OUT := (others=>'0');
	   SUBTRACTOR_OUT := (others=>'0');
	   cntavg         := 0;
	   AM_DC_Removed 		<= (others=>'0');
	   AM_LAST_DC 		<= (others=>'0');
	   AM_val 		<= '0';
	elsif rising_edge(CLK_102_4M) then
	   if (AM_Data_val = '1') then
            SUBTRACTOR_OUT := AM_Data - AM_LAST_DC( Na-1+10 downto 10);
            MULTIPLIER_OUT := SXT(SUBTRACTOR_OUT, Na+10);
            AM_LAST_DC <= MULTIPLIER_OUT + AM_LAST_DC;
            if cntavg >= 37 then
                AM_out 		<= SUBTRACTOR_OUT;
            else
                cntavg := cntavg + 1;
                AM_out		<=  (others =>'0');
            end if;
		end if;
		AM_val <= AM_Data_val;
	end if;
end process;
--AM_out <= AM_DC_Removed;
----	Phase_detector_inst_DC: entity work.DC_Removal_v3
----	generic map(Data_width => Nb,Integ_mag=>11)
----	port map(
----		rst 			=> Rst,
----		Clk			    => CLK_102_4M,
----		x				=> FM_out,
----		x_val			=> CORDIC_RDY,
----		DC_out			=> Phase_DC
----		);
----        PM_DC_Removed <=	Phase_DC;
		
----		AM_DC_Remover: entity work.DC_Removal_v3
----			generic map(Data_width => 24,Integ_mag=>18)
----			port map(
----				rst 			=> Rst,
----				Clk			    => CLK_102_4M,
----				x				=> AM_Data,
----				x_val			=> CORDIC_RDY,
----				y				=> AM_out
----				);	
-------------------------------------------------------------------------------------------------------------------------
--------------------------------------------------- FM Demodulator		--------------------------------------------------
-------------------------------------------------------------------------------------------------------------------------
--process(CLK_102_4M,RST)
--begin
--if (RST='1') then
--    PHASE_BUF <= (others => '0');
--    --Data_dif <= (others => '0');
--    FM_out <= (others => '0');
--elsif(rising_edge(CLK_102_4M)) then
--	if(CORDIC_RDY='1') then
--	   PHASE_BUF<=CORDIC_PHASE_OUT;
----	   Data_dif<= SIGNED(CORDIC_PHASE_OUT) - SIGNED(PHASE_BUF);	
	   
--	  FM_out(Nb - 4 downto 0) 			 <= Data_dif(Nb - 4 downto 0);
--       if   (Data_dif(Nb - 1 downto Nb - 3) = "001") then                                            
--              FM_out(Nb - 1 downto Nb - 3) 		 <= "111" ;
--       elsif (Data_dif(Nb - 1 downto Nb - 3) = "110") then
--              FM_out(Nb - 1 downto Nb - 3) 		 <= "000" ;
--       else
--              FM_out(Nb - 1 downto Nb - 3) 		 <= Data_dif(Nb - 1 downto Nb - 3);
--       end if;                                       
--	end if;	
					
-- end if;				
--end process;
--phase_sub_inst :entity work.phase_sub
--  PORT MAP (
--    A => CORDIC_PHASE_OUT,
--    B => PHASE_BUF,
--    CLK => CLK_102_4M,
--    ADD => '0',
--    CE => CORDIC_RDY,
--    SCLR => RST,
--    S => Data_dif
--  );
-------------------------------------------------------------------------------------------------------------------------
--------------------------------------------------- PM Demodulator		--------------------------------------------------
-------------------------------------------------------------------------------------------------------------------------

----PM_DC_Remover: entity work.DC_Removal_v3
----	generic map(Data_width => 24,Integ_mag=>13)
----	port map(
----		rst 			=> Rst,
----		Clk			    => CLK_102_4M,
----		x				=> CORDIC_PHASE_OUT,
----		x_val			=> CORDIC_RDY,
----		DC_out			=> PM_DC_Removed
----		);


--process(CLK_102_4M,RST)
--begin
--    if RST = '1' then 
--        CORDIC_PHASE_OUT_d1<= (others => '0');
--        PM_OUT_scaled<= (others => '0');
--    elsif(rising_edge(CLK_102_4M)) then
--        if (CORDIC_RDY = '1') then
--            CORDIC_PHASE_OUT_d1<=CORDIC_PHASE_OUT;	
--            PM_OUT_scaled <= FM_out ;--+ PM_OUT_VAR;			
--        end if;				
--    end if;				
--end process;

----process(CLK_102_4M,RST)
----		variable St_Cnt : integer range 0 to 31;
----	begin
----if(rising_edge(CLK_102_4M)) then
----    if (CORDIC_RDY = '1') then
----		if(signed(Data_dif) > 10000) then					
----			PM_OUT_VAR <= PM_OUT_VAR - x"4000" ;
----		elsif(signed(Data_dif) < -10000) then
----			PM_OUT_VAR <= PM_OUT_VAR + x"4000" ;	
----		end if;	
----    end if;	
----end if;
----end process;


	 
end Behavioral;

