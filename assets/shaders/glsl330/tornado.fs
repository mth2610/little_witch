#version 330

// Các biến Input/Output chuẩn của Raylib 2D pipeline
in vec2 fragTexCoord;
in vec4 fragColor;

uniform vec2 center;
uniform float radius;
uniform float time;
uniform float height;

out vec4 finalColor;

// =====================================================================
// KHU VỰC THAM SỐ CẤU HÌNH ĐÃ ĐIỀU CHỈNH CHUẨN ĐẸP
// =====================================================================
// 1. Cấu trúc hình dạng phễu
#define FUNNEL_BASE     1.00   // Độ rộng ở gốc (càng nhỏ gốc càng nhọn)
#define FUNNEL_TOP      2.00  // Độ loe ở ngọn (càng to ngọn càng rộng hoành tráng)
#define FUNNEL_WAVE     0.12   // Độ gợn sóng của vách (giữ nhỏ để tránh giống lửa)

// 2. Chuyển động xoáy và Vân cát
#define SPIN_SPEED      4.0    // Tốc độ xoay ly tâm của cả lốc và hạt bụi đất
#define VERTICAL_TWIST  6.5    // Độ xoắn dọc của dải vân (thấp = vân nằm ngang hợp hệ Thổ)
#define TUBE_BASE       6.0    // Độ dày dải vân cát ở gốc
#define TUBE_SCALE      15.0   // Độ nở rộng dải vân cát khi lên ngọn

// 3. Mật độ hiển thị (Cân bằng giữa Đậm đặc và Mờ nhạt)
#define ACCUM_DENSITY   0.05   // Hệ số tích lũy hạt (Tăng = đặc khối, Giảm = mờ ảo)
#define TURB_CONTRAST   3.0    // Lũy thừa tương phản (Tăng = xé sợi dải cát rõ hơn)
#define ALPHA_BOOST     1.25   // Hệ số kích đục Alpha (Giúp lốc nổi bật trên nền tối)

// 4. Mảnh vỡ đất đá xoáy (Debris)
#define ROCK_SPEED      4.0    // Tốc độ cuộn vút lên trên hành trình xoáy
#define ROCK_SIZE       0.4    // Kích thước / Mật độ phân bổ của đá vụn
#define ROCK_THRESH_MIN 0.89   // Giới hạn lọc cạnh sắc cho hạt đá
#define ROCK_THRESH_MAX 0.93   
// =====================================================================

float hash3D(vec3 p) {
    return fract(sin(dot(p, vec3(127.1, 311.7, 74.7))) * 43758.5453123);
}

float valNoise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(mix(hash3D(i + vec3(0.0,0.0,0.0)), hash3D(i + vec3(1.0,0.0,0.0)), f.x),
            mix(hash3D(i + vec3(0.0,1.0,0.0)), hash3D(i + vec3(1.0,1.0,0.0)), f.x), f.y),
        mix(mix(hash3D(i + vec3(0.0,0.0,1.0)), hash3D(i + vec3(1.0,0.0,1.0)), f.x),
            mix(hash3D(i + vec3(0.0,1.0,1.0)), hash3D(i + vec3(1.0,1.0,1.0)), f.x), f.y),
        f.z
    );
}

float fbm3D(vec3 p) {
    float v = 0.0;
    float a = 0.5;
    mat3 rot = mat3(0.00, 0.80, 0.60,
                    -0.80, 0.36, -0.48,
                    -0.60, -0.48, 0.64);
    for (int i = 0; i < 3; i++) {
        v += a * valNoise3D(p);
        p = rot * p * 2.3 + vec3(100.0);
        a *= 0.5;
    }
    return v;
}

mat2 Spin(float angle) {
    return mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
}

void main() {
    finalColor = vec4(0.0);

    // Trả về hệ tọa độ chuẩn của Raylib game (Y-down)
    vec2 pixelPos = vec2(gl_FragCoord.x, 720.0 - gl_FragCoord.y);
    vec2 delta = pixelPos - center;

    float yNorm = -delta.y / height;

    if (yNorm < -0.15 || yNorm > 1.15) {
        return;
    }

    float funnelNoise = valNoise3D(vec3(yNorm * 1.8 - time * 1.0, 0.0, 0.0));
    float funnelWidth = radius * (FUNNEL_BASE + yNorm * FUNNEL_TOP) * ((1.0 - FUNNEL_WAVE) + FUNNEL_WAVE * funnelNoise);

    float zLimit = funnelWidth * 1.35;

    const int SAMPLES = 15;
    float stepSize = (2.0 * zLimit) / 22.0;

    float zStart  = -zLimit * 0.95;
    float zSpread =  zLimit * 0.20;

    float accumulatedDensity = 0.0;
    float accumulatedRock = 0.0; 

    for (int i = 0; i < SAMPLES; i++) {
        float tIdx = (float(i) + 0.5) / float(SAMPLES);
        float z = zStart + tIdx * zSpread;
        vec3 q = vec3(delta.x, -delta.y, z);

        float zcurve = radius * ((FUNNEL_BASE - 0.03) + yNorm * (FUNNEL_TOP - 0.20));

        float angleNoise = (valNoise3D(vec3(q.y * 0.01, z * 0.01, time * 0.2)) - 0.5) * 1.0;
        float angle = -time * SPIN_SPEED + yNorm * VERTICAL_TWIST + angleNoise;

        // Thực hiện xoay ma trận không gian
        vec2 rotXZ = Spin(angle) * q.xz;

        vec3 noisePos = vec3(rotXZ * 0.15, q.y * 0.05 - time * 1.5); 
        float turbulence = fbm3D(noisePos);

        float lenXZ = length(q.xz);
        float tubeThickness = TUBE_BASE + TUBE_SCALE * yNorm;

        float radiusNoise = (valNoise3D(vec3(rotXZ * 0.04, time * 0.2)) - 0.5) * radius * 0.12;

        float distToTube = abs(lenXZ - (zcurve + radiusNoise));

        float density = exp(-distToTube * distToTube / (2.0 * tubeThickness * tubeThickness));
        
        float sharpTurbulence = smoothstep(0.2, 0.7, turbulence);
        density *= (0.35 + 0.65 * pow(sharpTurbulence, TURB_CONTRAST)); 

        // ĐỒNG BỘ: Mảnh vỡ tính toán dựa trên rotXZ giúp chúng xoáy ly tâm theo cơn lốc
        float rNoise = valNoise3D(vec3(rotXZ * ROCK_SIZE, q.y * 0.06 - time * ROCK_SPEED));
        float rockEnvelope = exp(-distToTube * distToTube / (2.0 * (tubeThickness * 1.35) * (tubeThickness * 1.35)));
        float debris = smoothstep(ROCK_THRESH_MIN, ROCK_THRESH_MAX, rNoise) * rockEnvelope;

        accumulatedDensity += density * stepSize * (ACCUM_DENSITY * 3.0 / float(SAMPLES));
        accumulatedRock += debris * stepSize * (0.35 * 3.0 / float(SAMPLES));
    }

    // ===== BẢNG MÀU ĐẤT ĐÁ HỆ THỔ =====
    vec3 outerEarth = vec3(0.32, 0.18, 0.06); 
    vec3 midEarth   = vec3(0.92, 0.55, 0.15); 
    vec3 coreGold   = vec3(1.00, 0.88, 0.50); 

    vec3 finalRGB = outerEarth * accumulatedDensity;
    finalRGB += midEarth * (accumulatedDensity * accumulatedDensity * 0.85);
    finalRGB += coreGold * pow(clamp(accumulatedDensity, 0.0, 1.0), 4.2) * 1.6; 
    finalRGB *= 4.5;

    // Hiệu ứng Rim-light bắt sáng viền rìa phễu
    float rim = smoothstep(funnelWidth * 0.5, funnelWidth * 1.1, abs(delta.x));
    finalRGB += vec3(1.0, 0.70, 0.30) * (1.0 - rim) * accumulatedDensity * 1.3;

    // Cộng các mảnh bụi đá xoáy vàng kim bắt sáng vào hệ thống màu
    finalRGB += vec3(1.0, 0.85, 0.55) * accumulatedRock * 5.5;

    // ===== MẶT NẠ BIÊN =====
    float topFade = smoothstep(1.12, 0.90, yNorm);
    float bottomFade = smoothstep(-0.12, 0.08, yNorm);
    float edgeFade = smoothstep(funnelWidth * 1.55, funnelWidth * 0.85, abs(delta.x));
    float mask = topFade * bottomFade * mix(1.0, edgeFade, 0.6);

    // Alpha tổng hợp hòa trộn mượt mà cả thân lốc lẫn vụn đá ly tâm sát vách
    float alpha = (accumulatedDensity + accumulatedRock * 0.25) * ALPHA_BOOST * mask;
    alpha = clamp(alpha, 0.0, 0.96);

    finalColor = vec4(finalRGB * alpha, alpha);
}