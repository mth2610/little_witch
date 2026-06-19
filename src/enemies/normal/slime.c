#include "enemy.h"
#include "resource_loader.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static void UpdateSlime(Enemy *e, struct GameState *gs, float dt) {
    e->position.x += e->velocity.x * dt;
}

static void DrawSlimeProcedural(const Enemy *e, Color col) {
    float pulseY = sinf(e->animTimer * 8.0f) * 3.0f;
    float rx = e->collisionRadius * 1.1f;
    float ry = e->collisionRadius * 0.9f + pulseY;
    
    DrawEllipse(e->position.x, e->position.y + pulseY, rx, ry, col);
    DrawEllipseLines(e->position.x, e->position.y + pulseY, rx, ry, DARKGREEN);
    
    // Vẽ 2 mắt nhỏ tinh nghịch
    Vector2 eyeL = { e->position.x - 5.0f, e->position.y - 2.0f + pulseY };
    Vector2 eyeR = { e->position.x + 5.0f, e->position.y - 2.0f + pulseY };
    DrawCircleV(eyeL, 2.5f, BLACK);
    DrawCircleV(eyeR, 2.5f, BLACK);
    DrawCircle(eyeL.x - 0.8f, eyeL.y - 0.8f, 0.8f, WHITE);
    DrawCircle(eyeR.x - 0.8f, eyeR.y - 0.8f, 0.8f, WHITE);
}

EnemyDefinition slime_definition = {
    .type = ENEMY_SLIME,
    .name = "Slime",
    .maxHp = 1,
    .speed = 80.0f,
    .collisionRadius = 22.0f,
    .defaultShootCooldown = 0.0f,
    .texture = &enemySlimeTexture,
    .fallbackColor = (Color){ 0, 255, 128, 255 }, // Lime neon
    .InitState = NULL,
    .Update = UpdateSlime,
    .Draw = NULL,
    .DrawProcedural = DrawSlimeProcedural
};
