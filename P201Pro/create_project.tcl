# Recreate the CURRENT P201PRO Z7020 design using only this directory.
# Vivado 2019.2: vivado -mode batch -source create_project.tcl
# Options: --output_dir PATH --project_name NAME --generate --build --jobs N
# The previous exported script is preserved in docs/legacy/create_project.tcl.txt.
if {![string match "2019.2*" [version -short]]} {
  error "Use Vivado 2019.2. This script does not upgrade the project or IPs."
}
set source_root [file normalize [file dirname [info script]]]
set project_name pzp351
set output_dir ""
set do_generate 0
set do_build 0
set jobs 4
if {![info exists ::argv]} { set ::argv {} }
for {set i 0} {$i < [llength $::argv]} {incr i} {
  set option [lindex $::argv $i]
  switch -- $option {
    --generate { set do_generate 1 }
    --build { set do_build 1; set do_generate 1 }
    --output_dir - --project_name - --jobs {
      incr i
      if {$i >= [llength $::argv]} { error "Missing value for $option" }
      set value [lindex $::argv $i]
      switch -- $option {
        --output_dir { set output_dir [file normalize $value] }
        --project_name { set project_name $value }
        --jobs { set jobs $value }
      }
    }
    default { error "Unknown argument: $option" }
  }
}
if {![regexp {^[A-Za-z0-9_][A-Za-z0-9_.-]*$} $project_name]} {
  error "Invalid project name: $project_name"
}
if {![string is integer -strict $jobs] || $jobs < 1} {
  error "--jobs must be a positive integer"
}
if {$output_dir eq ""} {
  set output_dir [file join $source_root build $project_name]
}
if {[file exists $output_dir]} {
  if {![file isdirectory $output_dir]} { error "Not a directory: $output_dir" }
  foreach item [glob -nocomplain -tails -directory $output_dir * .*] {
    if {$item ne "." && $item ne ".."} {
      error "Output directory is not empty: $output_dir. Choose a new --output_dir."
    }
  }
}
set required_sources {
  third_party/adi_hdl/library/common/ad_iobuf.v
  pzp351.srcs/sources_1/bd/system/hdl/system_wrapper.v
  system_top.v
  third_party/adi_hdl/library/common/ad_bus_mux.v
  third_party/adi_hdl/library/util_cdc/sync_bits.v
  third_party/adi_hdl/library/common/util_pulse_gen.v
  coe/coefile_int.coe
  pzp351.srcs/sources_1/bd/system/ip/system_DeModulation_Block_0_0/src/Filter_LPF4KHz_sr64K_FM.coe
  pzp351.srcs/sources_1/bd/system/ip/system_FIR_Filter_Chain_0_6/src/EqRipple_Dec2_150KHz.coe
  pzp351.srcs/sources_1/bd/system/ip/system_FIR_Filter_Chain_0_6/src/EqRipple_Dec4_200KHz.coe
  pzp351.srcs/sources_1/bd/system/ip/system_FIR_Filter_Chain_0_6/src/EqRipple_Dec2_600Hz.coe
  pzp351.srcs/sources_1/bd/system/ip/system_FIR_Filter_Chain_0_6/src/EqRipple_Dec2_1_2KHz.coe
  pzp351.srcs/sources_1/bd/system/ip/system_FIR_Filter_Chain_0_6/src/EqRipple_Dec2_3KHz.coe
  pzp351.srcs/sources_1/bd/system/ip/system_FIR_Filter_Chain_0_6/src/EqRipple_Dec2_5KHz.coe
  pzp351.srcs/sources_1/bd/system/ip/system_FIR_Filter_Chain_0_6/src/EqRipple_Dec2_7_5KHz.coe
  pzp351.srcs/sources_1/bd/system/ip/system_FIR_Filter_Chain_0_6/src/EqRipple_Dec2_12KHz.coe
  pzp351.srcs/sources_1/bd/system/ip/system_FIR_Filter_Chain_0_6/src/EqRipple_Dec2_15KHz.coe
  pzp351.srcs/sources_1/bd/system/ip/system_FIR_Filter_Chain_0_6/src/EqRipple_Dec2_30KHz.coe
  pzp351.srcs/sources_1/bd/system/ip/system_FIR_Filter_Chain_0_6/src/EqRipple_Dec2_60KHz.coe
}
set required_constraints {
  system_constr.xdc
  adi_fir_filter_constr.xdc
}
foreach relative [concat $required_sources $required_constraints [list \
    pzp351.srcs/sources_1/bd/system/system.bd \
    third_party/adi_hdl/library/axi_ad9361/component.xml \
    third_party/adi_hdl/library/axi_dmac/component.xml \
    third_party/adi_hdl/library/util_pack/util_cpack2/component.xml \
    third_party/adi_hdl/library/util_axis_fifo/component.xml \
    third_party/adi_hdl/library/util_cdc/component.xml \
    third_party/adi_hdl/library/interfaces/fifo_wr.xml \
    third_party/adi_hdl/library/interfaces/fifo_wr_rtl.xml \
    third_party/adi_hdl/library/interfaces/fifo_rd.xml \
    third_party/adi_hdl/library/interfaces/fifo_rd_rtl.xml]] {
  if {![file isfile [file join $source_root $relative]]} {
    error "Missing project input: $relative"
  }
}
create_project $project_name $output_dir -part xc7z020clg400-2
set_property target_language Verilog [current_project]
set_property simulator_language Mixed [current_project]
set_property ip_cache_permissions disable [current_project]
set_property ip_repo_paths [list \
  [file join $source_root third_party adi_hdl library] \
  [file join $source_root ip_repo]] [current_project]
update_ip_catalog -rebuild
foreach relative $required_sources {
  add_files -norecurse [file join $source_root $relative]
}
foreach relative $required_constraints {
  add_files -fileset constrs_1 -norecurse [file join $source_root $relative]
}
set_property top system_top [get_filesets sources_1]
set_property top system_top [get_filesets sim_1]
set_property STEPS.PHYS_OPT_DESIGN.IS_ENABLED true [get_runs impl_1]
source [file join $source_root system_bd.tcl]
set bd_file [p201pro_import_bd $source_root]
validate_bd_design
save_bd_design
update_compile_order -fileset sources_1
report_ip_status -file [file join $output_dir adi_ip_status.rpt]
if {$do_generate} {
  generate_target all [get_files $bd_file]
}
if {$do_build} {
  launch_runs synth_1 -jobs $jobs
  wait_on_run synth_1
  if {[get_property PROGRESS [get_runs synth_1]] ne "100%"} {
    error "Synthesis failed: [get_property STATUS [get_runs synth_1]]"
  }
  launch_runs impl_1 -to_step write_bitstream -jobs $jobs
  wait_on_run impl_1
  if {[get_property PROGRESS [get_runs impl_1]] ne "100%"} {
    error "Implementation failed: [get_property STATUS [get_runs impl_1]]"
  }
}
puts "Created $output_dir/$project_name.xpr using only P201Pro inputs."
