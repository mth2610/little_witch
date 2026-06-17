// ============================================================
// main.c — Entry Point & Vòng lặp Gameplay chính
// ============================================================

#include "raylib.h"
#include "game.h"
#include "ui.h"
#include "save.h"
#include "admob_bridge.h"
#include "biome.h"
#include "raymath.h"
#include <stdlib.h>
#include <time.h>

// ----------------------------------------------------------------
// BIẾN TOÀN CỤC CHỨA TÀI NGUYÊN (LOAD Ở MAIN VÀ EXTERN Ở CÁC FILE KHÁC)
// ----------------------------------------------------------------
Texture2D witchFlyTexture;
Texture2D witchAttackHandTexture;
Texture2D witchAttackWeaponTexture;
Texture2D witchDefenseTexture;
Texture2D witchSpritesheetTexture;
bool witchSpritesheetLoaded = false;
Texture2D fluteTexture;
Texture2D starNormalTexture;
Texture2D starGoldTexture;
Texture2D starRainbowTexture;

Texture2D enemySlimeTexture;
Texture2D enemyBatTexture;
Texture2D enemyGhostTexture;
Texture2D enemyRobotTexture;
Texture2D enemyDroneTexture;
Texture2D enemyAlienTexture;
Texture2D enemyUfoTexture;

Texture2D bossForestTexture;
Texture2D bossCaveTexture;
Texture2D bossCityTexture;
Texture2D bossSpaceTexture;

Texture2D bgForestTexture;
Texture2D bgCaveTexture;
Texture2D bgCityTexture;
Texture2D bgSpaceTexture;

Sound collectStarSound;
Sound hitEnemySound;
Sound gameOverSound;
Sound shootSound;
Music bgMusic;

// Phông chữ vector toàn cục
Font gameFont;

// Shader khiên bảo vệ toàn cục
Shader shieldShader;
bool shieldShaderLoaded = false;
Texture2D shieldBackgroundTexture;

// Biến tỷ lệ và căn lề cho màn hình ảo (Letterbox)
static float s_scale = 1.0f;
static int s_offsetX = 0;
static int s_offsetY = 0;

// ----------------------------------------------------------------
// HÀM CÔNG KHAI
// ----------------------------------------------------------------

// Chuyển đổi tọa độ màn hình thật sang màn hình ảo 1280x720
Vector2 ScreenToVirtual(Vector2 screenPos) {
    Vector2 virtualPos = { 0.0f, 0.0f };
    virtualPos.x = (screenPos.x - s_offsetX) / s_scale;
    virtualPos.y = (screenPos.y - s_offsetY) / s_scale;
    return virtualPos;
}

// ----------------------------------------------------------------
// HÀM NỘI BỘ (static)
// ----------------------------------------------------------------

static void GenerateSkillChoices(GameState *gs) {
    // Chọn 3 kỹ năng duy nhất ngẫu nhiên từ 7 kỹ năng có sẵn (từ index 1 đến 7)
    int pool[7] = { 
        SKILL_SHURIKEN, 
        SKILL_POISON_CLOUD, 
        SKILL_ICE_BLAST, 
        SKILL_FIREBALL, 
        SKILL_LIGHTNING, 
        SKILL_MAGNET, 
        SKILL_SHIELD 
    };
    
    // Trộn ngẫu nhiên Fisher-Yates
    for (int i = 6; i > 0; i--) {
        int j = GetRandomValue(0, i);
        int temp = pool[i];
        pool[i] = pool[j];
        pool[j] = temp;
    }
    
    gs->skillChoices[0] = pool[0];
    gs->skillChoices[1] = pool[1];
    gs->skillChoices[2] = pool[2];
}

// Gọi khi tiêu diệt boss để bắt đầu quá trình nhận thẻ kỹ năng và chuyển vùng đất tiếp theo
static void AdvanceToNextBiome(GameState *gs) {
    if (gs == NULL) return;
    
    // 1. Sinh các hạt bụi ăn mừng nhiều màu sắc rực rỡ xung quanh phù thủy
    for (int i = 0; i < 45; i++) {
        float vx = (float)GetRandomValue(-250, 250);
        float vy = (float)GetRandomValue(-250, 250);
        Color colors[] = { GOLD, YELLOW, ORANGE, WHITE, PINK, SKYBLUE, LIME, PURPLE };
        Color col = colors[GetRandomValue(0, 7)];
        float lifetime = 1.2f + ((float)GetRandomValue(0, 10) / 10.0f);
        float size = 4.5f + ((float)GetRandomValue(0, 40) / 10.0f);
        SpawnParticle(&gs->particlePool, gs->witch.position, (Vector2){ vx, vy }, lifetime, size, col);
    }
    
    // Rung màn hình cực mạnh ăn mừng chiến thắng
    gs->shakeIntensity = 8.0f;
    gs->shakeDuration = 0.45f;
    
    // 2. Sinh các lựa chọn kỹ năng ngẫu nhiên cho màn hình chọn thẻ
    GenerateSkillChoices(gs);
    
    // 3. Chuyển sang màn hình chọn kỹ năng (SCREEN_SKILL_SELECT) để người chơi nâng cấp
    gs->currentScreen = SCREEN_SKILL_SELECT;
    
    // 4. Tắt cờ hoạt động của boss hiện tại
    gs->enemyPool.bossActive = false;
    gs->enemyPool.bossIndex = -1;
}

// Cập nhật toàn bộ các va chạm trong game loop gameplay
static void checkCollisions(GameState *gs) {
    Vector2 witchPos = gs->witch.position;
    float witchRadius = gs->witch.collisionRadius;
    float deltaTime = GetFrameTime();
    
    // 0. VA CHẠM: KHIÊN MA PHÁP VỚI QUÁI VẬT VÀ ĐẠN (ĐẨY LÙI & PHẢN ĐẠN)
    if (gs->witch.manaShieldHealth > 0.0f) {
        float shieldRadius = 100.0f + 12.0f * gs->witch.effectiveThuyStars;
        
        // 0.1 Đẩy lùi quái vật
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!gs->enemyPool.enemies[e].active) continue;
            Enemy *enemy = &gs->enemyPool.enemies[e];
            if (enemy->type == ENEMY_GHOST && !enemy->isVisible) continue;
            
            float dist = Vector2Distance(enemy->position, witchPos);
            float minDist = shieldRadius + enemy->collisionRadius;
            if (dist < minDist) {
                // Tính toán hướng đẩy lùi từ phù thủy sang quái vật
                Vector2 dir = { 1.0f, 0.0f };
                if (dist > 0.1f) {
                    dir = Vector2Normalize(Vector2Subtract(enemy->position, witchPos));
                } else {
                    dir = (Vector2){ (float)GetRandomValue(-10, 10), (float)GetRandomValue(-10, 10) };
                    if (dir.x == 0 && dir.y == 0) dir.x = 1.0f;
                    dir = Vector2Normalize(dir);
                }
                
                // Cập nhật vị trí ra ngoài rìa khiên
                enemy->position = Vector2Add(witchPos, Vector2Scale(dir, minDist + 5.0f));
                
                // Đẩy lùi và choáng
                float force = 650.0f;
                if (enemy->type >= ENEMY_BOSS_FOREST) {
                    force = 250.0f; // Boss đẩy lùi nhẹ hơn
                }
                enemy->knockbackVelocity = Vector2Scale(dir, force);
                enemy->knockbackTimer = 0.40f;
                
                // Hiệu ứng nổ hạt tại điểm chạm
                Vector2 contactPos = Vector2Add(witchPos, Vector2Scale(dir, shieldRadius));
                for (int i = 0; i < 5; i++) {
                    float vx = dir.x * 120.0f + (float)GetRandomValue(-45, 45);
                    float vy = dir.y * 120.0f + (float)GetRandomValue(-45, 45);
                    SpawnParticle(&gs->particlePool, contactPos, (Vector2){ vx, vy }, 0.35f, 2.0f, SKYBLUE);
                }
                
                // Gây sát thương nhỏ
                DamageEnemy(&gs->enemyPool, e, 2);
            }
        }
        
        // 0.2 Phản đạn của quái vật
        for (int p = 0; p < MAX_PROJECTILES; p++) {
            if (!gs->projectilePool.projectiles[p].active || !gs->projectilePool.projectiles[p].isEnemy) continue;
            Projectile *proj = &gs->projectilePool.projectiles[p];
            
            float dist = Vector2Distance(proj->position, witchPos);
            float minDist = shieldRadius + proj->radius;
            if (dist < minDist) {
                // Hướng đẩy đạn ra ngoài
                Vector2 dir = { 1.0f, 0.0f };
                if (dist > 0.1f) {
                    dir = Vector2Normalize(Vector2Subtract(proj->position, witchPos));
                }
                
                // Định vị lại đạn
                proj->position = Vector2Add(witchPos, Vector2Scale(dir, minDist + 2.0f));
                
                // Phản hồi vận tốc ra ngoài và tăng tốc độ bay
                float currentSpeed = Vector2Length(proj->velocity);
                if (currentSpeed < 100.0f) currentSpeed = 300.0f;
                proj->velocity = Vector2Scale(dir, currentSpeed * 1.25f);
                
                // Đổi phe đạn tiêu diệt quái vật
                proj->isEnemy = false;
                proj->color = SKYBLUE;
                proj->damage = 15; // Sát thương phản hồi cố định
                
                // Tạo hạt lấp lánh khi phản đạn
                for (int i = 0; i < 4; i++) {
                    float vx = dir.x * 80.0f + (float)GetRandomValue(-30, 30);
                    float vy = dir.y * 80.0f + (float)GetRandomValue(-30, 30);
                    SpawnParticle(&gs->particlePool, proj->position, (Vector2){ vx, vy }, 0.25f, 1.8f, CYAN);
                }
            }
        }
    }

    // 1. VA CHẠM: SAO ĐANG XOAY VỚI QUÁI VẬT (Sát thương tăng theo Mộc)
    for (int s = 0; s < MAX_STARS; s++) {
        if (!gs->starPool.stars[s].active || !gs->starPool.stars[s].isOrbiting) continue;
        
        Star *star = &gs->starPool.stars[s];
        
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!gs->enemyPool.enemies[e].active) continue;
            
            Enemy *enemy = &gs->enemyPool.enemies[e];
            if (enemy->type == ENEMY_GHOST && !enemy->isVisible) continue;
            
            float dist = Vector2Distance(star->position, enemy->position);
            if (dist < STAR_COLLISION_RADIUS + enemy->collisionRadius) {
                if (star->type == STAR_RAINBOW) {
                    if (enemy->rainbowCooldown <= 0.0f) {
                        int dmg = 15 + 3 * gs->witch.effectiveMocStars;
                        bool killed = DamageEnemy(&gs->enemyPool, e, dmg);
                        enemy->rainbowCooldown = 0.5f;
                        SpawnExplosion(&gs->particlePool, star->position, 6, PINK);
                        
                        if (killed) {
                            gs->witch.score += (enemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
                            gs->witch.gold += GetEnemyGoldDrop(enemy->type);
                            SpawnExplosion(&gs->particlePool, enemy->position, 14, RED);
                            if (enemy->type >= ENEMY_BOSS_FOREST) {
                                AdvanceToNextBiome(gs);
                            }
                        }
                    }
                } else {
                    int dmg = 10 + 2 * gs->witch.effectiveMocStars;
                    bool killed = DamageEnemy(&gs->enemyPool, e, dmg);
                    SpawnExplosion(&gs->particlePool, star->position, 8, star->tintColor);
                    DeactivateStar(&gs->starPool, s);
                    
                    if (killed) {
                        gs->witch.score += (enemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
                        gs->witch.gold += GetEnemyGoldDrop(enemy->type);
                        SpawnExplosion(&gs->particlePool, enemy->position, 14, RED);
                        if (enemy->type >= ENEMY_BOSS_FOREST) {
                            AdvanceToNextBiome(gs);
                        }
                    }
                    break;
                }
            }
        }
    }
    
    // 2.1 VA CHẠM: ĐẠN DUY TRÌ (Poison Cloud, Tornado) CỦA PHÙ THỦY VỚI QUÁI VẬT
    for (int p = 0; p < MAX_PROJECTILES; p++) {
        if (!gs->projectilePool.projectiles[p].active || gs->projectilePool.projectiles[p].isEnemy) continue;
        
        Projectile *proj = &gs->projectilePool.projectiles[p];
        if (proj->type != PROJ_POISON && proj->type != PROJ_TORNADO) continue;
        if (proj->isWeak) continue; // Đạn yếu ớt bay thẳng, va chạm trực tiếp ở 2.2
        
        if (proj->damageTimer <= 0.0f) {
            bool dealtDamage = false;
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!gs->enemyPool.enemies[e].active) continue;
                
                Enemy *enemy = &gs->enemyPool.enemies[e];
                if (enemy->type == ENEMY_GHOST && !enemy->isVisible) continue;
                
                float dist = Vector2Distance(proj->position, enemy->position);
                if (dist < proj->radius + enemy->collisionRadius) {
                    bool killed = DamageEnemy(&gs->enemyPool, e, proj->damage);
                    dealtDamage = true;
                    
                    if (proj->type == PROJ_TORNADO) {
                        // Lực hút lốc xoáy điện hệ Thổ
                        Vector2 pullDir = Vector2Normalize(Vector2Subtract(proj->position, enemy->position));
                        enemy->position = Vector2Add(enemy->position, Vector2Scale(pullDir, 90.0f * deltaTime));
                        if (GetRandomValue(0, 100) < 15) {
                            SpawnParticle(&gs->particlePool, enemy->position, (Vector2){ 0, -20.0f }, 0.4f, 2.0f, YELLOW);
                        }
                    } else {
                        // Mây độc hệ Mộc
                        if (GetRandomValue(0, 100) < 15) {
                            SpawnParticle(&gs->particlePool, enemy->position, (Vector2){ (float)GetRandomValue(-15,15), (float)GetRandomValue(-15,15) }, 0.4f, 2.0f, GREEN);
                        }
                    }
                    
                    if (killed) {
                        gs->witch.score += (enemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
                        gs->witch.gold += GetEnemyGoldDrop(enemy->type);
                        SpawnExplosion(&gs->particlePool, enemy->position, 14, RED);
                        if (enemy->type >= ENEMY_BOSS_FOREST) {
                            AdvanceToNextBiome(gs);
                        }
                    }
                }
            }
            if (dealtDamage) {
                proj->damageTimer = (proj->type == PROJ_TORNADO) ? 0.20f : 0.25f;
            }
        }
    }
    
    // 2.2 VA CHẠM: ĐẠN VA CHẠM TRỰC TIẾP CỦA PHÙ THỦY VỚI QUÁI VẬT
    for (int p = 0; p < MAX_PROJECTILES; p++) {
        if (!gs->projectilePool.projectiles[p].active || gs->projectilePool.projectiles[p].isEnemy) continue;
        
        Projectile *proj = &gs->projectilePool.projectiles[p];
        if (!proj->isWeak) {
            if (proj->type == PROJ_POISON || proj->type == PROJ_TORNADO) continue;
        }
        
        for (int e = 0; e < MAX_ENEMIES; e++) {
            if (!gs->enemyPool.enemies[e].active) continue;
            
            Enemy *enemy = &gs->enemyPool.enemies[e];
            if (enemy->type == ENEMY_GHOST && !enemy->isVisible) continue;
            
            float dist = Vector2Distance(proj->position, enemy->position);
            if (dist < proj->radius + enemy->collisionRadius) {
                if (proj->isWeak) {
                    // Đạn yếu: va chạm trực tiếp, gây sát thương thường, nổ nhỏ và biến mất
                    bool killed = DamageEnemy(&gs->enemyPool, e, proj->damage);
                    SpawnExplosion(&gs->particlePool, proj->position, 6, proj->color);
                    proj->active = false;
                    
                    if (killed) {
                        gs->witch.score += (enemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
                        gs->witch.gold += GetEnemyGoldDrop(enemy->type);
                        SpawnExplosion(&gs->particlePool, enemy->position, 14, RED);
                        if (enemy->type >= ENEMY_BOSS_FOREST) {
                            AdvanceToNextBiome(gs);
                        }
                    }
                    break; // Phá hủy đạn nên dừng check quái khác
                }
                else if (proj->type == PROJ_SHURIKEN) {
                    // Phi tiêu vàng xuyên thấu hệ Kim
                    if (enemy->rainbowCooldown <= 0.0f) {
                        bool killed = DamageEnemy(&gs->enemyPool, e, proj->damage);
                        enemy->rainbowCooldown = 0.25f;
                        SpawnExplosion(&gs->particlePool, proj->position, 5, GOLD);
                        
                        if (killed) {
                            gs->witch.score += (enemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
                            gs->witch.gold += GetEnemyGoldDrop(enemy->type);
                            SpawnExplosion(&gs->particlePool, enemy->position, 14, RED);
                            if (enemy->type >= ENEMY_BOSS_FOREST) {
                                AdvanceToNextBiome(gs);
                            }
                        }
                    }
                } 
                else if (proj->type == PROJ_ICE) {
                    // Đạn băng đông cứng hệ Thủy
                    bool killed = DamageEnemy(&gs->enemyPool, e, proj->damage);
                    int lvl = gs->witch.skillLevels[SKILL_ICE_BLAST];
                    enemy->freezeTimer = 1.5f + 0.3f * gs->witch.effectiveThuyStars + 0.5f * (lvl - 1);
                    SpawnExplosion(&gs->particlePool, proj->position, 10, SKYBLUE);
                    proj->active = false;
                    
                    if (killed) {
                        gs->witch.score += (enemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
                        gs->witch.gold += GetEnemyGoldDrop(enemy->type);
                        SpawnExplosion(&gs->particlePool, enemy->position, 14, RED);
                        if (enemy->type >= ENEMY_BOSS_FOREST) {
                            AdvanceToNextBiome(gs);
                        }
                    }
                    break;
                }
                else if (proj->isUltimate) {
                    // Nổ lan chiêu cuối Ultimate
                    SpawnExplosion(&gs->particlePool, proj->position, 22, proj->color);
                    float aoeRadius = 100.0f;
                    for (int other = 0; other < MAX_ENEMIES; other++) {
                        if (gs->enemyPool.enemies[other].active) {
                            Enemy *oEnemy = &gs->enemyPool.enemies[other];
                            if (oEnemy->type == ENEMY_GHOST && !oEnemy->isVisible) continue;
                            
                            float aoeDist = Vector2Distance(proj->position, oEnemy->position);
                            if (aoeDist < aoeRadius + oEnemy->collisionRadius) {
                                bool killed = DamageEnemy(&gs->enemyPool, other, proj->damage);
                                SpawnExplosion(&gs->particlePool, oEnemy->position, 6, proj->color);
                                if (killed) {
                                    gs->witch.score += (oEnemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
                                    gs->witch.gold += GetEnemyGoldDrop(oEnemy->type);
                                    SpawnExplosion(&gs->particlePool, oEnemy->position, 14, RED);
                                    if (oEnemy->type >= ENEMY_BOSS_FOREST) {
                                        AdvanceToNextBiome(gs);
                                    }
                                }
                            }
                        }
                    }
                    gs->shakeIntensity = 6.0f;
                    gs->shakeDuration = 0.22f;
                    proj->active = false;
                    break;
                }
                else {
                    // Cầu lửa nổ lan hệ HỎA
                    proj->active = false;
                    int lvl = gs->witch.skillLevels[SKILL_FIREBALL];
                    float aoeRadius = (60.0f + 10.0f * gs->witch.effectiveHoaStars) * (1.0f + 0.10f * (lvl - 1));
                    int aoeDmg = (int)((12 + 6 * gs->witch.effectiveHoaStars) * (1.0f + 0.20f * (lvl - 1)));
                    SpawnExplosion(&gs->particlePool, proj->position, 12, ORANGE);
                    
                    bool directKilled = DamageEnemy(&gs->enemyPool, e, proj->damage);
                    if (directKilled) {
                        gs->witch.score += (enemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
                        gs->witch.gold += GetEnemyGoldDrop(enemy->type);
                        SpawnExplosion(&gs->particlePool, enemy->position, 14, RED);
                        if (enemy->type >= ENEMY_BOSS_FOREST) {
                            AdvanceToNextBiome(gs);
                        }
                    }
                    
                    for (int other = 0; other < MAX_ENEMIES; other++) {
                        if (other == e) continue;
                        if (!gs->enemyPool.enemies[other].active) continue;
                        
                        Enemy *oEnemy = &gs->enemyPool.enemies[other];
                        if (oEnemy->type == ENEMY_GHOST && !oEnemy->isVisible) continue;
                        
                        float aoeDist = Vector2Distance(proj->position, oEnemy->position);
                        if (aoeDist < aoeRadius + oEnemy->collisionRadius) {
                            bool killed = DamageEnemy(&gs->enemyPool, other, aoeDmg);
                            SpawnExplosion(&gs->particlePool, oEnemy->position, 6, ORANGE);
                            if (killed) {
                                gs->witch.score += (oEnemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
                                gs->witch.gold += GetEnemyGoldDrop(oEnemy->type);
                                SpawnExplosion(&gs->particlePool, oEnemy->position, 14, RED);
                                if (oEnemy->type >= ENEMY_BOSS_FOREST) {
                                    AdvanceToNextBiome(gs);
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
    
    // 3. VA CHẠM: ĐẠN CỦA QUÁI VỚI PHÙ THỦY
    for (int p = 0; p < MAX_PROJECTILES; p++) {
        if (!gs->projectilePool.projectiles[p].active || !gs->projectilePool.projectiles[p].isEnemy) continue;
        
        Projectile *proj = &gs->projectilePool.projectiles[p];
        
        float dist = Vector2Distance(proj->position, witchPos);
        if (dist < proj->radius + witchRadius) {
            bool alive = WitchTakeDamage(&gs->witch);
            SpawnExplosion(&gs->particlePool, proj->position, 8, proj->color);
            proj->active = false;
            
            gs->shakeIntensity = 8.0f;
            gs->shakeDuration = 0.3f;
            
            if (!alive) {
                gs->currentScreen = SCREEN_GAMEOVER;
                gs->shakeDuration = 0.0f;
                gs->shakeIntensity = 0.0f;
                if (IsSoundReady(gameOverSound)) PlaySound(gameOverSound);
            }
        }
    }
    
    // 4. VA CHẠM: QUÁI VẬT CHẠM TRỰC TIẾP VỚI PHÙ THỦY
    for (int e = 0; e < MAX_ENEMIES; e++) {
        if (!gs->enemyPool.enemies[e].active) continue;
        
        Enemy *enemy = &gs->enemyPool.enemies[e];
        if (enemy->type == ENEMY_GHOST && !enemy->isVisible) continue;
        
        float dist = Vector2Distance(enemy->position, witchPos);
        if (dist < enemy->collisionRadius + witchRadius) {
            bool alive = WitchTakeDamage(&gs->witch);
            SpawnExplosion(&gs->particlePool, witchPos, 14, RED);
            
            gs->shakeIntensity = 8.0f;
            gs->shakeDuration = 0.3f;
            
            if (enemy->type < ENEMY_BOSS_FOREST) {
                enemy->active = false;
                SpawnExplosion(&gs->particlePool, enemy->position, 10, RED);
            }
            
            if (!alive) {
                gs->currentScreen = SCREEN_GAMEOVER;
                gs->shakeDuration = 0.0f;
                gs->shakeIntensity = 0.0f;
                if (IsSoundReady(gameOverSound)) PlaySound(gameOverSound);
            }
        }
    }
}

// Cập nhật trạng thái gameplay màn chơi chính
static void UpdateGameplay(GameState *gs, float deltaTime) {
    if (gs == NULL) return;
    
    // 1. CẬP NHẬT PHÙ THỦY & VỆT KHÓI KHÍ ĐỘNG HỌC
    UpdateWitch(&gs->witch, deltaTime);
    float scrollSpeed = GetBiomeConfig(gs->currentBiome).scrollSpeed;
    UpdateWitchTrail(&gs->trail, &gs->witch, deltaTime, scrollSpeed);
    
    // Sinh hạt bụi gió lướt bay phía sau phù thủy để tăng cảm giác tốc độ và gió thổi
    if (GetRandomValue(0, 100) < 35) {
        float dir = gs->witch.facingRight ? -1.0f : 1.0f;
        Vector2 pPos = { 
            gs->witch.position.x + dir * -25.0f, 
            gs->witch.position.y + (float)GetRandomValue(-35, 35) 
        };
        Vector2 pVel = { 
            dir * (220.0f + (float)GetRandomValue(0, 120)), 
            (float)GetRandomValue(-15, 15) 
        };
        float life = 0.30f + (float)GetRandomValue(0, 30) / 100.0f;
        float rad = 1.2f + (float)GetRandomValue(0, 20) / 10.0f;
        
        // Màu sắc của hạt gió lướt bay đồng bộ với màu vệt khói khí động học
        Color trailCol = GetWitchTrailColor(&gs->witch);
        Color pCol = ColorAlpha(trailCol, 0.40f);
        SpawnParticle(&gs->particlePool, pPos, pVel, life, rad, pCol);
    }
    
    // Nếu hết mạng thì Game Over ngay lập tức
    if (gs->witch.lives <= 0) {
        gs->currentScreen = SCREEN_GAMEOVER;
        gs->shakeDuration = 0.0f;
        gs->shakeIntensity = 0.0f;
        if (IsSoundReady(gameOverSound)) PlaySound(gameOverSound);
        return;
    }
    
    // 2. PHÁT TIẾNG BGM NẾU CÓ
    if (IsMusicReady(bgMusic)) {
        UpdateMusicStream(bgMusic);
    }
    
    // 3. XỬ LÝ SPAWN SAO TỰ DO
    gs->starPool.spawnTimer += deltaTime;
    // Tốc độ spawn sao tăng gấp đôi (giảm interval một nửa) ở vùng Space
    float effectiveStarInterval = (gs->currentBiome == BIOME_SPACE) ? 1.5f : 3.0f;
    
    int activeStarCount = 0;
    for (int i = 0; i < MAX_STARS; i++) {
        if (gs->starPool.stars[i].active) activeStarCount++;
    }
    
    if (gs->starPool.spawnTimer >= effectiveStarInterval && activeStarCount < MAX_STARS) {
        // Sinh tọa độ ngẫu nhiên bắt đầu từ ngoài màn hình bên phải để bay vào
        float sx = VIRTUAL_WIDTH + 40.0f;
        float sy = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);
        
        // Tính tỷ lệ sao dựa trên Nâng cấp May mắn
        int luckLevel = gs->witch.upgrades.level[UPGRADE_LUCK];
        float rainbowChance = 0.05f + 0.10f * luckLevel; // Cơ bản 5%, tăng 10% mỗi cấp
        float goldChance = 0.15f + 0.10f * luckLevel;    // Cơ bản 15%
        
        float roll = (float)GetRandomValue(0, 100) / 100.0f;
        StarType type = STAR_KIM;
        
        if (roll < rainbowChance) {
            type = STAR_RAINBOW;
        } else {
            // Chọn ngẫu nhiên 5 hệ ngũ hành: Kim, Mộc, Thủy, Hỏa, Thổ
            type = (StarType)GetRandomValue(STAR_KIM, STAR_THO);
        }
        
        SpawnStar(&gs->starPool, sx, sy, type);
        gs->starPool.spawnTimer = 0.0f;
    }
    
    // 4. XỬ LÝ SPAWN QUÁI THƯỜNG
    // Nếu chưa xuất hiện Trùm và chưa diệt trùm của Biome này
    if (!gs->enemyPool.bossActive && !gs->bossSpawned) {
        int threshold = (gs->currentBiome == BIOME_FOREST) ? 600 : BOSS_SPAWN_SCORE;
        
        // Nếu điểm đạt mốc threshold tính từ đầu biome thì triệu hồi trùm
        if (gs->witch.score >= gs->biomeStartScore + threshold) {
            SpawnBoss(&gs->enemyPool, gs->currentBiome);
            gs->bossSpawned = true;
        } else {
            // Ngược lại, tiếp tục sinh quái thường
            gs->enemyPool.spawnTimer += deltaTime;
            BiomeConfig biomeCfg = GetBiomeConfig(gs->currentBiome);
            float spawnRateInterval = 1.0f / biomeCfg.enemySpawnRate;
            
            if (gs->currentBiome == BIOME_FOREST) {
                int relativeScore = gs->witch.score - gs->biomeStartScore;
                if (relativeScore < 150) {
                    // Phase 1: Forest Silence (0 to 150 score)
                    spawnRateInterval = 2.5f;
                    if (gs->enemyPool.spawnTimer >= spawnRateInterval) {
                        float sy = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);
                        SpawnEnemy(&gs->enemyPool, ENEMY_SLIME, VIRTUAL_WIDTH + 60.0f, sy);
                        gs->enemyPool.spawnTimer = 0.0f;
                    }
                } else if (relativeScore < 400) {
                    // Phase 2: Shadowy Canopy (150 to 400 score)
                    spawnRateInterval = 1.8f;
                    if (gs->enemyPool.spawnTimer >= spawnRateInterval) {
                        float sy = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);
                        EnemyType spawnType = (GetRandomValue(0, 100) < 60) ? ENEMY_SLIME : ENEMY_BAT;
                        SpawnEnemy(&gs->enemyPool, spawnType, VIRTUAL_WIDTH + 60.0f, sy);
                        
                        // 20% tỉ lệ sinh thêm quái đôi ở vị trí lệch Y
                        if (GetRandomValue(0, 100) < 20) {
                            float sy2 = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);
                            if (fabsf(sy2 - sy) < 100.0f) {
                                sy2 = fmodf(sy2 + 200.0f, VIRTUAL_HEIGHT - 120.0f) + 60.0f;
                            }
                            EnemyType spawnType2 = (GetRandomValue(0, 100) < 60) ? ENEMY_SLIME : ENEMY_BAT;
                            SpawnEnemy(&gs->enemyPool, spawnType2, VIRTUAL_WIDTH + 60.0f, sy2);
                        }
                        gs->enemyPool.spawnTimer = 0.0f;
                    }
                } else {
                    // Phase 3: The Gathering Storm (400 to 600 score)
                    spawnRateInterval = 1.2f;
                    if (gs->enemyPool.spawnTimer >= spawnRateInterval) {
                        float sy = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);
                        EnemyType spawnType = (GetRandomValue(0, 100) < 40) ? ENEMY_SLIME : ENEMY_BAT;
                        SpawnEnemy(&gs->enemyPool, spawnType, VIRTUAL_WIDTH + 60.0f, sy);
                        
                        // Bắt đầu rung màn hình nhẹ từ 500 điểm báo hiệu boss chuẩn bị trỗi dậy
                        if (relativeScore >= 500 && fmodf(gs->frameCount, 180.0f) < 8.0f) {
                            gs->shakeIntensity = 2.0f;
                            gs->shakeDuration = 0.12f;
                        }
                        gs->enemyPool.spawnTimer = 0.0f;
                    }
                }
            } else {
                // Các biome khác giữ nguyên cơ chế spawn quái thường mặc định
                if (gs->enemyPool.spawnTimer >= spawnRateInterval) {
                    int typeIdx = GetRandomValue(0, biomeCfg.enemyTypeCount - 1);
                    EnemyType spawnType = biomeCfg.enemyTypes[typeIdx];
                    
                    float sy = (float)GetRandomValue(60, VIRTUAL_HEIGHT - 60);
                    SpawnEnemy(&gs->enemyPool, spawnType, VIRTUAL_WIDTH + 60.0f, sy);
                    
                    gs->enemyPool.spawnTimer = 0.0f;
                }
            }
        }
    }
    
    // 5. KÍCH HOẠT KỸ NĂNG CHỦ ĐỘNG (RPG ACTIVE SKILLS & BUFFS)
    for (int s = 1; s < SKILL_COUNT; s++) {
        if (!gs->witch.activeSkills[s]) continue;
        
        bool keyPressed = false;
        if (s == SKILL_SHURIKEN && IsKeyPressed(KEY_ONE)) keyPressed = true;
        else if (s == SKILL_POISON_CLOUD && IsKeyPressed(KEY_TWO)) keyPressed = true;
        else if (s == SKILL_ICE_BLAST && IsKeyPressed(KEY_THREE)) keyPressed = true;
        else if (s == SKILL_FIREBALL && IsKeyPressed(KEY_FOUR)) keyPressed = true;
        else if (s == SKILL_LIGHTNING && IsKeyPressed(KEY_FIVE)) keyPressed = true;
        else if (s == SKILL_MAGNET && IsKeyPressed(KEY_E)) keyPressed = true;
        else if (s == SKILL_SHIELD && IsKeyPressed(KEY_Q)) keyPressed = true;
        
        Vector2 center = GetSkillButtonCenter(s);
        bool buttonPressed = IsCircleButtonClicked(center, 28.0f);
        
        if ((keyPressed || buttonPressed) && gs->witch.skillCooldown[s] <= 0.0f) {
            CastActiveSkill(gs, s);
        }
    }
    
    // 5.5 TẤN CÔNG ĐÁNH THƯỜNG (PC: SPACEBAR, MOBILE: NÚT BẤM TRÒN LỚN)
    Vector2 attackCenter = { 1150.0f, 600.0f };
    bool normalAttackBtnPressed = IsCircleButtonClicked(attackCenter, 48.0f);
    
    if (IsKeyPressed(KEY_SPACE) || IsKeyDown(KEY_SPACE) || normalAttackBtnPressed) {
        if (gs->witch.keyboardAttackCooldown <= 0.0f) {
            // Gọi hàm tấn công thường đã được module hóa
            // Lý do: Đảm bảo đòn đánh thường yếu hoạt động chính xác khi có 0 sao và đòn đánh chuẩn hoạt động khi có >= 1 sao
            CastNormalAttack(gs);
            
            // Thiết lập hồi chiêu đánh thường bằng phím cách (0.35s)
            gs->witch.keyboardAttackCooldown = 0.35f;
        }
    }
    
    // 6. CẬP NHẬT CÁC ĐỐI TƯỢNG TRONG POOL
    bool hasMagnet = true; // Nam châm bị động hệ Kim luôn kích hoạt
    BiomeConfig biomeCfg = GetBiomeConfig(gs->currentBiome);
    UpdateStars(&gs->starPool, &gs->witch, deltaTime, hasMagnet, biomeCfg.scrollSpeed, &gs->particlePool);
    UpdateEnemies(gs, deltaTime);
    
    // Cập nhật lan truyền sát thương sét chuỗi Chain Lightning thời gian thực
    UpdateSkills(gs, deltaTime);
    UpdateProjectiles(&gs->projectilePool, deltaTime, &gs->particlePool, &gs->enemyPool);
    UpdateParticles(&gs->particlePool, deltaTime);
    
    // Sinh hạt bụi sáng trôi nổi trong không khí (Ambient floating dust) phong cách Ori
    if (GetRandomValue(0, 100) < 12) {
        Vector2 pos = { VIRTUAL_WIDTH + 15.0f, (float)GetRandomValue(10, VIRTUAL_HEIGHT - 10) };
        Vector2 vel = { -(45.0f + (float)GetRandomValue(0, 50)), (float)GetRandomValue(-8, 8) };
        float lifetime = 5.0f + ((float)rand() / RAND_MAX) * 3.0f;
        float size = 1.2f + ((float)rand() / RAND_MAX) * 1.8f;
        
        Color col = LIME;
        if (gs->currentBiome == BIOME_FOREST) col = (GetRandomValue(0, 1) == 0 ? LIME : GOLD);
        else if (gs->currentBiome == BIOME_CAVE) col = (GetRandomValue(0, 1) == 0 ? SKYBLUE : VIOLET);
        else if (gs->currentBiome == BIOME_CITY) col = (GetRandomValue(0, 1) == 0 ? MAGENTA : CYAN);
        else if (gs->currentBiome == BIOME_SPACE) col = (GetRandomValue(0, 1) == 0 ? PINK : SKYBLUE);
        
        SpawnParticle(&gs->particlePool, pos, vel, lifetime, size, col);
    }
    
    // 7. XỬ LÝ COOLDOWN RUNG MÀN HÌNH (SCREEN SHAKE)
    if (gs->shakeDuration > 0.0f) {
        gs->shakeDuration -= deltaTime;
        gs->shakeIntensity = Lerp(gs->shakeIntensity, 0.0f, deltaTime * 6.0f);
        if (gs->shakeDuration <= 0.0f) {
            gs->shakeDuration = 0.0f;
            gs->shakeIntensity = 0.0f;
        }
    }
    
    // Cập nhật timer flash chớp sáng chiêu cuối Ultimate
    if (gs->ultimateFlashTimer > 0.0f) {
        gs->ultimateFlashTimer -= deltaTime;
        if (gs->ultimateFlashTimer < 0.0f) gs->ultimateFlashTimer = 0.0f;
    }
    
    // 8. KIỂM TRA VA CHẠM
    checkCollisions(gs);
    
    // 9. KÍCH HOẠT CHIÊU CUỐI ULTIMATE: ELEMENTAL BURST BẰNG BÀN PHÍM/TOUCH
    if (gs->currentScreen == SCREEN_GAMEPLAY) {
        int rainbowCount = 0;
        int kim = 0, moc = 0, thuy = 0, hoa = 0, tho = 0;
        for (int i = 0; i < MAX_STARS; i++) {
            if (gs->starPool.stars[i].active && gs->starPool.stars[i].isOrbiting) {
                if (gs->starPool.stars[i].type == STAR_RAINBOW) rainbowCount++;
                else if (gs->starPool.stars[i].type == STAR_KIM) kim++;
                else if (gs->starPool.stars[i].type == STAR_MOC) moc++;
                else if (gs->starPool.stars[i].type == STAR_THUY) thuy++;
                else if (gs->starPool.stars[i].type == STAR_HOA) hoa++;
                else if (gs->starPool.stars[i].type == STAR_THO) tho++;
            }
        }
        bool isUltReady = (rainbowCount > 0) || (kim > 0 && moc > 0 && thuy > 0 && hoa > 0 && tho > 0);
        
        Vector2 ultCenter = { 1150.0f, 360.0f };
        bool ultButtonClicked = IsCircleButtonClicked(ultCenter, 36.0f);
        
        if (isUltReady && (IsKeyPressed(KEY_F) || ultButtonClicked)) {
            TriggerUltimate(gs);
        }
    }
    
    // Cập nhật biến frame toàn cục để phục vụ nhấp nháy sao, v.v.
    gs->frameCount++;
}

// ----------------------------------------------------------------
// CHƯƠNG TRÌNH CHÍNH (main)
// ----------------------------------------------------------------
int main(void) {
#ifndef __ANDROID__
    // Tự động tìm và chuyển đổi thư mục làm việc về thư mục gốc chứa tài nguyên
    // nếu chạy từ thư mục con (ví dụ: build/ hay build/bin/)
    if (!DirectoryExists("assets")) {
        if (DirectoryExists("../assets")) {
            ChangeDirectory("..");
        } else if (DirectoryExists("../../assets")) {
            ChangeDirectory("../..");
        }
    }
#endif

    // Khởi tạo hạt giống ngẫu nhiên thời gian thực
    srand((unsigned int)time(NULL));
    
    // Khởi tạo Raylib Window
    // Trên máy tính chạy chế độ cửa sổ to 1280x720, trên Android tự động giãn toàn màn hình
#ifdef __ANDROID__
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Little Witch Adventure");
#else
    InitWindow(1280, 720, "Little Witch Adventure");
#endif

    SetTargetFPS(TARGET_FPS);
    InitAudioDevice();
    
    // Nạp gói dữ liệu lưu trữ
    SaveData save = LoadSaveData();
    
    // Khởi tạo cấu trúc GameState toàn cục
    GameState gs = {0};
    gs.currentScreen = SCREEN_MENU;
    gs.currentBiome = BIOME_FOREST;
    gs.savedUpgrades = save.upgrades;
    gs.language = save.language; // Đồng bộ ngôn ngữ từ file save
    gs.cachedSave = save;        // Cache save data tránh đọc file I/O mỗi frame
    gs.biomeStartScore = 0;      // Biome đầu tiên bắt đầu từ 0 điểm
    gs.bossSpawned = false;
    gs.shakeDuration = 0.0f;
    gs.shakeIntensity = 0.0f;
    
    // Khởi tạo hệ thống quảng cáo AdMob
    AdMob_Init();
    
    // Nạp phông chữ vector sắc nét nếu tồn tại, nếu không dùng phông mặc định của Raylib
    if (FileExists("assets/fonts/clean_font.ttf")) {
        gameFont = LoadFontEx("assets/fonts/clean_font.ttf", 64, NULL, 0);
        SetTextureFilter(gameFont.texture, TEXTURE_FILTER_BILINEAR);
    } else {
        gameFont = GetFontDefault();
    }
    
    // Khởi tạo Virtual Canvas kích thước cố định 1280x720
    gs.virtualCanvas = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(gs.virtualCanvas.texture, TEXTURE_FILTER_BILINEAR);
    
    // Khởi tạo Lightmap Canvas kích thước cố định 1280x720
    gs.lightmap = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(gs.lightmap.texture, TEXTURE_FILTER_BILINEAR);
    
    // Khởi tạo Screen Copy Canvas kích thước cố định 1280x720 cho hiệu ứng Distortion
    gs.screenCopyTex = LoadRenderTexture(VIRTUAL_WIDTH, VIRTUAL_HEIGHT);
    SetTextureFilter(gs.screenCopyTex.texture, TEXTURE_FILTER_BILINEAR);
    
    // Khởi tạo texture ảo 2x2 màu trắng để map UV cho shader
    Image dummyImg = GenImageColor(2, 2, WHITE);
    gs.dummyWhiteTex = LoadTextureFromImage(dummyImg);
    UnloadImage(dummyImg);
    
    // Khởi tạo Bloom Canvases (320x180 để tối ưu hóa hiệu năng)
    gs.bloomCanvas = LoadRenderTexture(320, 180);
    SetTextureFilter(gs.bloomCanvas.texture, TEXTURE_FILTER_BILINEAR);
    gs.blurCanvas = LoadRenderTexture(320, 180);
    SetTextureFilter(gs.blurCanvas.texture, TEXTURE_FILTER_BILINEAR);

#ifdef __ANDROID__
#define SHADER_PATH(name) "assets/shaders/gles/" name
#else
#define SHADER_PATH(name) "assets/shaders/glsl330/" name
#endif
    
    // Nạp Shader xích sét (Chain Lightning)
    if (FileExists(SHADER_PATH("lightning.fs"))) {
        gs.lightningShader = LoadShader(NULL, SHADER_PATH("lightning.fs"));
        gs.lightningShaderLoaded = true;
    } else {
        gs.lightningShaderLoaded = false;
    }
    
    // Nạp Shader khiên ma thuật (Mana Shield)
    if (FileExists(SHADER_PATH("shield.fs"))) {
        shieldShader = LoadShader(NULL, SHADER_PATH("shield.fs"));
        shieldShaderLoaded = true;
    } else {
        shieldShaderLoaded = false;
    }
    
    // Nạp Shader hiệu ứng kỹ năng Ngũ hành (5 hệ)
    if (FileExists(SHADER_PATH("fireball.fs"))) {
        gs.fireballShader = LoadShader(NULL, SHADER_PATH("fireball.fs"));
        gs.fireballShaderLoaded = true;
    } else {
        gs.fireballShaderLoaded = false;
    }
    if (FileExists(SHADER_PATH("ice_blast.fs"))) {
        gs.iceBlastShader = LoadShader(NULL, SHADER_PATH("ice_blast.fs"));
        gs.iceBlastShaderLoaded = true;
    } else {
        gs.iceBlastShaderLoaded = false;
    }
    if (FileExists(SHADER_PATH("poison_cloud.fs"))) {
        gs.poisonCloudShader = LoadShader(NULL, SHADER_PATH("poison_cloud.fs"));
        gs.poisonCloudShaderLoaded = true;
    } else {
        gs.poisonCloudShaderLoaded = false;
    }
    if (FileExists(SHADER_PATH("shuriken.fs"))) {
        gs.shurikenShader = LoadShader(NULL, SHADER_PATH("shuriken.fs"));
        gs.shurikenShaderLoaded = true;
    } else {
        gs.shurikenShaderLoaded = false;
    }
    if (FileExists(SHADER_PATH("tornado.fs"))) {
        gs.tornadoShader = LoadShader(NULL, SHADER_PATH("tornado.fs"));
        gs.tornadoShaderLoaded = true;
    } else {
        gs.tornadoShaderLoaded = false;
    }

    // Nạp Bloom Shaders cho hiệu ứng Ori-glow
    if (FileExists(SHADER_PATH("bloom_extract.fs"))) {
        gs.bloomExtractShader = LoadShader(NULL, SHADER_PATH("bloom_extract.fs"));
        gs.bloomExtractShaderLoaded = true;
    } else {
        gs.bloomExtractShaderLoaded = false;
    }
    if (FileExists(SHADER_PATH("bloom_blur.fs"))) {
        gs.bloomBlurShader = LoadShader(NULL, SHADER_PATH("bloom_blur.fs"));
        gs.bloomBlurShaderLoaded = true;
    } else {
        gs.bloomBlurShaderLoaded = false;
    }
    
    // Nạp Shader hạt sương giá (Frost Vapor)
    if (FileExists(SHADER_PATH("vapor.fs"))) {
        gs.vaporShader = LoadShader(NULL, SHADER_PATH("vapor.fs"));
        gs.vaporShaderLoaded = true;
    } else {
        gs.vaporShaderLoaded = false;
    }

#undef SHADER_PATH
    
    // ----------------------------------------------------------------
    // NẠP TÀI NGUYÊN GAME (LOAD TEXTURE & SOUNDS)
    // ----------------------------------------------------------------
    // Kiểm tra an toàn sự tồn tại của file trước khi load, tránh treo app
    if (FileExists("assets/little_witch.png")) {
        witchSpritesheetTexture = LoadTexture("assets/little_witch.png");
        witchSpritesheetLoaded = true;
    } else {
        witchSpritesheetLoaded = false;
    }
    if (FileExists("assets/textures/witch_fly.png")) {
        witchFlyTexture = LoadTexture("assets/textures/witch_fly.png");
    }
    if (FileExists("assets/textures/witch_attack_hand.png")) {
        witchAttackHandTexture = LoadTexture("assets/textures/witch_attack_hand.png");
    }
    if (FileExists("assets/textures/witch_attack_weapon.png")) {
        witchAttackWeaponTexture = LoadTexture("assets/textures/witch_attack_weapon.png");
    }
    if (FileExists("assets/textures/witch_defense.png")) {
        witchDefenseTexture = LoadTexture("assets/textures/witch_defense.png");
    }
    if (FileExists("assets/textures/star_normal.png")) {
        starNormalTexture = LoadTexture("assets/textures/star_normal.png");
    }
    if (FileExists("assets/textures/star_gold.png")) {
        starGoldTexture = LoadTexture("assets/textures/star_gold.png");
    }
    if (FileExists("assets/textures/star_rainbow.png")) {
        starRainbowTexture = LoadTexture("assets/textures/star_rainbow.png");
    }
    if (FileExists("assets/textures/flute.png")) {
        fluteTexture = LoadTexture("assets/textures/flute.png");
    }
    
    if (FileExists("assets/textures/enemies/slime.png")) {
        enemySlimeTexture = LoadTexture("assets/textures/enemies/slime.png");
    }
    if (FileExists("assets/textures/enemies/bat.png")) {
        enemyBatTexture = LoadTexture("assets/textures/enemies/bat.png");
    }
    if (FileExists("assets/textures/enemies/ghost.png")) {
        enemyGhostTexture = LoadTexture("assets/textures/enemies/ghost.png");
    }
    if (FileExists("assets/textures/enemies/robot.png")) {
        enemyRobotTexture = LoadTexture("assets/textures/enemies/robot.png");
    }
    if (FileExists("assets/textures/enemies/drone.png")) {
        enemyDroneTexture = LoadTexture("assets/textures/enemies/drone.png");
    }
    if (FileExists("assets/textures/enemies/alien.png")) {
        enemyAlienTexture = LoadTexture("assets/textures/enemies/alien.png");
    }
    if (FileExists("assets/textures/enemies/ufo.png")) {
        enemyUfoTexture = LoadTexture("assets/textures/enemies/ufo.png");
    }
    
    if (FileExists("assets/textures/enemies/boss_forest.png")) {
        bossForestTexture = LoadTexture("assets/textures/enemies/boss_forest.png");
    }
    if (FileExists("assets/textures/enemies/boss_cave.png")) {
        bossCaveTexture = LoadTexture("assets/textures/enemies/boss_cave.png");
    }
    if (FileExists("assets/textures/enemies/boss_city.png")) {
        bossCityTexture = LoadTexture("assets/textures/enemies/boss_city.png");
    }
    if (FileExists("assets/textures/enemies/boss_space.png")) {
        bossSpaceTexture = LoadTexture("assets/textures/enemies/boss_space.png");
    }
    
    if (FileExists("assets/textures/biomes/bg_forest.png")) {
        bgForestTexture = LoadTexture("assets/textures/biomes/bg_forest.png");
    }
    if (FileExists("assets/textures/biomes/bg_cave.png")) {
        bgCaveTexture = LoadTexture("assets/textures/biomes/bg_cave.png");
    }
    if (FileExists("assets/textures/biomes/bg_city.png")) {
        bgCityTexture = LoadTexture("assets/textures/biomes/bg_city.png");
    }
    if (FileExists("assets/textures/biomes/bg_space.png")) {
        bgSpaceTexture = LoadTexture("assets/textures/biomes/bg_space.png");
    }
    
    if (FileExists("assets/sounds/collect_star.wav")) {
        collectStarSound = LoadSound("assets/sounds/collect_star.wav");
    }
    if (FileExists("assets/sounds/hit_enemy.wav")) {
        hitEnemySound = LoadSound("assets/sounds/hit_enemy.wav");
    }
    if (FileExists("assets/sounds/game_over.wav")) {
        gameOverSound = LoadSound("assets/sounds/game_over.wav");
    }
    if (FileExists("assets/sounds/shoot_fireball.wav")) {
        shootSound = LoadSound("assets/sounds/shoot_fireball.wav");
    }
    if (FileExists("assets/sounds/bgm_forest.ogg")) {
        bgMusic = LoadMusicStream("assets/sounds/bgm_forest.ogg");
        bgMusic.looping = true;
        PlayMusicStream(bgMusic);
    }
    
    // ----------------------------------------------------------------
    // GAME LOOP CHÍNH
    // ----------------------------------------------------------------
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // --- 1. CẬP NHẬT LOGIC THEO SCREEN HIỆN TẠI ---
        switch (gs.currentScreen) {
            case SCREEN_MENU:
                UpdateMenu(&gs, dt);
                break;
                
            case SCREEN_GAMEPLAY:
                UpdateGameplay(&gs, dt);
                break;
                
            case SCREEN_SKILL_SELECT:
                UpdateSkillSelect(&gs);
                break;
                
            case SCREEN_UPGRADE_SHOP:
                UpdateUpgradeShop(&gs);
                break;
                
            case SCREEN_GAMEOVER:
                UpdateGameOver(&gs);
                break;
        }
        
        // --- 2. RENDER VÀO VIRTUAL CANVAS 1280x720 ---
        BeginTextureMode(gs.virtualCanvas);
            ClearBackground(BLACK);
            
            // Vẽ nền background cuộn dựa trên màn hình
            if (gs.currentScreen == SCREEN_GAMEPLAY || 
                gs.currentScreen == SCREEN_SKILL_SELECT || 
                gs.currentScreen == SCREEN_GAMEOVER) {
                // deltaTime được truyền là 0 khi đứng yên ở card chọn kỹ năng để dừng cuộn nền
                float bgDt = (gs.currentScreen == SCREEN_SKILL_SELECT) ? 0.0f : dt;
                
                // --- BƯỚC 1: Vẽ thế giới Game bình thường ---
                DrawBackground(&gs, bgDt);
                DrawStars(&gs.starPool, gs.witch.position);
                DrawEnemies(&gs.enemyPool);
                
                // Chụp màn hình hiện tại (background, stars, enemies) để truyền làm screenTexture cho Distortion
                EndTextureMode();
                BeginTextureMode(gs.screenCopyTex);
                    ClearBackground(BLACK);
                    DrawTextureRec(gs.virtualCanvas.texture, (Rectangle){ 0.0f, 0.0f, (float)VIRTUAL_WIDTH, -(float)VIRTUAL_HEIGHT }, (Vector2){ 0, 0 }, WHITE);
                EndTextureMode();
                BeginTextureMode(gs.virtualCanvas);
                
                shieldBackgroundTexture = gs.screenCopyTex.texture;
                
                DrawProjectiles(&gs.projectilePool, &gs);
                DrawParticlesEx(&gs.particlePool, &gs);
                DrawWitchTrail(&gs.trail);
                DrawWitch(&gs.witch);
                
                // --- Vẽ xích sét (Chain Lightning) ---
                DrawSkills(&gs);
                
                DrawForeground(&gs, bgDt);
                
                // Vẽ hiệu ứng chớp sáng trắng Ultimate đè lên thế giới game
                if (gs.ultimateFlashTimer > 0.0f) {
                    float flashAlpha = (gs.ultimateFlashTimer / 0.20f) * 0.45f;
                    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, ColorAlpha(WHITE, flashAlpha));
                }
                
                // --- BƯỚC 2: Vẽ bản đồ ánh sáng 2D (Lightmap) ---
                EndTextureMode(); // Tạm dừng vẽ lên virtualCanvas
                
                BeginTextureMode(gs.lightmap);
                    // Chọn màu tối của Biome hiện tại (Ambient Darkness - tối hơn để tăng độ sâu và tương phản phong cách Ori)
                    Color ambientColor = (Color){ 45, 52, 65, 255 }; // Forest ambient
                    if (gs.currentBiome == BIOME_FOREST) {
                        int relativeScore = gs.witch.score - gs.biomeStartScore;
                        if (relativeScore >= 500 && !gs.enemyPool.bossActive && !gs.bossSpawned) {
                            ambientColor = (Color){ 22, 26, 32, 255 }; // Tối dần khi boss sắp đến
                        } else if (gs.enemyPool.bossActive) {
                            ambientColor = (Color){ 12, 14, 18, 255 }; // Tối hẳn khi đấu boss
                        }
                    }
                    else if (gs.currentBiome == BIOME_CAVE) ambientColor = (Color){ 25, 20, 32, 255 }; // Cave ambient
                    else if (gs.currentBiome == BIOME_CITY) ambientColor = (Color){ 45, 45, 60, 255 }; // City ambient
                    else if (gs.currentBiome == BIOME_SPACE) ambientColor = (Color){ 18, 12, 30, 255 }; // Space ambient
                    
                    ClearBackground(ambientColor);
                    
                    BeginBlendMode(BLEND_ADDITIVE);
                        // A. Ánh sáng phát ra từ Phù thủy (Màu xanh cyan/trắng dịu phong cách Ori)
                        float witchPulse = sinf(gs.frameCount * 0.05f) * 12.0f;
                        // Vầng sáng rộng ngoài
                        DrawCircleGradient(gs.witch.position.x, gs.witch.position.y, 180.0f + witchPulse, ColorAlpha(SKYBLUE, 0.55f), ColorAlpha(SKYBLUE, 0.0f));
                        // Vầng sáng trung
                        DrawCircleGradient(gs.witch.position.x, gs.witch.position.y, 95.0f + witchPulse * 0.5f, ColorAlpha(SKYBLUE, 0.75f), ColorAlpha(SKYBLUE, 0.0f));
                        // Lõi sáng trắng cực mạnh tỏa rộng bao phủ hoàn toàn thân nhân vật 112x112px (tránh mờ nhạt/giống ma)
                        DrawCircle(gs.witch.position.x, gs.witch.position.y, 52.0f, ColorAlpha(WHITE, 1.0f));
                        DrawCircleGradient(gs.witch.position.x, gs.witch.position.y, 80.0f, ColorAlpha(WHITE, 1.0f), ColorAlpha(WHITE, 0.0f));
                        
                        // Nếu khiên ma pháp đang hoạt động, tỏa ra vầng sáng bảo hộ khổng lồ màu xanh trời trên lightmap
                        if (gs.witch.manaShieldHealth > 0.0f) {
                            float shieldPulse = sinf(gs.frameCount * 0.08f) * 15.0f;
                            DrawCircleGradient(gs.witch.position.x, gs.witch.position.y, 260.0f + shieldPulse, ColorAlpha(SKYBLUE, 0.65f), ColorAlpha(SKYBLUE, 0.0f));
                            DrawCircleGradient(gs.witch.position.x, gs.witch.position.y, 130.0f, ColorAlpha(WHITE, 0.45f), ColorAlpha(WHITE, 0.0f));
                        }
                        
                        // B. Ánh sáng phát ra từ các ngôi sao bảo vệ/sao tự do (Ori-style glowing stars)
                        for (int i = 0; i < MAX_STARS; i++) {
                            if (gs.starPool.stars[i].active) {
                                Vector2 starPos = gs.starPool.stars[i].position;
                                Color starGlow = gs.starPool.stars[i].tintColor;
                                float starPulse = sinf(gs.frameCount * 0.1f + i) * 3.0f;
                                // Vầng sáng màu rộng hơn, rực rỡ hơn
                                DrawCircleGradient(starPos.x, starPos.y, 75.0f + starPulse, ColorAlpha(starGlow, 0.8f), ColorAlpha(starGlow, 0.0f));
                                // Lõi sáng trắng cực mạnh ở tâm với bán kính lớn hơn để vật thể giữ màu gốc tốt hơn
                                DrawCircleGradient(starPos.x, starPos.y, 35.0f, ColorAlpha(WHITE, 1.0f), ColorAlpha(WHITE, 0.0f));
                                DrawCircleV(starPos, 12.0f, ColorAlpha(WHITE, 1.0f));
                            }
                        }
                        
                        // C. Ánh sáng phát ra từ đạn phép thuật (Projectiles)
                        for (int i = 0; i < MAX_PROJECTILES; i++) {
                            if (gs.projectilePool.projectiles[i].active) {
                                const Projectile *p = &gs.projectilePool.projectiles[i];
                                if (p->isEnemy) {
                                    // Đạn của quái giữ nguyên vẽ thông thường
                                    DrawCircleGradient(p->position.x, p->position.y, 55.0f, ColorAlpha(p->color, 0.75f), ColorAlpha(p->color, 0.0f));
                                    DrawCircleGradient(p->position.x, p->position.y, 18.0f, ColorAlpha(WHITE, 1.0f), ColorAlpha(WHITE, 0.0f));
                                } else {
                                    // Đạn của người chơi: Tỏa sáng theo hệ Ngũ hành
                                    if (p->type == PROJ_FIREBALL) {
                                        // Vầng sáng cam/đỏ rộng 80px, lõi vàng trắng, nhấp nháy nhanh giống ngọn lửa
                                        float flicker = 1.0f + sinf(GetTime() * 25.0f + i) * 0.15f;
                                        float size = 80.0f * flicker;
                                        DrawCircleGradient(p->position.x, p->position.y, size, ColorAlpha(ORANGE, 0.75f), ColorAlpha(ORANGE, 0.0f));
                                        DrawCircleGradient(p->position.x, p->position.y, size * 0.4f, ColorAlpha(YELLOW, 0.9f), ColorAlpha(YELLOW, 0.0f));
                                        DrawCircleGradient(p->position.x, p->position.y, size * 0.15f, ColorAlpha(WHITE, 1.0f), ColorAlpha(WHITE, 0.0f));
                                    } else if (p->type == PROJ_ICE) {
                                        // Vầng sáng cyan nhạt 60px, tĩnh lặng, hơi sương trắng mỏng xung quanh
                                        float size = 60.0f;
                                        DrawCircleGradient(p->position.x, p->position.y, size * 1.5f, ColorAlpha(WHITE, 0.25f), ColorAlpha(WHITE, 0.0f)); // Sương trắng mỏng
                                        DrawCircleGradient(p->position.x, p->position.y, size, ColorAlpha(CYAN, 0.8f), ColorAlpha(CYAN, 0.0f));
                                        DrawCircleGradient(p->position.x, p->position.y, size * 0.3f, ColorAlpha(WHITE, 1.0f), ColorAlpha(WHITE, 0.0f));
                                    } else if (p->type == PROJ_POISON) {
                                        // Vầng sáng xanh lá pulsating chậm 100px
                                        float pulse = 1.0f + sinf(GetTime() * 4.0f + i) * 0.12f;
                                        float size = 100.0f * pulse;
                                        DrawCircleGradient(p->position.x, p->position.y, size, ColorAlpha(LIME, 0.65f), ColorAlpha(LIME, 0.0f));
                                        DrawCircleGradient(p->position.x, p->position.y, size * 0.3f, ColorAlpha(GREEN, 0.8f), ColorAlpha(GREEN, 0.0f));
                                    } else if (p->type == PROJ_SHURIKEN) {
                                        // Vầng sáng vàng kim nhỏ gọn 40px nhưng rất sáng, flicker nhanh
                                        float flicker = 1.0f + sinf(GetTime() * 35.0f + i) * 0.18f;
                                        float size = 40.0f * flicker;
                                        DrawCircleGradient(p->position.x, p->position.y, size, ColorAlpha(GOLD, 0.9f), ColorAlpha(GOLD, 0.0f));
                                        DrawCircleGradient(p->position.x, p->position.y, size * 0.4f, ColorAlpha(WHITE, 1.0f), ColorAlpha(WHITE, 0.0f));
                                    } else if (p->type == PROJ_TORNADO) {
                                        // Vầng sáng dọc theo chiều cao lốc xoáy, cam-vàng, flash sét trắng ngẫu nhiên
                                        float heightVal = p->radius * 3.5f;
                                        float flash = (sinf(GetTime() * 50.0f + i) > 0.85f) ? 1.0f : 0.0f;
                                        // Vẽ nhiều nốt sáng chồng lên nhau theo chiều dọc phễu lốc xoáy
                                        for (int step = 0; step < 5; step++) {
                                            float t = step / 4.0f; // 0 đáy, 1 đỉnh
                                            float offsety = -t * heightVal;
                                            float size = p->radius * (1.5f - t * 0.8f) * 2.0f;
                                            Color glowColor = ColorAlpha(ORANGE, 0.4f - t * 0.2f);
                                            DrawCircleGradient(p->position.x, p->position.y + offsety, size, glowColor, ColorAlpha(ORANGE, 0.0f));
                                            
                                            // Sét nhấp nháy trắng ở lõi lốc xoáy
                                            if (flash > 0.5f) {
                                                DrawCircleGradient(p->position.x, p->position.y + offsety, size * 0.3f, ColorAlpha(WHITE, 0.8f), ColorAlpha(WHITE, 0.0f));
                                            }
                                        }
                                    } else {
                                        // Default fallback (nếu có loại đạn khác của người chơi)
                                        DrawCircleGradient(p->position.x, p->position.y, 55.0f, ColorAlpha(p->color, 0.75f), ColorAlpha(p->color, 0.0f));
                                        DrawCircleGradient(p->position.x, p->position.y, 18.0f, ColorAlpha(WHITE, 1.0f), ColorAlpha(WHITE, 0.0f));
                                    }
                                }
                            }
                        }
                        
                        // D. Ánh sáng phát ra từ hạt bụi (Particles)
                        for (int i = 0; i < MAX_PARTICLES; i++) {
                            if (gs.particlePool.particles[i].active) {
                                Vector2 pPos = gs.particlePool.particles[i].position;
                                Color pCol = gs.particlePool.particles[i].color;
                                float pAlpha = gs.particlePool.particles[i].alpha;
                                float r = gs.particlePool.particles[i].radius;
                                DrawCircleGradient(pPos.x, pPos.y, r * 5.0f, ColorAlpha(pCol, pAlpha * 0.5f), ColorAlpha(pCol, 0.0f));
                                DrawCircleGradient(pPos.x, pPos.y, r * 1.8f, ColorAlpha(WHITE, pAlpha * 0.7f), ColorAlpha(WHITE, 0.0f));
                            }
                        }
                        
                        // E. Ánh sáng phát ra từ quái vật và Boss (giúp chúng rực rỡ phong cách Ori)
                        for (int i = 0; i < MAX_ENEMIES; i++) {
                            if (gs.enemyPool.enemies[i].active) {
                                Enemy *enemy = &gs.enemyPool.enemies[i];
                                float pulse = sinf(gs.frameCount * 0.08f + i) * 4.0f;
                                float r = enemy->collisionRadius;
                                
                                Color glowColor = RED;
                                if (enemy->type == ENEMY_SLIME) glowColor = LIME;
                                else if (enemy->type == ENEMY_BAT) glowColor = ORANGE;
                                else if (enemy->type == ENEMY_GHOST) glowColor = VIOLET;
                                else if (enemy->type == ENEMY_ROBOT) glowColor = SKYBLUE;
                                else if (enemy->type == ENEMY_DRONE) glowColor = CYAN;
                                else if (enemy->type == ENEMY_ALIEN) glowColor = GREEN;
                                else if (enemy->type == ENEMY_UFO) glowColor = MAGENTA;
                                else if (enemy->type >= ENEMY_BOSS_FOREST) glowColor = RED;
                                
                                // Bỏ qua ghost tàng hình
                                if (enemy->type == ENEMY_GHOST && !enemy->isVisible) continue;
                                
                                // Vẽ vầng sáng màu rộng
                                DrawCircleGradient(enemy->position.x, enemy->position.y, r * 3.2f + pulse, ColorAlpha(glowColor, 0.6f), ColorAlpha(glowColor, 0.0f));
                                // Vẽ lõi sáng trắng cực mạnh để quái giữ nguyên màu sắc gốc và nổi bật
                                DrawCircleGradient(enemy->position.x, enemy->position.y, r * 1.3f, ColorAlpha(WHITE, 1.0f), ColorAlpha(WHITE, 0.0f));
                                DrawCircleV(enemy->position, r * 0.7f, ColorAlpha(WHITE, 1.0f));
                            }
                        }
                        
                        // F. Ánh sáng phát ra từ tia sét (Chain Lightning)
                        DrawSkillsLightmap(&gs);
                        
                        // G. Ánh sáng phát ra từ môi trường (Đom đóm, Tinh thể, Tinh vân phong cách Ori)
                        if (gs.currentBiome == BIOME_FOREST) {
                            float rawScroll = gs.bgScrollOffset;
                            for (int f = 0; f < 12; f++) {
                                float fireflyX = fmodf(f * 113.0f - rawScroll * 0.3f, VIRTUAL_WIDTH);
                                if (fireflyX < 0) fireflyX += VIRTUAL_WIDTH;
                                float fireflyY = fmodf(f * 89.0f + sinf(gs.frameCount * 0.04f + f) * 20.0f, VIRTUAL_HEIGHT);
                                float pulse = 1.0f + sinf(gs.frameCount * 0.1f + f) * 0.4f;
                                DrawCircleGradient(fireflyX, fireflyY, 50.0f * pulse, ColorAlpha(LIME, 0.45f), ColorAlpha(LIME, 0.0f));
                                DrawCircleGradient(fireflyX, fireflyY, 20.0f * pulse, ColorAlpha(YELLOW, 0.7f), ColorAlpha(YELLOW, 0.0f));
                            }
                        } else if (gs.currentBiome == BIOME_CAVE) {
                            float rawScroll = gs.bgScrollOffset;
                            for (int i = 0; i < 6; i++) {
                                float cx = fmodf(i * 240.0f - rawScroll, VIRTUAL_WIDTH + 100.0f) - 50.0f;
                                if (cx < -100.0f) cx += VIRTUAL_WIDTH + 100.0f;
                                float pulse = 0.5f + 0.5f * sinf(gs.frameCount * 0.06f + i);
                                DrawCircleGradient(cx + 40.0f, VIRTUAL_HEIGHT - 50.0f, 90.0f * pulse, ColorAlpha(SKYBLUE, 0.5f), ColorAlpha(SKYBLUE, 0.0f));
                                DrawCircleGradient(cx + 40.0f, VIRTUAL_HEIGHT - 50.0f, 30.0f * pulse, ColorAlpha(WHITE, 0.7f), ColorAlpha(WHITE, 0.0f));
                            }
                        } else if (gs.currentBiome == BIOME_SPACE) {
                            float nebSpeed = 0.08f;
                            float rawScroll = gs.bgScrollOffset;
                            for (int n = 0; n < 3; n++) {
                                float x = fmodf(n * 500.0f - rawScroll * nebSpeed, VIRTUAL_WIDTH + 400.0f) - 200.0f;
                                Vector2 center = { x, VIRTUAL_HEIGHT * 0.4f + n * 70.0f };
                                Color nebColor = (n == 0) ? PURPLE : (n == 1 ? DARKBLUE : MAGENTA);
                                float pulse = 1.0f + sinf(gs.frameCount * 0.02f + n) * 0.15f;
                                DrawCircleGradient(center.x, center.y, 260.0f * pulse, ColorAlpha(nebColor, 0.22f), ColorAlpha(nebColor, 0.0f));
                            }
                        }
                        
                    EndBlendMode();
                EndTextureMode();
                
                // --- BƯỚC 3: Áp dụng bản đồ ánh sáng (Multiply Blend) ---
                BeginTextureMode(gs.virtualCanvas);
                    BeginBlendMode(BLEND_MULTIPLIED);
                        DrawTexturePro(
                            gs.lightmap.texture,
                            (Rectangle){ 0.0f, 0.0f, (float)VIRTUAL_WIDTH, -(float)VIRTUAL_HEIGHT },
                            (Rectangle){ 0.0f, 0.0f, (float)VIRTUAL_WIDTH, (float)VIRTUAL_HEIGHT },
                            (Vector2){ 0, 0 }, 0.0f, WHITE
                        );
                    EndBlendMode();
                EndTextureMode(); // Tạm dừng vẽ lên virtualCanvas để áp dụng Bloom
                
                // --- BƯỚC 3.5: Áp dụng Bloom Pipeline ---
                if (gs.bloomExtractShaderLoaded && gs.bloomBlurShaderLoaded) {
                    Vector2 renderSz = { 320.0f, 180.0f };
                    // 1. Trích xuất các pixel sáng sang bloomCanvas (320x180)
                    BeginTextureMode(gs.bloomCanvas);
                        ClearBackground(BLACK);
                        BeginShaderMode(gs.bloomExtractShader);
                            float thresholdVal = 0.75f;
                            SetShaderValue(gs.bloomExtractShader, GetShaderLocation(gs.bloomExtractShader, "threshold"), &thresholdVal, SHADER_UNIFORM_FLOAT);
                            DrawTexturePro(
                                gs.virtualCanvas.texture,
                                (Rectangle){ 0.0f, 0.0f, (float)VIRTUAL_WIDTH, -(float)VIRTUAL_HEIGHT },
                                (Rectangle){ 0.0f, 0.0f, 320.0f, 180.0f },
                                (Vector2){ 0, 0 }, 0.0f, WHITE
                            );
                        EndShaderMode();
                    EndTextureMode();
                    
                    // 2. Blur ngang: bloomCanvas -> blurCanvas
                    BeginTextureMode(gs.blurCanvas);
                        ClearBackground(BLACK);
                        BeginShaderMode(gs.bloomBlurShader);
                            int horiz = 1;
                            SetShaderValue(gs.bloomBlurShader, GetShaderLocation(gs.bloomBlurShader, "horizontal"), &horiz, SHADER_UNIFORM_INT);
                            SetShaderValue(gs.bloomBlurShader, GetShaderLocation(gs.bloomBlurShader, "renderSize"), &renderSz, SHADER_UNIFORM_VEC2);
                            DrawTexturePro(
                                gs.bloomCanvas.texture,
                                (Rectangle){ 0.0f, 0.0f, 320.0f, -180.0f },
                                (Rectangle){ 0.0f, 0.0f, 320.0f, 180.0f },
                                (Vector2){ 0, 0 }, 0.0f, WHITE
                            );
                        EndShaderMode();
                    EndTextureMode();
                    
                    // 3. Blur dọc: blurCanvas -> bloomCanvas
                    BeginTextureMode(gs.bloomCanvas);
                        ClearBackground(BLACK);
                        BeginShaderMode(gs.bloomBlurShader);
                            int vert = 0;
                            SetShaderValue(gs.bloomBlurShader, GetShaderLocation(gs.bloomBlurShader, "horizontal"), &vert, SHADER_UNIFORM_INT);
                            SetShaderValue(gs.bloomBlurShader, GetShaderLocation(gs.bloomBlurShader, "renderSize"), &renderSz, SHADER_UNIFORM_VEC2);
                            DrawTexturePro(
                                gs.blurCanvas.texture,
                                (Rectangle){ 0.0f, 0.0f, 320.0f, -180.0f },
                                (Rectangle){ 0.0f, 0.0f, 320.0f, 180.0f },
                                (Vector2){ 0, 0 }, 0.0f, WHITE
                            );
                        EndShaderMode();
                    EndTextureMode();
                    
                    // 4. Cộng chập Bloom (BLEND_ADDITIVE) quay lại virtualCanvas
                    BeginTextureMode(gs.virtualCanvas);
                        BeginBlendMode(BLEND_ADDITIVE);
                            DrawTexturePro(
                                gs.bloomCanvas.texture,
                                (Rectangle){ 0.0f, 0.0f, 320.0f, -180.0f },
                                (Rectangle){ 0.0f, 0.0f, (float)VIRTUAL_WIDTH, (float)VIRTUAL_HEIGHT },
                                (Vector2){ 0, 0 }, 0.0f, WHITE
                            );
                        EndBlendMode();
                } else {
                    BeginTextureMode(gs.virtualCanvas);
                }
                
                // --- BƯỚC 4: Vẽ HUD lên trên cùng ---
                DrawHUD(&gs);
            } 
            else if (gs.currentScreen == SCREEN_MENU) {
                // Menu chính
                DrawBackground(&gs, dt * 0.2f); // Cuộn nền cực chậm ở menu
                DrawMenu(&gs);
            } 
            else if (gs.currentScreen == SCREEN_UPGRADE_SHOP) {
                // Cửa hàng shop
                DrawBackground(&gs, dt * 0.2f);
                DrawUpgradeShop(&gs);
            }
            
            // Lớp vẽ đè UI overlay
            if (gs.currentScreen == SCREEN_SKILL_SELECT) {
                DrawSkillSelect(&gs);
            } else if (gs.currentScreen == SCREEN_GAMEOVER) {
                DrawGameOver(&gs);
            }
        EndTextureMode();
        
        // --- 3. SCALE VÀ VẼ VIRTUAL CANVAS RA MÀN HÌNH THẬT (LETTERBOXING) ---
        BeginDrawing();
            ClearBackground(BLACK);
            
            // Tính toán tỷ lệ co giãn tối ưu giữ nguyên aspect ratio 16:9
            float scaleX = (float)GetScreenWidth() / VIRTUAL_WIDTH;
            float scaleY = (float)GetScreenHeight() / VIRTUAL_HEIGHT;
            s_scale = (scaleX < scaleY) ? scaleX : scaleY; // Lấy min scale
            
            // Tính toán khoảng đệm đen (Letterbox / Pillarbox)
            s_offsetX = (GetScreenWidth() - (int)(VIRTUAL_WIDTH * s_scale)) / 2;
            s_offsetY = (GetScreenHeight() - (int)(VIRTUAL_HEIGHT * s_scale)) / 2;
            
            // Cấu hình tọa độ vẽ virtual canvas
            Rectangle sourceRec = { 0.0f, 0.0f, (float)VIRTUAL_WIDTH, -(float)VIRTUAL_HEIGHT }; // Lật ngược Y vì RenderTexture lưu tọa độ ngược
            Rectangle destRec = { 
                (float)s_offsetX, 
                (float)s_offsetY, 
                VIRTUAL_WIDTH * s_scale, 
                VIRTUAL_HEIGHT * s_scale 
            };
            
            // Áp dụng rung giật màn hình (Screen Shake) lên tọa độ canvas đích nếu có và đang ở Gameplay
            if (gs.shakeDuration > 0.0f && gs.currentScreen == SCREEN_GAMEPLAY) {
                destRec.x += GetRandomValue(-gs.shakeIntensity, gs.shakeIntensity) * s_scale;
                destRec.y += GetRandomValue(-gs.shakeIntensity, gs.shakeIntensity) * s_scale;
            }
            
            // Vẽ Canvas ra màn hình thực tế
            DrawTexturePro(gs.virtualCanvas.texture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
            
            // Vẽ lại 2 dải đen che phủ rìa ngoài để letterboxing sắc nét hoàn hảo
            if (s_offsetX > 0) {
                DrawRectangle(0, 0, s_offsetX, GetScreenHeight(), BLACK);
                DrawRectangle(GetScreenWidth() - s_offsetX, 0, s_offsetX, GetScreenHeight(), BLACK);
            }
            if (s_offsetY > 0) {
                DrawRectangle(0, 0, GetScreenWidth(), s_offsetY, BLACK);
                DrawRectangle(0, GetScreenHeight() - s_offsetY, GetScreenWidth(), s_offsetY, BLACK);
            }
        EndDrawing();
    }
    
    // ----------------------------------------------------------------
    // GIẢI PHÓNG BỘ NHỚ VÀ TÀI NGUYÊN (CLEANUP)
    // ----------------------------------------------------------------
    UnloadRenderTexture(gs.virtualCanvas);
    UnloadRenderTexture(gs.lightmap);
    UnloadRenderTexture(gs.screenCopyTex);
    UnloadRenderTexture(gs.bloomCanvas);
    UnloadRenderTexture(gs.blurCanvas);
    if (IsTextureReady(gs.dummyWhiteTex)) UnloadTexture(gs.dummyWhiteTex);
    
    if (IsTextureReady(witchFlyTexture))          UnloadTexture(witchFlyTexture);
    if (IsTextureReady(witchAttackHandTexture))   UnloadTexture(witchAttackHandTexture);
    if (IsTextureReady(witchAttackWeaponTexture)) UnloadTexture(witchAttackWeaponTexture);
    if (IsTextureReady(witchDefenseTexture))      UnloadTexture(witchDefenseTexture);
    if (IsTextureReady(witchSpritesheetTexture))  UnloadTexture(witchSpritesheetTexture);
    if (IsTextureReady(starNormalTexture))   UnloadTexture(starNormalTexture);
    if (IsTextureReady(starGoldTexture))     UnloadTexture(starGoldTexture);
    if (IsTextureReady(starRainbowTexture))  UnloadTexture(starRainbowTexture);
    if (IsTextureReady(fluteTexture))        UnloadTexture(fluteTexture);
    
    if (IsTextureReady(enemySlimeTexture))   UnloadTexture(enemySlimeTexture);
    if (IsTextureReady(enemyBatTexture))     UnloadTexture(enemyBatTexture);
    if (IsTextureReady(enemyGhostTexture))   UnloadTexture(enemyGhostTexture);
    if (IsTextureReady(enemyRobotTexture))   UnloadTexture(enemyRobotTexture);
    if (IsTextureReady(enemyDroneTexture))   UnloadTexture(enemyDroneTexture);
    if (IsTextureReady(enemyAlienTexture))   UnloadTexture(enemyAlienTexture);
    if (IsTextureReady(enemyUfoTexture))     UnloadTexture(enemyUfoTexture);
    
    if (IsTextureReady(bossForestTexture))   UnloadTexture(bossForestTexture);
    if (IsTextureReady(bossCaveTexture))     UnloadTexture(bossCaveTexture);
    if (IsTextureReady(bossCityTexture))     UnloadTexture(bossCityTexture);
    if (IsTextureReady(bossSpaceTexture))    UnloadTexture(bossSpaceTexture);
    
    if (IsTextureReady(bgForestTexture))     UnloadTexture(bgForestTexture);
    if (IsTextureReady(bgCaveTexture))       UnloadTexture(bgCaveTexture);
    if (IsTextureReady(bgCityTexture))       UnloadTexture(bgCityTexture);
    if (IsTextureReady(bgSpaceTexture))      UnloadTexture(bgSpaceTexture);
    
    if (IsSoundReady(collectStarSound))      UnloadSound(collectStarSound);
    if (IsSoundReady(hitEnemySound))         UnloadSound(hitEnemySound);
    if (IsSoundReady(gameOverSound))         UnloadSound(gameOverSound);
    if (IsSoundReady(shootSound))            UnloadSound(shootSound);
    if (IsMusicReady(bgMusic))               UnloadMusicStream(bgMusic);
    
    // Giải phóng phông chữ vector
    if (gameFont.texture.id != GetFontDefault().texture.id) {
        UnloadFont(gameFont);
    }
    
    if (gs.lightningShaderLoaded) {
        UnloadShader(gs.lightningShader);
    }
    
    if (shieldShaderLoaded) {
        UnloadShader(shieldShader);
    }

    if (gs.fireballShaderLoaded)    UnloadShader(gs.fireballShader);
    if (gs.iceBlastShaderLoaded)    UnloadShader(gs.iceBlastShader);
    if (gs.poisonCloudShaderLoaded) UnloadShader(gs.poisonCloudShader);
    if (gs.shurikenShaderLoaded)    UnloadShader(gs.shurikenShader);
    if (gs.tornadoShaderLoaded)     UnloadShader(gs.tornadoShader);
    if (gs.bloomExtractShaderLoaded) UnloadShader(gs.bloomExtractShader);
    if (gs.bloomBlurShaderLoaded)    UnloadShader(gs.bloomBlurShader);
    if (gs.vaporShaderLoaded)        UnloadShader(gs.vaporShader);
    
    CloseAudioDevice();
    CloseWindow();
    
    return 0;
}
