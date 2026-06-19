#ifndef ENEMY_H
#define ENEMY_H

#include "raylib.h"
#include "config.h"
#include "skill.h" // Cần ProjectilePool để quái bắn đạn

struct Witch;

typedef enum EnemyType {
    ENEMY_SLIME,
    ENEMY_BAT,
    ENEMY_GHOST,
    ENEMY_ROBOT,
    ENEMY_DRONE,
    ENEMY_ALIEN,
    ENEMY_UFO,
    ENEMY_BOSS_FOREST,
    ENEMY_BOSS_CAVE,
    ENEMY_BOSS_CITY,
    ENEMY_BOSS_SPACE,
    ENEMY_COUNT
} EnemyType;

typedef struct Enemy {
    bool        active;
    EnemyType   type;
    Vector2     position;
    Vector2     velocity;
    float       collisionRadius;
    int         hp;
    int         maxHp;
    float       speed;
    bool        isVisible;          // Cho Ghost (ẩn hình)
    float       invisTimer;         // Timer ẩn/hiện
    float       shootCooldown;      // Cho Robot/UFO/Boss
    float       attackTimer;        // Timer đặc biệt của Boss
    int         attackPhase;        // Giai đoạn tấn công Boss
    float       animTimer;
    int         animFrame;
    Color       flashColor;         // Nhấp nháy khi bị đánh
    float       flashTimer;
    float       rainbowCooldown;    // Cooldown nhận sát thương từ sao cầu vồng
    float       freezeTimer;        // Thời gian còn lại bị đóng băng (hệ Thủy)
    float       knockbackTimer;     // Thời gian bị đẩy lùi
    Vector2     knockbackVelocity;  // Vận tốc đẩy lùi
} Enemy;

typedef struct EnemyPool {
    Enemy   enemies[MAX_ENEMIES];
    float   spawnTimer;
    float   spawnInterval;          // Giảm dần theo biome
    bool    bossActive;             // Chỉ 1 boss cùng lúc
    int     bossIndex;              // Index boss trong mảng
} EnemyPool;

typedef struct EnemyDefinition {
    EnemyType type;
    const char *name;
    int maxHp;
    float speed;
    float collisionRadius;
    float defaultShootCooldown;
    Texture2D *texture;
    Color fallbackColor;
    void (*InitState)(Enemy *e);
    void (*Update)(Enemy *e, struct GameState *gs, float dt);
    void (*Draw)(const Enemy *e, Color drawTint);
    void (*DrawProcedural)(const Enemy *e, Color col);
} EnemyDefinition;

struct GameState;

void  InitEnemyPool(EnemyPool *pool);
int   SpawnEnemy(EnemyPool *pool, EnemyType type, float x, float y);
void  UpdateEnemies(struct GameState *gs, float deltaTime);
void  DrawEnemies(const EnemyPool *pool);
bool  DamageEnemy(EnemyPool *pool, int index, int damage); // true = chết
void  SpawnBoss(EnemyPool *pool, BiomeState biome);

// Helper functions for dynamic dispatch
void  UpdateBossMovement(Enemy *e, float dt);
void  DrawEnemyDefault(const Enemy *e, Color drawTint, Color fallbackColor);
const EnemyDefinition* GetEnemyDefinition(EnemyType type);

// Trả về số vàng rơi ra khi enemy chết
int   GetEnemyGoldDrop(EnemyType type);

#endif // ENEMY_H
