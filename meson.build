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
egl = dependency('egl')
gl = dependency('gl')
gles3 = dependency('glesv2')

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
  'src/workspace.c',
  'src/animation.c',
  'src/effects.c',
  'src/effects_realtime.c',
  'src/renderer.c',
  'src/window_rules.c',
  'src/smart_gaps.c'
)

# Executable
proto_inc = declare_dependency(
  sources: server_protos_headers,
)

global_deps = [wayland_server, wlroots, xkbcommon, proto_inc, math_lib, egl, gles3]
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

test_animation_sources = files(
  'tests/test_animation.c',
  'tests/animation_test_impl.c'
)

test_animation_executable = executable('test_animation',
  sources: test_animation_sources,
  dependencies: [math_lib, wayland_server],
  include_directories: inc,
  install: false,
)

test('basic_tests', test_executable)
test('config_tests', test_config_executable)
test('animation_tests', test_animation_executable)

# Animation config test
test_animation_config_sources = files(
  'tests/test_animation_config.c',
  'src/config.c'
)

test_animation_config = executable('test_animation_config',
    sources: test_animation_config_sources,
    dependencies: [math_lib],
    include_directories: inc,
    install: false,
)
test('animation_config_tests', test_animation_config)

# Effects test
test_effects_sources = files(
  'tests/test_effects.c',
  'src/effects.c'
)

test_effects = executable('test_effects',
    sources: test_effects_sources,
    dependencies: [math_lib, wayland_server, wlroots, proto_inc],
    include_directories: inc,
    install: false,
)
test('effects_tests', test_effects)

# Window rules test (basic functionality only)
test_window_rules_sources = files(
  'tests/test_window_rules.c'
)

test_window_rules = executable('test_window_rules',
    sources: test_window_rules_sources,
    dependencies: [wayland_server],
    include_directories: inc,
    install: false,
)
test('window_rules_tests', test_window_rules)

