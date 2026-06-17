// shuriken.fs — Phi tiêu vàng kim loại Fresnel phép thuật tối tân (Hệ Kim)
#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform vec2 center;
uniform float radius;
uniform float time;
uniform float rotation;   // Góc xoay phi tiêu (radian, tăng liên tục)

out vec4 finalColor;

// Hàm tạo nhiễu ngẫu nhiên
float hash(float n) { return fract(sin(n) * 43758.5453123); }
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float n = mix(mix(hash(i.x + i.y * 113.0), hash(i.x + 1.0 + i.y * 113.0), f.x),
                  mix(hash(i.x + (i.y + 1.0) * 113.0), hash(i.x + 1.0 + (i.y + 1.0) * 113.0), f.x), f.y);
    return n;
}

void main() {
    vec2 pixelPos = vec2(gl_FragCoord.x, 720.0 - gl_FragCoord.y);
    vec2 delta = pixelPos - center;
    float dist = length(delta);

    // Mở rộng bán kính render để vệt sáng xoay tỏa rộng hơn
    if (dist > radius * 3.5) discard;

    float r = dist / radius;
    float angle = atan(delta.y, delta.x) - rotation;

    // === HÌNH DẠNG SHURIKEN 4 CÁNH SẮC CẠNH ===
    // Tạo hình sao 4 cánh đối xứng dùng polar coordinate
    float blade = cos(angle * 4.0); // 4 cánh
    float bladeShape = 0.35 + (blade * 0.5) * 0.65; // Đỉnh nhọn hơn và co lõm sâu ở giữa
    float shurikenMask = smoothstep(bladeShape + 0.12, bladeShape - 0.03, r);

    // === CHẤT LIỆU KIM LOẠI VÀNG CAO CẤP ===
    vec3 darkGold  = vec3(0.48, 0.33, 0.04);
    vec3 brightGold = vec3(1.0, 0.88, 0.32);
    vec3 lightGold  = vec3(1.0, 0.98, 0.75);
    vec3 whiteHot   = vec3(1.0, 1.0, 0.95);

    // Fresnel phản quang: Sáng bóng ở rìa lưỡi dao sắc bén
    float bladeFresnel = pow(1.0 - abs(cos(angle * 2.0)), 3.0);
    vec3 metalColor = mix(darkGold, brightGold, bladeFresnel);

    // Ánh chớp kim loại quét qua cánh khi xoay nhanh
    float specular = pow(max(0.0, cos(angle * 4.0 + time * 12.0)), 24.0);
    metalColor = mix(metalColor, whiteHot, specular * 0.85);

    // Rãnh rập khắc hoa văn chìm trên cánh
    float engrave = sin(r * 28.0 + angle * 4.0) * 0.5 + 0.5;
    engrave = smoothstep(0.42, 0.58, engrave);
    metalColor *= 0.8 + 0.2 * engrave;

    // === VIÊN NGỌC HỒNG KHÔNG GIAN Ở TRUNG TÂM (CORE GEM) ===
    float coreDist = r / 0.32;
    if (coreDist < 1.0) {
        float coreGlow = pow(1.0 - coreDist, 1.5);
        // Viên ngọc chuyển sắc kỳ ảo (Hồng ngọc / Hồng ngoại)
        vec3 gemColor = mix(vec3(1.0, 0.2, 0.65), vec3(1.0, 0.95, 0.85), coreGlow);
        metalColor = mix(metalColor, gemColor, coreGlow * 0.95);
    }

    // === VÒNG TRÒN ĐIỆN NĂNG LƯỢNG (ELECTRIC RING) BAO QUANH ===
    // Tạo vòng tròn năng lượng chập chờn ở rìa ngoài cánh shuriken
    float ringRadius = 1.05 + 0.1 * noise(vec2(angle * 5.0, time * 8.0));
    float ringWidth = 0.09;
    float electricRing = smoothstep(ringWidth, 0.0, abs(r - ringRadius));
    vec3 electricColor = mix(brightGold, lightGold, noise(vec2(r, time * 15.0))) * electricRing * 1.5;

    // === VỆT XOAY MA THUẬT VÀ PHÂN RÃ MÀU SẮC (CHROMATIC MOTION TRAIL) ===
    float trail = 0.0;
    for (int i = 0; i < 4; i++) {
        float bladeAngle = float(i) * 1.570796;
        float diff = abs(mod(angle - bladeAngle + 3.14159 - rotation * 0.15, 1.570796) - 0.785398);
        trail += exp(-diff * 14.0) * exp(-r * 1.2) * 0.45;
    }

    // === TIA SÁNG MẶT TRỜI TỎA TRÒN (SOLAR FLARE RAYS) ===
    float rays = 0.0;
    for (int i = 0; i < 6; i++) {
        float rayAngle = float(i) * 1.047197 + rotation * 1.2; // 6 tia xoay ngược nhẹ
        float angleDiff = abs(sin(atan(delta.y, delta.x) - rayAngle));
        rays += exp(-angleDiff * 20.0) * exp(-r * 2.2) * 0.28;
    }

    // === TỔNG HỢP MÀU ===
    float alpha = shurikenMask;
    vec3 trailColor = brightGold * (trail + rays * 0.8);
    
    // Quầng sáng huyền ảo bao ngoài
    float outerGlow = exp(-dist / (radius * 1.6)) * 0.6;
    trailColor += brightGold * outerGlow;

    vec3 result = metalColor * alpha + electricColor + trailColor * (1.0 - alpha * 0.55);
    float finalAlpha = max(alpha, trail + rays * 0.5 + outerGlow + electricRing * 0.5);
    finalAlpha = clamp(finalAlpha, 0.0, 1.0);

    finalColor = vec4(result, finalAlpha);
}
