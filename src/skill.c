// ============================================================
// skill.c — Logic các kỹ năng (RPG Active Skills) và Đạn (Projectiles)
// ============================================================

#include "skill.h"
#include "game.h"
#include "particle.h"
#include "raymath.h"
#include <stddef.h>
#include <stdlib.h>
#include <math.h>

// Âm thanh được load ở main.c
extern Sound collectStarSound;
extern Sound shootSound;

// ----------------------------------------------------------------
// QUẢN LÝ POOL ĐẠN (PROJECTILES)
// ----------------------------------------------------------------

// Khởi tạo pool quản lý đạn
void InitProjectilePool(ProjectilePool *pool) {
    if (pool == NULL) return;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        pool->projectiles[i].active = false;
    }
}

// Sinh đạn mới vào pool
int SpawnProjectile(ProjectilePool *pool, Vector2 position, Vector2 velocity,
                     float radius, int damage, bool isEnemy, Color color, ProjectileType type) {
    if (pool == NULL) return -1;
    
    // Tìm slot trống
    int freeIdx = -1;
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!pool->projectiles[i].active) {
            freeIdx = i;
            break;
        }
    }
    
    // Nếu đầy pool thì bỏ qua
    if (freeIdx == -1) return -1;
    
    Projectile *p = &pool->projectiles[freeIdx];
    p->active = true;
    p->position = position;
    p->velocity = velocity;
    p->radius = radius;
    p->damage = damage;
    p->isEnemy = isEnemy;
    p->color = color;
    p->type = type;
    p->timer = 0.0f;
    p->damageTimer = 0.0f;
    p->isUltimate = false;
    p->rotation = 0.0f;
    
    return freeIdx;
}

// Cập nhật tọa độ đạn, bẻ lái homing của Ultimate và hủy nếu ra ngoài màn hình ảo
void UpdateProjectiles(ProjectilePool *pool, float deltaTime, ParticlePool *partPool, struct EnemyPool *enemyPool) {
    if (pool == NULL) return;
    
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!pool->projectiles[i].active) continue;
        
        Projectile *p = &pool->projectiles[i];
        
        // 1. Xử lý bẻ lái homing của đạn Ultimate
        if (p->isUltimate && enemyPool != NULL) {
            int targetIdx = -1;
            float minDist = 999999.0f;
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (enemyPool->enemies[e].active) {
                    if (enemyPool->enemies[e].type == ENEMY_GHOST && !enemyPool->enemies[e].isVisible) continue;
                    
                    float dist = Vector2Distance(p->position, enemyPool->enemies[e].position);
                    if (dist < minDist) {
                        minDist = dist;
                        targetIdx = e;
                    }
                }
            }
            
            if (targetIdx != -1) {
                Vector2 targetPos = enemyPool->enemies[targetIdx].position;
                Vector2 targetDir = Vector2Normalize(Vector2Subtract(targetPos, p->position));
                Vector2 currentDir = Vector2Normalize(p->velocity);
                Vector2 lerpDir = Vector2Normalize(Vector2Lerp(currentDir, targetDir, 8.0f * deltaTime));
                p->velocity = Vector2Scale(lerpDir, 920.0f);
            }
        }
        
        // 2. Cập nhật góc xoay và logic loại đạn
        if (p->type == PROJ_SHURIKEN) {
            p->rotation += 850.0f * deltaTime;
        } else if (p->type == PROJ_TORNADO) {
            p->rotation += 400.0f * deltaTime;
            p->timer -= deltaTime;
            if (p->timer <= 0.0f) {
                p->active = false;
                continue;
            }
            p->position.y += sinf(GetTime() * 12.0f + i) * 60.0f * deltaTime;
        } else if (p->type == PROJ_POISON) {
            p->timer -= deltaTime;
            if (p->timer <= 0.0f) {
                p->active = false;
                continue;
            }
            if (p->damageTimer > 0.0f) {
                p->damageTimer -= deltaTime;
            }
        } else if (p->type == PROJ_ICE) {
            p->rotation = atan2f(p->velocity.y, p->velocity.x) * RAD2DEG;
        }
        
        // Di chuyển đạn
        p->position.x += p->velocity.x * deltaTime;
        p->position.y += p->velocity.y * deltaTime;
        
        // Sinh hạt bụi ma thuật lấp lánh (sparkle trail) ở phía sau đuôi đạn
        if (partPool != NULL) {
            float roll = (float)GetRandomValue(0, 100) / 100.0f;
            float chance = 0.35f;
            if (p->type == PROJ_POISON) chance = 0.65f;
            else if (p->type == PROJ_TORNADO) chance = 0.70f;
            
            if (roll < chance) {
                float size = p->radius * 0.3f + ((float)rand() / RAND_MAX) * 2.0f;
                Vector2 trailVel = { -p->velocity.x * 0.25f, -p->velocity.y * 0.25f };
                
                if (p->type == PROJ_POISON) {
                    trailVel = (Vector2){ (float)GetRandomValue(-20, 20), (float)GetRandomValue(-40, -10) };
                    size = 2.0f + ((float)rand() / RAND_MAX) * 3.5f;
                } else if (p->type == PROJ_TORNADO) {
                    float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
                    Vector2 offset = { cosf(angle) * p->radius * 0.8f, sinf(angle) * p->radius * 0.4f };
                    Vector2 partPos = Vector2Add(p->position, offset);
                    SpawnParticle(partPool, partPos, (Vector2){ 0, -50.0f }, 0.5f, 2.0f, p->color);
                    continue;
                }
                
                float lifetime = 0.3f + ((float)rand() / RAND_MAX) * 0.3f;
                if (p->type == PROJ_POISON) lifetime = 0.5f + ((float)rand() / RAND_MAX) * 0.6f;
                
                SpawnParticle(partPool, p->position, trailVel, lifetime, size, p->color);
            }
        }
        
        // Hủy đạn nếu bay quá xa ngoài vùng hiển thị
        if (p->type != PROJ_POISON) {
            if (p->position.x < -100 || p->position.x > VIRTUAL_WIDTH + 100 ||
                p->position.y < -100 || p->position.y > VIRTUAL_HEIGHT + 100) {
                p->active = false;
            }
        }
    }
}

// Vẽ toàn bộ các viên đạn đang hoạt động
void DrawProjectiles(const ProjectilePool *pool) {
    if (pool == NULL) return;
    
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        if (!pool->projectiles[i].active) continue;
        
        const Projectile *p = &pool->projectiles[i];
        
        // 1. Vẽ đạn Ultimate
        if (p->isUltimate) {
            float t = GetTime() * 12.0f + i;
            Color ultColor = ColorFromHSV(fmodf(t * 30.0f, 360.0f), 0.8f, 1.0f);
            DrawCircleV(p->position, p->radius * 1.5f, ColorAlpha(ultColor, 0.35f));
            DrawCircleV(p->position, p->radius, ultColor);
            DrawCircleV(p->position, p->radius * 0.4f, WHITE);
            continue;
        }
        
        // 2. Vẽ theo loại đạn
        if (p->type == PROJ_SHURIKEN) {
            float rotRad = p->rotation * DEG2RAD;
            for (int j = 0; j < 4; j++) {
                float angle = rotRad + j * (PI / 2.0f);
                Vector2 tip = { p->position.x + cosf(angle) * p->radius * 1.5f, p->position.y + sinf(angle) * p->radius * 1.5f };
                Vector2 side1 = { p->position.x + cosf(angle + 0.25f) * p->radius * 0.4f, p->position.y + sinf(angle + 0.25f) * p->radius * 0.4f };
                Vector2 side2 = { p->position.x + cosf(angle - 0.25f) * p->radius * 0.4f, p->position.y - sinf(angle - 0.25f) * p->radius * 0.4f };
                // Sửa nhẹ cosf/sinf cho side2 để đối xứng
                side2 = (Vector2){ p->position.x + cosf(angle - 0.25f) * p->radius * 0.4f, p->position.y + sinf(angle - 0.25f) * p->radius * 0.4f };
                DrawTriangle(tip, side1, side2, p->color);
                DrawTriangle(tip, side2, side1, p->color);
            }
            DrawCircleV(p->position, p->radius * 0.35f, WHITE);
            DrawCircleLines(p->position.x, p->position.y, p->radius * 0.4f, YELLOW);
        } else if (p->type == PROJ_POISON) {
            float pulse = 1.0f + sinf(GetTime() * 6.0f + i) * 0.12f;
            DrawCircleGradient(p->position.x, p->position.y, p->radius * pulse, ColorAlpha(p->color, 0.28f), ColorAlpha(p->color, 0.0f));
            DrawCircleGradient(p->position.x, p->position.y, p->radius * 0.5f * pulse, ColorAlpha(WHITE, 0.15f), ColorAlpha(p->color, 0.0f));
        } else if (p->type == PROJ_TORNADO) {
            float tRot = p->rotation * DEG2RAD;
            for (int ring = 0; ring < 6; ring++) {
                float offsetOffset = ring * 9.0f;
                float ringRadX = p->radius * (1.1f - ring * 0.13f);
                float ringRadY = ringRadX * 0.35f;
                Vector2 ringCenter = { p->position.x, p->position.y - offsetOffset + 20.0f };
                
                float angleShift = tRot + ring * 0.4f;
                Vector2 pt1 = { ringCenter.x + cosf(angleShift) * ringRadX, ringCenter.y + sinf(angleShift) * ringRadY };
                Vector2 pt2 = { ringCenter.x + cosf(angleShift + PI) * ringRadX, ringCenter.y + sinf(angleShift + PI) * ringRadY };
                
                DrawLineEx(pt1, pt2, 4.0f - ring * 0.5f, ColorAlpha(p->color, 0.45f));
                DrawCircleV(pt1, 2.8f, WHITE);
            }
        } else if (p->type == PROJ_ICE) {
            float rotRad = p->rotation * DEG2RAD;
            Vector2 p1 = { p->position.x + cosf(rotRad) * p->radius * 1.7f, p->position.y + sinf(rotRad) * p->radius * 1.7f };
            Vector2 p2 = { p->position.x + cosf(rotRad + PI/2.0f) * p->radius * 0.5f, p->position.y + sinf(rotRad + PI/2.0f) * p->radius * 0.5f };
            Vector2 p3 = { p->position.x - cosf(rotRad) * p->radius * 0.6f, p->position.y - sinf(rotRad) * p->radius * 0.6f };
            Vector2 p4 = { p->position.x - cosf(rotRad + PI/2.0f) * p->radius * 0.5f, p->position.y - sinf(rotRad + PI/2.0f) * p->radius * 0.5f };
            DrawTriangle(p1, p2, p4, p->color);
            DrawTriangle(p3, p4, p2, p->color);
            DrawTriangle(p1, p4, p2, WHITE);
        } else {
            DrawCircleV(p->position, p->radius * 1.8f, ColorAlpha(p->color, 0.35f));
            DrawCircleV(p->position, p->radius, p->color);
            DrawCircleV(p->position, p->radius * 0.4f, WHITE);
        }
    }
    EndBlendMode();
}

// ----------------------------------------------------------------
// QUẢN LÝ KÍCH HOẠT VÀ TÁC ĐỘNG KỸ NĂNG (ACTIVE RPG SKILLS)
// ----------------------------------------------------------------

// Kích hoạt kỹ năng chủ động khi nhấn phím hoặc click HUD
void CastActiveSkill(struct GameState *gs, SkillType skill) {
    if (gs == NULL) return;
    
    switch (skill) {
        case SKILL_SHURIKEN: {
            // Hệ KIM: Phi tiêu vàng lấp lánh xuyên thấu
            int lvl = gs->witch.skillLevels[SKILL_SHURIKEN];
            int count = 1 + (gs->witch.effectiveKimStars / 2) + (lvl - 1);
            if (count > 6) count = 6;
            
            float baseAngle = 0.0f;
            float spread = 15.0f * DEG2RAD;
            for (int j = 0; j < count; j++) {
                float angle = baseAngle + (j - (count - 1) / 2.0f) * spread;
                Vector2 vel = { cosf(angle) * 550.0f, sinf(angle) * 550.0f };
                float radius = (11.0f + 1.5f * gs->witch.effectiveKimStars) * (1.0f + 0.08f * (lvl - 1));
                int dmg = (20 + 8 * gs->witch.effectiveKimStars) * (1.0f + 0.15f * (lvl - 1));
                
                SpawnProjectile(&gs->projectilePool, gs->witch.position, vel, radius, dmg, false, GOLD, PROJ_SHURIKEN);
            }
            gs->witch.skillCooldown[SKILL_SHURIKEN] = fmaxf(1.0f, 2.0f - 0.15f * (lvl - 1));
            gs->witch.animState = WITCH_STATE_ATTACK_HAND;
            gs->witch.stateTimer = 0.3f;
            if (IsSoundReady(shootSound)) PlaySound(shootSound);
            
            // Hiệu ứng hạt
            for (int i = 0; i < 10; i++) {
                float vx = (float)GetRandomValue(-80, 80);
                float vy = (float)GetRandomValue(-80, 80);
                SpawnParticle(&gs->particlePool, gs->witch.position, (Vector2){ vx, vy }, 0.4f, 2.5f, GOLD);
            }
            break;
        }
        
        case SKILL_POISON_CLOUD: {
            // Hệ MỘC: Làn hơi độc màu xanh lá
            int lvl = gs->witch.skillLevels[SKILL_POISON_CLOUD];
            int targetIdx = -1;
            float minDist = 999999.0f;
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (gs->enemyPool.enemies[e].active) {
                    if (gs->enemyPool.enemies[e].type == ENEMY_GHOST && !gs->enemyPool.enemies[e].isVisible) continue;
                    float dist = Vector2Distance(gs->witch.position, gs->enemyPool.enemies[e].position);
                    if (dist < minDist) {
                        minDist = dist;
                        targetIdx = e;
                    }
                }
            }
            
            Vector2 spawnPos = { gs->witch.position.x + 180.0f, gs->witch.position.y };
            if (targetIdx != -1) {
                spawnPos = gs->enemyPool.enemies[targetIdx].position;
            }
            
            float radius = (60.0f + 12.0f * gs->witch.effectiveMocStars) * (1.0f + 0.12f * (lvl - 1));
            int dmg = (5 + 2 * gs->witch.effectiveMocStars) * (1.0f + 0.25f * (lvl - 1));
            
            int idx = SpawnProjectile(&gs->projectilePool, spawnPos, (Vector2){ -20.0f, 0.0f }, radius, dmg, false, (Color){ 0, 230, 100, 255 }, PROJ_POISON);
            if (idx != -1) {
                gs->projectilePool.projectiles[idx].timer = 4.0f + 0.5f * (lvl - 1); // Tồn tại lâu hơn
                gs->projectilePool.projectiles[idx].damageTimer = 0.0f;
            }
            
            gs->witch.skillCooldown[SKILL_POISON_CLOUD] = fmaxf(1.5f, 3.5f - 0.25f * (lvl - 1));
            gs->witch.animState = WITCH_STATE_ATTACK_HAND;
            gs->witch.stateTimer = 0.3f;
            if (IsSoundReady(collectStarSound)) PlaySound(collectStarSound);
            break;
        }
        
        case SKILL_ICE_BLAST: {
            // Hệ THỦY: Chưởng lực băng đóng băng kẻ địch
            int lvl = gs->witch.skillLevels[SKILL_ICE_BLAST];
            Vector2 vel = { 650.0f, 0.0f };
            float radius = (18.0f + 2.0f * gs->witch.effectiveThuyStars) * (1.0f + 0.08f * (lvl - 1));
            int dmg = (25 + 10 * gs->witch.effectiveThuyStars) * (1.0f + 0.20f * (lvl - 1));
            
            SpawnProjectile(&gs->projectilePool, gs->witch.position, vel, radius, dmg, false, SKYBLUE, PROJ_ICE);
            
            gs->witch.skillCooldown[SKILL_ICE_BLAST] = fmaxf(2.0f, 4.5f - 0.3f * (lvl - 1));
            gs->witch.animState = WITCH_STATE_ATTACK_WEAPON;
            gs->witch.stateTimer = 0.35f;
            if (IsSoundReady(shootSound)) PlaySound(shootSound);
            
            // Hạt sương giá
            for (int i = 0; i < 12; i++) {
                float vx = (float)GetRandomValue(100, 250);
                float vy = (float)GetRandomValue(-50, 50);
                SpawnParticle(&gs->particlePool, gs->witch.position, (Vector2){ vx, vy }, 0.5f, 3.0f, CYAN);
            }
            break;
        }
        
        case SKILL_FIREBALL: {
            // Hệ HỎA: Cầu lửa nổ diện rộng (AOE)
            int lvl = gs->witch.skillLevels[SKILL_FIREBALL];
            int dmg = (20 + 10 * gs->witch.effectiveHoaStars) * (1.0f + 0.20f * (lvl - 1));
            float radius = (10.0f + 1.5f * gs->witch.effectiveHoaStars) * (1.0f + 0.1f * (lvl - 1));
            
            int targetIdx = -1;
            float minDist = 999999.0f;
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (gs->enemyPool.enemies[e].active) {
                    if (gs->enemyPool.enemies[e].type == ENEMY_GHOST && !gs->enemyPool.enemies[e].isVisible) continue;
                    float dist = Vector2Distance(gs->witch.position, gs->enemyPool.enemies[e].position);
                    if (dist < minDist) {
                        minDist = dist;
                        targetIdx = e;
                    }
                }
            }
            
            Vector2 projVel;
            if (targetIdx != -1) {
                Vector2 targetPos = gs->enemyPool.enemies[targetIdx].position;
                Vector2 dir = Vector2Normalize(Vector2Subtract(targetPos, gs->witch.position));
                projVel = Vector2Scale(dir, 440.0f);
            } else {
                projVel = (Vector2){ 440.0f, 0.0f };
            }
            
            Color ballColor = ORANGE;
            if (gs->witch.effectiveHoaStars >= 3 || lvl >= 3) {
                ballColor = RED;
            }
            
            SpawnProjectile(&gs->projectilePool, gs->witch.position, projVel, radius, dmg, false, ballColor, PROJ_FIREBALL);
            gs->witch.skillCooldown[SKILL_FIREBALL] = fmaxf(0.5f, 1.0f - 0.07f * (lvl - 1));
            gs->witch.animState = WITCH_STATE_ATTACK_WEAPON;
            gs->witch.stateTimer = 0.4f;
            if (IsSoundReady(shootSound)) PlaySound(shootSound);
            break;
        }
        
        case SKILL_LIGHTNING: {
            // Hệ THỔ: Sấm sét & Lốc xoáy cuồng phong
            int lvl = gs->witch.skillLevels[SKILL_LIGHTNING];
            gs->lightningPointCount = 1;
            gs->lightningPoints[0] = gs->witch.position;
            
            gs->lightningTargets[0] = -1;
            gs->lightningTargets[1] = -1;
            gs->lightningTargets[2] = -1;
            
            gs->lightningDamageApplied[0] = false;
            gs->lightningDamageApplied[1] = false;
            gs->lightningDamageApplied[2] = false;
            
            Vector2 currentSource = gs->witch.position;
            
            int e1 = -1;
            float minDist = 999999.0f;
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (!gs->enemyPool.enemies[e].active) continue;
                if (gs->enemyPool.enemies[e].type == ENEMY_GHOST && !gs->enemyPool.enemies[e].isVisible) continue;
                float dist = Vector2Distance(currentSource, gs->enemyPool.enemies[e].position);
                if (dist < minDist) {
                    minDist = dist;
                    e1 = e;
                }
            }
            
            if (e1 != -1) {
                gs->lightningTargets[0] = e1;
                gs->lightningTargetLastPos[0] = gs->enemyPool.enemies[e1].position;
                gs->lightningPoints[1] = gs->enemyPool.enemies[e1].position;
                gs->lightningPointCount = 2;
                currentSource = gs->enemyPool.enemies[e1].position;
                
                int e2 = -1;
                minDist = 250.0f + 50.0f * (lvl - 1);
                for (int e = 0; e < MAX_ENEMIES; e++) {
                    if (e == e1) continue;
                    if (!gs->enemyPool.enemies[e].active) continue;
                    if (gs->enemyPool.enemies[e].type == ENEMY_GHOST && !gs->enemyPool.enemies[e].isVisible) continue;
                    float dist = Vector2Distance(currentSource, gs->enemyPool.enemies[e].position);
                    if (dist < minDist) {
                        minDist = dist;
                        e2 = e;
                    }
                }
                
                if (e2 != -1) {
                    gs->lightningTargets[1] = e2;
                    gs->lightningTargetLastPos[1] = gs->enemyPool.enemies[e2].position;
                    gs->lightningPoints[2] = gs->enemyPool.enemies[e2].position;
                    gs->lightningPointCount = 3;
                    currentSource = gs->enemyPool.enemies[e2].position;
                    
                    int e3 = -1;
                    minDist = 250.0f + 50.0f * (lvl - 1);
                    for (int e = 0; e < MAX_ENEMIES; e++) {
                        if (e == e1 || e == e2) continue;
                        if (!gs->enemyPool.enemies[e].active) continue;
                        if (gs->enemyPool.enemies[e].type == ENEMY_GHOST && !gs->enemyPool.enemies[e].isVisible) continue;
                        float dist = Vector2Distance(currentSource, gs->enemyPool.enemies[e].position);
                        if (dist < minDist) {
                            minDist = dist;
                            e3 = e;
                        }
                    }
                    
                    if (e3 != -1) {
                        gs->lightningTargets[2] = e3;
                        gs->lightningTargetLastPos[2] = gs->enemyPool.enemies[e3].position;
                        gs->lightningPoints[3] = gs->enemyPool.enemies[e3].position;
                        gs->lightningPointCount = 4;
                    }
                }
            }
            
            gs->witch.skillActiveTimer[SKILL_LIGHTNING] = 0.5f;
            gs->witch.skillCooldown[SKILL_LIGHTNING] = fmaxf(3.0f, 6.0f - 0.4f * (lvl - 1));
            gs->witch.animState = WITCH_STATE_ATTACK_HAND;
            gs->witch.stateTimer = 0.3f;
            if (IsSoundReady(shootSound)) PlaySound(shootSound);
            
            // Sinh thêm Lốc xoáy điện hệ Thổ
            float tornRadius = (50.0f + 6.0f * gs->witch.effectiveThoStars) * (1.0f + 0.12f * (lvl - 1));
            int tornDmg = (8 + 3 * gs->witch.effectiveThoStars) * (1.0f + 0.20f * (lvl - 1));
            int idx = SpawnProjectile(&gs->projectilePool, gs->witch.position, (Vector2){ 150.0f, 0.0f }, tornRadius, tornDmg, false, (Color){ 240, 180, 50, 255 }, PROJ_TORNADO);
            if (idx != -1) {
                gs->projectilePool.projectiles[idx].timer = 5.0f + 0.5f * (lvl - 1); // Tồn tại lâu hơn
                gs->projectilePool.projectiles[idx].damageTimer = 0.0f;
            }
            
            gs->shakeIntensity = 6.0f;
            gs->shakeDuration = 0.25f;
            break;
        }
        
        case SKILL_MAGNET: {
            int lvl = gs->witch.skillLevels[SKILL_MAGNET];
            // Buff Nam châm: kích hoạt lâu hơn theo level, giảm hồi chiêu
            gs->witch.skillActiveTimer[SKILL_MAGNET] = 6.0f + 1.5f * (lvl - 1);
            gs->witch.skillCooldown[SKILL_MAGNET] = fmaxf(6.0f, 12.0f - 0.8f * (lvl - 1));
            if (IsSoundReady(collectStarSound)) PlaySound(collectStarSound);
            
            // Hiệu ứng hạt hội tụ vào phù thủy
            for (int i = 0; i < 20; i++) {
                float angle = ((float)i / 20.0f) * 2.0f * PI;
                float dist = 120.0f;
                Vector2 startPos = { gs->witch.position.x + cosf(angle) * dist, gs->witch.position.y + sinf(angle) * dist };
                Vector2 vel = { -cosf(angle) * 250.0f, -sinf(angle) * 250.0f };
                SpawnParticle(&gs->particlePool, startPos, vel, 0.45f, 2.0f, GOLD);
            }
            break;
        }
        
        case SKILL_SHIELD: {
            int lvl = gs->witch.skillLevels[SKILL_SHIELD];
            // Buff Khiên ma pháp chủ động: kích hoạt khiên ma lực, thời gian tăng theo level, giảm hồi chiêu
            gs->witch.manaShieldHealth = 1.67f + 0.33f * (lvl - 1); // Tăng thời gian (0.33f = 1 giây khi chia 3.0f ở witch.c)
            gs->witch.skillCooldown[SKILL_SHIELD] = fmaxf(8.0f, 15.0f - 1.0f * (lvl - 1));
            
            // Chuyển tư thế DEFENSE
            gs->witch.animState = WITCH_STATE_DEFENSE;
            gs->witch.stateTimer = 0.5f;
            if (IsSoundReady(shootSound)) PlaySound(shootSound);
            
            // Hiệu ứng hạt tỏa tròn từ phù thủy
            for (int i = 0; i < 15; i++) {
                float angle = ((float)i / 15.0f) * 2.0f * PI;
                float speed = 160.0f;
                Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
                SpawnParticle(&gs->particlePool, gs->witch.position, vel, 0.4f, 2.5f, SKYBLUE);
            }
            break;
        }
        
        default:
            break;
    }
}

// Cập nhật lan truyền sát thương sét chuỗi Chain Lightning thời gian thực
void UpdateSkills(struct GameState *gs, float deltaTime) {
    if (gs == NULL) return;
    
    // Cập nhật vị trí xích sét (Chain Lightning) thời gian thực theo quái/người chơi
    if (gs->witch.skillActiveTimer[SKILL_LIGHTNING] > 0.0f) {
        gs->lightningPoints[0] = gs->witch.position;
        
        float elapsed = 0.5f - gs->witch.skillActiveTimer[SKILL_LIGHTNING];
        
        // Cập nhật vị trí của các quái làm mục tiêu nếu chúng còn sống, nếu chết giữ nguyên vị trí cũ
        for (int i = 0; i < 3; i++) {
            int targetIdx = gs->lightningTargets[i];
            if (targetIdx != -1) {
                if (gs->enemyPool.enemies[targetIdx].active) {
                    gs->lightningTargetLastPos[i] = gs->enemyPool.enemies[targetIdx].position;
                }
                gs->lightningPoints[i + 1] = gs->lightningTargetLastPos[i];
                
                // Gây sát thương và nổ hạt theo thứ tự lan truyền (0.1s mỗi bước nhảy)
                float hitTime = 0.1f * (i + 1);
                if (elapsed >= hitTime && !gs->lightningDamageApplied[i]) {
                    gs->lightningDamageApplied[i] = true;
                    
                    int lvl = gs->witch.skillLevels[SKILL_LIGHTNING];
                    int damage = (i == 0) ? 35 : ((i == 1) ? 25 : 20);
                    damage = (int)(damage * (1.0f + 0.20f * (lvl - 1)));
                    damage += 12 * gs->witch.effectiveThoStars; // Tăng sát thương sét theo sao Thổ
                    bool killed = DamageEnemy(&gs->enemyPool, targetIdx, damage);
                    
                    // Sinh hiệu ứng nổ tại mục tiêu
                    SpawnExplosion(&gs->particlePool, gs->lightningTargetLastPos[i], 8 - i, SKYBLUE);
                    
                    // Nếu quái bị hạ gục thì cộng điểm/vàng cho phù thủy
                    if (killed) {
                        gs->witch.score += (gs->enemyPool.enemies[targetIdx].type >= ENEMY_BOSS_FOREST) ? 200 : 25;
                        gs->witch.gold += GetEnemyGoldDrop(gs->enemyPool.enemies[targetIdx].type);
                        SpawnExplosion(&gs->particlePool, gs->lightningTargetLastPos[i], 14, RED);
                        
                        // Nếu là Boss bị hạ gục
                        if (gs->enemyPool.enemies[targetIdx].type >= ENEMY_BOSS_FOREST) {
                            // Chuyển biome (gọi hàm được khai báo ở main.c thông qua cơ chế quản lý màn chơi)
                            // Lưu ý: Logic chuyển biome sẽ được xử lý ở main.c sau checkCollisions,
                            // tuy nhiên ta vẫn gọi DamageEnemy và đánh dấu chết ở đây, main.c check trong vòng lặp chính.
                        }
                    }
                }
            }
        }
    }
}

// Vẽ tia sét xích (Chain Lightning)
void DrawSkills(struct GameState *gs) {
    if (gs == NULL) return;
    
    if (gs->witch.skillActiveTimer[SKILL_LIGHTNING] > 0.0f && gs->lightningPointCount >= 2) {
        float elapsed = 0.5f - gs->witch.skillActiveTimer[SKILL_LIGHTNING];
        
        if (gs->lightningShaderLoaded) {
            // VẼ TIA SÉT BẰNG SHADER CHUYÊN BIỆT (LIGHTNING SHADER)
            BeginBlendMode(BLEND_ADDITIVE);
            BeginShaderMode(gs->lightningShader);
            
            int startPosLoc = GetShaderLocation(gs->lightningShader, "startPos");
            int endPosLoc = GetShaderLocation(gs->lightningShader, "endPos");
            int timeLoc = GetShaderLocation(gs->lightningShader, "time");
            int glowColorLoc = GetShaderLocation(gs->lightningShader, "glowColor");
            
            float timeVal = GetTime();
            Vector4 glowCol = { 0.45f, 0.35f, 1.0f, 1.0f }; // Màu tím điện neon
            
            SetShaderValue(gs->lightningShader, timeLoc, &timeVal, SHADER_UNIFORM_FLOAT);
            SetShaderValue(gs->lightningShader, glowColorLoc, &glowCol, SHADER_UNIFORM_VEC4);
            
            for (int i = 0; i < gs->lightningPointCount - 1; i++) {
                float startT = 0.1f * i;
                float endT = 0.1f * (i + 1);
                
                if (elapsed < startT) continue;
                
                Vector2 segmentStart = gs->lightningPoints[i];
                Vector2 segmentEnd;
                
                if (elapsed < endT) {
                    float p = (elapsed - startT) / 0.1f;
                    segmentEnd = Vector2Lerp(segmentStart, gs->lightningPoints[i+1], p);
                } else {
                    segmentEnd = gs->lightningPoints[i+1];
                }
                
                SetShaderValue(gs->lightningShader, startPosLoc, &segmentStart, SHADER_UNIFORM_VEC2);
                SetShaderValue(gs->lightningShader, endPosLoc, &segmentEnd, SHADER_UNIFORM_VEC2);
                
                // Vẽ full-screen quad để shader render tia sét
                DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, WHITE);
            }
            
            EndShaderMode();
            EndBlendMode();
        } else {
            // VẼ VECTOR THAY THẾ (FALLBACK) KHI KHÔNG CÓ SHADER
            BeginBlendMode(BLEND_ADDITIVE);
            for (int i = 0; i < gs->lightningPointCount - 1; i++) {
                float startT = 0.1f * i;
                float endT = 0.1f * (i + 1);
                
                if (elapsed < startT) continue;
                
                Vector2 segmentStart = gs->lightningPoints[i];
                Vector2 segmentEnd;
                
                if (elapsed < endT) {
                    float p = (elapsed - startT) / 0.1f;
                    segmentEnd = Vector2Lerp(segmentStart, gs->lightningPoints[i+1], p);
                } else {
                    segmentEnd = gs->lightningPoints[i+1];
                }
                
                // Vẽ tia sét nhánh ziczac phụ họa đơn giản bằng cách tạo điểm ngẫu nhiên ở giữa segment
                Vector2 mid = Vector2Scale(Vector2Add(segmentStart, segmentEnd), 0.5f);
                Vector2 perp = { -(segmentEnd.y - segmentStart.y), segmentEnd.x - segmentStart.x };
                perp = Vector2Normalize(perp);
                float offsetVal = sinf(GetTime() * 45.0f + i) * 15.0f;
                mid = Vector2Add(mid, Vector2Scale(perp, offsetVal));
                
                // Nét vẽ điện neon dày tím ngoài, lõi trắng mảnh ở giữa
                DrawLineEx(segmentStart, mid, 6.0f, (Color){ 120, 90, 255, 255 });
                DrawLineEx(mid, segmentEnd, 6.0f, (Color){ 120, 90, 255, 255 });
                
                DrawLineEx(segmentStart, mid, 2.0f, WHITE);
                DrawLineEx(mid, segmentEnd, 2.0f, WHITE);
            }
            EndBlendMode();
        }
    }
}

// Vẽ quầng sáng Chain Lightning lên Lightmap để tạo hiệu ứng ánh sáng Ori
void DrawSkillsLightmap(struct GameState *gs) {
    if (gs == NULL) return;
    
    if (gs->witch.skillActiveTimer[SKILL_LIGHTNING] > 0.0f && gs->lightningPointCount >= 2) {
        float elapsed = 0.5f - gs->witch.skillActiveTimer[SKILL_LIGHTNING];
        
        // Điểm nguồn phát sáng tại vị trí phù thủy
        DrawCircleGradient(gs->lightningPoints[0].x, gs->lightningPoints[0].y, 130.0f, ColorAlpha(VIOLET, 0.35f), ColorAlpha(VIOLET, 0.0f));
        DrawCircleGradient(gs->lightningPoints[0].x, gs->lightningPoints[0].y, 45.0f, ColorAlpha(WHITE, 0.55f), ColorAlpha(WHITE, 0.0f));
        
        for (int i = 0; i < gs->lightningPointCount - 1; i++) {
            float startT = 0.1f * i;
            float endT = 0.1f * (i + 1);
            
            if (elapsed < startT) continue;
            
            Vector2 segmentStart = gs->lightningPoints[i];
            Vector2 segmentEnd;
            
            if (elapsed < endT) {
                float p = (elapsed - startT) / 0.1f;
                segmentEnd = Vector2Lerp(segmentStart, gs->lightningPoints[i+1], p);
            } else {
                segmentEnd = gs->lightningPoints[i+1];
                
                // Điểm đích đã đạt tới hoàn toàn tỏa sáng rực rỡ
                DrawCircleGradient(segmentEnd.x, segmentEnd.y, 130.0f, ColorAlpha(VIOLET, 0.35f), ColorAlpha(VIOLET, 0.0f));
                DrawCircleGradient(segmentEnd.x, segmentEnd.y, 45.0f, ColorAlpha(WHITE, 0.55f), ColorAlpha(WHITE, 0.0f));
            }
            
            // Vẽ quầng sáng mờ dọc theo thân tia sét (trung điểm)
            Vector2 mid = Vector2Scale(Vector2Add(segmentStart, segmentEnd), 0.5f);
            DrawCircleGradient(mid.x, mid.y, 110.0f, ColorAlpha(SKYBLUE, 0.25f), ColorAlpha(SKYBLUE, 0.0f));
        }
    }
}

// Kích hoạt chiêu cuối Ultimate: Elemental Burst
void TriggerUltimate(GameState *gs) {
    if (gs == NULL) return;
    
    // Đếm số sao đang xoay quanh
    int count = 0;
    for (int i = 0; i < MAX_STARS; i++) {
        if (gs->starPool.stars[i].active && gs->starPool.stars[i].isOrbiting) {
            count++;
        }
    }
    
    // Không có sao thì bỏ qua
    if (count == 0) return;
    
    // 1. Kích hoạt hiệu ứng chớp flash và rung màn hình
    gs->ultimateFlashTimer = 0.20f;
    gs->shakeIntensity = 13.0f;
    gs->shakeDuration = 0.65f;
    
    // 2. Âm thanh hoành tráng
    if (IsSoundReady(shootSound)) {
        PlaySound(shootSound);
    }
    
    // 3. Hiệu ứng hạt tỏa 360 độ từ Phù thủy
    for (int p = 0; p < 40; p++) {
        float angle = ((float)p / 40.0f) * 2.0f * PI;
        float speed = 200.0f + ((float)rand() / RAND_MAX) * 260.0f;
        Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
        
        Color col = WHITE;
        int r = GetRandomValue(0, 5);
        if (r == 0) col = (Color){ 230, 230, 250, 255 }; // Kim
        else if (r == 1) col = (Color){ 0, 230, 100, 255 }; // Mộc
        else if (r == 2) col = (Color){ 0, 180, 255, 255 }; // Thủy
        else if (r == 3) col = (Color){ 255, 80, 0, 255 }; // Hỏa
        else if (r == 4) col = (Color){ 240, 180, 50, 255 }; // Thổ
        else col = PINK; // Cầu vồng
        
        SpawnParticle(&gs->particlePool, gs->witch.position, vel, 0.65f, 3.5f, col);
    }
    
    // 4. Phóng tất cả sao xoay quanh thành đạn Ultimate
    for (int i = 0; i < MAX_STARS; i++) {
        if (gs->starPool.stars[i].active && gs->starPool.stars[i].isOrbiting) {
            Star *s = &gs->starPool.stars[i];
            
            // Tìm quái gần nhất lúc xuất phát
            int targetIdx = -1;
            float minDist = 999999.0f;
            for (int e = 0; e < MAX_ENEMIES; e++) {
                if (gs->enemyPool.enemies[e].active) {
                    if (gs->enemyPool.enemies[e].type == ENEMY_GHOST && !gs->enemyPool.enemies[e].isVisible) continue;
                    float dist = Vector2Distance(s->position, gs->enemyPool.enemies[e].position);
                    if (dist < minDist) {
                        minDist = dist;
                        targetIdx = e;
                    }
                }
            }
            
            Vector2 projVel;
            if (targetIdx != -1) {
                Vector2 targetPos = gs->enemyPool.enemies[targetIdx].position;
                Vector2 dir = Vector2Normalize(Vector2Subtract(targetPos, s->position));
                projVel = Vector2Scale(dir, 800.0f);
            } else {
                projVel = (Vector2){ 800.0f, 0.0f };
            }
            
            // Sinh đạn Ultimate homing
            int idx = SpawnProjectile(&gs->projectilePool, s->position, projVel, 22.0f, 100, false, s->tintColor, PROJ_FIREBALL);
            if (idx != -1) {
                gs->projectilePool.projectiles[idx].isUltimate = true;
            }
            
            // Xóa sao khỏi quỹ đạo
            DeactivateStar(&gs->starPool, i);
        }
    }
}
