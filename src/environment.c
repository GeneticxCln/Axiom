#include "environment.h"
#include "logging.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

/**
 * Set up essential environment variables for Wayland compositor operation
 */
bool axiom_environment_setup_wayland(void) {
    AXIOM_LOG_INFO("ENV", "Setting up Wayland environment variables");
    
    // Core session type
    if (setenv("XDG_SESSION_TYPE", "wayland", 1) != 0) {
        AXIOM_LOG_ERROR("ENV", "Failed to set XDG_SESSION_TYPE");
        return false;
    }
    
    if (setenv("XDG_SESSION_DESKTOP", "axiom", 1) != 0) {
        AXIOM_LOG_ERROR("ENV", "Failed to set XDG_SESSION_DESKTOP");
        return false;
    }
    
    if (setenv("XDG_CURRENT_DESKTOP", "Axiom", 1) != 0) {
        AXIOM_LOG_ERROR("ENV", "Failed to set XDG_CURRENT_DESKTOP");
        return false;
    }
    
    if (setenv("DESKTOP_SESSION", "axiom", 1) != 0) {
        AXIOM_LOG_ERROR("ENV", "Failed to set DESKTOP_SESSION");
        return false;
    }
    
    AXIOM_LOG_DEBUG("ENV", "Core Wayland session variables set");
    return true;
}

/**
 * Set up application compatibility environment variables
 */
bool axiom_environment_setup_application_support(void) {
    AXIOM_LOG_INFO("ENV", "Setting up application compatibility environment");
    
    // Firefox/Mozilla Wayland support
    if (setenv("MOZ_ENABLE_WAYLAND", "1", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set MOZ_ENABLE_WAYLAND");
    }
    
    // Qt Wayland support
    if (setenv("QT_QPA_PLATFORM", "wayland;xcb", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set QT_QPA_PLATFORM");
    }
    
    if (setenv("QT_WAYLAND_DISABLE_WINDOWDECORATION", "1", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set QT_WAYLAND_DISABLE_WINDOWDECORATION");
    }
    
    // GTK/GDK Wayland support
    if (setenv("GDK_BACKEND", "wayland,x11", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set GDK_BACKEND");
    }
    
    // SDL Wayland support  
    if (setenv("SDL_VIDEODRIVER", "wayland", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set SDL_VIDEODRIVER");
    }
    
    // Clutter Wayland support
    if (setenv("CLUTTER_BACKEND", "wayland", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set CLUTTER_BACKEND");
    }
    
    // Java AWT compatibility
    if (setenv("_JAVA_AWT_WM_NONREPARENTING", "1", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set _JAVA_AWT_WM_NONREPARENTING");
    }
    
    AXIOM_LOG_DEBUG("ENV", "Application compatibility variables set");
    return true;
}

/**
 * Set up XWayland environment variables
 */
bool axiom_environment_setup_xwayland(void) {
    AXIOM_LOG_INFO("ENV", "Setting up XWayland environment");
    
    // X11 compatibility variables - these will be updated when XWayland starts
    const char *xauth = getenv("XAUTHORITY");
    if (!xauth) {
        char xauth_path[256];
        snprintf(xauth_path, sizeof(xauth_path), "%s/.Xauthority", getenv("HOME"));
        if (setenv("XAUTHORITY", xauth_path, 1) != 0) {
            AXIOM_LOG_WARN("ENV", "Failed to set XAUTHORITY");
        }
    }
    
    // Font configuration for X11 apps
    if (setenv("FONTCONFIG_PATH", "/etc/fonts:/usr/share/fonts", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set FONTCONFIG_PATH");
    }
    
    AXIOM_LOG_DEBUG("ENV", "XWayland environment prepared");
    return true;
}

/**
 * Set up hardware acceleration environment
 */
bool axiom_environment_setup_hardware_acceleration(void) {
    AXIOM_LOG_INFO("ENV", "Setting up hardware acceleration environment");
    
    // VAAPI hardware acceleration
    if (setenv("LIBVA_DRIVER_NAME", "auto", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set LIBVA_DRIVER_NAME");
    }
    
    // VDPAU hardware acceleration
    if (setenv("VDPAU_DRIVER", "auto", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set VDPAU_DRIVER");
    }
    
    AXIOM_LOG_DEBUG("ENV", "Hardware acceleration environment set");
    return true;
}

/**
 * Set up cursor theme environment
 */
bool axiom_environment_setup_cursor_theme(void) {
    AXIOM_LOG_INFO("ENV", "Setting up cursor theme environment");
    
    // Set default cursor theme if not already set
    const char *cursor_theme = getenv("XCURSOR_THEME");
    if (!cursor_theme) {
        if (setenv("XCURSOR_THEME", "default", 1) != 0) {
            AXIOM_LOG_WARN("ENV", "Failed to set XCURSOR_THEME");
        }
    }
    
    // Set default cursor size if not already set
    const char *cursor_size = getenv("XCURSOR_SIZE");
    if (!cursor_size) {
        if (setenv("XCURSOR_SIZE", "24", 1) != 0) {
            AXIOM_LOG_WARN("ENV", "Failed to set XCURSOR_SIZE");
        }
    }
    
    AXIOM_LOG_DEBUG("ENV", "Cursor theme: %s, size: %s", 
                    getenv("XCURSOR_THEME"), getenv("XCURSOR_SIZE"));
    return true;
}

/**
 * Set up Qt scaling environment
 */
bool axiom_environment_setup_qt_scaling(void) {
    AXIOM_LOG_INFO("ENV", "Setting up Qt scaling environment");
    
    // Qt scaling settings
    if (setenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set QT_AUTO_SCREEN_SCALE_FACTOR");
    }
    
    if (setenv("QT_SCALE_FACTOR", "1", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set QT_SCALE_FACTOR");
    }
    
    if (setenv("QT_FONT_DPI", "96", 0) != 0) {
        AXIOM_LOG_WARN("ENV", "Failed to set QT_FONT_DPI");
    }
    
    AXIOM_LOG_DEBUG("ENV", "Qt scaling environment set");
    return true;
}

/**
 * Set up runtime directories for Axiom
 */
bool axiom_environment_setup_runtime_dirs(void) {
    AXIOM_LOG_INFO("ENV", "Setting up Axiom runtime directories");
    
    const char *home = getenv("HOME");
    const char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");
    
    if (!home) {
        AXIOM_LOG_ERROR("ENV", "HOME environment variable not set");
        return false;
    }
    
    // Create runtime directory structure
    char path[512];
    
    // ~/.local/share/axiom
    snprintf(path, sizeof(path), "%s/.local/share/axiom", home);
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        AXIOM_LOG_WARN("ENV", "Failed to create %s", path);
    }
    setenv("AXIOM_HOME", path, 1);
    
    // ~/.config/axiom
    snprintf(path, sizeof(path), "%s/.config/axiom", home);
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        AXIOM_LOG_WARN("ENV", "Failed to create %s", path);
    }
    setenv("AXIOM_CONFIG_DIR", path, 1);
    
    // ~/.cache/axiom
    snprintf(path, sizeof(path), "%s/.cache/axiom", home);
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        AXIOM_LOG_WARN("ENV", "Failed to create %s", path);
    }
    setenv("AXIOM_CACHE_DIR", path, 1);
    
    // ~/.cache/axiom/logs
    snprintf(path, sizeof(path), "%s/.cache/axiom/logs", home);
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        AXIOM_LOG_WARN("ENV", "Failed to create %s", path);
    }
    
    // Runtime directory
    if (xdg_runtime_dir) {
        snprintf(path, sizeof(path), "%s/axiom-%s", xdg_runtime_dir, getenv("USER"));
    } else {
        snprintf(path, sizeof(path), "/tmp/axiom-%s", getenv("USER"));
    }
    if (mkdir(path, 0700) != 0 && errno != EEXIST) {
        AXIOM_LOG_WARN("ENV", "Failed to create runtime directory %s", path);
    }
    setenv("AXIOM_RUNTIME_DIR", path, 1);
    
    AXIOM_LOG_DEBUG("ENV", "Runtime directories created and set");
    return true;
}

/**
 * Complete environment setup for Axiom compositor
 */
bool axiom_environment_setup(void) {
    AXIOM_LOG_INFO("ENV", "Setting up complete Axiom environment");
    
    bool success = true;
    
    // Set up all environment components
    success &= axiom_environment_setup_runtime_dirs();
    success &= axiom_environment_setup_wayland();
    success &= axiom_environment_setup_application_support();
    success &= axiom_environment_setup_xwayland();
    success &= axiom_environment_setup_hardware_acceleration();
    success &= axiom_environment_setup_cursor_theme();
    success &= axiom_environment_setup_qt_scaling();
    
    if (success) {
        AXIOM_LOG_INFO("ENV", "Axiom environment setup completed successfully");
    } else {
        AXIOM_LOG_WARN("ENV", "Axiom environment setup completed with warnings");
    }
    
    return success;
}

/**
 * Check if running in nested mode (within another compositor)
 */
bool axiom_environment_is_nested(void) {
    return (getenv("WAYLAND_DISPLAY") != NULL) || (getenv("DISPLAY") != NULL);
}

/**
 * Get environment information for debugging
 */
void axiom_environment_print_info(void) {
    AXIOM_LOG_INFO("ENV", "Environment Information:");
    AXIOM_LOG_INFO("ENV", "  XDG_SESSION_TYPE: %s", getenv("XDG_SESSION_TYPE") ?: "(unset)");
    AXIOM_LOG_INFO("ENV", "  XDG_CURRENT_DESKTOP: %s", getenv("XDG_CURRENT_DESKTOP") ?: "(unset)");
    AXIOM_LOG_INFO("ENV", "  WAYLAND_DISPLAY: %s", getenv("WAYLAND_DISPLAY") ?: "(unset)");
    AXIOM_LOG_INFO("ENV", "  DISPLAY: %s", getenv("DISPLAY") ?: "(unset)");
    AXIOM_LOG_INFO("ENV", "  MOZ_ENABLE_WAYLAND: %s", getenv("MOZ_ENABLE_WAYLAND") ?: "(unset)");
    AXIOM_LOG_INFO("ENV", "  QT_QPA_PLATFORM: %s", getenv("QT_QPA_PLATFORM") ?: "(unset)");
    AXIOM_LOG_INFO("ENV", "  GDK_BACKEND: %s", getenv("GDK_BACKEND") ?: "(unset)");
    AXIOM_LOG_INFO("ENV", "  XCURSOR_THEME: %s", getenv("XCURSOR_THEME") ?: "(unset)");
    AXIOM_LOG_INFO("ENV", "  XCURSOR_SIZE: %s", getenv("XCURSOR_SIZE") ?: "(unset)");
    AXIOM_LOG_INFO("ENV", "  AXIOM_CONFIG_DIR: %s", getenv("AXIOM_CONFIG_DIR") ?: "(unset)");
    AXIOM_LOG_INFO("ENV", "  AXIOM_RUNTIME_DIR: %s", getenv("AXIOM_RUNTIME_DIR") ?: "(unset)");
}

/**
 * Set the DISPLAY environment variable when XWayland is ready
 */
void axiom_environment_set_display(const char *display_name) {
    if (!display_name) {
        AXIOM_LOG_ERROR("ENV", "Invalid display name provided");
        return;
    }
    
    if (setenv("DISPLAY", display_name, 1) != 0) {
        AXIOM_LOG_ERROR("ENV", "Failed to set DISPLAY environment variable");
        return;
    }
    
    AXIOM_LOG_INFO("ENV", "DISPLAY set to: %s", display_name);
}

/**
 * Set the WAYLAND_DISPLAY environment variable when Wayland is ready
 */
void axiom_environment_set_wayland_display(const char *socket_name) {
    if (!socket_name) {
        AXIOM_LOG_ERROR("ENV", "Invalid socket name provided");
        return;
    }
    
    if (setenv("WAYLAND_DISPLAY", socket_name, 1) != 0) {
        AXIOM_LOG_ERROR("ENV", "Failed to set WAYLAND_DISPLAY environment variable");
        return;
    }
    
    AXIOM_LOG_INFO("ENV", "WAYLAND_DISPLAY set to: %s", socket_name);
}
