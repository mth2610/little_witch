#version 330

in vec2 fragTexCoord;
in vec4 fragColor; // Khai báo thêm dòng này để Mac không báo lỗi

uniform sampler2D texture0;
uniform float u_time; 

out vec4 finalColor;

void main() {
    float density = texture(texture0, fragTexCoord).a;
    
    // Xóa những vùng nền đen/trong suốt dư thừa
    if (density < 0.05) {
        discard;
    }
    
    // Bảng màu Hoàng Kim (Gold)
    vec3 colorOuter = vec3(1.0, 0.6, 0.0);  // Cam đồng
    vec3 colorInner = vec3(1.0, 0.9, 0.1);  // Vàng sáng chói
    vec3 colorCore  = vec3(1.0, 1.0, 1.0);  // Lõi trắng lóa
    
    vec3 mixedColor;
    
    // Gradient ép màu dựa theo độ mờ của hình ảnh
    if (density < 0.5) {
        float t = smoothstep(0.05, 0.5, density);
        mixedColor = mix(colorOuter, colorInner, t);
    } else {
        float t = smoothstep(0.5, 0.85, density);
        mixedColor = mix(colorInner, colorCore, t);
    }
    
    // Vệt sáng chớp nháy chạy dọc thanh kiếm (Gleam)
    float gleam = pow(sin(fragTexCoord.x * 50.0 + u_time * 20.0) * cos(fragTexCoord.y * 50.0),
     4.0);
     
    if (density < 0.02) {
        discard;
    }
    
    float alpha = smoothstep(0.05, 0.2, density);
    finalColor = vec4(mixedColor, alpha);
}