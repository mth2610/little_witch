// ============================================================
// particle.c — Hệ thống hiệu ứng hạt visual (Particles)
// ============================================================

#include "particle.h"
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

// Sinh 1 hạt đơn lẻ
void SpawnParticle(ParticlePool *pool, Vector2 pos, Vector2 vel,
                   float lifetime, float radius, Color color) {
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
        
        // Di chuyển hạt theo vận tốc kèm theo dao động hình sin nhẹ ở trục Y cho sinh động
        float age = p->maxLifetime - p->lifetime;
        p->position.x += p->velocity.x * deltaTime;
        p->position.y += (p->velocity.y + sinf(age * 3.0f + p->maxLifetime * 10.0f) * 25.0f) * deltaTime;
        
        // Tính toán độ mờ (alpha) giảm dần theo thời gian còn lại
        p->alpha = p->lifetime / p->maxLifetime;
    }
}

// Vẽ toàn bộ hạt đang kích hoạt lên màn hình (Hiệu ứng lấp lánh hào quang ma thuật)
void DrawParticles(const ParticlePool *pool) {
    if (pool == NULL) return;
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!pool->particles[i].active) continue;
        
        const Particle *p = &pool->particles[i];
        Color col = Fade(p->color, p->alpha);
        
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
