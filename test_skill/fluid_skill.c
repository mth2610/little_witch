#include "fluid_skill.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>  // BỔ SUNG DÒNG NÀY ĐỂ C NĂM ĐƯỢC "NULL" LÀ GÌ

#define PARTICLES_PER_SECOND 225.0f // Đã chia đôi vì giờ mỗi tia tự spawn hạt riêng
#define SPAWN_INTERVAL (1.0f / PARTICLES_PER_SECOND)
#define MAX_PARTICLES 100000 
#define MAX_EMITTERS 10 // Hỗ trợ tối đa 10 tia nước cùng lúc

// Cấu trúc Nguồn phát tia
typedef struct {
    bool active;
    Vector2 startPos;
    Vector2 targetPos;
    Vector2 p1;
    Vector2 p2;
    float progress;
    float durationTimer;
    float spawnAccumulator;
    float twistPhase;
} StreamEmitter;

// Cấu trúc Hạt nước độc lập
typedef struct {
    Vector2 position;
    Vector2 velocity;    
    Vector2 startPos, p1, p2, targetPos; // Lưu trữ quỹ đạo cá nhân
    float progress;      
    float speed;         
    float radius;
    float lifetime;      
    float maxLifetime;
    float spreadOffset;   
    float twistPhase;
    int type;            // 0: Stream, 2: Splash
    bool active;
} WaterParticle;

static WaterParticle waterPool[MAX_PARTICLES];
static StreamEmitter emitters[MAX_EMITTERS];
static int lastUsedParticle = 0;

static RenderTexture2D canvasTexture;
static Shader fluidShader;
static int timeLoc;

static Vector2 GetBezierPoint(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float t) {
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    Vector2 p = Vector2Scale(p0, uuu);
    p = Vector2Add(p, Vector2Scale(p1, 3.0f * uu * t));
    p = Vector2Add(p, Vector2Scale(p2, 3.0f * u * tt));
    p = Vector2Add(p, Vector2Scale(p3, ttt));
    return p;
}

static void SpawnParticle(int type, Vector2 pos, Vector2 vel, float speed, float progress, float maxLife, float baseRadius, StreamEmitter* em) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        int index = (lastUsedParticle + i) % MAX_PARTICLES;
        if (!waterPool[index].active) {
            waterPool[index].type = type;
            waterPool[index].position = pos;
            waterPool[index].velocity = vel;
            waterPool[index].progress = progress; 
            waterPool[index].speed = speed; 
            waterPool[index].radius = baseRadius;
            waterPool[index].maxLifetime = maxLife; 
            waterPool[index].lifetime = maxLife; 
            waterPool[index].spreadOffset = (float)GetRandomValue(-15, 15);
            waterPool[index].active = true;
            
            // Nếu hạt sinh ra từ luồng, copy quỹ đạo của luồng cho nó tự bay
            if (em != NULL) {
                waterPool[index].startPos = em->startPos;
                waterPool[index].targetPos = em->targetPos;
                waterPool[index].p1 = em->p1;
                waterPool[index].p2 = em->p2;
                waterPool[index].twistPhase = em->twistPhase;
            }
            
            lastUsedParticle = (index + 1) % MAX_PARTICLES;
            break;
        }
    }
}

void InitFluidSkill(int screenWidth, int screenHeight) {
    canvasTexture = LoadRenderTexture(screenWidth, screenHeight);
    fluidShader = LoadShader(0, "fluid.fs");
    timeLoc = GetShaderLocation(fluidShader, "u_time"); 
    for (int i = 0; i < MAX_PARTICLES; i++) waterPool[i].active = false;
    for (int i = 0; i < MAX_EMITTERS; i++) emitters[i].active = false;
}

// Bác gọi hàm này 1 lần thì bắn 1 tia, gọi n lần thì n tia bay ra.
void CastFluidSkill(Vector2 startPos, Vector2 target, float twistPhase) {
    for (int i = 0; i < MAX_EMITTERS; i++) {
        if (!emitters[i].active) {
            emitters[i].active = true;
            emitters[i].startPos = startPos;
            emitters[i].targetPos = target;
            emitters[i].progress = 0.0f;
            emitters[i].spawnAccumulator = 0.0f;
            emitters[i].durationTimer = 0.8f;
            emitters[i].twistPhase = twistPhase;

            float midX = (startPos.x + target.x) / 2.0f;
            // Dựa vào góc pha để uốn cong quỹ đạo ra 2 phía đối lập (âm/dương)
            float curveSign = (twistPhase == 0.0f) ? -1.0f : 1.0f;
            
            emitters[i].p1 = (Vector2){ midX + curveSign * 100.0f, startPos.y - curveSign * (float)GetRandomValue(150, 250) };
            emitters[i].p2 = (Vector2){ midX - curveSign * 100.0f, target.y + curveSign * (float)GetRandomValue(150, 250) };
            break; // Tìm được 1 slot trống để bắn là thoát
        }
    }

    // Nổ bọt nước ở tay (Muzzle Flash)
    int burstCount = GetRandomValue(15, 25);
    for(int s = 0; s < burstCount; s++) {
        Vector2 burstVel = {
            (float)GetRandomValue(-150, 250), // Cho văng rộng hơn
            (float)GetRandomValue(-250, 250) 
        };
        float burstRad = (float)GetRandomValue(8, 20); // Tăng bán kính hạt
        float burstLife = (float)GetRandomValue(3, 8) / 10.0f;
        SpawnParticle(2, startPos, burstVel, 0, 0, burstLife, burstRad, NULL);
    }
}

void UpdateFluidSkill(float dt) {
    float time = GetTime();

    // 1. CẬP NHẬT CÁC ĐẦU PHUN NƯỚC (EMITTERS)
    for (int e = 0; e < MAX_EMITTERS; e++) {
        if (!emitters[e].active) continue;

        float commonSpeed = 1.5f; 
        if (emitters[e].progress < 1.0f) {
            emitters[e].progress += dt * commonSpeed; 
            if (emitters[e].progress > 1.0f) emitters[e].progress = 1.0f;
        }

        emitters[e].durationTimer -= dt;
        if (emitters[e].durationTimer <= 0.0f) {
            emitters[e].active = false; // Tắt vòi phun
            continue;
        }
        
        emitters[e].spawnAccumulator += dt;
        int maxSpawnsThisFrame = 30; 
        while (emitters[e].spawnAccumulator >= SPAWN_INTERVAL && maxSpawnsThisFrame-- > 0) {
            Vector2 zeroVel = {0, 0};
            float baseRad = (float)GetRandomValue(25, 45); 
            SpawnParticle(0, emitters[e].startPos, zeroVel, commonSpeed, 0.0f, 1.2f, baseRad, &emitters[e]); 
            emitters[e].spawnAccumulator -= SPAWN_INTERVAL;
        }
    }

    // 2. CẬP NHẬT CÁC HẠT NƯỚC (Bây giờ chúng tự trị hoàn toàn)
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!waterPool[i].active) continue;

        waterPool[i].lifetime -= dt;

        if (waterPool[i].lifetime <= 0.0f) {
            waterPool[i].active = false;
            continue;
        }

        if (waterPool[i].type == 2) { // Splash
            waterPool[i].position.x += waterPool[i].velocity.x * dt;
            waterPool[i].position.y += waterPool[i].velocity.y * dt;
            waterPool[i].velocity.y += 800.0f * dt; 
            continue;
        }

        // Stream
        waterPool[i].progress += waterPool[i].speed * dt;

        if (waterPool[i].progress >= 1.0f) {
            waterPool[i].active = false;
            int splashCount = GetRandomValue(3, 6);
            for(int s = 0; s < splashCount; s++) {
                Vector2 splashVel = {
                    (float)GetRandomValue(-250, 200),
                    (float)GetRandomValue(-400, 50)
                };
                float splashRad = (float)GetRandomValue(5, 20);
                float splashLife = (float)GetRandomValue(4, 10) / 10.0f;
                SpawnParticle(2, waterPool[i].position, splashVel, 0, 0, splashLife, splashRad, NULL);
            }
            continue;
        }

        Vector2 basePos = GetBezierPoint(
            waterPool[i].startPos, 
            waterPool[i].p1, 
            waterPool[i].p2, 
            waterPool[i].targetPos, 
            waterPool[i].progress
        );

        float focusFactor = 1.0f - powf(waterPool[i].progress, 4.0f); 
        float waveFreq = waterPool[i].progress * 15.0f - time * 25.0f; 
        
        // Cộng thêm twistPhase của riêng tia đó để pha sóng không bị trùng nhau
        float noiseX = sinf(waveFreq + waterPool[i].spreadOffset + waterPool[i].twistPhase) * (15.0f * focusFactor); 
        float noiseY = cosf(waveFreq * 1.2f - waterPool[i].spreadOffset + waterPool[i].twistPhase) * (15.0f * focusFactor);

        waterPool[i].position.x = basePos.x + noiseX;
        waterPool[i].position.y = basePos.y + noiseY;

     // Xé gió rớt lại bụi nước
        if (GetRandomValue(1, 100) <= 8) { 
            // 1. Tính hướng bay tới trước (đà của luồng chưởng)
            Vector2 forwardDir = Vector2Normalize(Vector2Subtract(waterPool[i].targetPos, waterPool[i].position));
            
            // 2. Vận tốc = Quán tính bay tới (lực mạnh) + Trọng lượng rơi xuống (lực yếu) + Nhiễu loạn
            Vector2 dropVel = {
                (forwardDir.x * GetRandomValue(150, 300)) + GetRandomValue(-30, 30),
                (forwardDir.y * GetRandomValue(150, 300)) + GetRandomValue(50, 150) 
            };
            
            float mistRad = (float)GetRandomValue(2, 5); 
            float mistLife = (float)GetRandomValue(2, 4) / 10.0f; 
            
            SpawnParticle(2, waterPool[i].position, dropVel, 0, 0, mistLife, mistRad, NULL);
        }
    }
}

void DrawFluidSkill(void) {
    float time = GetTime();

    BeginTextureMode(canvasTexture);
        ClearBackground(BLANK); 
        BeginBlendMode(BLEND_ALPHA);
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (!waterPool[i].active) continue;

                float currentRadius = waterPool[i].radius;
                float currentAlpha = 0.6f;
                float lifeRatio = waterPool[i].lifetime / waterPool[i].maxLifetime;

                if (waterPool[i].type < 2) {
                    float bulge = sinf(waterPool[i].progress * PI); 
                    float sizeScale = 0.3f + (0.7f * bulge); 
                    float fadeScale = (lifeRatio < 0.3f) ? (lifeRatio / 0.3f) : 1.0f;
                    currentRadius *= (sizeScale * fadeScale);
                    currentAlpha *= fadeScale; 
                } else {
                    currentRadius *= lifeRatio;
                    // FIX LỖI TÀNG HÌNH: Ép Alpha gốc lên 1.0 để dù đứng 1 mình, hạt vẫn sáng rực
                    currentAlpha = lifeRatio * 1.0f; 
                }

                DrawCircleGradient((int)waterPool[i].position.x, (int)waterPool[i].position.y, currentRadius, ColorAlpha(WHITE, currentAlpha), BLANK);
            }
        EndBlendMode();
    EndTextureMode();

    SetShaderValue(fluidShader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(fluidShader);
        DrawTextureRec(canvasTexture.texture, (Rectangle){ 0, 0, (float)canvasTexture.texture.width, (float)-canvasTexture.texture.height }, (Vector2){ 0, 0 }, WHITE);
    EndShaderMode();
}

void UnloadFluidSkill(void) {
    UnloadShader(fluidShader);
    UnloadRenderTexture(canvasTexture);
}