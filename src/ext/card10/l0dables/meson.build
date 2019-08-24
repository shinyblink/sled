name = 'sled'

freertos = static_library(
  'freertos',
  freertos_sources,
  freertos_heap3,
  dependencies: periphdriver,
  include_directories: [
    freertos_includes,
    include_directories('./'),
  ],
)

#elf = executable(
#  name + '.elf',
#  'main.c',
#  build_by_default: true,
#  dependencies: [libff13, l0dable_startup, api_caller],
#  link_whole: [l0dable_startup_lib],
#  include_directories: [freertos_includes],
#  link_args: [
#    '-Wl,-Map=' + meson.current_build_dir() + '/' + name + '.map',
#	#meson.current_source_dir() + 'lib/libsled.a',
#    '-lsled',
#	'-L' + meson.current_source_dir() + 'lib',
#  ],
#  link_with: [freertos],
#  pie: true,
#)

#elf = executable(
#  name + '.elf',
#  'main.c',
#  #module_sources,
#  #l0der_sources,
#  build_by_default: true,
#  #dependencies: [libcard10, max32665_startup_core0, maxusb, libff13, l0dable_startup, api_caller],
#  dependencies: [libff13, l0dable_startup, api_caller],
#  #link_with: [api_dispatcher_lib, freertos],
#  link_with: [freertos],
#  #link_whole: [max32665_startup_core0_lib, board_card10_lib, newlib_heap_lib, l0dable_startup_lib],
#  #include_directories: [freertos_includes],
#  link_args: [
#    '-Wl,-Map=' + meson.current_build_dir() + '/' + name + '.map',
#	
#    #'-Wl,--defsym=_write=_write_epicardium',
#    '-lsled',
#	'-L' + meson.current_source_dir() + 'lib',
#  ],
#  pie: true,
#)

#api_caller_lib = static_library(
#  'api-caller',
#  'api/caller.c',
#  'api/interrupt-receiver.c',
#  api[0], # Caller
#  dependencies: periphdriver,
#)
#
#api_caller = declare_dependency(
#  include_directories: include_directories('.'),
#  link_with: api_caller_lib,
#  dependencies: periphdriver,
#)

#api_dispatcher_lib = static_library(
#  'api-dispatcher',
#  'api/dispatcher.c',
#  'api/interrupt-sender.c',
#  'api/control.c',
#  api[1], # Dispatcher
#  dependencies: [libcard10, periphdriver],
#)



elf = executable(
  name + '.elf',
  'main.c',
  #'support.c',
  build_by_default: true,
  #dependencies: [libcard10, libff13, l0dable_startup, api_caller],
  dependencies: [libff13, l0dable_startup, api_caller],
  link_with: [api_dispatcher_lib, freertos],
  #link_whole: [board_card10_lib, newlib_heap_lib, l0dable_startup_lib],
  link_whole: [l0dable_startup_lib],
  include_directories: [freertos_includes],
  link_args: [
    '-Wl,-Map=' + meson.current_build_dir() + '/' + name + '.map',
	'-lgcc',
    '-lsled',
	'-L' + meson.current_source_dir() + 'lib',
  ],
  pie: true,
)
