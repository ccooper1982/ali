project('ali',
  'cpp',
  meson_version: '>= 1.1',  # for meson.options
  version : '0.1',
  default_options : ['cpp_std=c++20']) # TODO warning_level


if build_machine.system() != 'linux'  
  # could check for Arch by running: cat /etc/os-release | grep ^ID=
  # but that may be too restrictive
  error('Must be Linux')
endif


# ALI_DEV is used to signal we're compiling for dev machine, 
# rather than production. Although in development the app is ran
# without sudo, this can restrict further. Also, certain programs
# are only available on Arch live (i.e. arch-install-scripts).

if get_option('is_dev')
  add_project_arguments('-DALI_DEV', language: 'cpp')
else
  add_project_arguments('-DALI_PROD', language: 'cpp')
endif

blkid = dependency('blkid', required: true)
libmount = dependency('mount', required: true)

sources = [
    'src/test.cpp',
    'src/console.cpp',
    'src/commands.cpp',
    'src/utils.cpp'
    ]

includes = include_directories('include')

executable( meson.project_name(),
            sources,
            include_directories: includes,
            dependencies: [blkid, libmount])