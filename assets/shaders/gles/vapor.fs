#version 100
precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform float time;

void main() {
    // Distance from center of the particle (0.5, 0.5)
    vec2 uv = fragTexCoord - vec2(0.5);
    float dist = length(uv);
    
    // Soft vignette/alpha boundary
    float alphaLimit = 0.5;
    float borderFade = clamp((alphaLimit - dist) / 0.15, 0.0, 1.0);
    
    if (borderFade < 0.01) {
        discard;
    }
    
    // Swirling/wavy noise simulation using sine/cosine fractals
    vec2 noiseCoord = uv * 10.0;
    float n1 = sin(noiseCoord.x + time * 4.0) * cos(noiseCoord.y - time * 3.0);
    float n2 = sin(noiseCoord.y * 1.8 + time * 3.2) * cos(noiseCoord.x * 1.2 - time * 2.5);
    float n3 = sin((noiseCoord.x + noiseCoord.y) * 2.5 + time * 2.0);
    float noise = (n1 + n2 + n3) / 3.0 * 0.35 + 0.65; // Scale noise to [0.3, 1.0]
    
    // Make it rotate/swirl slightly
    float angle = atan(uv.y, uv.x);
    float swirl = sin(angle * 3.0 + time * 3.5 - dist * 8.0) * 0.12 + 0.88;
    
    // Combine noise and swirl
    float intensity = borderFade * noise * swirl;
    
    // Icy/frosty colors: Skyblue/Cyan at core, fading to white at edges
    vec3 vaporCore = vec3(0.40, 0.78, 1.0);
    vec3 vaporEdge = vec3(0.85, 0.95, 1.0);
    
    // Center is more core color, edge is white
    vec3 finalRGB = mix(vaporEdge, vaporCore, dist * 2.0);
    
    // Add a bit of white glow in the center
    finalRGB += vec3(0.15, 0.25, 0.35) * max(0.0, 1.0 - dist * 2.5);
    
    // Final output alpha modulated by particle's alpha (fragColor.a)
    float finalAlpha = intensity * fragColor.a * 0.75;
    
    gl_FragColor = vec4(finalRGB * finalAlpha, finalAlpha);
}
