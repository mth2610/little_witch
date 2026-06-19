#include "enemy.h"
#include "game.h"
#include "resource_loader.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static void UpdateCityBoss(Enemy *e, struct GameState *gs, float dt) {
    UpdateBossMovement(e, dt);
    
    // Tấn công chỉ khi đã vào vị trí
    if (e->position.x <= VIRTUAL_WIDTH - 220.0f) {
        e->shootCooldown -= dt;
        if (e->shootCooldown <= 0.0f) {
            // Triệu hồi 5 Drone tại vị trí của Boss
            for (int d = 0; d < 5; d++) {
                float offsetY = -70.0f + d * 35.0f;
                SpawnEnemy(&gs->enemyPool, ENEMY_DRONE, e->position.x - 30.0f, e->position.y + offsetY);
            }
            e->shootCooldown = 6.0f; // Triệu hồi đợt tiếp theo sau 6 giây
        }
    }
}

static void DrawCityBossProcedural(const Enemy *e, Color col) {
    float tankW = e->collisionRadius * 1.6f;
    float tankH = e->collisionRadius * 1.1f;
    DrawRectangle(e->position.x - tankW / 2.0f, e->position.y - tankH / 2.0f, tankW, tankH, col);
    DrawRectangleLines(e->position.x - tankW / 2.0f, e->position.y - tankH / 2.0f, tankW, tankH, BLACK);
    
    DrawRectangle(e->position.x - tankW / 2.0f - 15.0f, e->position.y - 8.0f, 15.0f, 8.0f, DARKGRAY);
    DrawRectangle(e->position.x - tankW / 4.0f, e->position.y - tankH / 2.0f - 10.0f, tankW / 2.0f, 10.0f, GRAY);
}

EnemyDefinition boss_city_definition = {
    .type = ENEMY_BOSS_CITY,
    .name = "City Boss",
    .maxHp = 80,
    .speed = 60.0f,
    .collisionRadius = 65.0f,
    .defaultShootCooldown = 4.0f,
    .texture = &bossCityTexture,
    .fallbackColor = (Color){ 255, 0, 128, 255 }, // Cyberpunk hot pink
    .InitState = NULL,
    .Update = UpdateCityBoss,
    .Draw = NULL,
    .DrawProcedural = DrawCityBossProcedural
};
