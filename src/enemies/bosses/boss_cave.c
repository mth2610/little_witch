#include "enemy.h"
#include "game.h"
#include "witch.h"
#include "resource_loader.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static void UpdateCaveBoss(Enemy *e, struct GameState *gs, float dt) {
    UpdateBossMovement(e, dt);
    
    // Tấn công chỉ khi đã vào vị trí
    if (e->position.x <= VIRTUAL_WIDTH - 220.0f) {
        e->attackTimer += dt;
        
        if (e->attackPhase == 0) {
            // Giai đoạn cảnh báo (2.0s)
            if (e->attackTimer >= 2.0f) {
                e->attackPhase = 1;
                e->attackTimer = 0.0f;
            }
        } else if (e->attackPhase == 1 || e->attackPhase == 2) {
            // Giai đoạn quét laser (1.5s)
            float sweepY = (e->attackTimer / 1.5f) * VIRTUAL_HEIGHT;
            
            // Chỉ gây sát thương 1 lần mỗi đợt quét laser
            if (e->attackPhase == 1 && gs->witch.position.x < e->position.x) {
                if (fabsf(gs->witch.position.y - sweepY) < gs->witch.collisionRadius + 9.0f) {
                    WitchTakeDamage(&gs->witch);
                    e->attackPhase = 2; // Đánh dấu đã gây sát thương đợt này
                }
            }
            
            if (e->attackTimer >= 1.5f) {
                e->attackPhase = 0;
                e->attackTimer = 0.0f;
            }
        }
    }
}

static void DrawCaveBoss(const Enemy *e, Color drawTint) {
    if (e->attackPhase >= 1) {
        // Tia laser đỏ của Cave Boss
        float sweepY = (e->attackTimer / 1.5f) * VIRTUAL_HEIGHT;
        DrawLineEx((Vector2){ 0.0f, sweepY }, (Vector2){ e->position.x, sweepY }, 18.0f, ColorAlpha(RED, 0.75f));
        DrawLineEx((Vector2){ 0.0f, sweepY }, (Vector2){ e->position.x, sweepY }, 6.0f, WHITE);
        DrawCircle(e->position.x - 30.0f, sweepY, 20.0f, ColorAlpha(RED, 0.9f));
    }
    
    // Vẽ thân boss
    DrawEnemyDefault(e, drawTint, (Color){ 255, 160, 0, 255 });
}

static void DrawCaveBossProcedural(const Enemy *e, Color col) {
    DrawCircleV(e->position, e->collisionRadius, col);
    DrawCircleLinesV(e->position, e->collisionRadius, BLACK);
    
    float crystalPulse = 0.7f + 0.3f * sinf(e->animTimer * 12.0f);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircle(e->position.x, e->position.y, e->collisionRadius * 0.5f * crystalPulse, ColorAlpha(SKYBLUE, 0.4f));
    DrawCircle(e->position.x, e->position.y, e->collisionRadius * 0.3f * crystalPulse, WHITE);
    EndBlendMode();
}

EnemyDefinition boss_cave_definition = {
    .type = ENEMY_BOSS_CAVE,
    .name = "Cave Boss",
    .maxHp = 50,
    .speed = 60.0f,
    .collisionRadius = 60.0f,
    .defaultShootCooldown = 2.0f,
    .texture = &bossCaveTexture,
    .fallbackColor = (Color){ 255, 160, 0, 255 }, // Glowing gold
    .InitState = NULL,
    .Update = UpdateCaveBoss,
    .Draw = DrawCaveBoss,
    .DrawProcedural = DrawCaveBossProcedural
};
