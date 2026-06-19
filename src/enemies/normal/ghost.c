#include "enemy.h"
#include "resource_loader.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static void UpdateGhost(Enemy *e, struct GameState *gs, float dt) {
    e->position.x += e->velocity.x * dt;
    e->invisTimer += dt;
    // Ẩn/hiện định kỳ mỗi 1.5s
    if (e->invisTimer >= 1.5f) {
        e->isVisible = !e->isVisible;
        e->invisTimer = 0.0f;
    }
}

static void DrawGhostProcedural(const Enemy *e, Color col) {
    float floatY = sinf(e->animTimer * 6.0f) * 4.0f;
    Vector2 gPos = { e->position.x, e->position.y + floatY };
    
    DrawCircleV(gPos, e->collisionRadius * 0.9f, col);
    DrawTriangle(
        (Vector2){ gPos.x - e->collisionRadius * 0.9f, gPos.y },
        (Vector2){ gPos.x + e->collisionRadius * 0.9f, gPos.y },
        (Vector2){ gPos.x - 8.0f - sinf(e->animTimer * 12.0f) * 3.0f, gPos.y + e->collisionRadius * 1.4f },
        col
    );
    
    // 2 mắt rỗng tối tăm của linh hồn
    DrawCircle(gPos.x - 4.5f, gPos.y - 3.0f, 2.5f, (Color){ 30, 30, 50, 255 });
    DrawCircle(gPos.x + 4.5f, gPos.y - 3.0f, 2.5f, (Color){ 30, 30, 50, 255 });
    DrawCircle(gPos.x, gPos.y + 3.0f, 1.8f, (Color){ 30, 30, 50, 255 });
}

EnemyDefinition ghost_definition = {
    .type = ENEMY_GHOST,
    .name = "Ghost",
    .maxHp = 1,
    .speed = 100.0f,
    .collisionRadius = 20.0f,
    .defaultShootCooldown = 0.0f,
    .texture = &enemyGhostTexture,
    .fallbackColor = (Color){ 210, 160, 255, 255 }, // Glowing lavender
    .InitState = NULL,
    .Update = UpdateGhost,
    .Draw = NULL,
    .DrawProcedural = DrawGhostProcedural
};
