#ifndef PARTICLE_H
#define PARTICLE_H

#include "raylib.h"
#include "config.h"

typedef enum ParticleType {
    PARTICLE_SPARKLE = 0,       // Hạt ma thuật lấp lánh (mặc định)
    PARTICLE_FROST_VAPOR = 1    // Hạt hơi sương băng giá
} ParticleType;

typedef struct Particle {
    bool         active;
    Vector2      position;
    Vector2      velocity;
    float        lifetime;       // Thời gian sống còn lại (giây)
    float        maxLifetime;    // Thời gian sống ban đầu
    float        radius;
    Color        color;
    float        alpha;          // Opacity, giảm dần theo lifetime
    ParticleType type;           // Loại hạt để vẽ khác nhau
} Particle;

typedef struct ParticlePool {
    Particle particles[MAX_PARTICLES];
} ParticlePool;

void InitParticlePool(ParticlePool *pool);
void SpawnParticle(ParticlePool *pool, Vector2 pos, Vector2 vel,
                   float lifetime, float radius, Color color);
void SpawnParticleEx(ParticlePool *pool, Vector2 pos, Vector2 vel,
                     float lifetime, float radius, Color color, ParticleType type);
// Hàm tiện ích: tạo vụ nổ n hạt tỏa ra từ 1 điểm
void SpawnExplosion(ParticlePool *pool, Vector2 pos, int count, Color color);
void UpdateParticles(ParticlePool *pool, float deltaTime);
void DrawParticles(const ParticlePool *pool);
// Thêm khai báo để DrawParticles có thể truy cập GameState nhằm dùng vaporShader
struct GameState;
void DrawParticlesEx(const ParticlePool *pool, const struct GameState *gs);

#endif // PARTICLE_H
