#include "metal_skill.h"
#include "raymath.h"
#include <stddef.h>

#define MAX_METAL_PARTICLES 15000 

typedef struct {
    Vector2 position;
    Vector2 velocity;
    Vector2 target;     
    float length;       
    float thickness;    
    float lifetime;
    float maxLifetime;
    int type;           
    bool active;
} MetalParticle;

static MetalParticle metalPool[MAX_METAL_PARTICLES];
static int lastUsedIndex = 0;

static RenderTexture2D metalCanvas;
static Shader metalShader;
static int timeLocMetal;
static Texture2D swordSprite; // Khai báo biến chứa ảnh kiếm

static void SpawnMetal(int type, Vector2 pos, Vector2 vel, float len, float thick, float life, Vector2 target) {
    for (int i = 0; i < MAX_METAL_PARTICLES; i++) {
        int index = (lastUsedIndex + i) % MAX_METAL_PARTICLES;
        if (!metalPool[index].active) {
            metalPool[index].type = type;
            metalPool[index].position = pos;
            metalPool[index].velocity = vel;
            metalPool[index].target = target;
            metalPool[index].length = len;
            metalPool[index].thickness = thick;
            metalPool[index].lifetime = life;
            metalPool[index].maxLifetime = life;
            metalPool[index].active = true;
            
            lastUsedIndex = (index + 1) % MAX_METAL_PARTICLES;
            break;
        }
    }
}

void InitMetalSkill(int screenWidth, int screenHeight) {
    metalCanvas = LoadRenderTexture(screenWidth, screenHeight);
    metalShader = LoadShader(0, "metal.fs");
    timeLocMetal = GetShaderLocation(metalShader, "u_time");
    
    // Tải ảnh thanh kiếm (Nhớ để file sword.png cùng chỗ với file chạy)
    // LƯU Ý: Ảnh mũi kiếm phải hướng sang phải (Góc 0 độ)
    swordSprite = LoadTexture("sword.png"); 
    
    for (int i = 0; i < MAX_METAL_PARTICLES; i++) {
        metalPool[i].active = false;
    }
}

void CastMetalSkill(Vector2 startPos, Vector2 target, int count) {
    Vector2 baseDir = Vector2Normalize(Vector2Subtract(target, startPos));
    Vector2 zeroTarget = {0, 0};
    
    // Nổ luồng khí kim loại tại tay (Muzzle Flash)
    for (int i = 0; i < 20; i++) {
        Vector2 flashVel = {
            baseDir.x * GetRandomValue(200, 500) + GetRandomValue(-200, 200),
            baseDir.y * GetRandomValue(200, 500) + GetRandomValue(-200, 200)
        };
        SpawnMetal(1, startPos, flashVel, 15.0f, (float)GetRandomValue(4, 10), 0.25f, zeroTarget);
    }

    // Phóng Phi Kiếm (Tản rộng như cánh quạt rồi mới gom lại)
    for (int i = 0; i < count; i++) {
        float angleOffset = (float)GetRandomValue(-70, 70) * DEG2RAD; 
        Vector2 dir = {
            baseDir.x * cosf(angleOffset) - baseDir.y * sinf(angleOffset),
            baseDir.x * sinf(angleOffset) + baseDir.y * cosf(angleOffset)
        };
        
        float speed = (float)GetRandomValue(1000, 1800);
        Vector2 vel = Vector2Scale(dir, speed);
        
        // Cấp độ dài 80-120, độ dày 15-25 để scale thanh kiếm
        SpawnMetal(0, startPos, vel, (float)GetRandomValue(80, 120), (float)GetRandomValue(15, 25), 1.5f, target);
    }
}

void UpdateMetalSkill(float dt) {
    Vector2 zeroTarget = {0, 0};

    for (int i = 0; i < MAX_METAL_PARTICLES; i++) {
        if (!metalPool[i].active) continue;

        metalPool[i].lifetime -= dt;
        if (metalPool[i].lifetime <= 0.0f) {
            metalPool[i].active = false;
            continue;
        }

        metalPool[i].position.x += metalPool[i].velocity.x * dt;
        metalPool[i].position.y += metalPool[i].velocity.y * dt;

        if (metalPool[i].type == 0) { 
            Vector2 toTarget = Vector2Subtract(metalPool[i].target, metalPool[i].position);
            float distToTarget = Vector2Length(toTarget);

            if (distToTarget > 20.0f) {
                Vector2 desiredDir = Vector2Normalize(toTarget);
                float currentSpeed = Vector2Length(metalPool[i].velocity);
                Vector2 desiredVel = Vector2Scale(desiredDir, currentSpeed + 1500.0f * dt); 
                metalPool[i].velocity = Vector2Lerp(metalPool[i].velocity, desiredVel, dt * 6.0f); 
            }

            // TIA LỬA ĐIỆN XẸT XUNG QUANH (Không dùng làm đuôi chính nữa)
            if (GetRandomValue(1, 100) <= 50) { 
                Vector2 sparkVel = {
                    metalPool[i].velocity.x * 0.3f + GetRandomValue(-300, 300), // Văng tản ra hai bên
                    metalPool[i].velocity.y * 0.3f + GetRandomValue(-300, 300)
                };
                // Tia lửa điện rất mỏng (dày 1-3) và ngắn
                SpawnMetal(1, metalPool[i].position, sparkVel, (float)GetRandomValue(10, 25), GetRandomValue(1, 3), 0.2f, zeroTarget);
            }
            
            if (distToTarget < 30.0f || metalPool[i].lifetime < 0.1f) {
                metalPool[i].active = false; 
                
                int shardCount = GetRandomValue(30, 50);
                for (int s = 0; s < shardCount; s++) {
                    Vector2 shardVel = {
                        GetRandomValue(-1500, 1500), 
                        GetRandomValue(-1500, 1500)
                    };
                    SpawnMetal(2, metalPool[i].position, shardVel, (float)GetRandomValue(10, 30), GetRandomValue(2, 6), 0.35f, zeroTarget);
                }
            }
        } 
        else if (metalPool[i].type == 2) { 
            metalPool[i].velocity.x *= 0.85f;
            metalPool[i].velocity.y *= 0.85f;
            metalPool[i].velocity.y += 1200.0f * dt; 
        }
    }
}

void DrawMetalSkill(void) {
    float time = (float)GetTime();

    BeginTextureMode(metalCanvas);
        ClearBackground(BLANK);
        // BeginBlendMode(BLEND_ADD); // Cực kỳ quan trọng: Dùng BLEND_ADD để màu chồng lên nhau tạo độ sáng rực (Glow)
        
        for (int i = 0; i < MAX_METAL_PARTICLES; i++) {
            if (!metalPool[i].active) continue;

            // 1. KIẾM KHÍ (AURA) - Vẽ bằng nhiều lớp hình tròn mờ chồng lên nhau
            if (metalPool[i].type == 0) {
                // Lớp lõi sáng trắng
                DrawCircleV(metalPool[i].position, 15.0f, ColorAlpha(WHITE, 0.8f));
                // Lớp kiếm khí tỏa ra xung quanh (mềm mại)
                DrawCircleGradient((int)metalPool[i].position.x, (int)metalPool[i].position.y, 40.0f, ColorAlpha(GOLD, 0.4f), BLANK);
                
                // 2. VỆT TRAIL MỀM MẠI (Dùng đường cong giả lập bằng cách vẽ 3 lớp độ dày)
                // Thay vì tam giác, ta vẽ một dải dài cong mềm bằng các đoạn nối
                Vector2 dir = Vector2Normalize(metalPool[i].velocity);
                Vector2 tail = { metalPool[i].position.x - dir.x * 120.0f, metalPool[i].position.y - dir.y * 120.0f };
                
                // Lớp dày mờ (cái bóng khí)
                DrawLineEx(metalPool[i].position, tail, 25.0f, ColorAlpha(GOLD, 0.1f));
                // Lớp lõi vàng
                DrawLineEx(metalPool[i].position, tail, 8.0f, ColorAlpha(WHITE, 0.5f));

                // 3. VẼ THANH KIẾM (Để nó nằm trên cùng)
                float rotation = atan2f(metalPool[i].velocity.y, metalPool[i].velocity.x) * RAD2DEG;
                Rectangle sourceRec = { 0.0f, 0.0f, (float)swordSprite.width, (float)swordSprite.height };
                Rectangle destRec = { metalPool[i].position.x, metalPool[i].position.y, metalPool[i].length, metalPool[i].thickness * 1.5f };
                Vector2 origin = { destRec.width / 2.0f, destRec.height / 2.0f }; 
                DrawTexturePro(swordSprite, sourceRec, destRec, origin, rotation, WHITE);
            }
        }
        EndBlendMode();
    EndTextureMode();

    // Shader sẽ làm nốt phần việc: Hút các luồng sáng lại với nhau thành kiếm khí
    SetShaderValue(metalShader, timeLocMetal, &time, SHADER_UNIFORM_FLOAT);
    BeginShaderMode(metalShader);
        DrawTextureRec(metalCanvas.texture, (Rectangle){ 0, 0, (float)metalCanvas.texture.width, (float)-metalCanvas.texture.height }, (Vector2){ 0, 0 }, WHITE);
    EndShaderMode();
}

void UnloadMetalSkill(void) {
    UnloadTexture(swordSprite); // Giải phóng ảnh kiếm khỏi RAM
    UnloadShader(metalShader);
    UnloadRenderTexture(metalCanvas);
}