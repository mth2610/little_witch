// ice_blast.fs — GLES 2.0 Tinh thể băng lấp lánh & sương tuyết cực đỉnh (Hệ Thủy)
#version 100
precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform vec2 center;      // Vị trí tâm đạn (virtual canvas)
uniform float radius;      // Bán kính đạn
uniform float time;        // Thời gian để xoay/lấp lánh
uniform float rotation;    // Hướng bay gốc (radian)
uniform sampler2D screenTexture; // Background screen copy

// Hàm hash ngẫu nhiên
vec2 hash2(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 43758.5453) * 2.0 - 1.0;
}

// Tạo Voronoi tế bào băng
float voronoi(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float minDist = 1.0;
    // Vòng lặp đơn giản hóa cho GLES 2.0 tương thích tốt
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash2(i + neighbor) * 0.5 + 0.5;
            point += 0.15 * sin(time * 3.0 + 6.2831 * hash2(i + neighbor));
            float d = length(neighbor + point - f);
            if (d < minDist) {
                minDist = d;
            }
        }
    }
    return minDist;
}

// Cạnh cell Voronoi (vết nứt băng)
float voronoiEdge(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float d1 = 1.0, d2 = 1.0;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(float(x), float(y));
            vec2 point = hash2(i + neighbor) * 0.5 + 0.5;
            point += 0.12 * sin(time * 2.5 + 6.2831 * hash2(i + neighbor));
            float d = length(neighbor + point - f);
            if (d < d1) { 
                d2 = d1; 
                d1 = d; 
            } else if (d < d2) { 
                d2 = d; 
            }
        }
    }
    return d2 - d1;
}

void main() {
    // 1. CHUYỂN TỌA ĐỘ VỀ UV CỦA QUAD [-0.5, 0.5]
    vec2 uv = fragTexCoord - vec2(0.5);
    float dist = length(uv);
    
    // Tự động xoay tinh thể theo thời gian để tạo độ lung linh sinh động
    float spinTime = time * 2.5;
    float c = cos(rotation + spinTime), s = sin(rotation + spinTime);
    vec2 rotUv = vec2(uv.x * c - uv.y * s, uv.x * s + uv.y * c);
    
    // Tạo hình học ngôi sao băng 6 cánh sắc bén bằng toán học
    float angle = atan(rotUv.y, rotUv.x);
    float shape = 0.38 + 0.08 * cos(angle * 6.0); // 6 cánh đối xứng
    
    // Cắt viền cực mịn, ép độ mờ ở biên về 0
    float borderFade = clamp((shape - dist) / 0.04, 0.0, 1.0);
    
    if (borderFade < 0.01) {
        discard; // Loại bỏ hoàn toàn các mảnh pixel bên ngoài hình tinh thể băng
    }
    
    // 2. TẠO VẾT NỨT BĂNG VÀ VÂN VORONOI
    vec2 voroUv = rotUv * 6.5;
    float cellDist = voronoi(voroUv);
    float crack = voronoiEdge(voroUv);
    
    // 3. THIẾT LẬP MÀU TINH THỂ
    vec3 deepBlue   = vec3(0.04, 0.16, 0.45);
    vec3 iceBlue    = vec3(0.38, 0.76, 1.0);
    vec3 frostWhite = vec3(0.90, 0.97, 1.0);
    
    vec3 crystalCol = mix(deepBlue, iceBlue, cellDist);
    
    // Vân nứt sáng rực của băng ma thuật
    float crackLine = clamp((0.05 - crack) / 0.05, 0.0, 1.0);
    crystalCol = mix(crystalCol, frostWhite, crackLine * 0.75);
    
    // Lấp lánh ma thuật theo thời gian
    float sparkle = pow(cellDist, 8.0) * (0.3 + 0.7 * sin(time * 12.0 + cellDist * 25.0));
    crystalCol += sparkle * vec3(0.7, 0.9, 1.0);
    
    // Sáng rực ở phần viền rìa của tinh thể (Fresnel Glow)
    float rim = pow(dist / shape, 3.0);
    crystalCol = mix(crystalCol, vec3(0.5, 0.9, 1.0), rim * 0.5);
    
    // 4. KHÚC XẠ ÁNH SÁNG NỀN (DISTORTION)
    // Tọa độ màn hình tương ứng với fragment
    vec2 screenUV = gl_FragCoord.xy / vec2(1280.0, 720.0);
    
    // Hướng biến dạng từ tâm ra ngoài
    vec2 dir = (dist > 0.001) ? normalize(uv) : vec2(1.0, 0.0);
    
    // Cường độ méo tỉ lệ với vết nứt và độ dày tinh thể
    float distortStrength = (rim * 0.012 + (1.0 - crack) * 0.006) * borderFade;
    
    // Rung lắc nhẹ vì nhiệt độ cực lạnh
    float shiver = sin(time * 25.0 + dist * 5.0) * 0.0015 * borderFade;
    
    vec2 distortedUV = screenUV - dir * (distortStrength + shiver);
    distortedUV = clamp(distortedUV, vec2(0.001), vec2(0.999));
    
    // Đọc màu nền bị méo (Dùng texture2D trong GLES 2.0)
    vec3 bg = texture2D(screenTexture, distortedUV).rgb;
    
    // 5. HÒA TRỘN TINH THỂ LÊN NỀN
    float alpha = borderFade * 0.82; // Độ mờ tổng thể của băng
    vec3 finalRGB = mix(bg, crystalCol, alpha);
    
    // Thêm quầng sáng sương giá mờ nhẹ xung quanh lõi
    float glow = exp(-dist * 4.5) * 0.35 * borderFade;
    finalRGB += vec3(0.4, 0.8, 1.0) * glow;
    
    gl_FragColor = vec4(finalRGB, 1.0);
}
