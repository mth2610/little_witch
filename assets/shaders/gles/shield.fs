#version 100
precision highp float;

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform float time;
uniform vec4 glowColor; // Base color of the shield (e.g. Cyan/Skyblue)
uniform vec2 center;    // Center of shield in virtual screen coordinates
uniform float radius;   // Radius of shield in pixels
uniform sampler2D screenTexture; // Background screen copy

// Simple pseudo-random noise/shimmer function
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main() {
    // Convert gl_FragCoord (bottom-left origin) to top-left virtual screen space (1280x720)
    vec2 pixelPos = vec2(gl_FragCoord.x, 720.0 - gl_FragCoord.y);
    float dist = distance(pixelPos, center);

    // Hard circular boundary at radius
    if (dist > radius) {
        discard;
    }

    // Normalized radius from 0 to 1
    float r = dist / radius;

    // Rim lighting (Fresnel-like effect)
    float rim = pow(r, 4.0);

    // Calculate angle from center
    float angle = atan(pixelPos.y - center.y, pixelPos.x - center.x);

    // Swirling plasma ripples
    float ripple1 = sin(r * 12.0 - time * 6.5 + angle * 3.0) * 0.5 + 0.5;
    float ripple2 = sin(r * 8.0 + time * 3.5 - angle * 2.0) * 0.5 + 0.5;
    float ripple = mix(ripple1, ripple2, 0.45);
    float rippleGlow = ripple * smoothstep(0.2, 1.0, r) * 0.55;

    // Fast shimmering plasma noise
    float shimmer = hash(pixelPos * 0.05 + vec2(0.0, time * 1.5)) * 0.1;

    // Combine intensities
    float intensity = rim * 1.8 + rippleGlow + 0.08 + shimmer * r;

    // Colors
    vec4 baseColor = glowColor;
    vec4 rimColor = vec4(baseColor.r * 0.5, baseColor.g * 0.8, 1.0, 1.0);
    vec4 finalGlow = mix(baseColor, rimColor, rim);

    // Shield alpha (glow factor)
    float alpha = rim * 0.75 + rippleGlow * 0.35 + 0.08;
    alpha = clamp(alpha, 0.0, 0.9);

    // Space distortion logic
    vec2 screenUV = gl_FragCoord.xy / vec2(1280.0, 720.0);
    vec2 dir = normalize(pixelPos - center);
    
    // Distort using a lens refraction + plasma ripple offset
    float distortStrength = (rim * 0.018 + rippleGlow * 0.015) * (1.0 - r);
    vec2 distortedUV = screenUV - dir * distortStrength;
    distortedUV = clamp(distortedUV, vec2(0.001), vec2(0.999));

    // Sample background
    vec3 bg = texture2D(screenTexture, distortedUV).rgb;

    // Blend background with the shield plasma glow
    vec3 result = mix(bg, finalGlow.rgb * intensity, alpha);

    gl_FragColor = vec4(result, 1.0);
}
