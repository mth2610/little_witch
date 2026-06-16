#ifndef PARTICLE_H
#define PARTICLE_H

#include "raylib.h"
#include "config.h"

typedef struct Particle {
    bool    active;
    Vector2 position;
    Vector2 velocity;
    float   lifetime;       // Thời gian sống còn lại (giây)
    float   maxLifetime;    // Thời gian sống ban đầu
    float   radius;
    Color   color;
    float   alpha;          // Opacity, giảm dần theo lifetime
} Particle;

typedef struct ParticlePool {
    Particle particles[MAX_PARTICLES];
} ParticlePool;

void InitParticlePool(ParticlePool *pool);
void SpawnParticle(ParticlePool *pool, Vector2 pos, Vector2 vel,
                   float lifetime, float radius, Color color);
// Hàm tiện ích: tạo vụ nổ n hạt tỏa ra từ 1 điểm
void SpawnExplosion(ParticlePool *pool, Vector2 pos, int count, Color color);
void UpdateParticles(ParticlePool *pool, float deltaTime);
void DrawParticles(const ParticlePool *pool);

#endif // PARTICLE_H
