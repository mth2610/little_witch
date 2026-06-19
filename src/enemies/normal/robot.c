#include "enemy.h"
#include "game.h"
#include "resource_loader.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static void UpdateRobot(Enemy *e, struct GameState *gs, float dt) {
    e->position.x += e->velocity.x * dt;
    e->shootCooldown -= dt;
    // Bắn đạn thẳng nhắm về phía witch mỗi 2.5s
    if (e->shootCooldown <= 0.0f) {
        Vector2 dir = Vector2Normalize(Vector2Subtract(gs->witch.position, e->position));
        Vector2 projVel = Vector2Scale(dir, 240.0f);
        SpawnProjectile(&gs->projectilePool, e->position, projVel, 7.0f, 1, true, ORANGE, PROJ_ENEMY_NORMAL);
        e->shootCooldown = 2.5f;
    }
}

static void DrawRobotProcedural(const Enemy *e, Color col) {
    float hoverY = sinf(e->animTimer * 10.0f) * 2.0f;
    Vector2 rPos = { e->position.x, e->position.y + hoverY };
    
    Rectangle rect = { rPos.x - e->collisionRadius * 0.9f, rPos.y - e->collisionRadius * 0.9f, e->collisionRadius * 1.8f, e->collisionRadius * 1.8f };
    DrawRectangleRec(rect, col);
    DrawRectangleLinesEx(rect, 2.0f, BLACK);
    
    // Kính chắn (Visor) robot phát sáng xanh
    BeginBlendMode(BLEND_ADDITIVE);
    DrawRectangle(rPos.x - e->collisionRadius * 0.6f, rPos.y - 6.0f, e->collisionRadius * 1.2f, 5.0f, ColorAlpha(SKYBLUE, 0.4f));
    DrawRectangle(rPos.x - e->collisionRadius * 0.4f, rPos.y - 5.0f, e->collisionRadius * 0.8f, 3.0f, WHITE);
    EndBlendMode();
}

EnemyDefinition robot_definition = {
    .type = ENEMY_ROBOT,
    .name = "Robot",
    .maxHp = 3,
    .speed = 90.0f,
    .collisionRadius = 26.0f,
    .defaultShootCooldown = 2.5f,
    .texture = &enemyRobotTexture,
    .fallbackColor = (Color){ 0, 191, 255, 255 }, // Neon blue
    .InitState = NULL,
    .Update = UpdateRobot,
    .Draw = NULL,
    .DrawProcedural = DrawRobotProcedural
};
