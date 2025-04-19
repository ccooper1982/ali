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

blkid_dep = dependency('blkid', required: true)
libmount_dep = dependency('mount', required: true)
qt6_dep = dependency('qt6', required: true, modules: ['Core', 'Gui', 'Widgets', 'Network'])

sources = [
    'src/install.cpp',
    'src/packages.cpp',
    'src/commands.cpp',
    'src/disk_utils.cpp',
    'src/locale_utils.cpp',
    'src/profiles.cpp',
    'src/widgets/partitions_widget.cpp',
    'src/widgets/accounts_widget.cpp',
    'src/widgets/packages_widget.cpp',
    'src/widgets/install_widget.cpp',
    'src/widgets/start_widget.cpp',
    'src/widgets/profile_widget.cpp',
    'src/widgets/video_widget.cpp',
    'src/widgets/widgets.cpp',    
    'src/ali.cpp'
    ]

includes = include_directories('include')

# qt6 wrangle
moc_includes = ['include/ali/install.hpp',
                          'include/ali/widgets/install_widget.hpp',
                          'include/ali/widgets/packages_widget.hpp'
                         ]

qt6 = import('qt6')
moc_files = qt6.compile_moc( headers : moc_includes,
                                              include_directories: includes,
                                              dependencies: qt6_dep)


executable( meson.project_name(),
            sources,
            moc_files,
            include_directories: includes,
            dependencies: [blkid_dep, libmount_dep, qt6_dep])