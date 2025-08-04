project('axiom', 'c',
version: '4.3.8',
  license: 'GPL-3.0',
  meson_version: '>=0.60.0',
  default_options: ['c_std=c11', 'warning_level=2', 'werror=false', 'buildtype=release'],
)

add_project_arguments('-DAXIOM_VERSION="@0@"'.format(meson.project_version()), '-D_GNU_SOURCE', '-DWLR_USE_UNSTABLE', language: 'c')

# Optional feature flags
if get_option('enable_debug_logs')
  add_project_arguments('-DAXIOM_DEBUG_LOGS', language: 'c')
endif

if get_option('enable_performance_monitoring')
  add_project_arguments('-DAXIOM_PERFORMANCE_MONITORING', language: 'c')
endif

cc = meson.get_compiler('c')

# Enhanced dependency management with better error reporting
message('Checking core dependencies...')

# Core Wayland dependencies (required)
wayland_server = dependency('wayland-server', version: '>=1.20.0',
                           required: true,
                           not_found_message: 'wayland-server >= 1.20.0 is required. Install with: sudo pacman -S wayland (Arch) or sudo apt install libwayland-dev (Ubuntu)')
wayland_client = dependency('wayland-client', version: '>=1.20.0',
                           required: true,
                           not_found_message: 'wayland-client >= 1.20.0 is required. Install with: sudo pacman -S wayland (Arch) or sudo apt install libwayland-dev (Ubuntu)')
wayland_protos = dependency('wayland-protocols', version: '>=1.25',
                           required: true,
                           not_found_message: 'wayland-protocols >= 1.25 is required. Install with: sudo pacman -S wayland-protocols (Arch) or sudo apt install wayland-protocols (Ubuntu)')
wlr_protos = dependency('wlr-protocols',
                       required: true,
                       not_found_message: 'wlr-protocols is required. Install wlroots development package: sudo pacman -S wlroots (Arch) or build from source (Ubuntu)')
wlroots = dependency('wlroots-0.19',
                    required: true,
                    not_found_message: 'wlroots 0.19 is required. Install with: sudo pacman -S wlroots (Arch) or build from source (Ubuntu)')

# Keyboard and input dependencies (required)
xkbcommon = dependency('xkbcommon',
                      required: true,
                      not_found_message: 'libxkbcommon is required. Install with: sudo pacman -S libxkbcommon (Arch) or sudo apt install libxkbcommon-dev (Ubuntu)')
xkbcommon_x11 = dependency('xkbcommon-x11',
                          required: true,
                          not_found_message: 'libxkbcommon-x11 is required. Install with: sudo pacman -S libxkbcommon-x11 (Arch) or sudo apt install libxkbcommon-x11-dev (Ubuntu)')

# XWayland support dependencies (required for X11 app compatibility)
xcb = dependency('xcb',
                required: true,
                not_found_message: 'libxcb is required for XWayland support. Install with: sudo pacman -S libxcb (Arch) or sudo apt install libxcb1-dev (Ubuntu)')
xcb_composite = dependency('xcb-composite',
                          required: true,
                          not_found_message: 'xcb-composite is required. Install with: sudo pacman -S xcb-util-wm (Arch) or sudo apt install libxcb-composite0-dev (Ubuntu)')
xcb_icccm = dependency('xcb-icccm',
                      required: true,
                      not_found_message: 'xcb-icccm is required. Install with: sudo pacman -S xcb-util-wm (Arch) or sudo apt install libxcb-icccm4-dev (Ubuntu)')
xcb_render = dependency('xcb-render',
                       required: true,
                       not_found_message: 'xcb-render is required. Install with: sudo pacman -S libxcb (Arch) or sudo apt install libxcb-render0-dev (Ubuntu)')
xcb_res = dependency('xcb-res',
                    required: true,
                    not_found_message: 'xcb-res is required. Install with: sudo pacman -S libxcb (Arch) or sudo apt install libxcb-res0-dev (Ubuntu)')
xcb_xfixes = dependency('xcb-xfixes',
                       required: true,
                       not_found_message: 'xcb-xfixes is required. Install with: sudo pacman -S libxcb (Arch) or sudo apt install libxcb-xfixes0-dev (Ubuntu)')

# Math library (universally available)
math_lib = cc.find_library('m', required: true)

# OpenGL/EGL dependencies (required for hardware acceleration)
egl = dependency('egl',
                required: true,
                not_found_message: 'EGL is required for hardware acceleration. Install with: sudo pacman -S mesa (Arch) or sudo apt install libegl-dev (Ubuntu)')
gl = dependency('gl',
               required: false,  # OpenGL is optional, we primarily use GLES
               not_found_message: 'OpenGL is optional but recommended. Install with: sudo pacman -S mesa (Arch) or sudo apt install libgl-dev (Ubuntu)')
gles3 = dependency('glesv2',
                  required: true,
                  not_found_message: 'OpenGL ES 2.0 is required for rendering. Install with: sudo pacman -S mesa (Arch) or sudo apt install libgles2-mesa-dev (Ubuntu)')

# Verify critical paths and programs
wayland_scanner = find_program('wayland-scanner',
                               required: true,
                               disabler: true,
                               native: true)
if not wayland_scanner.found()
  error('wayland-scanner is required but not found. Install wayland development packages.')
endif

message('All core dependencies found successfully!')

# Protocol generation
wayland_scanner = find_program('wayland-scanner')
wl_protocol_dir = wayland_protos.get_variable('pkgdatadir')
wlr_protocol_dir = wlr_protos.get_variable('pkgdatadir')

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

# Pre-generated protocol source files
protocol_sources = [
  'protocols/wayland-server-protocol.c',
  'protocols/xdg-shell-protocol.c',
  'protocols/linux-dmabuf-v1-protocol.c',
  'protocols/presentation-time-protocol.c',
  'protocols/viewporter-protocol.c',
  'protocols/tablet-v2-protocol.c',
  'protocols/xdg-activation-v1-protocol.c',
  'protocols/fractional-scale-v1-protocol.c',
  'protocols/single-pixel-buffer-v1-protocol.c',
  'protocols/cursor-shape-v1-protocol.c',
  'protocols/tearing-control-v1-protocol.c',
  'protocols/security-context-v1-protocol.c',
  'protocols/ext-session-lock-v1-protocol.c',
  'protocols/ext-foreign-toplevel-list-v1-protocol.c',
  'protocols/ext-idle-notify-v1-protocol.c',
  'protocols/ext-data-control-v1-protocol.c',
  'protocols/xdg-dialog-v1-protocol.c',
  'protocols/xdg-toplevel-drag-v1-protocol.c',
  'protocols/content-type-v1-protocol.c',
  'protocols/xdg-output-unstable-v1-protocol.c',
  'protocols/xdg-decoration-unstable-v1-protocol.c',
  'protocols/wlr-layer-shell-unstable-v1-protocol.c',
  'protocols/input-method-unstable-v1-protocol.c',
  'protocols/text-input-unstable-v3-protocol.c',
  'protocols/primary-selection-unstable-v1-protocol.c',
  'protocols/idle-inhibit-unstable-v1-protocol.c',
  'protocols/pointer-constraints-unstable-v1-protocol.c',
  'protocols/relative-pointer-unstable-v1-protocol.c',
  'protocols/pointer-gestures-unstable-v1-protocol.c',
  'protocols/keyboard-shortcuts-inhibit-unstable-v1-protocol.c',
  'protocols/xdg-foreign-unstable-v2-protocol.c',
  'protocols/input-timestamps-unstable-v1-protocol.c',
  'protocols/fullscreen-shell-unstable-v1-protocol.c',
  'protocols/linux-explicit-synchronization-unstable-v1-protocol.c',
  'protocols/xwayland-keyboard-grab-unstable-v1-protocol.c',
  'protocols/xwayland-shell-v1-protocol.c',
  'protocols/wlr-export-dmabuf-unstable-v1-protocol.c',
  'protocols/wlr-screencopy-unstable-v1-protocol.c',
  'protocols/wlr-gamma-control-unstable-v1-protocol.c',
  'protocols/wlr-output-power-management-unstable-v1-protocol.c',
  'protocols/wlr-virtual-pointer-unstable-v1-protocol.c',
]

# Optional protocols
if get_option('enable_image_capture')
  protocol_sources += 'protocols/ext-image-copy-capture-v1-protocol.c'
  add_project_arguments('-DAXIOM_IMAGE_CAPTURE_SUPPORT', language: 'c')
endif

# Include directories
inc = include_directories('include', 'protocols')

# Source Code
axiom_sources = files(
  'src/main.c',
  'src/compositor.c',    # Core compositor module
  'src/logging.c',
  'src/errors.c',        # New comprehensive error handling
  'src/memory.c',        # Advanced memory management system
  'src/environment.c',   # Environment management system
  'src/input.c',
  'src/cursor.c',
  'src/config.c',
  'src/process.c',
  'src/workspace.c',     # Using workspace.c instead of tiling.c
  'src/animation.c',
  'src/effects.c',
  'src/effects_realtime.c',
  'src/renderer.c',
  'src/window_rules.c',
  'src/smart_gaps.c',
  'src/window_snapping.c',
  'src/pip_manager.c',
  'src/thumbnail_manager.c',
  'src/xwayland.c',
  'src/enhanced_xwayland.c',  # Enhanced XWayland support
  # Enhanced systems
  'src/tagging.c',
  'src/keybindings.c',
  'src/focus.c',
  'src/window_manager.c',
  'src/advanced_tiling.c',
  # Desktop integration protocols
  'src/layer_shell.c',
  'src/screenshot.c',
  'src/session.c',
  'src/multi_session.c',
  'src/performance.c',
)

# Executable - no need for separate proto_inc since we use pre-generated files

global_deps = [
  wayland_server,
  wayland_protos,
  wlroots,
  xkbcommon,
  xkbcommon_x11,
  xcb,
  xcb_composite,
  xcb_icccm,
  xcb_render,
  xcb_res,
  xcb_xfixes,
  math_lib,
  egl,
  gles3,
]
executable('axiom',
  sources: axiom_sources + protocol_sources,
  dependencies: global_deps,
  include_directories: inc,
  install: true,
)

# Minimal nested compositor for testing
executable('minimal_nested',
  sources: 'minimal_nested.c',
  dependencies: global_deps,
  include_directories: inc,
  install: false,
)

# Install desktop integration files
install_data('axiom.desktop',
  install_dir: get_option('datadir') / 'wayland-sessions'
)

# Install session script
install_data('axiom-session',
  install_dir: get_option('bindir'),
  install_mode: 'rwxr-xr-x'
)

# Thumbnail manager test
test_thumbnail_manager_sources = files(
  'tests/test_thumbnail_manager.c',
  'src/thumbnail_manager.c',
  'src/logging.c'
)

test_thumbnail_manager = executable('test_thumbnail_manager',
    sources: test_thumbnail_manager_sources,
    dependencies: [wayland_server, wlroots, gles3, egl],
    include_directories: inc,
    install: false,
)
test('thumbnail_manager_tests', test_thumbnail_manager)

install_data('axiom.session',
  install_dir: get_option('datadir') / 'wayland-sessions'
)

# Install configuration examples
install_data('examples/axiom.conf',
  install_dir: get_option('sysconfdir') / 'axiom'
)

install_data('examples/rules.conf',
  install_dir: get_option('sysconfdir') / 'axiom'
)

# Install manual pages
install_man('man/axiom.1')

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

# Enhanced keybindings test - disabled due to missing test file
# TODO: Implement proper keybindings tests

# Enhanced input test - disabled due to missing dependencies
# TODO: Fix input test dependencies

test_config_sources = files(
  'tests/test_config.c',
  'src/logging.c',
  'src/memory.c'
)

test_config_executable = executable('test_config',
  sources: test_config_sources,
  dependencies: [math_lib],
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
# Skip tests that require additional dependencies for now
# test('keybindings_tests', test_keybindings)
# test('input_tests', test_input)
test('config_tests', test_config_executable)
test('animation_tests', test_animation_executable)

# Animation config test
test_animation_config_sources = files(
  'tests/test_animation_config.c',
  'src/config.c',
  'src/logging.c',
  'src/memory.c'
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
  'src/effects.c',
  'src/renderer.c',
  'src/logging.c'
)

test_effects = executable('test_effects',
    sources: test_effects_sources,
    dependencies: [math_lib, wayland_server, wlroots, gles3, egl, gl],
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

# PiP manager test
test_pip_manager_sources = files(
  'tests/test_pip_manager.c',
  'src/pip_manager.c',
  'src/logging.c'
)

test_pip_manager = executable('test_pip_manager',
    sources: test_pip_manager_sources,
    dependencies: [wayland_server, wlroots],
    include_directories: inc,
    install: false,
)
test('pip_manager_tests', test_pip_manager)

# Memory management test
test_memory_sources = files(
  'tests/test_memory.c',
  'src/memory.c',
  'src/logging.c'
)

test_memory = executable('test_memory',
    sources: test_memory_sources,
    dependencies: [math_lib],
    include_directories: inc,
    install: false,
)
test('memory_tests', test_memory)

# Protocol testing suite
test_protocols_sources = files(
  'tests/test_protocols.c'
)

test_protocols = executable('test_protocols',
    sources: test_protocols_sources,
    dependencies: [math_lib],
    include_directories: inc,
    install: false,
)
test('protocol_tests', test_protocols)

# Multi-session test
test_multi_session_sources = files(
  'tests/test_multi_session.c',
  'src/multi_session.c',
  'src/logging.c',
  'src/memory.c'
)

test_multi_session = executable('test_multi_session',
    sources: test_multi_session_sources,
    dependencies: [math_lib, wayland_server],
    include_directories: inc,
    install: false,
)
test('multi_session_tests', test_multi_session)

# ==============================================
# DEVELOPER TOOLS & STANDALONE TESTS
# ==============================================

# Standalone test executables (optional developer tools)
if get_option('build_dev_tools')
  # Minimal compositor for testing
  axiom_minimal = executable('axiom-minimal',
      'axiom-minimal.c',
      dependencies: global_deps,
      include_directories: inc,
      install: false,
  )
  
  # Animation demos
  easing_demo = executable('easing-demo',
      'easing_demo.c',
      dependencies: [math_lib],
      include_directories: inc,
      install: false,
  )
  
  simple_animation_demo = executable('simple-animation-demo',
      ['simple_animation_demo.c', 'src/animation.c', 'src/logging.c', 'src/window_manager.c', 'src/config.c', 'src/memory.c'],
      dependencies: global_deps,
      include_directories: inc,
      install: false,
  )
  
  # Rendering system tests
  test_renderer = executable('test-renderer',
      'test_renderer.c',
      dependencies: [math_lib],
      include_directories: inc,
      install: false,
  )
  
  test_rendering_pipeline = executable('test-rendering-pipeline',
      ['test_rendering_pipeline.c', 'test_shaders.c'],
      dependencies: global_deps,
      include_directories: inc,
      c_args: ['-DSHADER_LIBRARY_MODE'],
      install: false,
  )
  
  test_shaders = executable('test-shaders',
      'test_shaders.c',
      dependencies: [math_lib, gles3, egl],
      include_directories: inc,
      install: false,
  )
  
  # Window snapping tests (with required source dependencies)
  test_snapping = executable('test-snapping',
      ['test_snapping.c', 'src/window_snapping.c', 'src/config.c', 'src/logging.c', 'src/memory.c'],
      dependencies: global_deps,
      include_directories: inc,
      install: false,
  )
  
  test_snapping_simple = executable('test-snapping-simple',
      ['test_snapping_simple.c', 'src/window_snapping.c', 'src/config.c', 'src/logging.c', 'src/memory.c'],
      dependencies: [math_lib, wayland_server, wlroots],
      include_directories: inc,
      install: false,
  )
endif

# ==============================================
# BUILD SUMMARY
# ==============================================

summary({
    'Version': meson.project_version(),
    'Build type': get_option('buildtype'),
    'C standard': get_option('c_std'),
    'Install prefix': get_option('prefix'),
}, section: 'Axiom Wayland Compositor')

summary({
    'wlroots': wlroots.version(),
    'Wayland server': wayland_server.version(),
    'Wayland protocols': wayland_protos.version(),
    'OpenGL ES': gles3.found() ? 'yes' : 'no',
    'EGL': egl.found() ? 'yes' : 'no',
}, section: 'Core Dependencies')

summary({
    'Debug logging': get_option('enable_debug_logs') ? 'enabled' : 'disabled',
    'Performance monitoring': get_option('enable_performance_monitoring') ? 'enabled' : 'disabled',
    'Image capture protocol': get_option('enable_image_capture') ? 'enabled' : 'disabled',
    'Developer tools': get_option('build_dev_tools') ? 'enabled' : 'disabled',
}, section: 'Optional Features')

summary({
    'Main protocols': protocol_sources.length(),
    'Source files': axiom_sources.length(),
    'Test suites': '10 comprehensive tests',
    'Dev tools': get_option('build_dev_tools') ? '8 standalone tools' : 'disabled',
}, section: 'Project Statistics')
