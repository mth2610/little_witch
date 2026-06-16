#ifndef STAR_H
#define STAR_H

#include "raylib.h"
#include "config.h"

typedef enum StarType {
    STAR_KIM     = 0, // Kim -> Magnet
    STAR_MOC     = 1, // Mộc -> Star Storm
    STAR_THUY    = 2, // Thủy -> Mana Shield
    STAR_HOA     = 3, // Hỏa -> Fireball
    STAR_THO     = 4, // Thổ -> Lightning
    STAR_RAINBOW = 5, // Ngũ sắc
    STAR_COUNT   = 6
} StarType;

typedef struct Star {
    bool        active;
    bool        isOrbiting;
    StarType    type;
    Vector2     position;
    Vector2     spawnPosition;
    float       angle;
    float       orbitRadius;
    float       orbitSpeed;
    float       bobTimer;
    float       bobOffset;
    bool        isBeingAttracted;
    float       animTimer;
    float       scale;
    Color       tintColor;
    float       orbitTiltAlpha;  // Góc nghiêng so với mặt phẳng màn hình (X-rotation)
    float       orbitTiltBeta;   // Góc xoay của quỹ đạo trên màn hình (Z-rotation)
    float       orbitDepth;      // Độ sâu Z hiện tại phục vụ vẽ xa/gần (3D depth)
} Star;

typedef struct StarPool {
    Star    stars[MAX_STARS];
    int     orbitingCount;
    float   spawnTimer;
    float   spawnInterval;
} StarPool;

struct Witch;

struct ParticlePool;

void  InitStarPool(StarPool *pool);
int   SpawnStar(StarPool *pool, float x, float y, StarType type);
void  UpdateStars(StarPool *pool, struct Witch *witch,
                  float deltaTime, bool hasMagnet, float scrollSpeed, struct ParticlePool *partPool);
void  DrawStars(const StarPool *pool, Vector2 witchPos);
void  ApplyStarStormEffect(StarPool *pool);
Star* GetOrbitingStar(StarPool *pool, int orbitIndex);
void  DeactivateStar(StarPool *pool, int starIndex);
void  UpdateEffectiveStarCounts(StarPool *pool, struct Witch *witch);

#endif // STAR_H
