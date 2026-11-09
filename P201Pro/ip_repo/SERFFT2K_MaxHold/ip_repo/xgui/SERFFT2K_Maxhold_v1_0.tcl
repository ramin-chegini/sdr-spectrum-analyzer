# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "DPRam_DATA_WIDTH" -parent ${Page_0}
  ipgui::add_param $IPINST -name "PSDIdx_WIDTH" -parent ${Page_0}


}

proc update_PARAM_VALUE.DPRam_DATA_WIDTH { PARAM_VALUE.DPRam_DATA_WIDTH } {
	# Procedure called to update DPRam_DATA_WIDTH when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.DPRam_DATA_WIDTH { PARAM_VALUE.DPRam_DATA_WIDTH } {
	# Procedure called to validate DPRam_DATA_WIDTH
	return true
}

proc update_PARAM_VALUE.PSDIdx_WIDTH { PARAM_VALUE.PSDIdx_WIDTH } {
	# Procedure called to update PSDIdx_WIDTH when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.PSDIdx_WIDTH { PARAM_VALUE.PSDIdx_WIDTH } {
	# Procedure called to validate PSDIdx_WIDTH
	return true
}


proc update_MODELPARAM_VALUE.PSDIdx_WIDTH { MODELPARAM_VALUE.PSDIdx_WIDTH PARAM_VALUE.PSDIdx_WIDTH } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.PSDIdx_WIDTH}] ${MODELPARAM_VALUE.PSDIdx_WIDTH}
}

proc update_MODELPARAM_VALUE.DPRam_DATA_WIDTH { MODELPARAM_VALUE.DPRam_DATA_WIDTH PARAM_VALUE.DPRam_DATA_WIDTH } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DPRam_DATA_WIDTH}] ${MODELPARAM_VALUE.DPRam_DATA_WIDTH}
}

