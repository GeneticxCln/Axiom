# Axiom Compositor - Protocol Implementation Status

## Overview

This document analyzes which Wayland protocols are included, implemented, and functional in the Axiom compositor.

## Protocol Categories

### ✅ Fully Implemented (with custom managers)

1. **XDG Shell** (`xdg-shell-protocol`)
   - ✅ Implemented: Basic XDG surface management
   - ✅ Manager: Built into main.c (`wlr_xdg_shell_create`)
   - ✅ Handlers: server_new_xdg_toplevel

2. **Layer Shell** (`wlr-layer-shell-unstable-v1-protocol`)
   - ✅ Implemented: `src/layer_shell.c`
   - ✅ Manager: `axiom_layer_shell_manager`
   - ✅ Features: Panel/bar support, proper layering

3. **Screenshot/Screen Capture** 
   - ✅ Implemented: `src/screenshot.c`
   - ✅ Manager: `axiom_screenshot_manager`
   - ✅ Protocols: `wlr-screencopy-unstable-v1`, `wlr-export-dmabuf-unstable-v1`
   - ✅ Features: Full screen capture, region capture

4. **Session Management**
   - ✅ Implemented: `src/session.c`
   - ✅ Manager: `axiom_session_manager`
   - ✅ Protocols: `ext-session-lock-v1`, `idle-inhibit-unstable-v1`, `ext-idle-notify-v1`
   - ✅ Features: Screen locking, idle detection, inhibitors

5. **XWayland Support**
   - ✅ Implemented: `src/xwayland.c`
   - ✅ Manager: `axiom_xwayland_manager`
   - ✅ Features: X11 application compatibility

### ✅ Core Protocols (built into wlroots/main)

6. **Compositor** (`wayland-server-protocol`)
   - ✅ Built-in: `wlr_compositor_create`

7. **Data Device Manager**
   - ✅ Built-in: `wlr_data_device_manager_create`

8. **Seat Management**
   - ✅ Built-in: `wlr_seat_create`

9. **Output Management**
   - ✅ Built-in: `wlr_output_layout_create`

### ✅ Included (protocols compiled but may need implementation)

10. **Linux DMA-BUF** (`linux-dmabuf-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

11. **Presentation Time** (`presentation-time-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

12. **Viewporter** (`viewporter-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

13. **Tablet Support** (`tablet-v2-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Needs verification

14. **XDG Activation** (`xdg-activation-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

15. **Fractional Scale** (`fractional-scale-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

16. **Single Pixel Buffer** (`single-pixel-buffer-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

17. **Cursor Shape** (`cursor-shape-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

18. **Tearing Control** (`tearing-control-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

19. **Security Context** (`security-context-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

20. **Foreign Toplevel List** (`ext-foreign-toplevel-list-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: May need custom handling

21. **Data Control** (`ext-data-control-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

22. **XDG Dialog** (`xdg-dialog-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

23. **XDG Toplevel Drag** (`xdg-toplevel-drag-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

24. **Content Type** (`content-type-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

25. **XDG Output** (`xdg-output-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

26. **XDG Decoration** (`xdg-decoration-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

27. **Input Method** (`input-method-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Needs verification

28. **Text Input** (`text-input-unstable-v3-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Needs verification

29. **Primary Selection** (`primary-selection-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

30. **Pointer Constraints** (`pointer-constraints-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

31. **Relative Pointer** (`relative-pointer-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

32. **Pointer Gestures** (`pointer-gestures-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

33. **Keyboard Shortcuts Inhibit** (`keyboard-shortcuts-inhibit-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

34. **XDG Foreign** (`xdg-foreign-unstable-v2-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

35. **Input Timestamps** (`input-timestamps-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

36. **Fullscreen Shell** (`fullscreen-shell-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

37. **Linux Explicit Sync** (`linux-explicit-synchronization-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

38. **XWayland Keyboard Grab** (`xwayland-keyboard-grab-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

39. **XWayland Shell** (`xwayland-shell-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

40. **Gamma Control** (`wlr-gamma-control-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

41. **Output Power Management** (`wlr-output-power-management-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

42. **Virtual Pointer** (`wlr-virtual-pointer-unstable-v1-protocol`)
    - ✅ Compiled: Yes
    - ❓ Implementation: Handled by wlroots

### ❌ Disabled/Missing

43. **Image Copy Capture** (`ext-image-copy-capture-v1-protocol`)
    - ❌ Status: Disabled due to incomplete interface definition
    - ❌ Issue: Missing `ext_image_capture_source_v1_interface`
    - ❌ Resolution needed: Obtain complete protocol specification

## Summary

- **Total Protocols Available**: 42
- **Fully Implemented with Custom Managers**: 5
- **Core Protocols (Built-in)**: 4
- **Included (May work via wlroots)**: 32
- **Disabled/Broken**: 1

## Recommendations

1. **High Priority**: 
   - Verify input method and text input protocols work correctly
   - Test tablet support functionality
   - Fix or remove ext-image-copy-capture-v1

2. **Medium Priority**:
   - Implement custom handling for ext-foreign-toplevel-list-v1 if needed
   - Add protocol-specific configuration options

3. **Low Priority**:
   - Add protocol status reporting/debugging tools
   - Document which protocols are essential vs optional

## Notes

Most protocols marked as "❓ Implementation: Handled by wlroots" should work automatically as wlroots provides the implementation. However, some may require compositor-specific event handling or configuration.
