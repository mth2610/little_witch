#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform float time;
uniform vec4 glowColor; // Base color of the shield (e.g. Cyan/Skyblue)
uniform vec2 center;    // Center of shield in virtual screen coordinates
uniform float radius;   // Radius of shield in pixels

// Output fragment color
out vec4 finalColor;

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

    // Tính toán góc từ tâm khiên để tạo hiệu ứng cuộn xoáy tròn (swirling plasma)
    float angle = atan(pixelPos.y - center.y, pixelPos.x - center.x);

    // Sóng năng lượng cuộn xoáy 1 (theo chiều kim đồng hồ)
    float ripple1 = sin(r * 12.0 - time * 6.5 + angle * 3.0) * 0.5 + 0.5;
    // Sóng năng lượng cuộn xoáy 2 đối nghịch (ngược chiều kim đồng hồ)
    float ripple2 = sin(r * 8.0 + time * 3.5 - angle * 2.0) * 0.5 + 0.5;
    
    // Trộn hai luồng sóng năng lượng để tạo nhiễu động plasma
    float ripple = mix(ripple1, ripple2, 0.45);
    // Vầng sóng tăng mạnh dần về phía rìa khiên
    float rippleGlow = ripple * smoothstep(0.2, 1.0, r) * 0.55;

    // Fast shimmering plasma noise
    float shimmer = hash(pixelPos * 0.05 + vec2(0.0, time * 1.5)) * 0.1;

    // Combine intensities
    float intensity = rim * 1.8 + rippleGlow + 0.08 + shimmer * r;

    // Shift color slightly towards lighter cyan/white at the bright edge
    vec4 baseColor = glowColor;
    vec4 rimColor = vec4(baseColor.r * 0.5, baseColor.g * 0.8, 1.0, 1.0);
    vec4 finalGlow = mix(baseColor, rimColor, rim);

    // Transparency: clear center so the Witch is visible, glowing edge
    float alpha = rim * 0.75 + rippleGlow * 0.35 + 0.08;
    alpha = clamp(alpha, 0.0, 0.9);

    finalColor = vec4(finalGlow.rgb * intensity, alpha);
}
