#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform float u_time; 

out vec4 finalColor;

// 2D Pseudo-Random Noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// 2D Value Noise
float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i + vec2(0.0,0.0)), hash(i + vec2(1.0,0.0)), u.x),
               mix(hash(i + vec2(0.0,1.0)), hash(i + vec2(1.0,1.0)), u.x), u.y);
}

// 2-Octave FBM
float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.5));
    for (int i = 0; i < 2; ++i) {
        v += a * noise(p);
        p = rot * p * 2.0;
        a *= 0.5;
    }
    return v;
}

void main() {
    // 1. UV warping for organic movement (wind blowing / growth pulse)
    vec2 warpUV = fragTexCoord;
    vec2 flow = vec2(u_time * 0.4, -u_time * 0.6);
    float nDistort = fbm(fragTexCoord * 8.0 + flow);
    warpUV.x += (nDistort - 0.5) * 0.006;
    warpUV.y += (nDistort - 0.5) * 0.006;

    vec4 texColor = texture(texture0, warpUV);
    
    // texColor.a represents the interpolated opacity / fade
    if (texColor.a < 0.01) {
        discard;
    }

    float localU = texColor.r;
    float localV = texColor.g;
    float elementType = texColor.b;
    
    vec3 finalRGB = vec3(0.0);
    float finalAlpha = 0.0;

    // --- 1. RỄ CÂY / THÂN DÂY LEO (elementType < 0.2) ---
    if (elementType < 0.2) {
        // Disintegrate wood/bark crumbling
        if (texColor.a < 0.99) {
            float barkNoise = noise(vec2(localU * 40.0, localV * 50.0));
            if (barkNoise * 0.95 > texColor.a) {
                discard;
            }
        }

        // Màu vỏ cây: Nâu đậm -> Nâu ấm -> Xanh rêu
        vec3 darkBark  = vec3(0.12, 0.07, 0.03);
        vec3 midBark   = vec3(0.28, 0.17, 0.08);
        vec3 mossGreen = vec3(0.15, 0.32, 0.12);
        
        // 3D Cylinder normal shading (peak at center localV = 0.5)
        float normal = sin(localV * 3.14159265);
        float shading = 0.35 + 0.65 * normal;
        
        vec3 barkColor = mix(darkBark, midBark, normal);
        // moss patches based on noise
        barkColor = mix(barkColor, mossGreen, noise(vec2(localU * 6.0, localV * 3.0)) * 0.35);
        
        // Wither bark color (shift to charcoal/dry wood grey)
        vec3 dryWoodColor = vec3(0.09, 0.07, 0.05);
        barkColor = mix(dryWoodColor, barkColor, smoothstep(0.4, 0.9, texColor.a));

        // Vân dọc thớ gỗ chạy dọc thân rễ
        vec2 barkUV = vec2(localU * 12.0, localV * 2.5);
        float barkTexture = noise(barkUV + noise(barkUV * 1.5) * 0.3) * 0.22;
        barkColor += vec3(barkTexture * (1.0 - normal * 0.3));
        
        // Dòng nhựa ma thuật phát sáng dọc lõi rễ (V = 0.5)
        float distToCenter = abs(localV - 0.5) * 2.0;
        float sapLine = smoothstep(0.22, 0.0, distToCenter);
        float sapPulse = sin(localU * 22.0 - u_time * 6.0) * 0.5 + 0.5;
        float sapStrength = smoothstep(0.0, 0.3, texColor.a);
        vec3 sapColor = vec3(0.1, 0.95, 0.3) * sapLine * sapPulse * 0.85 * sapStrength;
        
        barkColor += sapColor;
        barkColor += vec3(0.1, 0.25, 0.1) * sapLine * 0.45 * sapStrength; // sap glow bleed
        
        // Specular highlight along center cylinder
        barkColor += vec3(0.15, 0.25, 0.15) * pow(normal, 6.0) * sapStrength;
        
        finalRGB = barkColor * shading;
        finalAlpha = texColor.a;
    }
    // --- 2. LÁ CÂY (0.2 <= elementType < 0.7) ---
    else if (elementType < 0.7) {
        // Disintegrate leaf crumbling
        if (texColor.a < 0.99) {
            float leafNoise = noise(vec2(localU * 60.0, localV * 60.0));
            if (leafNoise * 0.95 > texColor.a) {
                discard;
            }
        }

        // Shapes boundary using sine width envelope (taper at leaf base/tip)
        float leafWidth = sin(localU * 3.14159265) * 0.5;
        float d = abs(localV - 0.5);
        float edgeAlpha = smoothstep(leafWidth, leafWidth - 0.06, d);
        
        if (edgeAlpha < 0.01) {
            discard;
        }

        // Màu lá cây: Xanh đậm -> Xanh lục tươi -> Vàng chanh
        vec3 darkLeaf      = vec3(0.03, 0.18, 0.06);
        vec3 brightLeaf    = vec3(0.10, 0.60, 0.18);
        vec3 highlightLeaf = vec3(0.60, 0.90, 0.20);
        
        vec3 leafColor = mix(darkLeaf, brightLeaf, localU);
        leafColor = mix(leafColor, highlightLeaf, (1.0 - d / leafWidth) * 0.3);
        
        // Wither leaf color (shift to dry yellow/brown)
        vec3 dryLeafColor = vec3(0.18, 0.12, 0.05);
        leafColor = mix(dryLeafColor, leafColor, smoothstep(0.4, 0.9, texColor.a));

        // Gân lá chính và gân phụ chéo
        float centralVein = smoothstep(0.03, 0.0, d);
        float sideVein = sin((localU - abs(localV - 0.5) * 0.3) * 28.0) * 0.5 + 0.5;
        sideVein = smoothstep(0.88, 0.96, sideVein) * smoothstep(0.05, 0.1, localU) * edgeAlpha;
        
        leafColor = mix(leafColor, vec3(0.65, 0.95, 0.35), centralVein * 0.45);
        leafColor = mix(leafColor, vec3(0.55, 0.90, 0.28), sideVein * 0.35);
        
        // Hào quang lá cây phát sáng
        float sapStrength = smoothstep(0.0, 0.3, texColor.a);
        leafColor += vec3(0.08, 0.22, 0.08) * edgeAlpha * sapStrength;

        finalRGB = leafColor;
        finalAlpha = edgeAlpha * texColor.a;
    }
    // --- 3. HOA / PHẤN HOA MA THUẬT (elementType >= 0.7) ---
    else {
        // Disintegrate flower crumbling
        if (texColor.a < 0.99) {
            float flowerNoise = noise(vec2(localU * 70.0, localV * 70.0));
            if (flowerNoise * 0.95 > texColor.a) {
                discard;
            }
        }

        // Radial math relative to center (0.5, 0.5)
        vec2 toCenter = vec2(localU, localV) - vec2(0.5);
        float dist = length(toCenter) * 2.0;
        float angle = atan(toCenter.y, toCenter.x);
        
        // 5-Petal shape envelope
        float petals = cos(angle * 5.0) * 0.25 + 0.75;
        float flowerEdge = smoothstep(petals, petals - 0.05, dist);
        
        if (flowerEdge < 0.01) {
            discard;
        }

        // Hoa ma thuật: Vàng hoàng kim ấm áp -> Lõi trắng
        vec3 bloomOuter = vec3(0.95, 0.45, 0.0);
        vec3 bloomMid   = vec3(0.98, 0.85, 0.1);
        vec3 bloomCore  = vec3(1.0, 1.0, 0.95);
        
        vec3 flowerColor = mix(bloomMid, bloomOuter, dist);
        flowerColor = mix(bloomCore, flowerColor, smoothstep(0.08, 0.35, dist));
        
        // Wither flower color (shift to dry brown/grey)
        vec3 dryFlowerColor = vec3(0.15, 0.10, 0.08);
        flowerColor = mix(dryFlowerColor, flowerColor, smoothstep(0.4, 0.9, texColor.a));

        // Radial petal lines
        float petalLines = sin(angle * 15.0) * 0.12 * (1.0 - dist);
        flowerColor += petalLines;

        // Glowing core
        float sapStrength = smoothstep(0.0, 0.3, texColor.a);
        float coreGlow = smoothstep(0.2, 0.0, dist) * 0.7;
        flowerColor += vec3(1.0, 1.0, 0.85) * coreGlow * sapStrength;

        // Soft bloom glow
        flowerColor += vec3(0.35, 0.2, 0.0) * (1.0 - dist) * 0.45 * sapStrength;

        finalRGB = flowerColor;
        finalAlpha = flowerEdge * texColor.a;
    }

    finalColor = vec4(finalRGB, clamp(finalAlpha, 0.0, 1.0));
}

