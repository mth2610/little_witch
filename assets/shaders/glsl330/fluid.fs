#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float u_time; 

out vec4 finalColor;

void main() {
    float density = texture(texture0, fragTexCoord).a;
    
    if (density < 0.05) {
        discard;
    }
    
    vec3 colorOuter = vec3(0.0, 0.35, 0.9); // Deep blue border
    vec3 colorInner = vec3(0.4, 0.85, 1.0); // Bright cyan middle
    vec3 colorCore  = vec3(1.0, 1.0, 1.0);  // Pure white core

    vec3 mixedColor;
    
    if (density < 0.4) {
        float t = smoothstep(0.05, 0.4, density);
        mixedColor = mix(colorOuter, colorInner, t);
    } else {
        float t = smoothstep(0.4, 0.8, density);
        mixedColor = mix(colorInner, colorCore, t);
    }
    
    vec2 flow = vec2(u_time * 8.0, u_time * -5.0);
    vec2 pixelCoords = fragTexCoord * textureSize(texture0, 0);
    float sparkle = sin((pixelCoords.x + flow.x) * 0.2) * cos((pixelCoords.y + flow.y) * 0.2);
    sparkle = pow(clamp(sparkle, 0.0, 1.0), 3.0);
    
    if (density > 0.4) {
        mixedColor += vec3(sparkle * 0.6); 
    }
    
    float alpha = smoothstep(0.05, 0.25, density);
    finalColor = vec4(mixedColor, alpha);
}
