#version 330

in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform float u_time; 

out vec4 finalColor;

void main() {
    vec4 texColor = texture(texture0, fragTexCoord);
    
    // FIX SHADER: Render Texture đã blend Alpha vào màu (RGB) rồi.
    // Lấy trực tiếp kênh R làm mật độ (density) để tránh lỗi giảm trừ kép.
    float density = texColor.r; 
    
    if (density < 0.01) {
        discard;
    }
    
    // Bảng màu lửa
    vec3 darkFire   = vec3(0.45, 0.05, 0.02); // đỏ tối
    vec3 redFire    = vec3(0.80, 0.10, 0.05); 
    vec3 orangeFire = vec3(1.00, 0.40, 0.00); 
    vec3 yellowFire = vec3(1.00, 0.85, 0.10);
    vec3 coreFire   = vec3(1.00, 1.00, 0.90);

    vec3 mixedColor;
    float alphaOut = smoothstep(0.01, 0.3, density); 

    if (density < 0.15) {
        float t = smoothstep(0.01, 0.15, density);
        mixedColor = mix(darkFire, redFire, t);
        alphaOut *= 0.85; 
    } else if (density < 0.4) {
        float t = smoothstep(0.15, 0.4, density);
        mixedColor = mix(redFire, orangeFire, t);
    } else if (density < 0.75) {
        float t = smoothstep(0.4, 0.75, density);
        mixedColor = mix(orangeFire, yellowFire, t);
    } else {
        float t = smoothstep(0.75, 1.0, density);
        mixedColor = mix(yellowFire, coreFire, t);
    }
    
    // Hiệu ứng méo nhiệt (heat ripple)
    vec2 flow = vec2(u_time * 5.0, u_time * -15.0); 
    vec2 pixelCoords = fragTexCoord * textureSize(texture0, 0);
    float heatRipple = sin((pixelCoords.x + flow.x) * 0.1) * cos((pixelCoords.y + flow.y) * 0.1);
    
    if (density > 0.2) {
        mixedColor += vec3(heatRipple * 0.15); 
    }
    
    finalColor = vec4(mixedColor, alphaOut);
}