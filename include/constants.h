#ifndef AXIOM_CONSTANTS_H
#define AXIOM_CONSTANTS_H

/**
 * @file constants.h
 * @brief Named constants to replace hardcoded values throughout Axiom
 * 
 * This implements the roadmap Phase 1.1 requirement:
 * "Remove hardcoded values - Replace magic numbers with named constants"
 */

// =============================================================================
// WINDOW DIMENSIONS AND POSITIONING
// =============================================================================

/** Default window dimensions */
#define AXIOM_DEFAULT_WINDOW_WIDTH          800
#define AXIOM_DEFAULT_WINDOW_HEIGHT         600

/** Default window positioning */
#define AXIOM_DEFAULT_WINDOW_X              100
#define AXIOM_DEFAULT_WINDOW_Y              100

/** Minimum window dimensions */
#define AXIOM_MIN_WINDOW_WIDTH              200
#define AXIOM_MIN_WINDOW_HEIGHT             150

/** Maximum reasonable window dimensions */
#define AXIOM_MAX_WINDOW_WIDTH              7680  // 8K width
#define AXIOM_MAX_WINDOW_HEIGHT             4320  // 8K height

// =============================================================================
// WORKSPACE AND DISPLAY CONSTANTS
// =============================================================================

/** Default workspace dimensions (fallback) */
#define AXIOM_DEFAULT_WORKSPACE_WIDTH       1920
#define AXIOM_DEFAULT_WORKSPACE_HEIGHT      1080

/** Maximum number of workspaces */
#define AXIOM_MAX_WORKSPACES               10

/** Workspace switching constants */
#define AXIOM_WORKSPACE_SWITCH_DELAY_MS    100

// =============================================================================
// UI ELEMENT DIMENSIONS
// =============================================================================

/** Title bar dimensions */
#define AXIOM_TITLE_BAR_HEIGHT             24
#define AXIOM_TITLE_BAR_MIN_WIDTH          100

/** Border dimensions */
#define AXIOM_BORDER_WIDTH                 3
#define AXIOM_BORDER_WIDTH_FOCUSED         4
#define AXIOM_BORDER_WIDTH_UNFOCUSED       2

/** Corner rounding simulation */
#define AXIOM_CORNER_RADIUS_LARGE          2
#define AXIOM_CORNER_RADIUS_SMALL          1

/** Gap and spacing constants */
#define AXIOM_DEFAULT_GAP_SIZE             10
#define AXIOM_MIN_GAP_SIZE                 0
#define AXIOM_MAX_GAP_SIZE                 100

// =============================================================================
// ANIMATION AND TIMING CONSTANTS
// =============================================================================

/** Animation durations (milliseconds) */
#define AXIOM_ANIMATION_DURATION_FAST      150
#define AXIOM_ANIMATION_DURATION_NORMAL    200
#define AXIOM_ANIMATION_DURATION_SLOW      300

/** Specific animation durations for different window states */
#define AXIOM_ANIMATION_WINDOW_APPEAR_DURATION      300
#define AXIOM_ANIMATION_WINDOW_DISAPPEAR_DURATION   200
#define AXIOM_ANIMATION_WINDOW_MOVE_DURATION        250
#define AXIOM_ANIMATION_WINDOW_RESIZE_DURATION      200
#define AXIOM_ANIMATION_WORKSPACE_DURATION          400
#define AXIOM_ANIMATION_FOCUS_RING_DURATION         150
#define AXIOM_ANIMATION_LAYOUT_CHANGE_DURATION      300

/** Frame rate and timing */
#define AXIOM_TARGET_FPS                   60
#define AXIOM_FRAME_TIME_MS                16    // 1000/60
#define AXIOM_ANIMATION_THRESHOLD_MS       16

/** Animation easing constants */
#define AXIOM_EASING_FACTOR               0.8f
#define AXIOM_BOUNCE_FACTOR               0.2f

// =============================================================================
// MEMORY AND BUFFER SIZES
// =============================================================================

/** String buffer sizes */
#define AXIOM_BUFFER_SIZE_SMALL            256
#define AXIOM_BUFFER_SIZE_MEDIUM           512
#define AXIOM_BUFFER_SIZE_LARGE            1024
#define AXIOM_BUFFER_SIZE_XLARGE           2048

/** Command line buffer sizes */
#define AXIOM_COMMAND_BUFFER_SIZE          512
#define AXIOM_PATH_BUFFER_SIZE             1024

/** Configuration buffer sizes */
#define AXIOM_CONFIG_LINE_BUFFER           512
#define AXIOM_CONFIG_VALUE_BUFFER          256

// =============================================================================
// CURSOR AND INPUT CONSTANTS
// =============================================================================

/** Default cursor theme and size */
#define AXIOM_DEFAULT_CURSOR_THEME         "default"
#define AXIOM_DEFAULT_CURSOR_SIZE          24

/** Input timing constants */
#define AXIOM_KEY_REPEAT_DELAY_MS          500
#define AXIOM_KEY_REPEAT_RATE_MS           50

/** Mouse/touchpad constants */
#define AXIOM_DOUBLE_CLICK_TIME_MS         400
#define AXIOM_SCROLL_THRESHOLD             10

// =============================================================================
// THUMBNAIL AND PREVIEW CONSTANTS
// =============================================================================

/** Thumbnail dimensions */
#define AXIOM_THUMBNAIL_WIDTH              200
#define AXIOM_THUMBNAIL_HEIGHT             150
#define AXIOM_THUMBNAIL_MAX_WIDTH          400
#define AXIOM_THUMBNAIL_MAX_HEIGHT         300

/** Picture-in-Picture constants */
#define AXIOM_PIP_DEFAULT_WIDTH            320
#define AXIOM_PIP_DEFAULT_HEIGHT           240
#define AXIOM_PIP_MIN_WIDTH                160
#define AXIOM_PIP_MIN_HEIGHT               120

// =============================================================================
// EFFECTS AND GRAPHICS CONSTANTS
// =============================================================================

/** Shadow constants */
#define AXIOM_SHADOW_OFFSET_X              4
#define AXIOM_SHADOW_OFFSET_Y              4
#define AXIOM_SHADOW_BLUR_RADIUS           8
#define AXIOM_SHADOW_OPACITY               0.3f

/** Transparency and opacity */
#define AXIOM_OPACITY_TRANSPARENT          0.0f
#define AXIOM_OPACITY_TRANSLUCENT          0.7f
#define AXIOM_OPACITY_OPAQUE               1.0f

/** Color constants (RGBA format) */
#define AXIOM_COLOR_TRANSPARENT            {0.0f, 0.0f, 0.0f, 0.0f}
#define AXIOM_COLOR_BLACK                  {0.0f, 0.0f, 0.0f, 1.0f}
#define AXIOM_COLOR_WHITE                  {1.0f, 1.0f, 1.0f, 1.0f}
#define AXIOM_COLOR_BLUE                   {0.3f, 0.5f, 0.9f, 1.0f}
#define AXIOM_COLOR_BLUE_LIGHT             {0.4f, 0.6f, 1.0f, 1.0f}

/** Background colors */
#define AXIOM_BG_COLOR_DARK               {0.1f, 0.1f, 0.15f, 1.0f}
#define AXIOM_BG_COLOR_TITLE              {0.15f, 0.15f, 0.15f, 0.95f}
#define AXIOM_BG_COLOR_ACCENT             {0.25f, 0.35f, 0.55f, 0.8f}

// =============================================================================
// PERFORMANCE AND LIMITS
// =============================================================================

/** Process and resource limits */
#define AXIOM_MAX_PROCESSES                100
#define AXIOM_MAX_WINDOWS_PER_WORKSPACE    50
#define AXIOM_MAX_EFFECTS_PER_WINDOW       10

/** Performance thresholds */
#define AXIOM_PERFORMANCE_WARNING_FPS      30
#define AXIOM_MEMORY_WARNING_MB            500
#define AXIOM_CPU_WARNING_PERCENT          80

// =============================================================================
// FILE AND PATH CONSTANTS
// =============================================================================

/** Configuration file names */
#define AXIOM_CONFIG_FILE                  "axiom.conf"
#define AXIOM_RULES_CONFIG_FILE           "rules.conf"
#define AXIOM_SESSION_FILE                "axiom.session"

/** Default configuration paths */
#define AXIOM_CONFIG_DIR                  "axiom"
#define AXIOM_SYSTEM_CONFIG_DIR           "/etc/axiom"
#define AXIOM_USER_CONFIG_DIR             "~/.config/axiom"

/** Log file constants */
#define AXIOM_LOG_FILE_MAX_SIZE_MB        10
#define AXIOM_LOG_FILE_MAX_COUNT          5

// =============================================================================
// NETWORK AND IPC CONSTANTS
// =============================================================================

/** Wayland protocol constants */
#define AXIOM_WAYLAND_DISPLAY_NAME        "wayland-0"
#define AXIOM_XDG_SHELL_VERSION           3
#define AXIOM_COMPOSITOR_VERSION          5

/** Seat and input constants */
#define AXIOM_DEFAULT_SEAT_NAME           "seat0"
#define AXIOM_MAX_INPUT_DEVICES           20

// =============================================================================
// ERROR AND RETRY CONSTANTS
// =============================================================================

/** Retry attempts for various operations */
#define AXIOM_MAX_RETRY_ATTEMPTS          3
#define AXIOM_RETRY_DELAY_MS              100

/** Timeout constants */
#define AXIOM_STARTUP_TIMEOUT_MS          5000
#define AXIOM_SHUTDOWN_TIMEOUT_MS         3000
#define AXIOM_PROCESS_TIMEOUT_MS          1000

// =============================================================================
// LAYOUT AND TILING CONSTANTS
// =============================================================================

/** Master-stack layout constants */
#define AXIOM_MASTER_RATIO_DEFAULT        0.6f
#define AXIOM_MASTER_RATIO_MIN            0.1f
#define AXIOM_MASTER_RATIO_MAX            0.9f
#define AXIOM_MASTER_RATIO_STEP           0.05f

/** Grid layout constants */
#define AXIOM_GRID_MIN_COLUMNS            1
#define AXIOM_GRID_MAX_COLUMNS            10
#define AXIOM_GRID_DEFAULT_COLUMNS        3

/** Spiral layout constants */
#define AXIOM_SPIRAL_MAIN_RATIO           0.7f
#define AXIOM_SPIRAL_SECONDARY_RATIO      0.3f

// =============================================================================
// DEBUGGING AND DEVELOPMENT CONSTANTS
// =============================================================================

/** Debug output limits */
#define AXIOM_DEBUG_MAX_STRING_LENGTH     512
#define AXIOM_DEBUG_MAX_STACK_DEPTH       20

/** Test constants */
#define AXIOM_TEST_TIMEOUT_MS             1000
#define AXIOM_TEST_MAX_ITERATIONS         100

// =============================================================================
// CONFIGURATION CONSTANTS
// =============================================================================

/** Configuration parsing limits */
#define AXIOM_CONFIG_MAX_LINE_LENGTH      512
#define AXIOM_CONFIG_MAX_SECTION_LENGTH   64

/** Input and cursor defaults */
#define AXIOM_DEFAULT_CURSOR_SIZE         24
#define AXIOM_DEFAULT_REPEAT_RATE         25
#define AXIOM_DEFAULT_REPEAT_DELAY        600

// =============================================================================
// UTILITY MACROS
// =============================================================================

/** Common calculations */
#define AXIOM_CLAMP(value, min, max)      ((value) < (min) ? (min) : ((value) > (max) ? (max) : (value)))
#define AXIOM_MIN(a, b)                   ((a) < (b) ? (a) : (b))
#define AXIOM_MAX(a, b)                   ((a) > (b) ? (a) : (b))

/** Pixel to logical unit conversions */
#define AXIOM_PIXELS_TO_LOGICAL(pixels, scale)  ((int)((pixels) / (scale)))
#define AXIOM_LOGICAL_TO_PIXELS(logical, scale) ((int)((logical) * (scale)))

/** Time conversions */
#define AXIOM_SECONDS_TO_MS(seconds)      ((seconds) * 1000)
#define AXIOM_MS_TO_SECONDS(ms)           ((ms) / 1000.0f)

#endif /* AXIOM_CONSTANTS_H */
