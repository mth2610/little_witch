#include "enemy.h"
#include "game.h"
#include "resource_loader.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static void InitAlien(Enemy *e) {
    e->velocity = (Vector2){ -e->speed, e->speed };
}

static void UpdateAlien(Enemy *e, struct GameState *gs, float dt) {
    // Đi chéo, đảo hướng Y mỗi 0.8s để tạo đường zigzag
    e->shootCooldown += dt;
    if (e->shootCooldown >= 0.8f) {
        e->velocity.y = -e->velocity.y;
        e->shootCooldown = 0.0f;
    }
    e->position.x += e->velocity.x * dt;
    e->position.y += e->velocity.y * dt;
}

static void DrawAlienProcedural(const Enemy *e, Color col) {
    float wiggleX = sinf(e->animTimer * 12.0f) * 2.0f;
    Vector2 aPos = { e->position.x + wiggleX, e->position.y };
    
    DrawCircle(aPos.x, aPos.y - 3.0f, e->collisionRadius * 0.8f, col);
    DrawTriangle(
        (Vector2){ aPos.x - e->collisionRadius * 0.8f, aPos.y - 3.0f },
        (Vector2){ aPos.x + e->collisionRadius * 0.8f, aPos.y - 3.0f },
        (Vector2){ aPos.x, aPos.y + e->collisionRadius * 0.7f },
        col
    );
    DrawEllipse(aPos.x - 5.0f, aPos.y - 5.0f, 4.0f, 6.0f, BLACK);
    DrawEllipse(aPos.x + 5.0f, aPos.y - 5.0f, 4.0f, 6.0f, BLACK);
}

EnemyDefinition alien_definition = {
    .type = ENEMY_ALIEN,
    .name = "Alien",
    .maxHp = 4,
    .speed = 150.0f,
    .collisionRadius = 24.0f,
    .defaultShootCooldown = 0.0f,
    .texture = &enemyAlienTexture,
    .fallbackColor = (Color){ 140, 255, 10, 255 }, // Neon yellow-green
    .InitState = InitAlien,
    .Update = UpdateAlien,
    .Draw = NULL,
    .DrawProcedural = DrawAlienProcedural
};
