project('ali',
  'cpp',
  version : '0.1',
  meson_version: '>= 1.1',  # for meson.options
  default_options : ['cpp_std=c++20']) # TODO warning_level/werror


if build_machine.system() != 'linux'  
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

blkid_dep = dependency('blkid', required: true)
libmount_dep = dependency('mount', required: true)
ftxui_dep = dependency('ftxui', required: true, static: true)

sources = ['src/ali2.cpp']

includes = include_directories('include')

executable( meson.project_name(),
            sources,
            include_directories: includes,
            dependencies: [blkid_dep, libmount_dep, ftxui_dep])

