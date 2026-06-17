// tornado.fs — Lốc xoáy ánh sáng (Tornado of Light) tích hợp Raymarching 3D giả lập
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform vec2 center;       // Tâm lốc xoáy (đáy phễu)
uniform float radius;      // Bán kính lốc xoáy
uniform float time;
uniform float height;      // Chiều cao lốc xoáy (px)

out vec4 finalColor;

// Hàm tạo số ngẫu nhiên 3D
float hash3D(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);
}

// Nhiễu giá trị 3D (3D Value Noise) mượt mà
float valNoise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    return mix(
        mix(mix(hash3D(i + vec3(0,0,0)), hash3D(i + vec3(1,0,0)), f.x),
            mix(hash3D(i + vec3(0,1,0)), hash3D(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash3D(i + vec3(0,0,1)), hash3D(i + vec3(1,0,1)), f.x),
            mix(hash3D(i + vec3(0,1,1)), hash3D(i + vec3(1,1,1)), f.x), f.y),
        f.z
    );
}

// Fractal Brownian Motion 3D (3 octaves để cân bằng hiệu năng và chi tiết)
float fbm3D(vec3 p) {
    float v = 0.0;
    float a = 0.5;
    mat3 rot = mat3(0.00, 0.80, 0.60,
                    -0.80, 0.36, -0.48,
                    -0.60, -0.48, 0.64);
    for (int i = 0; i < 3; i++) {
        v += a * valNoise3D(p);
        p = rot * p * 2.2 + vec3(100.0);
        a *= 0.5;
    }
    return v;
}

mat2 Spin(float angle) {
    return mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
}

void main() {
    vec2 pixelPos = vec2(gl_FragCoord.x, 720.0 - gl_FragCoord.y);
    vec2 delta = pixelPos - center;

    // yNorm: 0 ở đáy (rộng), 1 ở đỉnh phễu (hẹp)
    float yNorm = -delta.y / height;
    if (yNorm < -0.15 || yNorm > 1.15) discard;

    // Phễu co dần từ dưới lên
    float funnelWidth = radius * (1.15 - yNorm * 0.55);
    if (abs(delta.x) > funnelWidth * 1.6) discard;

    // === RAYMARCHING KHÔNG GIAN 3D TRÊN CANVAS 2D ===
    // Chiếu tia song song (Orthographic Raymarching) qua hình trụ giới hạn
    float zLimit = funnelWidth * 1.35;
    
    // Sử dụng số bước tĩnh làm hằng số để tương thích với tất cả driver OpenGL
    float stepSize = (2.0 * zLimit) / 22.0;

    float accumulatedDensity = 0.0;
    float accumulatedCore = 0.0;

    for (int i = 0; i < 22; i++) {
        // Tọa độ Z chạy từ trước ra sau
        float z = -zLimit + (float(i) + 0.5) * stepSize;
        vec3 q = vec3(delta.x, -delta.y, z); // q.y đại diện cho chiều cao thực tế từ đáy

        // Bán kính xoáy thay đổi theo chiều cao
        float zcurve = radius * (0.28 + yNorm * 0.72);

        // Góc xoay xoắn ốc theo chiều cao và thời gian
        float angle = -time * 5.5 + yNorm * 8.0 + q.y * 0.005;
        vec2 rotXZ = Spin(angle) * q.xz;

        // Vị trí tính nhiễu động 3D
        vec3 noisePos = vec3(rotXZ * 0.08, q.y * 0.06 - time * 1.5);
        float turbulence = fbm3D(noisePos);

        // Khoảng cách tới vỏ ống lốc xoáy
        float lenXZ = length(q.xz);
        float tubeThickness = 7.0 + 10.0 * yNorm;
        float distToTube = abs(lenXZ - zcurve);

        // Mật độ của luồng gió lốc
        float density = exp(-distToTube * distToTube / (2.0 * tubeThickness * tubeThickness));
        density *= (0.35 + 0.65 * turbulence);

        accumulatedDensity += density * stepSize * 0.065;

        // Lõi sét ánh sáng chạy dọc tâm (Electric light filament core)
        float distToCenter = length(q.xz);
        float coreNoise = valNoise3D(vec3(0.0, q.y * 0.15 - time * 12.0, 0.0)) * 6.0;
        float centerGlow = exp(-pow(distToCenter - coreNoise, 2.0) / 12.0);
        accumulatedCore += centerGlow * density * stepSize * 0.15;
    }

    // === PHỐI MÀU LỐC XOÁY ÁNH SÁNG THẦN THOẠI (COSMIC LIGHT GRADIENT) ===
    // Viền ngoài xanh ngọc (Electric Cyan) -> Thân vàng kim (Aurum Gold) -> Lõi trắng siêu sáng
    vec3 outerLight = vec3(0.25, 0.82, 1.0); // Cyan quang năng
    vec3 midLight   = vec3(1.0, 0.78, 0.15); // Vàng kim rực rỡ
    vec3 coreLight  = vec3(1.0, 1.0, 1.0);    // Trắng nóng

    // Kết hợp các lớp ánh sáng
    vec3 finalRGB = outerLight * accumulatedDensity;
    finalRGB += midLight * (accumulatedDensity * accumulatedDensity * 0.65);
    finalRGB += coreLight * accumulatedCore * (0.75 + 0.25 * sin(time * 25.0)); // Lõi sét nhấp nháy mạnh

    // Tăng cường độ phát sáng
    finalRGB *= 1.35;

    // === HÀO QUANG VÀ GIỚI HẠN BIÊN (BOUNDS FADING) ===
    float topFade = smoothstep(1.12, 0.88, yNorm);
    float bottomFade = smoothstep(-0.12, 0.08, yNorm);
    float edgeFade = smoothstep(funnelWidth * 1.55, funnelWidth * 0.75, abs(delta.x));
    float mask = topFade * bottomFade * edgeFade;

    float alpha = (accumulatedDensity * 0.78 + accumulatedCore * 1.15) * mask;
    alpha = clamp(alpha, 0.0, 0.95);

    finalColor = vec4(finalRGB, alpha);
}
