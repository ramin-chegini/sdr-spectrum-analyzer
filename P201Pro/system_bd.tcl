# Import the authoritative current BD into a NEW project, without changing
# the source BD or relying on the historical ADI project-generation helpers.
# Called by create_project.tcl after adding the module reference HDL sources.
proc p201pro_import_bd {source_root} {
  set original [file normalize [file join $source_root \
    pzp351.srcs sources_1 bd system system.bd]]
  set output_root [get_property DIRECTORY [current_project]]
  set name [get_property NAME [current_project]]
  set folder [file join $output_root ${name}.srcs sources_1 bd system]
  set destination [file normalize [file join $folder system.bd]]
  if {$destination eq $original} {
    error "Recreate into a separate directory; refusing to overwrite the source BD."
  }
  if {[file exists $destination]} {
    error "Block design destination already exists: $destination"
  }
  file mkdir $folder
  file copy $original $destination
  read_bd $destination
  open_bd_design $destination
  return $destination
}
