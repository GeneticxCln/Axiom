#ifndef AXIOM_COMPOSITOR_H
#define AXIOM_COMPOSITOR_H

#include "axiom.h"

/**
 * @file compositor.h
 * @brief Clean compositor initialization and management
 * 
 * Following Hyprland's pattern of separating main() from compositor initialization
 */

/**
 * Initialize the Axiom compositor with all subsystems
 * @param server The server structure to initialize
 * @param nested Whether running in nested mode
 * @return true on success, false on failure
 */
bool axiom_compositor_init(struct axiom_server *server, bool nested);

/**
 * Start the compositor main loop
 * @param server The initialized server
 */
void axiom_compositor_run(struct axiom_server *server);

/**
 * Clean shutdown of all compositor subsystems
 * @param server The server to cleanup
 */
void axiom_compositor_cleanup(struct axiom_server *server);

/**
 * Reload compositor configuration
 * @param server The server instance
 */
void axiom_compositor_reload_config(struct axiom_server *server);

#endif /* AXIOM_COMPOSITOR_H */
