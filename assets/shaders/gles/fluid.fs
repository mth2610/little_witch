#version 100
precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float u_time;

void main() {
    float density = texture2D(texture0, fragTexCoord).a;
    
    if (density < 0.05) {
        discard;
    }
    
    vec3 colorOuter = vec3(0.0, 0.35, 0.9); // Xanh biển sâu (Viền)
    vec3 colorInner = vec3(0.4, 0.85, 1.0); // Xanh lơ sáng (Lõi giữa)
    vec3 colorCore  = vec3(1.0, 1.0, 1.0);  // Trắng tinh khiết (Tâm bạo kích)
 
    vec3 mixedColor;
    
    if (density < 0.4) {
        float t = (density - 0.05) / (0.4 - 0.05);
        t = clamp(t, 0.0, 1.0);
        t = t * t * (3.0 - 2.0 * t); // smoothstep
        mixedColor = mix(colorOuter, colorInner, t);
    } else {
        float t = (density - 0.4) / (0.8 - 0.4);
        t = clamp(t, 0.0, 1.0);
        t = t * t * (3.0 - 2.0 * t); // smoothstep
        mixedColor = mix(colorInner, colorCore, t);
    }
    
    // Caustics
    vec2 flow = vec2(u_time * 8.0, u_time * -5.0);
    vec2 pixelCoords = fragTexCoord * vec2(1280.0, 720.0);
    float sparkle = sin((pixelCoords.x + flow.x) * 0.2) * cos((pixelCoords.y + flow.y) * 0.2);
    sparkle = pow(clamp(sparkle, 0.0, 1.0), 3.0);
    
    if (density > 0.4) {
        mixedColor += vec3(sparkle * 0.6); 
    }
    
    float alpha = (density - 0.05) / (0.25 - 0.05);
    alpha = clamp(alpha, 0.0, 1.0);
    alpha = alpha * alpha * (3.0 - 2.0 * alpha); // smoothstep
    
    gl_FragColor = vec4(mixedColor, alpha);
}
