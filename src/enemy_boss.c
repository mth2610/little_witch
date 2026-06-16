#include "enemy_boss.h"
#include "game.h"
#include "witch.h"
#include "skill.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>

// Texture Boss (khai báo ở main.c)
extern Texture2D bossForestTexture;
extern Texture2D bossCaveTexture;
extern Texture2D bossCityTexture;
extern Texture2D bossSpaceTexture;

// Cập nhật các hành vi tấn công đặc trưng của Trùm
void UpdateBossAttack(EnemyPool *pool, Enemy *boss, struct GameState *gs, float deltaTime) {
    if (pool == NULL || boss == NULL || gs == NULL) return;
    
    struct Witch *witch = &gs->witch;
    ProjectilePool *projPool = &gs->projectilePool;
    
    boss->attackTimer += deltaTime;
    
    switch (boss->type) {
        case ENEMY_BOSS_FOREST: {
            // BOSS RỪNG: Ném đá từ trên trời rơi xuống ngẫu nhiên
            bool isRage = (boss->hp < boss->maxHp / 2);
            float currentCooldown = isRage ? 0.65f : 1.3f;
            float rockSpeed = isRage ? 430.0f : 320.0f;
            Color rockColor = isRage ? MAROON : BROWN;
            
            boss->shootCooldown -= deltaTime;
            if (boss->shootCooldown <= 0.0f) {
                // Sinh đá rơi từ đỉnh màn hình ảo xuống
                float spawnX = 100.0f + ((float)rand() / RAND_MAX) * (VIRTUAL_WIDTH - 200.0f);
                Vector2 pos = { spawnX, -20.0f };
                Vector2 vel = { 0.0f, rockSpeed }; 
                SpawnProjectile(projPool, pos, vel, 16.0f, 1, true, rockColor, PROJ_ENEMY_SPECIAL);
                
                // Trạng thái Rage làm rung màn hình nhẹ khi ném đá
                if (isRage) {
                    gs->shakeIntensity = 3.0f;
                    gs->shakeDuration = 0.16f;
                }
                
                boss->shootCooldown = currentCooldown; 
            }
            break;
        }
            
        case ENEMY_BOSS_CAVE:
            // BOSS HANG ĐỘNG: Quét tia laser ngang màn hình
            if (boss->attackPhase == 0) {
                // Giai đoạn cảnh báo (2.0s)
                if (boss->attackTimer >= 2.0f) {
                    boss->attackPhase = 1;
                    boss->attackTimer = 0.0f;
                }
            } else if (boss->attackPhase == 1 || boss->attackPhase == 2) {
                // Giai đoạn quét laser (1.5s)
                // Phase 1 = chưa trúng phù thủy, Phase 2 = đã trúng đợt này
                float sweepY = (boss->attackTimer / 1.5f) * VIRTUAL_HEIGHT;
                
                // Chỉ gây sát thương 1 lần mỗi đợt quét laser
                if (boss->attackPhase == 1 && witch->position.x < boss->position.x) {
                    if (fabsf(witch->position.y - sweepY) < witch->collisionRadius + 9.0f) {
                        WitchTakeDamage(witch);
                        boss->attackPhase = 2; // Đánh dấu đã gây sát thương đợt này
                    }
                }
                
                if (boss->attackTimer >= 1.5f) {
                    boss->attackPhase = 0;
                    boss->attackTimer = 0.0f;
                }
            }
            break;
            
        case ENEMY_BOSS_CITY:
            // BOSS THÀNH PHỐ: Triệu hồi 5 con Drone bu xung quanh nhắm tới phù thủy
            boss->shootCooldown -= deltaTime;
            if (boss->shootCooldown <= 0.0f) {
                // Triệu hồi 5 Drone tại vị trí của Boss
                for (int d = 0; d < 5; d++) {
                    float offsetY = -70.0f + d * 35.0f;
                    SpawnEnemy(pool, ENEMY_DRONE, boss->position.x - 30.0f, boss->position.y + offsetY);
                }
                boss->shootCooldown = 6.0f; // Triệu hồi đợt tiếp theo sau 6 giây
            }
            break;
            
        case ENEMY_BOSS_SPACE:
            // BOSS VŨ TRỤ: Kích hoạt Hố đen kéo phù thủy vào trung tâm
            if (boss->attackPhase == 0) {
                // Giai đoạn chuẩn bị (4.0s)
                if (boss->attackTimer >= 4.0f) {
                    boss->attackPhase = 1;
                    boss->attackTimer = 0.0f;
                }
            } else if (boss->attackPhase == 1 || boss->attackPhase == 2) {
                // Giai đoạn hố đen hoạt động (3.0s)
                // Phase 1 = chưa trúng, Phase 2 = đã trúng đợt này
                Vector2 center = { VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f };
                Vector2 toCenter = Vector2Subtract(center, witch->position);
                float dist = Vector2Length(toCenter);
                
                if (dist > 5.0f) {
                    // Lực kéo tăng dần nếu càng gần tâm
                    Vector2 pullDir = Vector2Normalize(toCenter);
                    float pullSpeed = 160.0f; 
                    Vector2 pullForce = Vector2Scale(pullDir, pullSpeed * deltaTime);
                    witch->position = Vector2Add(witch->position, pullForce);
                }
                
                // Chỉ gây sát thương 1 lần mỗi đợt hố đen
                if (boss->attackPhase == 1 && dist < witch->collisionRadius + 30.0f) {
                    WitchTakeDamage(witch);
                    boss->attackPhase = 2; // Đánh dấu đã gây sát thương đợt này
                }
                
                if (boss->attackTimer >= 3.0f) {
                    boss->attackPhase = 0;
                    boss->attackTimer = 0.0f;
                }
            }
            break;
            
        default:
            break;
    }
}

// Vẽ vector/thủ thuật cho Trùm khi không load được texture
void DrawProceduralBoss(const Enemy *e, Color col) {
    if (e == NULL) return;

    switch (e->type) {
        case ENEMY_BOSS_FOREST: {
            // Boss Rừng: Gốc cây cổ thụ mắt đỏ phát sáng
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
            break;
        }
        
        case ENEMY_BOSS_CAVE: {
            // Boss Hang Động: Cự thạch nhãn tinh thể phát sáng
            DrawCircleV(e->position, e->collisionRadius, col);
            DrawCircleLinesV(e->position, e->collisionRadius, BLACK);
            
            float crystalPulse = 0.7f + 0.3f * sinf(e->animTimer * 12.0f);
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircle(e->position.x, e->position.y, e->collisionRadius * 0.5f * crystalPulse, ColorAlpha(SKYBLUE, 0.4f));
            DrawCircle(e->position.x, e->position.y, e->collisionRadius * 0.3f * crystalPulse, WHITE);
            EndBlendMode();
            break;
        }
        
        case ENEMY_BOSS_CITY: {
            // Boss Thành Phố: Mech Tank bọc thép khổng lồ
            float tankW = e->collisionRadius * 1.6f;
            float tankH = e->collisionRadius * 1.1f;
            DrawRectangle(e->position.x - tankW / 2.0f, e->position.y - tankH / 2.0f, tankW, tankH, col);
            DrawRectangleLines(e->position.x - tankW / 2.0f, e->position.y - tankH / 2.0f, tankW, tankH, BLACK);
            
            DrawRectangle(e->position.x - tankW / 2.0f - 15.0f, e->position.y - 8.0f, 15.0f, 8.0f, DARKGRAY);
            DrawRectangle(e->position.x - tankW / 4.0f, e->position.y - tankH / 2.0f - 10.0f, tankW / 2.0f, 10.0f, GRAY);
            break;
        }
        
        case ENEMY_BOSS_SPACE: {
            // Boss Vũ Trụ: Đĩa bay mẹ tinh vân khổng lồ
            float ufoW = e->collisionRadius * 1.8f;
            float ufoH = e->collisionRadius * 0.7f;
            DrawEllipse(e->position.x, e->position.y, ufoW, ufoH, col);
            DrawEllipseLines(e->position.x, e->position.y, ufoW, ufoH, BLACK);
            
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleSector((Vector2){ e->position.x, e->position.y - 4.0f }, e->collisionRadius * 0.6f, 180.0f, 360.0f, 16, ColorAlpha(RED, 0.5f));
            DrawCircleSector((Vector2){ e->position.x, e->position.y - 4.0f }, e->collisionRadius * 0.3f, 180.0f, 360.0f, 16, ColorAlpha(WHITE, 0.6f));
            EndBlendMode();
            break;
        }
        
        default: {
            DrawCircleV(e->position, e->collisionRadius, col);
            DrawCircleLinesV(e->position, e->collisionRadius + 2.0f, BLACK);
            break;
        }
    }
}
