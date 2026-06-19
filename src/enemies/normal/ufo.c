#include "enemy.h"
#include "game.h"
#include "resource_loader.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static void UpdateUfo(Enemy *e, struct GameState *gs, float dt) {
    e->position.x += e->velocity.x * dt;
    e->shootCooldown -= dt;
    // Bắn đạn chùm 3 tia tỏa ra nhắm về witch
    if (e->shootCooldown <= 0.0f) {
        Vector2 toWitch = Vector2Normalize(Vector2Subtract(gs->witch.position, e->position));
        float baseAngle = atan2f(toWitch.y, toWitch.x);
        
        // 3 tia lệch góc nhau 20 độ (~0.35 rad)
        float angles[3] = { baseAngle - 0.35f, baseAngle, baseAngle + 0.35f };
        for (int a = 0; a < 3; a++) {
            Vector2 projVel = { cosf(angles[a]) * 200.0f, sinf(angles[a]) * 200.0f };
            SpawnProjectile(&gs->projectilePool, e->position, projVel, 8.0f, 1, true, RED, PROJ_ENEMY_SPECIAL);
        }
        e->shootCooldown = 3.0f;
    }
}

static void DrawUfoProcedural(const Enemy *e, Color col) {
    float ufoPulse = sinf(e->animTimer * 10.0f) * 3.0f;
    Vector2 uPos = { e->position.x, e->position.y + ufoPulse };
    
    DrawEllipse(uPos.x, uPos.y, e->collisionRadius * 1.2f, e->collisionRadius * 0.5f, col);
    DrawEllipseLines(uPos.x, uPos.y, e->collisionRadius * 1.2f, e->collisionRadius * 0.5f, BLACK);
    DrawCircleSector((Vector2){ uPos.x, uPos.y - 2.0f }, e->collisionRadius * 0.6f, 180.0f, 360.0f, 16, ColorAlpha(SKYBLUE, 0.6f));
}

EnemyDefinition ufo_definition = {
    .type = ENEMY_UFO,
    .name = "UFO",
    .maxHp = 5,
    .speed = 80.0f,
    .collisionRadius = 30.0f,
    .defaultShootCooldown = 3.0f,
    .texture = &enemyUfoTexture,
    .fallbackColor = (Color){ 255, 0, 255, 255 }, // Neon magenta
    .InitState = NULL,
    .Update = UpdateUfo,
    .Draw = NULL,
    .DrawProcedural = DrawUfoProcedural
};
