# Axiom Compositor Keybinding Reference

## Current Status
- Compositor PID: Running with 3 test clients
- Test Clients: weston-terminal, weston-simple-egl, weston-flower
- Keybinding Manager: Initialized with default bindings

## Application Launchers
- **Super+Enter**: Launch terminal (foot)
- **Super+d**: Launch application launcher (rofi -show drun)

## Window Management
- **Super+w**: Close window (graceful close)
- **Super+k**: Kill window (force kill client)
- **Super+f**: Toggle fullscreen
- **Super+m**: Toggle maximize
- **Super+Space**: Toggle floating mode
- **Super+s**: Toggle sticky mode

## Layout Management
- **Super+l**: Cycle through layouts
- **Super+h**: Decrease master ratio
- **Super+j**: Increase master ratio

## Focus Management (Alt+Tab Style)
- **Alt+Tab**: Focus next window (forward cycling)
- **Alt+Shift+Tab**: Focus previous window (backward cycling)

## Tag/Workspace Management
### View Tags (Super+Number)
- **Super+1-9**: Switch to workspace/tag 1-9
- **Super+0**: View all tags
- **Super+Tab**: View previous tags

### Move Windows to Tags (Super+Shift+Number)
- **Super+Shift+1-9**: Move focused window to workspace/tag 1-9

### Advanced Tag Operations
- **Super+Ctrl+1-9**: Toggle view tag (show/hide tag)
- **Super+Ctrl+Shift+1-9**: Toggle window tag assignment

## System Control
- **Super+q**: Quit compositor
- **Super+r**: Reload configuration

## Test Results

### Window Focus Test (Alt+Tab)
- **Status**: READY FOR TESTING
- **Test Environment**: 3 active Wayland clients
- **Expected Behavior**: 
  - Alt+Tab should cycle through windows forward
  - Alt+Shift+Tab should cycle through windows backward
  - Focus cycling should be visual and immediate

### Window Management Test
- **Status**: READY FOR TESTING  
- **Test Environment**: Multiple windows available
- **Key Bindings to Test**:
  - Super+f (fullscreen toggle)
  - Super+m (maximize toggle)
  - Super+w (close window)
  - Super+Space (floating toggle)

### Tag/Workspace Test
- **Status**: READY FOR TESTING
- **Expected Behavior**:
  - Super+1-9 should switch workspaces
  - Super+Shift+1-9 should move windows between workspaces
  - Super+0 should show all workspaces

### Application Launcher Test
- **Status**: READY FOR TESTING
- **Dependencies**: foot terminal, rofi launcher
- **Expected Behavior**:
  - Super+Enter should launch foot terminal
  - Super+d should launch rofi application launcher

## Next Testing Steps

1. **Manual Alt+Tab Test**: 
   - Focus the compositor window
   - Press Alt+Tab to cycle through the 3 test clients
   - Verify visual feedback and focus changes

2. **Window Management Test**:
   - Test fullscreen toggle on each client
   - Test maximize/minimize functionality  
   - Test window closing

3. **Multi-Window Workflow Test**:
   - Create additional windows with Super+Enter
   - Test workspace switching
   - Test moving windows between workspaces

4. **Performance Test**:
   - Rapid Alt+Tab cycling
   - Multiple rapid keybinding presses
   - Memory usage monitoring during intensive use

## Implementation Notes

The keybinding system is fully implemented with:
- ✅ Default bindings loaded (30+ keybindings)
- ✅ Alt+Tab focus cycling with proper state management
- ✅ Window management actions (close, kill, fullscreen, maximize)
- ✅ Tag/workspace system with view and move operations
- ✅ Application launcher integration
- ✅ System control (quit, reload)
- ✅ Modifier key combinations (Super, Alt, Ctrl, Shift)

The next step is manual testing of these keybindings in the nested compositor environment.
