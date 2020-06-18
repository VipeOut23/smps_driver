project( 'smps_driver', 'c',
         version : '1.0',
         default_options : ['warning_level=3',
                            'optimization=s',
                            'c_std=c17',
                            'debug=false'] )

# Build ########################################################################
src = files([
             'src/smps_driver.c'
           ])

c_args = [
  '-mmcu=' + host_machine.cpu(),
  '-DF_CPU=' + meson.get_cross_property('f_cpu').to_string()
]
link_args = []

# Dependencies #################################################################
avr_incdir = include_directories('/usr/avr/include')
avr_device_dep = meson.get_compiler('c').find_library(meson.get_cross_property('cpu_class')+'/lib'+host_machine.cpu())
avr_libc_dep = meson.get_compiler('c').find_library(meson.get_cross_property('cpu_class')+'/libc')
avr_libm_dep = meson.get_compiler('c').find_library(meson.get_cross_property('cpu_class')+'/libm')

# Programs #####################################################################
objcopy = find_program( 'avr-objcopy' )
avrdude = find_program( 'avrdude' )

# Optionals ####################################################################

if get_option('uart').enabled()
  message('Building with uart info')

  c_args += '-D__ENABLE_INFO__'
  c_args += '-DUART_TX_PIN='+get_option('uart_tx_pin')

  software_uart_dep = declare_dependency(
    link_with: static_library('software_uart',
                              sources: files(['src/software_uart/uart.c']),
                              dependencies: [avr_device_dep, avr_libc_dep, avr_libm_dep],
                              include_directories: avr_incdir,
                              c_args: c_args,
                              link_args: link_args,
                              pic: false),
    include_directories: include_directories('src/software_uart'))
else
  message('Building without uart info')
  software_uart_dep = dependency('', required: false)
endif

# Targets ######################################################################
elf = executable( 'rom.elf',
                  sources : src,
                  c_args: c_args,
                  link_args: link_args,
                  include_directories: avr_incdir,
                  dependencies: [avr_device_dep,
                                 avr_libc_dep,
                                 avr_libm_dep,
                                 software_uart_dep])

hex = custom_target( 'rom.hex',
                     output: 'rom.hex',
                     input: elf,
                     command: [objcopy, '-O', 'ihex', '@INPUT@', '@OUTPUT@'] )

run_target( 'flash',
            command: [avrdude, '-p', host_machine.cpu(),
                      '-c', get_option('flash_programmer'),
                      '-P', get_option('flash_port'),
                      '-b', get_option('flash_baud').to_string(),
                      '-U', 'flash:w:rom.hex'],
            depends: [hex] )
