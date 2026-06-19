#include "enemy.h"
#include "game.h"
#include "resource_loader.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static void InitDrone(Enemy *e) {
    e->velocity = (Vector2){ 0.0f, 0.0f };
}

static void UpdateDrone(Enemy *e, struct GameState *gs, float dt) {
    // Bay trực tiếp dí theo witch
    Vector2 toWitch = Vector2Subtract(gs->witch.position, e->position);
    Vector2 dir = Vector2Normalize(toWitch);
    e->velocity = Vector2Scale(dir, e->speed);
    e->position = Vector2Add(e->position, Vector2Scale(e->velocity, dt));
}

static void DrawDroneProcedural(const Enemy *e, Color col) {
    float angle = e->animTimer * 720.0f;
    Vector2 propL = { e->position.x - e->collisionRadius * 1.3f, e->position.y - 6.0f };
    Vector2 propR = { e->position.x + e->collisionRadius * 1.3f, e->position.y - 6.0f };
    
    DrawLineEx(e->position, propL, 2.0f, BLACK);
    DrawLineEx(e->position, propR, 2.0f, BLACK);
    DrawLineEx(propL, (Vector2){ propL.x + cosf(angle * DEG2RAD) * 12.0f, propL.y + sinf(angle * DEG2RAD) * 4.0f }, 2.0f, BLACK);
    DrawLineEx(propR, (Vector2){ propR.x - cosf(angle * DEG2RAD) * 12.0f, propR.y - sinf(angle * DEG2RAD) * 4.0f }, 2.0f, BLACK);
    
    DrawCircleV(e->position, e->collisionRadius * 0.8f, col);
    DrawCircleLines(e->position.x, e->position.y, e->collisionRadius * 0.8f, BLACK);
    DrawCircle(e->position.x + 3.0f, e->position.y, 2.5f, LIME);
}

EnemyDefinition drone_definition = {
    .type = ENEMY_DRONE,
    .name = "Drone",
    .maxHp = 2,
    .speed = 160.0f,
    .collisionRadius = 16.0f,
    .defaultShootCooldown = 0.0f,
    .texture = &enemyDroneTexture,
    .fallbackColor = (Color){ 0, 255, 255, 255 }, // Neon cyan
    .InitState = InitDrone,
    .Update = UpdateDrone,
    .Draw = NULL,
    .DrawProcedural = DrawDroneProcedural
};
