// lightning.fs — Sấm sét xích điện neon đa nhánh và plasma (Hệ Thổ)
#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform vec2 startPos; // Vị trí đầu tia sét (virtual screen coordinates, 1280x720)
uniform vec2 endPos;   // Vị trí cuối tia sét
uniform float time;
uniform vec4 glowColor; // Màu cơ bản (Skyblue/Violet)

// Output fragment color
out vec4 finalColor;

// Hàm tạo nhiễu ngẫu nhiên
float hash(float n) { return fract(sin(n) * 43758.5453123); }
float noise(float x) {
    float i = floor(x);
    float f = fract(x);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), u);
}

// Ridged FBM để tạo hình ziczac gồ ghề khúc khuỷu của tia điện
float ridgedFBM(float x) {
    float v = 0.0;
    float a = 0.5;
    float shift = 100.0;
    for (int i = 0; i < 5; ++i) {
        float n = noise(x);
        v += a * (1.0 - abs(n * 2.0 - 1.0));
        x = x * 2.5 + shift;
        a *= 0.45;
    }
    return v;
}

// Tính khoảng cách từ điểm p đến đoạn thẳng ab
float distToSegment(vec2 p, vec2 a, vec2 b, out float t) {
    vec2 ab = b - a;
    vec2 ap = p - a;
    float len2 = dot(ab, ab);
    if (len2 < 1.0) {
        t = 0.0;
        return distance(p, a);
    }
    t = dot(ap, ab) / len2;
    t = clamp(t, 0.0, 1.0);
    return distance(p, a + t * ab);
}

void main() {
    // Chuyển gl_FragCoord sang tọa độ canvas ảo
    vec2 pixelPos = vec2(gl_FragCoord.x, 720.0 - gl_FragCoord.y);

    vec2 ab = endPos - startPos;
    float len = length(ab);
    if (len < 1.0) {
        discard;
    }

    vec2 perp = normalize(vec2(-ab.y, ab.x));

    // --- 1. TIA SÉT CHÍNH (MAIN BOLT) ---
    float t_main;
    float dSegmentMain = distToSegment(pixelPos, startPos, endPos, t_main);
    
    // Taper factor: Giảm dần độ rộng về 0 ở hai đầu mút, phình ở giữa
    float fade = sin(t_main * 3.14159265);
    
    // Tạo sóng ziczac gồ ghề cho tia sét chính
    float mainNoise = ridgedFBM(t_main * 18.0 - time * 35.0) * 2.0 - 1.0;
    // Rung động biên độ ngẫu nhiên theo thời gian thực (vỡ không gian)
    float rumble = 1.0 + 0.15 * sin(time * 60.0);
    vec2 mainDisplacedPoint = startPos + t_main * ab + perp * mainNoise * 22.0 * fade * rumble;
    float distMain = distance(pixelPos, mainDisplacedPoint);

    // --- 2. TIA XOẮN PHỤ HỒNG NGOẠI (GHOST PLASMA FILAMENT) ---
    // Luồng năng lượng xoắn cuộn quanh tia chính tạo vẻ hỗn loạn điện trường
    float ghostNoise = ridgedFBM(t_main * 28.0 + time * 45.0 + 35.0) * 2.0 - 1.0;
    vec2 ghostDisplacedPoint = startPos + t_main * ab + perp * (mainNoise * 8.0 + ghostNoise * 15.0) * fade;
    float distGhost = distance(pixelPos, ghostDisplacedPoint);

    // --- 3. CÁC TIA SÉT NHÁNH PHỤ PHÂN TỎA (BRANCHES) ---
    float glowBranches = 0.0;
    float whiteCoreBranches = 0.0;

    // Nhánh 1: Tách ở t = 0.3, hướng lên trái
    {
        float t_b = 0.3;
        float startNoise = ridgedFBM(t_b * 18.0 - time * 35.0) * 2.0 - 1.0;
        vec2 bStart = startPos + t_b * ab + perp * startNoise * 22.0 * sin(t_b * 3.14159265);
        
        vec2 bDir = normalize(ab + perp * 0.82); 
        float bLen = len * 0.38;
        vec2 bEnd = bStart + bDir * bLen;
        
        float t_proj;
        float dSegmentBranch = distToSegment(pixelPos, bStart, bEnd, t_proj);
        float bFade = 1.0 - t_proj;
        
        float bNoise = ridgedFBM(t_proj * 24.0 + time * 30.0 + 12.3) * 2.0 - 1.0;
        vec2 bDisplaced = bStart + t_proj * bDir * bLen + normalize(vec2(-bDir.y, bDir.x)) * bNoise * 10.0 * bFade;
        
        float distBranch = distance(pixelPos, bDisplaced);
        glowBranches += exp(-distBranch * 0.12) * 0.85 * bFade * fade;
        whiteCoreBranches += smoothstep(1.2, 0.0, distBranch) * 0.6 * bFade * fade;
    }

    // Nhánh 2: Tách ở t = 0.62, hướng xuống phải
    {
        float t_b = 0.62;
        float startNoise = ridgedFBM(t_b * 18.0 - time * 35.0) * 2.0 - 1.0;
        vec2 bStart = startPos + t_b * ab + perp * startNoise * 22.0 * sin(t_b * 3.14159265);
        
        vec2 bDir = normalize(ab - perp * 0.95); 
        float bLen = len * 0.32;
        vec2 bEnd = bStart + bDir * bLen;
        
        float t_proj;
        float dSegmentBranch = distToSegment(pixelPos, bStart, bEnd, t_proj);
        float bFade = 1.0 - t_proj;
        
        float bNoise = ridgedFBM(t_proj * 20.0 - time * 28.0 - 45.6) * 2.0 - 1.0;
        vec2 bDisplaced = bStart + t_proj * bDir * bLen + normalize(vec2(-bDir.y, bDir.x)) * bNoise * 9.0 * bFade;
        
        float distBranch = distance(pixelPos, bDisplaced);
        glowBranches += exp(-distBranch * 0.15) * 0.75 * bFade * fade;
        whiteCoreBranches += smoothstep(1.1, 0.0, distBranch) * 0.55 * bFade * fade;
    }

    // Nhánh 3: Nhánh nhỏ tách ở t = 0.5, hướng lên trái
    {
        float t_b = 0.5;
        float startNoise = ridgedFBM(t_b * 18.0 - time * 35.0) * 2.0 - 1.0;
        vec2 bStart = startPos + t_b * ab + perp * startNoise * 22.0 * sin(t_b * 3.14159265);
        
        vec2 bDir = normalize(ab + perp * 0.45); 
        float bLen = len * 0.28;
        vec2 bEnd = bStart + bDir * bLen;
        
        float t_proj;
        float dSegmentBranch = distToSegment(pixelPos, bStart, bEnd, t_proj);
        float bFade = 1.0 - t_proj;
        
        float bNoise = ridgedFBM(t_proj * 26.0 + time * 33.0 + 88.9) * 2.0 - 1.0;
        vec2 bDisplaced = bStart + t_proj * bDir * bLen + normalize(vec2(-bDir.y, bDir.x)) * bNoise * 8.0 * bFade;
        
        float distBranch = distance(pixelPos, bDisplaced);
        glowBranches += exp(-distBranch * 0.18) * 0.65 * bFade * fade;
        whiteCoreBranches += smoothstep(1.0, 0.0, distBranch) * 0.5 * bFade * fade;
    }

    // --- 4. VIỀN HÀO QUANG VÀ CHUYỂN SẮC CHROMATIC CORONA GLOW ---
    // Mix màu từ Electric Cyan sang Neon Purple theo tỷ lệ ziczac và fade
    vec4 cyanGlow   = vec4(0.20, 0.85, 1.0, 1.0);
    vec4 purpleGlow = vec4(0.68, 0.32, 1.0, 1.0);
    vec4 electricCol = mix(cyanGlow, purpleGlow, sin(t_main * 6.0 + time * 5.0) * 0.5 + 0.5);

    // Phát quang rộng mờ ảo
    float glowMain = exp(-distMain * 0.032) * 0.65      // Quầng sáng mờ rất rộng
                   + exp(-distMain * 0.09) * 1.15       // Bao quanh vừa
                   + exp(-distMain * 0.48) * 2.4;       // Vùng lõi đặc

    float glowGhost = exp(-distGhost * 0.08) * 0.55 
                    + exp(-distGhost * 0.38) * 1.1;

    // Tổng hợp hào quang của tia chính và phụ
    float mainTotalGlow = (glowMain + glowGhost) * (0.35 + 0.65 * fade) * (0.8 + 0.25 * noise(time * 50.0));
    vec4 finalGlow = electricCol * mainTotalGlow + electricCol * glowBranches;

    // --- 5. LÕI SÉT SIÊU SÁNG (WHITE-HOT CORES) ---
    // Lõi trắng siêu đặc cho tia sét chính
    float thickness = 3.2 * fade;
    if (distMain < thickness) {
        finalGlow += vec4(1.0, 1.0, 1.0, 1.0) * (1.0 - distMain / thickness) * 2.0;
    }

    // Lõi sáng trắng cho tia xoắn phụ
    float ghostThickness = 1.35 * fade;
    if (distGhost < ghostThickness) {
        finalGlow += vec4(0.92, 0.96, 1.0, 1.0) * (1.0 - distGhost / ghostThickness) * 0.95;
    }

    // Lõi sáng trắng cho các nhánh phụ
    finalGlow += vec4(0.95, 0.97, 1.0, 1.0) * whiteCoreBranches;

    finalColor = finalGlow;
}
