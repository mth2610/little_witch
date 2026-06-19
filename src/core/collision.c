// ============================================================
// collision.c — Hệ thống phát hiện va chạm
// ============================================================

#include "collision.h"
#include "config.h"
#include "enemy.h"
#include "game.h"
#include "particle.h"
#include "raylib.h"
#include "raymath.h"
#include "skill.h"
#include "star.h"
#include "witch.h"

#include "gameplay.h"

// Extern âm thanh
extern Sound gameOverSound;

// Cập nhật toàn bộ các va chạm trong game loop gameplay
void CheckCollisions(GameState *gs) {
  Vector2 witchPos = gs->witch.position;
  float witchRadius = gs->witch.collisionRadius;
  float deltaTime = GetFrameTime();

  // 0. VA CHẠM: KHIÊN MA PHÁP VỚI QUÁI VẬT VÀ ĐẠN (ĐẨY LÙI & PHẢN ĐẠN)
  if (gs->witch.manaShieldHealth > 0.0f) {
    float shieldRadius = 100.0f + 12.0f * gs->witch.effectiveThuyStars;

    // 0.1 Đẩy lùi quái vật
    for (int e = 0; e < MAX_ENEMIES; e++) {
      if (!gs->enemyPool.enemies[e].active)
        continue;
      Enemy *enemy = &gs->enemyPool.enemies[e];
      if (enemy->type == ENEMY_GHOST && !enemy->isVisible)
        continue;

      float dist = Vector2Distance(enemy->position, witchPos);
      float minDist = shieldRadius + enemy->collisionRadius;
      if (dist < minDist) {
        // Tính toán hướng đẩy lùi từ phù thủy sang quái vật
        Vector2 dir = {1.0f, 0.0f};
        if (dist > 0.1f) {
          dir = Vector2Normalize(Vector2Subtract(enemy->position, witchPos));
        } else {
          dir = (Vector2){(float)GetRandomValue(-10, 10),
                          (float)GetRandomValue(-10, 10)};
          if (dir.x == 0 && dir.y == 0)
            dir.x = 1.0f;
          dir = Vector2Normalize(dir);
        }

        // Cập nhật vị trí ra ngoài rìa khiên
        enemy->position =
            Vector2Add(witchPos, Vector2Scale(dir, minDist + 5.0f));

        // Đẩy lùi và choáng
        float force = 650.0f;
        if (enemy->type >= ENEMY_BOSS_FOREST) {
          force = 250.0f; // Boss đẩy lùi nhẹ hơn
        }
        enemy->knockbackVelocity = Vector2Scale(dir, force);
        enemy->knockbackTimer = 0.40f;

        // Hiệu ứng nổ hạt tại điểm chạm
        Vector2 contactPos =
            Vector2Add(witchPos, Vector2Scale(dir, shieldRadius));
        for (int i = 0; i < 5; i++) {
          float vx = dir.x * 120.0f + (float)GetRandomValue(-45, 45);
          float vy = dir.y * 120.0f + (float)GetRandomValue(-45, 45);
          SpawnParticle(&gs->particlePool, contactPos, (Vector2){vx, vy}, 0.35f,
                        2.0f, SKYBLUE);
        }

        // Gây sát thương nhỏ
        DamageEnemy(&gs->enemyPool, e, 2);
      }
    }

    // 0.2 Phản đạn của quái vật
    for (int p = 0; p < MAX_PROJECTILES; p++) {
      if (!gs->projectilePool.projectiles[p].active ||
          !gs->projectilePool.projectiles[p].isEnemy)
        continue;
      Projectile *proj = &gs->projectilePool.projectiles[p];

      float dist = Vector2Distance(proj->position, witchPos);
      float minDist = shieldRadius + proj->radius;
      if (dist < minDist) {
        // Hướng đẩy đạn ra ngoài
        Vector2 dir = {1.0f, 0.0f};
        if (dist > 0.1f) {
          dir = Vector2Normalize(Vector2Subtract(proj->position, witchPos));
        }

        // Định vị lại đạn
        proj->position =
            Vector2Add(witchPos, Vector2Scale(dir, minDist + 2.0f));

        // Phản hồi vận tốc ra ngoài và tăng tốc độ bay
        float currentSpeed = Vector2Length(proj->velocity);
        if (currentSpeed < 100.0f)
          currentSpeed = 300.0f;
        proj->velocity = Vector2Scale(dir, currentSpeed * 1.25f);

        // Đổi phe đạn tiêu diệt quái vật
        proj->isEnemy = false;
        proj->color = SKYBLUE;
        proj->damage = 15; // Sát thương phản hồi cố định

        // Tạo hạt lấp lánh khi phản đạn
        for (int i = 0; i < 4; i++) {
          float vx = dir.x * 80.0f + (float)GetRandomValue(-30, 30);
          float vy = dir.y * 80.0f + (float)GetRandomValue(-30, 30);
          SpawnParticle(&gs->particlePool, proj->position, (Vector2){vx, vy},
                        0.25f, 1.8f, CYAN);
        }
      }
    }
  }

  // 1. VA CHẠM: SAO ĐANG XOAY VỚI QUÁI VẬT (Sát thương tăng theo Mộc)
  for (int s = 0; s < MAX_STARS; s++) {
    if (!gs->starPool.stars[s].active || !gs->starPool.stars[s].isOrbiting)
      continue;

    Star *star = &gs->starPool.stars[s];

    for (int e = 0; e < MAX_ENEMIES; e++) {
      if (!gs->enemyPool.enemies[e].active)
        continue;

      Enemy *enemy = &gs->enemyPool.enemies[e];
      if (enemy->type == ENEMY_GHOST && !enemy->isVisible)
        continue;

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
    if (!gs->projectilePool.projectiles[p].active ||
        gs->projectilePool.projectiles[p].isEnemy)
      continue;

    Projectile *proj = &gs->projectilePool.projectiles[p];
    if (proj->type != PROJ_POISON && proj->type != PROJ_TORNADO)
      continue;
    if (proj->isWeak)
      continue; // Đạn yếu ớt bay thẳng, va chạm trực tiếp ở 2.2

    if (proj->damageTimer <= 0.0f) {
      bool dealtDamage = false;
      for (int e = 0; e < MAX_ENEMIES; e++) {
        if (!gs->enemyPool.enemies[e].active)
          continue;

        Enemy *enemy = &gs->enemyPool.enemies[e];
        if (enemy->type == ENEMY_GHOST && !enemy->isVisible)
          continue;

        float dist = Vector2Distance(proj->position, enemy->position);
        if (dist < proj->radius + enemy->collisionRadius) {
          bool killed = DamageEnemy(&gs->enemyPool, e, proj->damage);
          dealtDamage = true;

          if (proj->type == PROJ_TORNADO) {
            // Lực hút lốc xoáy điện hệ Thổ
            Vector2 pullDir = Vector2Normalize(
                Vector2Subtract(proj->position, enemy->position));
            enemy->position = Vector2Add(
                enemy->position, Vector2Scale(pullDir, 90.0f * deltaTime));
            if (GetRandomValue(0, 100) < 15) {
              SpawnParticle(&gs->particlePool, enemy->position,
                            (Vector2){0, -20.0f}, 0.4f, 2.0f, YELLOW);
            }
          } else {
            // Mây độc hệ Mộc
            if (GetRandomValue(0, 100) < 15) {
              SpawnParticle(&gs->particlePool, enemy->position,
                            (Vector2){(float)GetRandomValue(-15, 15),
                                      (float)GetRandomValue(-15, 15)},
                            0.4f, 2.0f, GREEN);
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
    if (!gs->projectilePool.projectiles[p].active ||
        gs->projectilePool.projectiles[p].isEnemy)
      continue;

    Projectile *proj = &gs->projectilePool.projectiles[p];
    if (!proj->isWeak) {
      if (proj->type == PROJ_POISON || proj->type == PROJ_TORNADO)
        continue;
    }

    for (int e = 0; e < MAX_ENEMIES; e++) {
      if (!gs->enemyPool.enemies[e].active)
        continue;

      Enemy *enemy = &gs->enemyPool.enemies[e];
      if (enemy->type == ENEMY_GHOST && !enemy->isVisible)
        continue;

      float dist = Vector2Distance(proj->position, enemy->position);
      if (dist < proj->radius + enemy->collisionRadius) {
        if (proj->isWeak) {
          // Đạn yếu: va chạm trực tiếp, gây sát thương thường, nổ nhỏ và biến
          // mất
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
        } else if (proj->type == PROJ_SHURIKEN) {
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
        } else if (proj->type == PROJ_ICE) {
          // Đạn băng đông cứng hệ Thủy
          bool killed = DamageEnemy(&gs->enemyPool, e, proj->damage);
          int lvl = gs->witch.skillLevels[SKILL_ICE_BLAST];
          enemy->freezeTimer =
              1.5f + 0.3f * gs->witch.effectiveThuyStars + 0.5f * (lvl - 1);
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
        } else if (proj->isUltimate) {
          // Nổ lan chiêu cuối Ultimate
          SpawnExplosion(&gs->particlePool, proj->position, 22, proj->color);
          float aoeRadius = 100.0f;
          for (int other = 0; other < MAX_ENEMIES; other++) {
            if (gs->enemyPool.enemies[other].active) {
              Enemy *oEnemy = &gs->enemyPool.enemies[other];
              if (oEnemy->type == ENEMY_GHOST && !oEnemy->isVisible)
                continue;

              float aoeDist = Vector2Distance(proj->position, oEnemy->position);
              if (aoeDist < aoeRadius + oEnemy->collisionRadius) {
                bool killed = DamageEnemy(&gs->enemyPool, other, proj->damage);
                SpawnExplosion(&gs->particlePool, oEnemy->position, 6,
                               proj->color);
                if (killed) {
                  gs->witch.score +=
                      (oEnemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
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
        } else {
          // Cầu lửa nổ lan hệ HỎA
          proj->active = false;
          int lvl = gs->witch.skillLevels[SKILL_FIREBALL];
          float aoeRadius = (60.0f + 10.0f * gs->witch.effectiveHoaStars) *
                            (1.0f + 0.10f * (lvl - 1));
          int aoeDmg = (int)((12 + 6 * gs->witch.effectiveHoaStars) *
                             (1.0f + 0.20f * (lvl - 1)));
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
            if (other == e)
              continue;
            if (!gs->enemyPool.enemies[other].active)
              continue;

            Enemy *oEnemy = &gs->enemyPool.enemies[other];
            if (oEnemy->type == ENEMY_GHOST && !oEnemy->isVisible)
              continue;

            float aoeDist = Vector2Distance(proj->position, oEnemy->position);
            if (aoeDist < aoeRadius + oEnemy->collisionRadius) {
              bool killed = DamageEnemy(&gs->enemyPool, other, aoeDmg);
              SpawnExplosion(&gs->particlePool, oEnemy->position, 6, ORANGE);
              if (killed) {
                gs->witch.score +=
                    (oEnemy->type >= ENEMY_BOSS_FOREST) ? 200 : 25;
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
    if (!gs->projectilePool.projectiles[p].active ||
        !gs->projectilePool.projectiles[p].isEnemy)
      continue;

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
        if (IsSoundReady(gameOverSound))
          PlaySound(gameOverSound);
      }
    }
  }

  // 4. VA CHẠM: QUÁI VẬT CHẠM TRỰC TIẾP VỚI PHÙ THỦY
  for (int e = 0; e < MAX_ENEMIES; e++) {
    if (!gs->enemyPool.enemies[e].active)
      continue;

    Enemy *enemy = &gs->enemyPool.enemies[e];
    if (enemy->type == ENEMY_GHOST && !enemy->isVisible)
      continue;

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
        if (IsSoundReady(gameOverSound))
          PlaySound(gameOverSound);
      }
    }
  }
}
