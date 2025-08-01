project('axiom', 'c',
  version: '1.0.0',
  license: 'GPL-3.0',
  meson_version: '>=0.60.0',
  default_options: ['c_std=c11', 'warning_level=2', 'werror=false', 'buildtype=release'],
)

add_project_arguments('-DAXIOM_VERSION="@0@"'.format(meson.project_version()), '-D_GNU_SOURCE', '-DWLR_USE_UNSTABLE', language: 'c')

cc = meson.get_compiler('c')

# Dependencies
wayland_server = dependency('wayland-server', version: '>=1.20.0')
wayland_protos = dependency('wayland-protocols', version: '>=1.25')
wlroots = dependency('wlroots-0.19')
xkbcommon = dependency('xkbcommon')
math_lib = cc.find_library('m', required: true)

# Protocol generation
wayland_scanner = find_program('wayland-scanner')
wl_protocol_dir = wayland_protos.get_variable('pkgdatadir')

wayland_scanner_server = generator(
  wayland_scanner,
  output: '@BASENAME@-protocol.h',
  arguments: ['server-header', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_code = generator(
  wayland_scanner,
  output: '@BASENAME@-protocol.c',
  arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

server_protocols = [
  wl_protocol_dir / 'stable/xdg-shell/xdg-shell.xml',
]

server_protos_src = []
server_protos_headers = []

foreach xml : server_protocols
  server_protos_src += wayland_scanner_code.process(xml)
  server_protos_headers += wayland_scanner_server.process(xml)
endforeach

# Include directories
inc = include_directories('include')

# Source Code
axiom_sources = files(
  'src/main.c',
  'src/logging.c',
  'src/input.c',
  'src/cursor.c',
  'src/config.c',
  'src/process.c',
  'src/tiling.c',
  'src/workspace.c'
)

# Executable
proto_inc = declare_dependency(
  sources: server_protos_headers,
)

global_deps = [wayland_server, wlroots, xkbcommon, proto_inc, math_lib]
executable('axiom',
  sources: axiom_sources + server_protos_src,
  dependencies: global_deps,
  include_directories: inc,
  install: true,
)

# Tests
test_sources = files(
  'tests/test_basic.c'
)

test_executable = executable('test_axiom',
  sources: test_sources,
  dependencies: [math_lib],  # Only need math lib for tests
  include_directories: inc,
  install: false,
)

test_config_sources = files(
  'tests/test_config.c'
)

test_config_executable = executable('test_config',
  sources: test_config_sources,
  include_directories: inc,
  install: false,
)

test('basic_tests', test_executable)
test('config_tests', test_config_executable)

