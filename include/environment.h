#ifndef AXIOM_ENVIRONMENT_H
#define AXIOM_ENVIRONMENT_H

#include <stdbool.h>

/**
 * @file environment.h
 * @brief Environment setup and management for Axiom Wayland compositor
 * 
 * This module handles proper environment variable setup for:
 * - Wayland compositor operation
 * - XWayland integration
 * - Application compatibility
 * - Hardware acceleration
 * - Runtime directory management
 */

// ===== Core Environment Setup =====

/**
 * Set up essential environment variables for Wayland compositor operation
 * @return true if successful, false on error
 */
bool axiom_environment_setup_wayland(void);

/**
 * Set up application compatibility environment variables
 * @return true if successful, false on error
 */
bool axiom_environment_setup_application_support(void);

/**
 * Set up XWayland environment variables
 * @return true if successful, false on error
 */
bool axiom_environment_setup_xwayland(void);

/**
 * Set up hardware acceleration environment
 * @return true if successful, false on error
 */
bool axiom_environment_setup_hardware_acceleration(void);

/**
 * Set up cursor theme environment
 * @return true if successful, false on error
 */
bool axiom_environment_setup_cursor_theme(void);

/**
 * Set up Qt scaling environment
 * @return true if successful, false on error
 */
bool axiom_environment_setup_qt_scaling(void);

/**
 * Set up runtime directories for Axiom
 * @return true if successful, false on error
 */
bool axiom_environment_setup_runtime_dirs(void);

// ===== Complete Setup =====

/**
 * Complete environment setup for Axiom compositor
 * Calls all individual setup functions
 * @return true if all setup succeeded, false if any failed
 */
bool axiom_environment_setup(void);

// ===== Runtime Environment Management =====

/**
 * Check if running in nested mode (within another compositor)
 * @return true if nested, false if standalone
 */
bool axiom_environment_is_nested(void);

/**
 * Get environment information for debugging
 */
void axiom_environment_print_info(void);

/**
 * Set the DISPLAY environment variable when XWayland is ready
 * @param display_name X11 display name (e.g., ":0")
 */
void axiom_environment_set_display(const char *display_name);

/**
 * Set the WAYLAND_DISPLAY environment variable when Wayland is ready
 * @param socket_name Wayland socket name (e.g., "wayland-0")
 */
void axiom_environment_set_wayland_display(const char *socket_name);

#endif /* AXIOM_ENVIRONMENT_H */
