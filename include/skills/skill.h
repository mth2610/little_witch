#ifndef SKILL_H
#define SKILL_H

#include "raylib.h"
#include "config.h"
#include "witch.h"

// Struct đại diện cho một viên đạn (hỏa cầu của phù thủy hoặc đạn của quái)
typedef enum ProjectileType {
    PROJ_FIREBALL = 0,
    PROJ_SHURIKEN,
    PROJ_POISON,
    PROJ_ICE,
    PROJ_TORNADO,
    PROJ_ENEMY_NORMAL,
    PROJ_ENEMY_SPECIAL
} ProjectileType;

typedef struct Projectile {
    bool            active;
    Vector2         position;
    Vector2         velocity;
    float           radius;
    int             damage;
    bool            isEnemy; // true: đạn của quái, false: đạn của người chơi
    Color           color;
    ProjectileType  type;
    float           timer;        // Dành cho thời gian tồn tại (Poison cloud, Tornado)
    float           damageTimer;  // Hồi chiêu gây sát thương duy trì lên mỗi quái
    bool            isUltimate;   // Xác định đạn Ultimate tự bẻ lái (homing)
    float           rotation;     // Góc xoay tự động (Shuriken, Tornado)
    bool            isWeak;       // Xác định đạn yếu ớt (khi không có sao)
} Projectile;

// Object Pool quản lý đạn
typedef struct ProjectilePool {
    Projectile projectiles[MAX_PROJECTILES];
} ProjectilePool;

struct ParticlePool;
struct EnemyPool;
struct GameState;

// Struct đại diện cho một lớp mô tả kỹ năng chủ động (VTable)
typedef struct SkillDefinition {
    SkillType type;
    const char *name;
    void (*Init)(void);
    void (*Cast)(struct GameState *gs);
    void (*Update)(struct GameState *gs, float dt);
    void (*Draw)(struct GameState *gs);
    void (*DrawLightmap)(struct GameState *gs);
    void (*Unload)(void);
} SkillDefinition;

// Prototypes quản lý đạn
void InitProjectilePool(ProjectilePool *pool);
int  SpawnProjectile(ProjectilePool *pool, Vector2 position, Vector2 velocity,
                     float radius, int damage, bool isEnemy, Color color, ProjectileType type);
void UpdateProjectiles(ProjectilePool *pool, float deltaTime, struct ParticlePool *partPool, struct EnemyPool *enemyPool);
void DrawProjectiles(const ProjectilePool *pool, const struct GameState *gs);

// Đăng ký và quản lý hệ thống kỹ năng động
void InitSkills(void);
void UnloadSkills(void);

// API Mới: Kích hoạt, Cập nhật và Vẽ các Kỹ năng
void CastNormalAttack(struct GameState *gs);
void CastActiveSkill(struct GameState *gs, SkillType skill);
void UpdateSkills(struct GameState *gs, float deltaTime);
void DrawSkills(struct GameState *gs);
void DrawSkillsLightmap(struct GameState *gs);
void TriggerUltimate(struct GameState *gs);

#endif // SKILL_H
