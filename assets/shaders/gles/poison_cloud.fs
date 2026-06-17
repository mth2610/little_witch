#version 100
precision highp float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform vec2 center;
uniform float radius;
uniform float time;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(hash(i), hash(i + vec2(1,0)), f.x),
        mix(hash(i + vec2(0,1)), hash(i + vec2(1,1)), f.x),
        f.y
    );
}

// FBM 6 octave — Sương mù mật độ cao
float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    mat2 rot = mat2(0.866, 0.5, -0.5, 0.866);
    for (int i = 0; i < 6; i++) {
        v += a * noise(p);
        p = rot * p * 2.05 + vec2(100.0);
        a *= 0.5;
    }
    return v;
}

// Bọt khí độc nổi lên và vỡ ra sinh động
float bubbles(vec2 p, float t) {
    float total = 0.0;
    for (int i = 0; i < 6; i++) {
        vec2 bPos = vec2(
            hash(vec2(float(i) * 17.3, 1.2)) * 2.0 - 1.0,
            mod(hash(vec2(float(i) * 9.7, 3.4)) - t * (0.25 + hash(vec2(float(i), 1.5)) * 0.35), 2.0) - 1.0
        );
        float bDist = length(p - bPos * 0.7);
        // Kích thước bọt phập phồng rồi tự xẹp nhanh (mô phỏng nổ)
        float phase = mod(t * 2.0 + float(i), 3.14159);
        float bSize = (0.04 + hash(vec2(float(i), 5.7)) * 0.07) * sin(phase);
        
        total += smoothstep(bSize + 0.015, bSize, bDist) * step(0.01, bSize);
    }
    return total;
}

void main() {
    vec2 pixelPos = vec2(gl_FragCoord.x, 720.0 - gl_FragCoord.y);
    vec2 delta = pixelPos - center;
    float dist = length(delta);

    if (dist > radius * 2.2) discard;

    float r = dist / radius;
    vec2 uv = delta / radius; // -1 đến 1 trong vùng bán kính

    // === HỆ THỐNG XOẮN XOÁY VORTEX (SPIRAL VORTEX) ===
    // Tạo chuyển động xoáy hút khí độc vào trung tâm
    float theta = atan(uv.y, uv.x);
    float swirl = 3.5 * r - time * 1.5;
    float ct = cos(swirl), st = sin(swirl);
    vec2 spiralUV = vec2(uv.x * ct - uv.y * st, uv.x * st + uv.y * ct);

    // FBM với biến dạng xoáy và thời gian kéo dãn
    float cloud1 = fbm(spiralUV * 2.2 + vec2(time * 0.35, -time * 0.15));
    float cloud2 = fbm(uv * 1.7 - vec2(time * 0.2, time * 0.4) + vec2(40.0));
    float cloud = mix(cloud1, cloud2, 0.52);

    // === PHỐI MÀU SINH HỌC 3 CHIỀU (VOLUMETRIC HYBRID COLORS) ===
    // Trộn lẫn các dải màu độc tố: Tím đậm (bóng), xanh lá mạ (thân sương), vàng acid (lõi), xanh neon sáng
    vec3 shadowPurple = vec3(0.12, 0.02, 0.18); // Tạo độ sâu lập thể cho khói độc
    vec3 toxicGreen   = vec3(0.08, 0.62, 0.18);
    vec3 acidYellow   = vec3(0.55, 0.85, 0.05);
    vec3 neonCyan     = vec3(0.15, 0.92, 0.45);

    // Gradient phân lớp phức tạp
    vec3 color = mix(neonCyan, toxicGreen, r * 0.7 + cloud * 0.25);
    color = mix(color, shadowPurple, smoothstep(0.4, 1.35, r));

    // Dải năng lượng acid phát sáng xoáy theo sương mù
    float acidStreak = pow(cloud, 2.8) * smoothstep(0.85, 0.15, r);
    color = mix(color, acidYellow, acidStreak * 0.65);

    // === ĐIỆN SINH HỌC PHÓNG ĐIỆN (BIO-ELECTRIC DISCHARGES) ===
    // Phóng ra các tia chớp plasma sinh học màu xanh lá neon ngẫu nhiên bên trong đám mây
    float veinNoise = fbm(spiralUV * 5.0 + time * 6.0);
    float bioLightning = smoothstep(0.045, 0.0, abs(veinNoise - 0.5)) * smoothstep(0.85, 0.1, r);
    // Nhấp nháy theo thời gian
    float flash = step(0.82, sin(time * 18.0 + r * 5.0));
    color = mix(color, neonCyan * 1.8, bioLightning * flash * 0.85);

    // === BỌT KHÍ ĐỘC PHẬP PHỒNG ===
    float bubs = bubbles(uv, time * 1.2);
    color = mix(color, acidYellow * 1.3, bubs * 0.7);

    // === ĐỘ TRONG SUỐT PHỨC HỢP (VOLUMETRIC FADE) ===
    float cloudAlpha = cloud * 0.72 + 0.28;
    float edgeFade = smoothstep(1.38, 0.28, r);
    float alpha = cloudAlpha * edgeFade * 0.68;

    // Nhịp tim phát sáng toàn thể
    float pulse = 0.88 + 0.12 * sin(time * 4.5);
    alpha *= pulse;

    gl_FragColor = vec4(color, alpha);
}
