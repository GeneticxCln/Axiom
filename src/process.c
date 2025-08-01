#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "axiom.h"

struct axiom_process {
    struct wl_list link;
    pid_t pid;
    char *command;
    bool autostart;
    struct wl_event_source *sigchld_source;
};

static struct wl_list processes;
static bool processes_initialized = false;

static int handle_sigchld(int signal, void *data) {
    (void)signal;
    struct axiom_server *server = data;
    
    pid_t pid;
    int status;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        AXIOM_LOG_INFO("Process %d exited with status %d", pid, status);
        
        // Find and remove the process from our list
        struct axiom_process *process, *tmp;
        wl_list_for_each_safe(process, tmp, &processes, link) {
            if (process->pid == pid) {
                AXIOM_LOG_INFO("Cleaning up process: %s", process->command);
                wl_list_remove(&process->link);
                free(process->command);
                free(process);
                break;
            }
        }
    }
    
    return 0;
}

void axiom_process_init(struct axiom_server *server) {
    if (processes_initialized) {
        return;
    }
    
    wl_list_init(&processes);
    
    // Set up SIGCHLD handler to clean up finished processes
    struct wl_event_loop *loop = wl_display_get_event_loop(server->wl_display);
    wl_event_loop_add_signal(loop, SIGCHLD, handle_sigchld, server);
    
    processes_initialized = true;
    AXIOM_LOG_INFO("Process management initialized");
}

void axiom_process_cleanup(void) {
    if (!processes_initialized) {
        return;
    }
    
    struct axiom_process *process, *tmp;
    wl_list_for_each_safe(process, tmp, &processes, link) {
        AXIOM_LOG_INFO("Terminating process: %s (PID: %d)", process->command, process->pid);
        kill(process->pid, SIGTERM);
        wl_list_remove(&process->link);
        free(process->command);
        free(process);
    }
    
    processes_initialized = false;
}

pid_t axiom_spawn_process(const char *command, bool autostart) {
    if (!command) {
        AXIOM_LOG_ERROR("Cannot spawn process: command is NULL");
        return -1;
    }
    
    pid_t pid = fork();
    
    if (pid < 0) {
        AXIOM_LOG_ERROR("Failed to fork process: %s", strerror(errno));
        return -1;
    }
    
    if (pid == 0) {
        // Child process
        
        // Set up environment for Wayland applications
        // WAYLAND_DISPLAY should already be set by the main process
        
        // Detach from controlling terminal
        setsid();
        
        // Execute the command using shell
        execl("/bin/sh", "sh", "-c", command, NULL);
        
        // If we get here, exec failed
        AXIOM_LOG_ERROR("Failed to execute command: %s", command);
        exit(EXIT_FAILURE);
    }
    
    // Parent process
    struct axiom_process *process = calloc(1, sizeof(struct axiom_process));
    if (!process) {
        AXIOM_LOG_ERROR("Failed to allocate memory for process");
        kill(pid, SIGTERM);
        return -1;
    }
    
    process->pid = pid;
    process->command = strdup(command);
    process->autostart = autostart;
    
    wl_list_insert(&processes, &process->link);
    
    AXIOM_LOG_INFO("Spawned process: %s (PID: %d)", command, pid);
    return pid;
}

void axiom_spawn_terminal(void) {
    // Try different terminals in order of preference
    const char *terminals[] = {
        "alacritty",
        "kitty", 
        "foot",
        "wezterm",
        "gnome-terminal",
        "xterm",
        NULL
    };
    
    for (int i = 0; terminals[i]; i++) {
        // Check if terminal exists
        char check_cmd[256];
        snprintf(check_cmd, sizeof(check_cmd), "which %s > /dev/null 2>&1", terminals[i]);
        
        if (system(check_cmd) == 0) {
            AXIOM_LOG_INFO("Launching terminal: %s", terminals[i]);
            axiom_spawn_process(terminals[i], false);
            return;
        }
    }
    
    AXIOM_LOG_ERROR("No suitable terminal found");
}

void axiom_spawn_rofi(void) {
    // Use rofi-wayland for application launching
    const char *rofi_cmd = "rofi -show drun -show-icons";
    
    // Check if rofi is available
    if (system("which rofi > /dev/null 2>&1") != 0) {
        AXIOM_LOG_ERROR("rofi not found in PATH");
        return;
    }
    
    AXIOM_LOG_INFO("Launching rofi application launcher");
    axiom_spawn_process(rofi_cmd, false);
}

void axiom_spawn_waybar(struct axiom_server *server) {
    // Check if waybar is already running
    if (system("pgrep waybar > /dev/null 2>&1") == 0) {
        AXIOM_LOG_INFO("Waybar is already running");
        return;
    }
    
    // Check if waybar is available
    if (system("which waybar > /dev/null 2>&1") != 0) {
        AXIOM_LOG_ERROR("waybar not found in PATH");
        return;
    }
    
    AXIOM_LOG_INFO("Starting waybar");
    axiom_spawn_process("waybar", true);
}

void axiom_kill_waybar(void) {
    struct axiom_process *process;
    wl_list_for_each(process, &processes, link) {
        if (strstr(process->command, "waybar")) {
            AXIOM_LOG_INFO("Terminating waybar (PID: %d)", process->pid);
            kill(process->pid, SIGTERM);
            return;
        }
    }
    
    // If not found in our process list, try system-wide
    system("pkill waybar");
}

bool axiom_process_exists(const char *name) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pgrep %s > /dev/null 2>&1", name);
    return system(cmd) == 0;
}
