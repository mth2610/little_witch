#include "enemy.h"
#include "resource_loader.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static void UpdateBat(Enemy *e, struct GameState *gs, float dt) {
    e->position.x += e->velocity.x * dt;
    // Di chuyển hình sin theo trục Y
    e->position.y += sinf(e->animTimer * 5.0f) * 140.0f * dt;
}

static void DrawBatProcedural(const Enemy *e, Color col) {
    float wingFlap = sinf(e->animTimer * 18.0f) * 10.0f;
    Vector2 wingTipL = { e->position.x - e->collisionRadius * 2.0f, e->position.y - wingFlap };
    Vector2 wingTipR = { e->position.x + e->collisionRadius * 2.0f, e->position.y - wingFlap };
    
    DrawTriangle(e->position, (Vector2){ e->position.x - e->collisionRadius * 0.4f, e->position.y + 4.0f }, wingTipL, col);
    DrawTriangle(e->position, wingTipR, (Vector2){ e->position.x + e->collisionRadius * 0.4f, e->position.y + 4.0f }, col);
    
    DrawCircleV(e->position, e->collisionRadius * 0.8f, col);
    DrawCircleLines(e->position.x, e->position.y, e->collisionRadius * 0.8f, BLACK);
    
    // Mắt dơi phát sáng đỏ neon
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle(e->position.x - 3.5f, e->position.y - 1.0f, 4.0f, ColorAlpha(RED, 0.6f));
    DrawCircle(e->position.x + 3.5f, e->position.y - 1.0f, 4.0f, ColorAlpha(RED, 0.6f));
    DrawCircle(e->position.x - 3.5f, e->position.y - 1.0f, 1.5f, WHITE);
    DrawCircle(e->position.x + 3.5f, e->position.y - 1.0f, 1.5f, WHITE);
    EndBlendMode();
}

EnemyDefinition bat_definition = {
    .type = ENEMY_BAT,
    .name = "Bat",
    .maxHp = 1,
    .speed = 120.0f,
    .collisionRadius = 18.0f,
    .defaultShootCooldown = 0.0f,
    .texture = &enemyBatTexture,
    .fallbackColor = (Color){ 255, 80, 0, 255 }, // Bright orange-red
    .InitState = NULL,
    .Update = UpdateBat,
    .Draw = NULL,
    .DrawProcedural = DrawBatProcedural
};
