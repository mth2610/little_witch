#version 100
precision highp float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform vec2 center;       // Tâm cầu lửa (virtual screen coordinates)
uniform float radius;      // Bán kính cầu lửa
uniform float time;        // GetTime()
uniform float intensity;   // Cường độ, dựa trên skillLevel/stars

// Hàm hash ngẫu nhiên 2D
vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)),
             dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

// Nhiễu Simplex 2D mượt mà
float noise(vec2 p) {
    const float K1 = 0.366025404;
    const float K2 = 0.211324865;

    vec2 i = floor(p + (p.x + p.y) * K1);
    vec2 a = p - i + (i.x + i.y) * K2;
    vec2 o = (a.x > a.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0 * K2;

    vec3 h = max(0.5 - vec3(dot(a,a), dot(b,b), dot(c,c)), 0.0);

    vec3 n = h * h * h * h * vec3(
         dot(a, hash(i + 0.0)),
         dot(b, hash(i + o)),
         dot(c, hash(i + 1.0))
    );

    return dot(n, vec3(70.0));
}

// Nhiễu lượn sóng cuộn trôi Blender
float getBlenderNoise(vec3 p) {
    vec3 q1 = p, q2 = p;
    float scale = 0.35;
    float offset = time * 7.5; // Tốc độ lửa cuộn cuồng phong

    q1.y += -offset;
    q2.z += offset;

    float n1 = noise(q1.xy * scale);
    float n2 = noise(q2.xz * scale);
    return (n1 + n2) / 2.0;
}

// Bản đồ khoảng cách 3D (Distance Field Map)
float map(vec3 p) {
    // Tăng kích cỡ mô hình quả cầu r và chiều dài h để ngọn lửa to và uy lực
    float r = 2.4; 
    float h = 8.0;

    vec3 dir = p;
    float n = getBlenderNoise(p) * (smoothstep(-3.0, 1.0, p.y) * 0.4 + 0.15);
    p += n * dir;

    // Quả cầu lửa chính ở đầu
    float d1 = length(p) - r;
    
    // Hình trụ đuôi lửa co hẹp dần về sau phễu
    float d2 = length(p.xz);
    d2 -= smoothstep(h, 0.0, p.y) * r;
    d2 = max(d2, -(-p.y + h));
    d2 = max(d2, -p.y);
    
    return min(d1, d2);
}

void main() {
    vec2 pixelPos = vec2(gl_FragCoord.x, 720.0 - gl_FragCoord.y);
    
    // Ánh xạ tọa độ pixel sang không gian 3D tương đối
    vec2 uv = (pixelPos - center) / radius;
    uv.y = -uv.y; // Đồng bộ hướng Y trong hệ tọa độ Raylib
    
    // Kiểm tra giới hạn bao (bounding box) rộng lớn để không bị cắt xén đuôi lửa dài
    if (uv.x < -11.0 || uv.x > 2.8 || abs(uv.y) > 2.8) discard;

    // Thiết lập Camera và Raymarching
    vec3 ro = vec3(0.0, 0.0, -10.0);
    vec3 rd = normalize(vec3(uv, 2.0)); // Tia phóng chiều sâu
    
    float d = 0.0;
    float d_acc = 0.0;
    float z = 0.0;
    vec3 p;
    
    // Tối ưu hóa: Giảm số bước chạy raymarching từ 35 xuống 20 để tăng hiệu năng trên Android
    for (int i = 0; i < 20; i++) {
        p = ro + rd * z;
        
        // Quay góc 3D để trục đuôi lửa chĩa thẳng về bên trái (dọc theo trục -X)
        vec3 q = vec3(p.y, -p.x, p.z);
        
        d = map(q);
        d = 0.04 + abs(d) * 0.25;
        d_acc += 0.1 / d;
        z += d;
        
        if (z > 25.0) break;
    }
    
    d_acc = pow(d_acc, 2.0);
    
    // Phối màu ngọn lửa đa sắc (Hào quang hồng tím -> Thân cam hoàng kim -> Lõi trắng siêu sáng)
    vec3 colorBase = sin(vec3(10.0, 6.0, 2.0) + time * 1.8 + p.z * 0.15) * 0.5 + 0.5;
    
    vec3 fireColor = mix(vec3(0.85, 0.05, 0.60), vec3(1.0, 0.48, 0.0), colorBase.r);
    fireColor = mix(fireColor, vec3(1.0, 0.95, 0.45), colorBase.g * 0.75);
    
    // Nén màu sắc qua hàm tanh để tránh bị cháy sáng trắng quá đà
    // Giải lập tanh(x) = (exp(2x)-1)/(exp(2x)+1) vì tanh không khả dụng trong GLSL ES 100
    vec3 x = fireColor * d_acc / z / 100.0;
    vec3 exp2x = exp(2.0 * x);
    vec3 rgb = (exp2x - 1.0) / (exp2x + 1.0);
    
    // Tính toán độ trong suốt tích lũy
    float alpha = clamp(d_acc / 450.0, 0.0, 1.0);
    
    // Làm mượt rìa ngoài cùng
    float distRatio = length(uv) / 3.0;
    alpha *= smoothstep(1.0, 0.25, distRatio);
    
    gl_FragColor = vec4(rgb * (0.95 + 0.45 * intensity), alpha);
}
