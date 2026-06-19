// ============================================================
// skill.c — Logic các kỹ năng (RPG Active Skills) và Đạn (Projectiles)
// ============================================================

#include "skill.h"
#include "game.h"
#include "particle.h"
#include "shuriken_skill.h"
#include "poison_cloud_skill.h"
#include "fluid_skill.h"
#include "fireball_skill.h"
#include "lightning_skill.h"
#include "magnet_skill.h"
#include "shield_skill.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

// Âm thanh được load ở main.c
extern Sound collectStarSound;
extern Sound shootSound;
extern Texture2D shieldBackgroundTexture;

// ----------------------------------------------------------------
// QUẢN LÝ POOL ĐẠN (PROJECTILES)
// ----------------------------------------------------------------

// Khởi tạo pool quản lý đạn
void InitProjectilePool(ProjectilePool *pool) {
  if (pool == NULL)
    return;
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    pool->projectiles[i].active = false;
  }
}

// Sinh đạn mới vào pool
int SpawnProjectile(ProjectilePool *pool, Vector2 position, Vector2 velocity,
                    float radius, int damage, bool isEnemy, Color color,
                    ProjectileType type) {
  if (pool == NULL)
    return -1;

  // Tìm slot trống
  int freeIdx = -1;
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    if (!pool->projectiles[i].active) {
      freeIdx = i;
      break;
    }
  }

  // Nếu đầy pool thì bỏ qua
  if (freeIdx == -1)
    return -1;

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
  p->isWeak = false;

  return freeIdx;
}

// Cập nhật tọa độ đạn, bẻ lái homing của Ultimate và hủy nếu ra ngoài màn hình
// ảo
void UpdateProjectiles(ProjectilePool *pool, float deltaTime,
                       ParticlePool *partPool, struct EnemyPool *enemyPool) {
  if (pool == NULL)
    return;

  for (int i = 0; i < MAX_PROJECTILES; i++) {
    if (!pool->projectiles[i].active)
      continue;

    Projectile *p = &pool->projectiles[i];

    // 1. Xử lý bẻ lái homing của đạn Ultimate
    if (p->isUltimate && enemyPool != NULL) {
      int targetIdx = -1;
      float minDist = 999999.0f;
      for (int e = 0; e < MAX_ENEMIES; e++) {
        if (enemyPool->enemies[e].active) {
          if (enemyPool->enemies[e].type == ENEMY_GHOST &&
              !enemyPool->enemies[e].isVisible)
            continue;

          float dist =
              Vector2Distance(p->position, enemyPool->enemies[e].position);
          if (dist < minDist) {
            minDist = dist;
            targetIdx = e;
          }
        }
      }

      if (targetIdx != -1) {
        Vector2 targetPos = enemyPool->enemies[targetIdx].position;
        Vector2 targetDir =
            Vector2Normalize(Vector2Subtract(targetPos, p->position));
        Vector2 currentDir = Vector2Normalize(p->velocity);
        Vector2 lerpDir = Vector2Normalize(
            Vector2Lerp(currentDir, targetDir, 8.0f * deltaTime));
        p->velocity = Vector2Scale(lerpDir, 920.0f);
      }
    }

    // 2. Cập nhật góc xoay và logic loại đạn
    if (p->type == PROJ_SHURIKEN) {
      p->rotation += 850.0f * deltaTime;
    } else if (p->type == PROJ_TORNADO) {
      p->rotation += 400.0f * deltaTime;
      if (!p->isWeak) {
        p->timer -= deltaTime;
        if (p->timer <= 0.0f) {
          p->active = false;
          continue;
        }
        p->position.y += sinf(GetTime() * 12.0f + i) * 60.0f * deltaTime;
      }
    } else if (p->type == PROJ_POISON) {
      if (!p->isWeak) {
        p->timer -= deltaTime;
        if (p->timer <= 0.0f) {
          p->active = false;
          continue;
        }
        if (p->damageTimer > 0.0f) {
          p->damageTimer -= deltaTime;
        }
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
      if (p->type == PROJ_POISON)
        chance = 0.65f;
      else if (p->type == PROJ_TORNADO)
        chance = 0.70f;
      else if (p->type == PROJ_ICE)
        chance = 0.85f; // Tỉ lệ sinh hơi sương cao

      if (roll < chance) {
        float size = p->radius * 0.3f + ((float)rand() / RAND_MAX) * 2.0f;
        Vector2 trailVel = {-p->velocity.x * 0.25f, -p->velocity.y * 0.25f};

        if (p->type == PROJ_POISON) {
          trailVel = (Vector2){(float)GetRandomValue(-20, 20),
                               (float)GetRandomValue(-40, -10)};
          size = 2.0f + ((float)rand() / RAND_MAX) * 3.5f;
        } else if (p->type == PROJ_TORNADO) {
          float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
          Vector2 offset = {cosf(angle) * p->radius * 0.8f,
                            sinf(angle) * p->radius * 0.4f};
          Vector2 partPos = Vector2Add(p->position, offset);
          SpawnParticle(partPool, partPos, (Vector2){0, -50.0f}, 0.5f, 2.0f,
                        p->color);
          continue;
        } else if (p->type == PROJ_ICE) {
          // Hơi sương bay lùi nhẹ và tán rộng ra 2 bên
          float randAng = ((float)rand() / RAND_MAX) * 2.0f * PI;
          float spread = 15.0f + ((float)rand() / RAND_MAX) * 30.0f;
          trailVel.x = -p->velocity.x * 0.12f + cosf(randAng) * spread;
          trailVel.y = -p->velocity.y * 0.12f + sinf(randAng) * spread;

          size = p->radius * 0.35f + ((float)rand() / RAND_MAX) * 2.5f;
          float lifetime = 0.4f + ((float)rand() / RAND_MAX) * 0.4f;

          SpawnParticleEx(partPool, p->position, trailVel, lifetime, size,
                          p->color, PARTICLE_FROST_VAPOR);
          continue;
        }

        float lifetime = 0.3f + ((float)rand() / RAND_MAX) * 0.3f;
        if (p->type == PROJ_POISON)
          lifetime = 0.5f + ((float)rand() / RAND_MAX) * 0.6f;

        SpawnParticle(partPool, p->position, trailVel, lifetime, size,
                      p->color);
      }
    }

    // Hủy đạn nếu bay quá xa ngoài vùng hiển thị
    if (p->type != PROJ_POISON || p->isWeak) {
      if (p->position.x < -100 || p->position.x > VIRTUAL_WIDTH + 100 ||
          p->position.y < -100 || p->position.y > VIRTUAL_HEIGHT + 100) {
        p->active = false;
      }
    }
  }
}

// Vẽ toàn bộ các viên đạn đang hoạt động
void DrawProjectiles(const ProjectilePool *pool, const struct GameState *gs) {
  if (pool == NULL)
    return;

  BeginBlendMode(BLEND_ADDITIVE);
  for (int i = 0; i < MAX_PROJECTILES; i++) {
    if (!pool->projectiles[i].active)
      continue;

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
      DrawShurikenProjectile(p, gs);
    } else if (p->type == PROJ_POISON) {
      DrawPoisonCloudProjectile(p, gs, i);
    } else if (p->type == PROJ_TORNADO) {
      DrawTornadoProjectile(p, gs);
    } else if (p->type == PROJ_FIREBALL) {
      DrawFireballProjectile(p, gs);
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

// Thực hiện bắn đòn đánh thường (Spacebar hoặc Touch)
// Tách biệt logic đòn đánh thường cơ bản tùy theo số lượng sao người chơi sở
// hữu
void CastNormalAttack(struct GameState *gs) {
  if (gs == NULL)
    return;

  // Tính tổng số sao ngũ hành mà Phù Thủy đang sở hữu (effective stars)
  int totalStars = gs->witch.effectiveKimStars + gs->witch.effectiveMocStars +
                   gs->witch.effectiveThuyStars + gs->witch.effectiveHoaStars +
                   gs->witch.effectiveThoStars;

  if (totalStars == 0) {
    // Bắn đòn đánh thường yếu (weak normal attack) khi không có bất kỳ sao nào
    // Lý do: Hạn chế sức mạnh tấn công khi chưa có sao để tạo tính cân bằng và
    // khuyến khích nhặt sao
    Vector2 projVel = {500.0f, 0.0f}; // Tốc độ bay chậm hơn đòn thường một chút
    SpawnProjectile(&gs->projectilePool, gs->witch.position, projVel, 4.0f, 4,
                    false, LIGHTGRAY, PROJ_FIREBALL);

    // Tạo một vài hạt nhỏ màu xám nhạt tượng trưng cho năng lượng yếu
    for (int i = 0; i < 3; i++) {
      float vx = (float)GetRandomValue(-30, 30);
      float vy = (float)GetRandomValue(-30, 30);
      SpawnParticle(&gs->particlePool, gs->witch.position, (Vector2){vx, vy},
                    0.25f, 1.2f, LIGHTGRAY);
    }
  } else {
    // Bắn đòn đánh thường bình thường khi có ít nhất 1 sao
    // Lý do: Duy trì cơ chế đánh thường tiêu chuẩn của game khi người chơi đã
    // bắt đầu thu thập sao
    Vector2 projVel = {550.0f, 0.0f};
    SpawnProjectile(&gs->projectilePool, gs->witch.position, projVel, 6.0f, 8,
                    false, ORANGE, PROJ_FIREBALL);

    // Hạt lửa màu cam bình thường
    for (int i = 0; i < 5; i++) {
      float vx = (float)GetRandomValue(-50, 50);
      float vy = (float)GetRandomValue(-50, 50);
      SpawnParticle(&gs->particlePool, gs->witch.position, (Vector2){vx, vy},
                    0.35f, 1.8f, ORANGE);
    }
  }

  // Kích hoạt hoạt ảnh tấn công sử dụng sáo/vũ khí
  gs->witch.animState = WITCH_STATE_ATTACK_WEAPON;
  gs->witch.stateTimer = 0.25f;

  if (IsSoundReady(shootSound))
    PlaySound(shootSound);
}

static SkillDefinition registeredSkills[SKILL_COUNT];

void InitSkills(void) {
  // Reset registry
  for (int i = 0; i < SKILL_COUNT; i++) {
    registeredSkills[i].type = SKILL_NONE;
    registeredSkills[i].name = NULL;
    registeredSkills[i].Init = NULL;
    registeredSkills[i].Cast = NULL;
    registeredSkills[i].Update = NULL;
    registeredSkills[i].Draw = NULL;
    registeredSkills[i].DrawLightmap = NULL;
    registeredSkills[i].Unload = NULL;
  }

  // 1. Hệ Kim: Golden Shuriken
  registeredSkills[SKILL_SHURIKEN] = (SkillDefinition){
    .type = SKILL_SHURIKEN,
    .name = "Golden Shuriken",
    .Init = InitShurikenSkill,
    .Cast = CastShurikenSkill,
    .Update = NULL,
    .Draw = NULL,
    .DrawLightmap = NULL,
    .Unload = UnloadShurikenSkill
  };

  // 2. Hệ Mộc: Poison Cloud
  registeredSkills[SKILL_POISON_CLOUD] = (SkillDefinition){
    .type = SKILL_POISON_CLOUD,
    .name = "Poison Cloud",
    .Init = InitPoisonCloudSkill,
    .Cast = CastPoisonCloudSkill,
    .Update = NULL,
    .Draw = NULL,
    .DrawLightmap = NULL,
    .Unload = UnloadPoisonCloudSkill
  };

  // 3. Hệ Thủy: Water Fluid (Ice Blast)
  registeredSkills[SKILL_ICE_BLAST] = (SkillDefinition){
    .type = SKILL_ICE_BLAST,
    .name = "Water Fluid",
    .Init = InitFluidSkill,
    .Cast = CastFluidSkill,
    .Update = UpdateFluidSkill,
    .Draw = DrawFluidSkill,
    .DrawLightmap = DrawFluidSkillLightmap,
    .Unload = UnloadFluidSkill
  };

  // 4. Hệ Hỏa: Fireball
  registeredSkills[SKILL_FIREBALL] = (SkillDefinition){
    .type = SKILL_FIREBALL,
    .name = "Fireball",
    .Init = InitFireballSkill,
    .Cast = CastFireballSkill,
    .Update = NULL,
    .Draw = NULL,
    .DrawLightmap = NULL,
    .Unload = UnloadFireballSkill
  };

  // 5. Hệ Thổ: Lightning & Tornado
  registeredSkills[SKILL_LIGHTNING] = (SkillDefinition){
    .type = SKILL_LIGHTNING,
    .name = "Lightning & Tornado",
    .Init = InitLightningSkill,
    .Cast = CastLightningSkill,
    .Update = UpdateLightningSkill,
    .Draw = DrawLightningSkill,
    .DrawLightmap = DrawLightningSkillLightmap,
    .Unload = UnloadLightningSkill
  };

  // 6. Nam châm thu hút (Buff)
  registeredSkills[SKILL_MAGNET] = (SkillDefinition){
    .type = SKILL_MAGNET,
    .name = "Magnet Buff",
    .Init = InitMagnetSkill,
    .Cast = CastMagnetSkill,
    .Update = NULL,
    .Draw = NULL,
    .DrawLightmap = NULL,
    .Unload = UnloadMagnetSkill
  };

  // 7. Khiên ma pháp bảo vệ (Buff)
  registeredSkills[SKILL_SHIELD] = (SkillDefinition){
    .type = SKILL_SHIELD,
    .name = "Mana Shield",
    .Init = InitShieldSkill,
    .Cast = CastShieldSkill,
    .Update = NULL,
    .Draw = NULL,
    .DrawLightmap = NULL,
    .Unload = UnloadShieldSkill
  };

  // Khởi tạo tất cả các kỹ năng đã đăng ký
  for (int i = 0; i < SKILL_COUNT; i++) {
    if (registeredSkills[i].type != SKILL_NONE && registeredSkills[i].Init != NULL) {
      registeredSkills[i].Init();
    }
  }
}

void UnloadSkills(void) {
  // Giải phóng tất cả các kỹ năng đã đăng ký
  for (int i = 0; i < SKILL_COUNT; i++) {
    if (registeredSkills[i].type != SKILL_NONE && registeredSkills[i].Unload != NULL) {
      registeredSkills[i].Unload();
    }
  }
}

// Kích hoạt kỹ năng chủ động khi nhấn phím hoặc click HUD
void CastActiveSkill(struct GameState *gs, SkillType skill) {
  if (gs == NULL)
    return;

  // Kiểm tra xem hệ kỹ năng chủ động tương ứng có sao nào không
  // Lý do: Theo Target_Architecture_2.0, khi cast kỹ năng của hệ có 0 sao, nhân
  // vật sẽ bắn đòn đánh thường yếu để tự vệ
  bool hasStars = false;
  if (skill == SKILL_SHURIKEN && gs->witch.effectiveKimStars > 0)
    hasStars = true;
  else if (skill == SKILL_POISON_CLOUD && gs->witch.effectiveMocStars > 0)
    hasStars = true;
  else if (skill == SKILL_ICE_BLAST && gs->witch.effectiveThuyStars > 0)
    hasStars = true;
  else if (skill == SKILL_FIREBALL && gs->witch.effectiveHoaStars > 0)
    hasStars = true;
  else if (skill == SKILL_LIGHTNING && gs->witch.effectiveThoStars > 0)
    hasStars = true;
  // MAGNET và SHIELD là kỹ năng buff bổ trợ, hoạt động độc lập không bắn đạn
  // trực tiếp nên luôn có thể cast
  else if (skill == SKILL_MAGNET || skill == SKILL_SHIELD)
    hasStars = true;

  if (!hasStars) {
    // Bắn đòn đánh thường yếu
    Vector2 projVel = {500.0f, 0.0f};
    SpawnProjectile(&gs->projectilePool, gs->witch.position, projVel, 4.0f, 4,
                    false, LIGHTGRAY, PROJ_FIREBALL);

    // Thiết lập hồi chiêu ngắn hơn cho đòn đánh thường yếu từ phím skill
    int lvl = gs->witch.skillLevels[skill];
    gs->witch.skillCooldown[skill] = fmaxf(0.5f, 1.0f - 0.1f * (lvl - 1));

    // Kích hoạt hoạt ảnh tấn công bằng tay
    gs->witch.animState = WITCH_STATE_ATTACK_HAND;
    gs->witch.stateTimer = 0.2f;

    if (IsSoundReady(shootSound))
      PlaySound(shootSound);

    // Hiệu ứng hạt nhỏ màu xám
    for (int i = 0; i < 4; i++) {
      float vx = (float)GetRandomValue(-40, 40);
      float vy = (float)GetRandomValue(-40, 40);
      SpawnParticle(&gs->particlePool, gs->witch.position, (Vector2){vx, vy},
                    0.3f, 1.5f, LIGHTGRAY);
    }
    return;
  }

  // Đa hình VTable: Gọi trực tiếp hàm Cast của kỹ năng tương ứng từ registry
  if (skill > SKILL_NONE && skill < SKILL_COUNT) {
    if (registeredSkills[skill].type != SKILL_NONE && registeredSkills[skill].Cast != NULL) {
      registeredSkills[skill].Cast(gs);
    }
  }
}

// Cập nhật trạng thái kỹ năng thời gian thực
void UpdateSkills(struct GameState *gs, float deltaTime) {
  for (int i = 0; i < SKILL_COUNT; i++) {
    if (registeredSkills[i].type != SKILL_NONE && registeredSkills[i].Update != NULL) {
      registeredSkills[i].Update(gs, deltaTime);
    }
  }
}

// Vẽ hiệu ứng kỹ năng
void DrawSkills(struct GameState *gs) {
  for (int i = 0; i < SKILL_COUNT; i++) {
    if (registeredSkills[i].type != SKILL_NONE && registeredSkills[i].Draw != NULL) {
      registeredSkills[i].Draw(gs);
    }
  }
}

// Vẽ quầng sáng kỹ năng lên Lightmap để tạo hiệu ứng ánh sáng Ori
void DrawSkillsLightmap(struct GameState *gs) {
  for (int i = 0; i < SKILL_COUNT; i++) {
    if (registeredSkills[i].type != SKILL_NONE && registeredSkills[i].DrawLightmap != NULL) {
      registeredSkills[i].DrawLightmap(gs);
    }
  }
}

// Kích hoạt chiêu cuối Ultimate: Elemental Burst
void TriggerUltimate(GameState *gs) {
  if (gs == NULL)
    return;

  // Đếm số sao đang xoay quanh
  int count = 0;
  for (int i = 0; i < MAX_STARS; i++) {
    if (gs->starPool.stars[i].active && gs->starPool.stars[i].isOrbiting) {
      count++;
    }
  }

  // Không có sao thì bỏ qua
  if (count == 0)
    return;

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
    Vector2 vel = {cosf(angle) * speed, sinf(angle) * speed};

    Color col = WHITE;
    int r = GetRandomValue(0, 5);
    if (r == 0)
      col = (Color){230, 230, 250, 255}; // Kim
    else if (r == 1)
      col = (Color){0, 230, 100, 255}; // Mộc
    else if (r == 2)
      col = (Color){0, 180, 255, 255}; // Thủy
    else if (r == 3)
      col = (Color){255, 80, 0, 255}; // Hỏa
    else if (r == 4)
      col = (Color){240, 180, 50, 255}; // Thổ
    else
      col = PINK; // Cầu vồng

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
          if (gs->enemyPool.enemies[e].type == ENEMY_GHOST &&
              !gs->enemyPool.enemies[e].isVisible)
            continue;
          float dist =
              Vector2Distance(s->position, gs->enemyPool.enemies[e].position);
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
        projVel = (Vector2){800.0f, 0.0f};
      }

      // Sinh đạn Ultimate homing
      int idx = SpawnProjectile(&gs->projectilePool, s->position, projVel,
                                22.0f, 100, false, s->tintColor, PROJ_FIREBALL);
      if (idx != -1) {
        gs->projectilePool.projectiles[idx].isUltimate = true;
      }

      // Xóa sao khỏi quỹ đạo
      DeactivateStar(&gs->starPool, i);
    }
  }
}
