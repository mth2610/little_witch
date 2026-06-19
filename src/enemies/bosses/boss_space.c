#include "enemy.h"
#include "game.h"
#include "witch.h"
#include "resource_loader.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static void UpdateSpaceBoss(Enemy *e, struct GameState *gs, float dt) {
    UpdateBossMovement(e, dt);
    
    // Tấn công chỉ khi đã vào vị trí
    if (e->position.x <= VIRTUAL_WIDTH - 220.0f) {
        e->attackTimer += dt;
        
        if (e->attackPhase == 0) {
            // Giai đoạn chuẩn bị (4.0s)
            if (e->attackTimer >= 4.0f) {
                e->attackPhase = 1;
                e->attackTimer = 0.0f;
            }
        } else if (e->attackPhase == 1 || e->attackPhase == 2) {
            // Giai đoạn hố đen hoạt động (3.0s)
            Vector2 center = { VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f };
            Vector2 toCenter = Vector2Subtract(center, gs->witch.position);
            float dist = Vector2Length(toCenter);
            
            if (dist > 5.0f) {
                // Lực kéo tăng dần nếu càng gần tâm
                Vector2 pullDir = Vector2Normalize(toCenter);
                float pullSpeed = 160.0f; 
                Vector2 pullForce = Vector2Scale(pullDir, pullSpeed * dt);
                gs->witch.position = Vector2Add(gs->witch.position, pullForce);
            }
            
            // Chỉ gây sát thương 1 lần mỗi đợt hố đen
            if (e->attackPhase == 1 && dist < gs->witch.collisionRadius + 30.0f) {
                WitchTakeDamage(&gs->witch);
                e->attackPhase = 2; // Đánh dấu đã gây sát thương đợt này
            }
            
            if (e->attackTimer >= 3.0f) {
                e->attackPhase = 0;
                e->attackTimer = 0.0f;
            }
        }
    }
}

static void DrawSpaceBoss(const Enemy *e, Color drawTint) {
    if (e->attackPhase >= 1) {
        // Vòng xoáy hố đen của Space Boss
        Vector2 center = { VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f };
        float spin = e->animTimer * 180.0f;
        DrawCircleSector(center, 140.0f, spin, spin + 120.0f, 16, ColorAlpha(DARKPURPLE, 0.4f));
        DrawCircleSector(center, 140.0f, spin + 180.0f, spin + 300.0f, 16, ColorAlpha(DARKPURPLE, 0.4f));
        DrawCircleV(center, 70.0f + sinf(e->animTimer * 5.0f) * 10.0f, ColorAlpha(BLACK, 0.9f));
        DrawCircleLines(center.x, center.y, 75.0f, PURPLE);
    }
    
    // Vẽ thân boss
    DrawEnemyDefault(e, drawTint, (Color){ 128, 0, 255, 255 });
}

static void DrawSpaceBossProcedural(const Enemy *e, Color col) {
    float ufoW = e->collisionRadius * 1.8f;
    float ufoH = e->collisionRadius * 0.7f;
    DrawEllipse(e->position.x, e->position.y, ufoW, ufoH, col);
    DrawEllipseLines(e->position.x, e->position.y, ufoW, ufoH, BLACK);
    
    BeginBlendMode(BLEND_ADDITIVE);
    DrawCircleSector((Vector2){ e->position.x, e->position.y - 4.0f }, e->collisionRadius * 0.6f, 180.0f, 360.0f, 16, ColorAlpha(RED, 0.5f));
    DrawCircleSector((Vector2){ e->position.x, e->position.y - 4.0f }, e->collisionRadius * 0.3f, 180.0f, 360.0f, 16, ColorAlpha(WHITE, 0.6f));
    EndBlendMode();
}

EnemyDefinition boss_space_definition = {
    .type = ENEMY_BOSS_SPACE,
    .name = "Space Boss",
    .maxHp = 120,
    .speed = 60.0f,
    .collisionRadius = 70.0f,
    .defaultShootCooldown = 5.0f,
    .texture = &bossSpaceTexture,
    .fallbackColor = (Color){ 128, 0, 255, 255 }, // Swirling purple
    .InitState = NULL,
    .Update = UpdateSpaceBoss,
    .Draw = DrawSpaceBoss,
    .DrawProcedural = DrawSpaceBossProcedural
};
