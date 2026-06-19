// ============================================================
// particle.c — Hệ thống hiệu ứng hạt visual (Particles)
// ============================================================

#include "particle.h"
#include "game.h"
#include "raylib.h"
#include "raymath.h"
#include <stdlib.h> // Để sử dụng rand()

// ----------------------------------------------------------------
// HÀM CÔNG KHAI
// ----------------------------------------------------------------

// Khởi tạo pool hạt rỗng
void InitParticlePool(ParticlePool *pool) {
    if (pool == NULL) return;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        pool->particles[i].active = false;
    }
}

// Sinh 1 hạt đơn lẻ (mặc định là hạt ma thuật lấp lánh)
void SpawnParticle(ParticlePool *pool, Vector2 pos, Vector2 vel,
                   float lifetime, float radius, Color color) {
    SpawnParticleEx(pool, pos, vel, lifetime, radius, color, PARTICLE_SPARKLE);
}

// Sinh hạt với loại tùy biến (Ex)
void SpawnParticleEx(ParticlePool *pool, Vector2 pos, Vector2 vel,
                     float lifetime, float radius, Color color, ParticleType type) {
    if (pool == NULL) return;
    
    // Tìm slot trống
    int index = -1;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!pool->particles[i].active) {
            index = i;
            break;
        }
    }
    
    // Nếu pool đầy, bỏ qua không sinh thêm
    if (index == -1) return;
    
    Particle *p = &pool->particles[index];
    p->active = true;
    p->position = pos;
    p->velocity = vel;
    p->lifetime = lifetime;
    p->maxLifetime = lifetime;
    p->radius = radius;
    p->color = color;
    p->alpha = 1.0f;
    p->type = type;
}

// Hàm tiện ích: Tạo vụ nổ gồm nhiều hạt bay tỏa ra từ tâm
void SpawnExplosion(ParticlePool *pool, Vector2 pos, int count, Color color) {
    if (pool == NULL) return;
    
    for (int i = 0; i < count; i++) {
        // Sinh hướng ngẫu nhiên
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float speed = 80.0f + ((float)rand() / RAND_MAX) * 120.0f; // Tốc độ từ 80 đến 200
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        
        float lifetime = 0.4f + ((float)rand() / RAND_MAX) * 0.4f; // Thời gian sống 0.4 - 0.8s
        float radius = 2.0f + ((float)rand() / RAND_MAX) * 4.0f;    // Bán kính hạt 2 - 6px
        
        SpawnParticle(pool, pos, vel, lifetime, radius, color);
    }
}

// Cập nhật trạng thái các hạt (di chuyển, giảm thời gian sống, mờ dần)
void UpdateParticles(ParticlePool *pool, float deltaTime) {
    if (pool == NULL) return;
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!pool->particles[i].active) continue;
        
        Particle *p = &pool->particles[i];
        p->lifetime -= deltaTime;
        
        if (p->lifetime <= 0) {
            p->active = false;
            continue;
        }
        
        // Cập nhật đặc thù cho từng loại hạt
        if (p->type == PARTICLE_FROST_VAPOR) {
            // Hơi nước/khói sương có lực cản không khí, giảm tốc rất nhanh
            p->velocity = Vector2Scale(p->velocity, fmaxf(0.0f, 1.0f - 4.5f * deltaTime));
            // Hơi sương nở to ra theo thời gian khi bay lên để mô tả sự tán sương
            p->radius += deltaTime * 20.0f;
            
            p->position.x += p->velocity.x * deltaTime;
            // Cho hơi sương bay nhẹ bốc lên trên (Y âm) kèm gợn sóng ngang hình sin
            float age = p->maxLifetime - p->lifetime;
            p->position.y += (p->velocity.y - 12.0f + sinf(age * 4.0f + p->maxLifetime * 12.0f) * 15.0f) * deltaTime;
        } else {
            // Di chuyển hạt ma thuật lấp lánh thông thường
            p->position.x += p->velocity.x * deltaTime;
            float age = p->maxLifetime - p->lifetime;
            p->position.y += (p->velocity.y + sinf(age * 3.0f + p->maxLifetime * 10.0f) * 25.0f) * deltaTime;
        }
        
        // Tính toán độ mờ (alpha) giảm dần theo thời gian còn lại
        p->alpha = p->lifetime / p->maxLifetime;
    }
}

// Vẽ toàn bộ hạt đang kích hoạt lên màn hình (Hào quang ma thuật) - Backward compatibility
void DrawParticles(const ParticlePool *pool) {
    DrawParticlesEx(pool, NULL);
}

// Vẽ toàn bộ hạt có shader hỗ trợ nếu GameState có sẵn
void DrawParticlesEx(const ParticlePool *pool, const struct GameState *gs) {
    if (pool == NULL) return;
    
    // BƯỚC 1: Vẽ hạt Hơi Sương Băng Giá bằng Shader chuyên dụng (Nếu có shader)
    bool useShader = (gs != NULL && gs->vaporShaderLoaded);
    if (useShader) {
        BeginShaderMode(gs->vaporShader);
        float timeVal = GetTime();
        SetShaderValue(gs->vaporShader, GetShaderLocation(gs->vaporShader, "time"), &timeVal, SHADER_UNIFORM_FLOAT);
        
        for (int i = 0; i < MAX_PARTICLES; i++) {
            const Particle *p = &pool->particles[i];
            if (p->active && p->type == PARTICLE_FROST_VAPOR) {
                // Modulate color and alpha
                Color tint = Fade(p->color, p->alpha);
                float size = p->radius * 2.2f;
                // Vẽ quad có map UV từ dummyWhiteTex để đưa vào shader
                DrawTexturePro(gs->dummyWhiteTex,
                               (Rectangle){ 0.0f, 0.0f, 2.0f, 2.0f },
                               (Rectangle){ p->position.x, p->position.y, size, size },
                               (Vector2){ size * 0.5f, size * 0.5f },
                               p->lifetime * 60.0f + i * 15.0f, // Tự xoay tròn nhẹ
                               tint);
            }
        }
        EndShaderMode();
    }
    
    // BƯỚC 2: Vẽ các hạt lấp lánh vector thông thường (Hoặc sương vẽ dự phòng nếu ko có shader)
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!pool->particles[i].active) continue;
        
        const Particle *p = &pool->particles[i];
        
        // Bỏ qua hạt sương đã được vẽ bằng shader ở Bước 1
        if (p->type == PARTICLE_FROST_VAPOR && useShader) continue;
        
        Color col = Fade(p->color, p->alpha);
        
        if (p->type == PARTICLE_FROST_VAPOR) {
            // Vẽ dự phòng sương giá bằng vòng tròn gradient soft nếu ko có shader
            DrawCircleGradient(p->position.x, p->position.y, p->radius, ColorAlpha(p->color, p->alpha * 0.3f), ColorAlpha(p->color, 0.0f));
            DrawCircleV(p->position, p->radius * 0.3f, ColorAlpha(WHITE, p->alpha * 0.2f));
        } else {
            // Vẽ hạt lấp lánh (Sparkle Star) có lõi trắng và 4 tia phát sáng chéo
            // 1. Vẽ lõi hạt phát sáng tròn (màu hạt pha trắng ở tâm)
            DrawCircleV(p->position, p->radius, col);
            DrawCircleV(p->position, p->radius * 0.5f, Fade(WHITE, p->alpha));
            
            // 2. Vẽ các tia sáng giao nhau tạo hiệu ứng hào quang lấp lánh (sparkle glare)
            if (p->radius > 2.0f) {
                float shine = p->radius * 2.2f * p->alpha;
                DrawLineEx(
                    (Vector2){ p->position.x - shine, p->position.y },
                    (Vector2){ p->position.x + shine, p->position.y },
                    1.5f, col
                );
                DrawLineEx(
                    (Vector2){ p->position.x, p->position.y - shine },
                    (Vector2){ p->position.x, p->position.y + shine },
                    1.5f, col
                );
            }
        }
    }
}
