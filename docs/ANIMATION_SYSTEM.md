# Axiom Animation System

## Overview

The Axiom Animation System provides smooth, configurable animations for window management operations in the Axiom Wayland compositor. It supports various animation types, easing functions, and provides both low-level and high-level APIs for integration.

## Features

### Animation Types
- **Window Appear**: Fade in new windows
- **Window Disappear**: Fade out closing windows  
- **Window Move**: Smooth window position transitions
- **Window Resize**: Smooth window size transitions
- **Layout Change**: Animate tiling layout changes
- **Workspace Switch**: Animate workspace transitions
- **Focus Ring**: Animate focus indicators
- **Fade**: Generic opacity animations
- **Slide**: Generic position animations
- **Scale**: Generic scaling animations

### Easing Functions
- **Linear**: Constant speed
- **Quadratic**: In, Out, In-Out variants
- **Cubic**: In, Out, In-Out variants
- **Bounce**: In, Out variants  
- **Spring**: Physics-based easing

### Animation States
- **Stopped**: Not running
- **Running**: Currently animating
- **Paused**: Temporarily stopped
- **Finished**: Completed successfully

## Architecture

### Core Components

1. **Animation Manager** (`axiom_animation_manager`)
   - Manages all active animations
   - Updates animation progress each frame
   - Tracks performance metrics
   - Handles global settings

2. **Animation Structure** (`axiom_animation`)
   - Individual animation instance
   - Contains timing, easing, and callback information
   - Tracks start/end/current values for properties

3. **Configuration** (`axiom_animation_config`)
   - Per-animation-type settings
   - Duration, easing defaults
   - Enable/disable flags

## API Usage

### Basic Usage

```c
// Initialize animation system
axiom_animation_manager_init(server);

// Create an animation
struct axiom_animation *anim = axiom_animation_create(AXIOM_ANIM_WINDOW_APPEAR, 300);

// Set callbacks
anim->on_update = my_update_callback;
anim->on_complete = my_complete_callback;
anim->user_data = my_data;

// Start the animation
axiom_animation_start(server->animation_manager, anim);
```

### Callback Functions

```c
void my_update_callback(struct axiom_animation *anim, void *user_data) {
    // Called each frame while animation is running
    float progress = anim->progress; // 0.0 to 1.0
    
    // Apply animation to your objects here
    // e.g., update window opacity, position, etc.
}

void my_complete_callback(struct axiom_animation *anim, void *user_data) {
    // Called when animation completes
    // Cleanup or trigger next animation here
}
```

### Integration Points

The animation system integrates with:
- **Window Management**: Animate window operations
- **Workspace System**: Animate workspace transitions  
- **Focus System**: Animate focus changes
- **Tiling System**: Animate layout changes

## Implementation Details

### Files
- `include/animation.h` - Header with structs and function declarations
- `src/animation.c` - Core implementation
- `tests/test_animation.c` - Unit tests
- `tests/animation_test_impl.c` - Standalone test implementation

### Dependencies
- Wayland server libraries for list management
- Math library for easing calculations
- Standard C libraries

### Performance
- Efficient linked list management
- Minimal memory allocations
- Frame rate tracking
- Configurable global speed multiplier

## Testing

The system includes comprehensive unit tests covering:
- Animation manager lifecycle
- Animation creation/destruction
- Easing function correctness
- Callback execution
- State transitions

Run tests with:
```bash
meson test -C builddir animation_tests
```

## Future Enhancements

Planned improvements include:
- Additional easing functions (elastic, back, etc.)
- Chain/sequence animation support
- Timeline-based animations
- Property-specific animation tracks
- Hardware acceleration integration
- Animation recording/playback

## Configuration

Animation behavior can be configured through:
- Duration settings per animation type
- Default easing functions
- Enable/disable flags for animation categories
- Global speed multiplier
- Debug mode for development

The animation system is designed to be lightweight, efficient, and easily extensible for future compositor features.
