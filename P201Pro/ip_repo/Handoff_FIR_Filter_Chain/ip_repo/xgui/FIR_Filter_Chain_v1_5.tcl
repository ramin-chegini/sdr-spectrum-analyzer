# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "Na" -parent ${Page_0}
  ipgui::add_param $IPINST -name "Nb" -parent ${Page_0}
  ipgui::add_param $IPINST -name "Nq" -parent ${Page_0}


}

proc update_PARAM_VALUE.Na { PARAM_VALUE.Na } {
	# Procedure called to update Na when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.Na { PARAM_VALUE.Na } {
	# Procedure called to validate Na
	return true
}

proc update_PARAM_VALUE.Nb { PARAM_VALUE.Nb } {
	# Procedure called to update Nb when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.Nb { PARAM_VALUE.Nb } {
	# Procedure called to validate Nb
	return true
}

proc update_PARAM_VALUE.Nq { PARAM_VALUE.Nq } {
	# Procedure called to update Nq when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.Nq { PARAM_VALUE.Nq } {
	# Procedure called to validate Nq
	return true
}


proc update_MODELPARAM_VALUE.Nq { MODELPARAM_VALUE.Nq PARAM_VALUE.Nq } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.Nq}] ${MODELPARAM_VALUE.Nq}
}

proc update_MODELPARAM_VALUE.Na { MODELPARAM_VALUE.Na PARAM_VALUE.Na } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.Na}] ${MODELPARAM_VALUE.Na}
}

proc update_MODELPARAM_VALUE.Nb { MODELPARAM_VALUE.Nb PARAM_VALUE.Nb } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.Nb}] ${MODELPARAM_VALUE.Nb}
}

