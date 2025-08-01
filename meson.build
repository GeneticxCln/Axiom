project('axiom', 'c',
  version: '1.0.0',
  license: 'GPL-3.0',
  meson_version: '>=0.60.0',
  default_options: ['c_std=c11', 'warning_level=2', 'werror=false', 'buildtype=release'],
)

add_project_arguments('-DAXIOM_VERSION="@0@"'.format(meson.project_version()), '-D_GNU_SOURCE', '-DWLR_USE_UNSTABLE', language: 'c')

cc = meson.get_compiler('c')

# Dependencies
dependency('wayland-server', version: '>=1.20.0')
dependency('wayland-protocols', version: '>=1.25')
dependency('wlroots-0.19')

# Include directories
inc = include_directories('include')

# Source Code
axiom_sources = files('src/main.c', 'src/logging.c')

# Executable
global_deps = [dependency('wayland-server'), dependency('wlroots-0.19')]
executable('axiom',
  sources: axiom_sources,
  dependencies: global_deps,
  include_directories: inc,
  install: true,
)

