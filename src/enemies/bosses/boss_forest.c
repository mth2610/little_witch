#include "enemy.h"
#include "game.h"
#include "resource_loader.h"
#include "raymath.h"
#include <math.h>
#include <stdlib.h>

static void UpdateForestBoss(Enemy *e, struct GameState *gs, float dt) {
    UpdateBossMovement(e, dt);
    
    // Chỉ tấn công khi đã vào vị trí của Boss (x <= VIRTUAL_WIDTH - 220.0f)
    if (e->position.x <= VIRTUAL_WIDTH - 220.0f) {
        bool isRage = (e->hp < e->maxHp / 2);
        float currentCooldown = isRage ? 0.65f : 1.3f;
        float rockSpeed = isRage ? 430.0f : 320.0f;
        Color rockColor = isRage ? MAROON : BROWN;
        
        e->shootCooldown -= dt;
        if (e->shootCooldown <= 0.0f) {
            // Sinh đá rơi từ đỉnh màn hình ảo xuống
            float spawnX = 100.0f + ((float)rand() / RAND_MAX) * (VIRTUAL_WIDTH - 200.0f);
            Vector2 pos = { spawnX, -20.0f };
            Vector2 vel = { 0.0f, rockSpeed }; 
            SpawnProjectile(&gs->projectilePool, pos, vel, 16.0f, 1, true, rockColor, PROJ_ENEMY_SPECIAL);
            
            // Trạng thái Rage làm rung màn hình nhẹ khi ném đá
            if (isRage) {
                gs->shakeIntensity = 3.0f;
                gs->shakeDuration = 0.16f;
            }
            
            e->shootCooldown = currentCooldown; 
        }
    }
}

static void DrawForestBossProcedural(const Enemy *e, Color col) {
    float treeW = e->collisionRadius * 0.8f;
    float treeH = e->collisionRadius * 1.4f;
    DrawRectangle(e->position.x - treeW / 2.0f, e->position.y - treeH / 2.0f, treeW, treeH, (Color){ 110, 75, 45, 255 });
    DrawRectangleLines(e->position.x - treeW / 2.0f, e->position.y - treeH / 2.0f, treeW, treeH, BLACK);
    
    DrawCircle(e->position.x, e->position.y - treeH / 2.0f, e->collisionRadius * 0.9f, col);
    DrawCircleLines(e->position.x, e->position.y - treeH / 2.0f, e->collisionRadius * 0.9f, BLACK);
    
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle(e->position.x - 12.0f, e->position.y - 10.0f, 8.0f, ColorAlpha(RED, 0.5f));
    DrawCircle(e->position.x + 12.0f, e->position.y - 10.0f, 8.0f, ColorAlpha(RED, 0.5f));
    DrawCircle(e->position.x - 12.0f, e->position.y - 10.0f, 3.0f, WHITE);
    DrawCircle(e->position.x + 12.0f, e->position.y - 10.0f, 3.0f, WHITE);
    EndBlendMode();
}

EnemyDefinition boss_forest_definition = {
    .type = ENEMY_BOSS_FOREST,
    .name = "Forest Boss",
    .maxHp = 150,
    .speed = 60.0f,
    .collisionRadius = 55.0f,
    .defaultShootCooldown = 1.3f,
    .texture = &bossForestTexture,
    .fallbackColor = (Color){ 0, 255, 0, 255 }, // Pure neon green
    .InitState = NULL,
    .Update = UpdateForestBoss,
    .Draw = NULL,
    .DrawProcedural = DrawForestBossProcedural
};
