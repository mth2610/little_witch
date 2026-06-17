#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform vec2 startPos; // Start position in virtual coordinates (1280x720)
uniform vec2 endPos;   // End position in virtual coordinates
uniform float time;
uniform vec4 glowColor; // Base color of the lightning

// Output fragment color
out vec4 finalColor;

// Simple pseudo-random noise
float hash(float n) { return fract(sin(n) * 43758.5453123); }
float noise(float x) {
    float i = floor(x);
    float f = fract(x);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), u);
}

// 5-octave Ridged FBM for angular, jagged lightning arcs
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

// Distance from pixel to line segment helper
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
    // Convert to virtual screen coordinates
    vec2 pixelPos = vec2(gl_FragCoord.x, 720.0 - gl_FragCoord.y);

    vec2 ab = endPos - startPos;
    float len = length(ab);
    if (len < 1.0) {
        discard;
    }

    vec2 perp = normalize(vec2(-ab.y, ab.x));

    // --- 1. MAIN BOLT ---
    float t_main;
    float dSegmentMain = distToSegment(pixelPos, startPos, endPos, t_main);
    
    // Taper factor: 0 at endpoints, 1 in the middle
    float fade = sin(t_main * 3.14159265);
    
    // Jagged displacement of the main bolt using Ridged FBM
    float mainNoise = ridgedFBM(t_main * 15.0 - time * 28.0) * 2.0 - 1.0;
    vec2 mainDisplacedPoint = startPos + t_main * ab + perp * mainNoise * 18.0 * fade;
    float distMain = distance(pixelPos, mainDisplacedPoint);

    // --- 2. GHOST PLASMA FILAMENT ---
    // A secondary, thinner, highly chaotic arc winding around the main bolt
    float ghostNoise = ridgedFBM(t_main * 25.0 + time * 32.0 + 50.0) * 2.0 - 1.0;
    vec2 ghostDisplacedPoint = startPos + t_main * ab + perp * (mainNoise * 6.0 + ghostNoise * 12.0) * fade;
    float distGhost = distance(pixelPos, ghostDisplacedPoint);

    // --- 3. PROCEDURAL BRANCHES (FORKS) ---
    float glowBranches = 0.0;
    float whiteCoreBranches = 0.0;

    // Branch 1: splits off at t = 0.35, goes left/up
    {
        float t_b = 0.35;
        float startNoise = ridgedFBM(t_b * 15.0 - time * 28.0) * 2.0 - 1.0;
        vec2 bStart = startPos + t_b * ab + perp * startNoise * 18.0 * sin(t_b * 3.14159265);
        
        vec2 bDir = normalize(ab + perp * 0.75); 
        float bLen = len * 0.35;
        vec2 bEnd = bStart + bDir * bLen;
        
        float t_proj;
        float dSegmentBranch = distToSegment(pixelPos, bStart, bEnd, t_proj);
        float bFade = 1.0 - t_proj;
        
        float bNoise = ridgedFBM(t_proj * 20.0 + time * 25.0 + 12.3) * 2.0 - 1.0;
        vec2 bDisplaced = bStart + t_proj * bDir * bLen + normalize(vec2(-bDir.y, bDir.x)) * bNoise * 8.0 * bFade;
        
        float distBranch = distance(pixelPos, bDisplaced);
        glowBranches += exp(-distBranch * 0.15) * 0.7 * bFade;
        whiteCoreBranches += smoothstep(1.0, 0.0, distBranch) * 0.5 * bFade;
    }

    // Branch 2: splits off at t = 0.65, goes right/down
    {
        float t_b = 0.65;
        float startNoise = ridgedFBM(t_b * 15.0 - time * 28.0) * 2.0 - 1.0;
        vec2 bStart = startPos + t_b * ab + perp * startNoise * 18.0 * sin(t_b * 3.14159265);
        
        vec2 bDir = normalize(ab - perp * 0.85); 
        float bLen = len * 0.3;
        vec2 bEnd = bStart + bDir * bLen;
        
        float t_proj;
        float dSegmentBranch = distToSegment(pixelPos, bStart, bEnd, t_proj);
        float bFade = 1.0 - t_proj;
        
        float bNoise = ridgedFBM(t_proj * 18.0 - time * 22.0 - 45.6) * 2.0 - 1.0;
        vec2 bDisplaced = bStart + t_proj * bDir * bLen + normalize(vec2(-bDir.y, bDir.x)) * bNoise * 7.0 * bFade;
        
        float distBranch = distance(pixelPos, bDisplaced);
        glowBranches += exp(-distBranch * 0.18) * 0.6 * bFade;
        whiteCoreBranches += smoothstep(0.9, 0.0, distBranch) * 0.45 * bFade;
    }

    // Branch 3: splits off at t = 0.48, goes left/down
    {
        float t_b = 0.48;
        float startNoise = ridgedFBM(t_b * 15.0 - time * 28.0) * 2.0 - 1.0;
        vec2 bStart = startPos + t_b * ab + perp * startNoise * 18.0 * sin(t_b * 3.14159265);
        
        vec2 bDir = normalize(ab - perp * 0.35); 
        float bLen = len * 0.25;
        vec2 bEnd = bStart + bDir * bLen;
        
        float t_proj;
        float dSegmentBranch = distToSegment(pixelPos, bStart, bEnd, t_proj);
        float bFade = 1.0 - t_proj;
        
        float bNoise = ridgedFBM(t_proj * 22.0 + time * 30.0 + 88.9) * 2.0 - 1.0;
        vec2 bDisplaced = bStart + t_proj * bDir * bLen + normalize(vec2(-bDir.y, bDir.x)) * bNoise * 6.0 * bFade;
        
        float distBranch = distance(pixelPos, bDisplaced);
        glowBranches += exp(-distBranch * 0.2) * 0.5 * bFade;
        whiteCoreBranches += smoothstep(0.8, 0.0, distBranch) * 0.4 * bFade;
    }

    // --- 4. GLOW FIELDS (GRADIENTS) ---
    // Wide multi-tone electric glow
    vec4 violetGlow = vec4(glowColor.r * 1.2, glowColor.g * 0.6, glowColor.b * 1.4, 1.0);
    vec4 richGlowColor = mix(glowColor, violetGlow, fade * 0.4);

    float glowMain = exp(-distMain * 0.04) * 0.55       // Wide misty purple aura
                   + exp(-distMain * 0.12) * 1.0        // Medium indigo envelope
                   + exp(-distMain * 0.55) * 2.0;       // Bright blue core envelope

    float glowGhost = exp(-distGhost * 0.1) * 0.45 
                    + exp(-distGhost * 0.45) * 0.95;

    float mainTotalGlow = (glowMain + glowGhost) * (0.35 + 0.65 * fade);
    vec4 finalGlow = richGlowColor * mainTotalGlow + richGlowColor * glowBranches * fade;

    // White-hot core for the main bolt
    float thickness = 2.4 * fade;
    if (distMain < thickness) {
        finalGlow += vec4(1.0, 1.0, 1.0, 1.0) * (1.0 - distMain / thickness) * 1.7;
    }

    // White core for the ghost filament
    float ghostThickness = 1.1 * fade;
    if (distGhost < ghostThickness) {
        finalGlow += vec4(0.9, 0.95, 1.0, 1.0) * (1.0 - distGhost / ghostThickness) * 0.8;
    }

    // White core for branches
    finalGlow += vec4(0.95, 0.95, 1.0, 1.0) * whiteCoreBranches * fade;

    finalColor = finalGlow;
}
