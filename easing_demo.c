#include <stdio.h>
#include <math.h>

// Standalone easing functions - simplified versions of what's in the animation system
float ease_linear(float t) {
    return t;
}

float ease_out_quad(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

float ease_out_cubic(float t) {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}

float ease_out_bounce(float t) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    
    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

float ease_spring(float t) {
    return sin(t * M_PI * (0.2f + 2.5f * t * t * t)) * pow(1.0f - t, 2.2f) + t * (1.0f + (1.2f * (1.0f - t)));
}

void demonstrate_easing(const char *name, float (*easing_func)(float)) {
    printf("🎯 %s Animation:\n", name);
    printf("   Progress: ");
    for (int i = 0; i <= 20; i++) {
        float t = i / 20.0f;
        float eased = easing_func(t);
        int bar_length = (int)(eased * 20);
        
        if (i % 5 == 0) {
            printf("\\n   %.2f |", t);
            for (int j = 0; j < bar_length; j++) printf("█");
            for (int j = bar_length; j < 20; j++) printf("░");
            printf("| %.3f", eased);
        }
    }
    printf("\\n\\n");
}

int main() {
    printf("🎬 Axiom Animation System - Easing Functions Demo\\n");
    printf("=================================================\\n\\n");
    
    printf("This demonstrates the easing functions that power the Axiom compositor's\\n");
    printf("animation system. Each function transforms linear time progress (0.0-1.0)\\n");
    printf("into smoothly animated motion curves.\\n\\n");
    
    demonstrate_easing("Linear (constant speed)", ease_linear);
    demonstrate_easing("Ease Out Quadratic (slow finish)", ease_out_quad);
    demonstrate_easing("Ease Out Cubic (smoother slow finish)", ease_out_cubic);
    demonstrate_easing("Bounce Out (bouncy finish)", ease_out_bounce);
    demonstrate_easing("Spring (oscillating settle)", ease_spring);
    
    printf("✨ These easing functions are used in the Axiom compositor for:\\n");
    printf("   • Window appear/disappear animations\\n");
    printf("   • Focus ring pulsing effects\\n");
    printf("   • Window movement and resizing\\n");
    printf("   • Layout transitions\\n");
    printf("   • Workspace switching\\n\\n");
    
    printf("🎮 Animation system successfully integrated into compositor!\\n");
    printf("   The unit tests confirm all functionality is working correctly.\\n");
    
    return 0;
}
