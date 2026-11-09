# Compatibility entry point for the current local P201PRO design.
# The previous Z7035 ADI reference is in docs/legacy/system_project.tcl.txt.
# Pass --build to create a bitstream; the default only recreates the project.
source [file join [file dirname [info script]] create_project.tcl]
