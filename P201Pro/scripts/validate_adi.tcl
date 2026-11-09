# Run using Vivado 2019.2 from any working directory:
# vivado -mode batch -source /path/to/P201Pro/scripts/validate_adi.tcl
# Add -tclargs --generate to regenerate the BD output products as well.
if {![string match "2019.2*" [version -short]]} {
  error "This project requires Vivado 2019.2; do not upgrade its IPs."
}
set project_root [file normalize [file join [file dirname [info script]] ..]]
open_project [file join $project_root pzp351.xpr]

# Explicitly use only repositories contained in this project.
set_property ip_repo_paths [list \
  [file join $project_root third_party adi_hdl library] \
  [file join $project_root ip_repo]] [current_project]
update_ip_catalog -rebuild
foreach name {axi_ad9361 axi_dmac util_cpack2 util_axis_fifo util_cdc} {
  set vlnv analog.com:user:${name}:1.0
  if {[llength [get_ipdefs -all -quiet $vlnv]] != 1} {
    error "Expected exactly one local IP definition for $vlnv"
  }
}
open_bd_design [file join $project_root pzp351.srcs sources_1 bd system system.bd]
validate_bd_design
report_ip_status -file [file join $project_root adi_ip_status.rpt]
if {[info exists ::argv] && [lsearch -exact $::argv --generate] >= 0} {
  generate_target all [get_files */system.bd]
}
puts "ADI local repository and block design validation finished. Review adi_ip_status.rpt."
close_project
