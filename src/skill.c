// ============================================================
// skill.c — Logic các kỹ năng (RPG Active Skills) và Đạn (Projectiles)
// ============================================================

#include "skill.h"
#include "game.h"
#include "particle.h"
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
      if (gs != NULL && gs->shurikenShaderLoaded && !p->isEnemy) {
        BeginShaderMode(gs->shurikenShader);
        float timeVal = GetTime();
        float rotVal = p->rotation * DEG2RAD;
        SetShaderValue(gs->shurikenShader,
                       GetShaderLocation(gs->shurikenShader, "time"), &timeVal,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(gs->shurikenShader,
                       GetShaderLocation(gs->shurikenShader, "center"),
                       &p->position, SHADER_UNIFORM_VEC2);
        SetShaderValue(gs->shurikenShader,
                       GetShaderLocation(gs->shurikenShader, "radius"),
                       &p->radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(gs->shurikenShader,
                       GetShaderLocation(gs->shurikenShader, "rotation"),
                       &rotVal, SHADER_UNIFORM_FLOAT);
        DrawRectangle((int)(p->position.x - p->radius * 3.0f),
                      (int)(p->position.y - p->radius * 3.0f),
                      (int)(p->radius * 6.0f), (int)(p->radius * 6.0f), WHITE);
        EndShaderMode();
      } else {
        float rotRad = p->rotation * DEG2RAD;
        for (int j = 0; j < 4; j++) {
          float angle = rotRad + j * (PI / 2.0f);
          Vector2 tip = {p->position.x + cosf(angle) * p->radius * 1.5f,
                         p->position.y + sinf(angle) * p->radius * 1.5f};
          Vector2 side1 = {
              p->position.x + cosf(angle + 0.25f) * p->radius * 0.4f,
              p->position.y + sinf(angle + 0.25f) * p->radius * 0.4f};
          Vector2 side2 = {
              p->position.x + cosf(angle - 0.25f) * p->radius * 0.4f,
              p->position.y - sinf(angle - 0.25f) * p->radius * 0.4f};
          // Sửa nhẹ cosf/sinf cho side2 để đối xứng
          side2 =
              (Vector2){p->position.x + cosf(angle - 0.25f) * p->radius * 0.4f,
                        p->position.y + sinf(angle - 0.25f) * p->radius * 0.4f};
          DrawTriangle(tip, side1, side2, p->color);
          DrawTriangle(tip, side2, side1, p->color);
        }
        DrawCircleV(p->position, p->radius * 0.35f, WHITE);
        DrawCircleLines(p->position.x, p->position.y, p->radius * 0.4f, YELLOW);
      }
    } else if (p->type == PROJ_POISON) {
      if (gs != NULL && gs->poisonCloudShaderLoaded && !p->isEnemy) {
        BeginShaderMode(gs->poisonCloudShader);
        float timeVal = GetTime();
        SetShaderValue(gs->poisonCloudShader,
                       GetShaderLocation(gs->poisonCloudShader, "time"),
                       &timeVal, SHADER_UNIFORM_FLOAT);
        SetShaderValue(gs->poisonCloudShader,
                       GetShaderLocation(gs->poisonCloudShader, "center"),
                       &p->position, SHADER_UNIFORM_VEC2);
        SetShaderValue(gs->poisonCloudShader,
                       GetShaderLocation(gs->poisonCloudShader, "radius"),
                       &p->radius, SHADER_UNIFORM_FLOAT);
        DrawRectangle((int)(p->position.x - p->radius * 2.0f),
                      (int)(p->position.y - p->radius * 2.0f),
                      (int)(p->radius * 4.0f), (int)(p->radius * 4.0f), WHITE);
        EndShaderMode();
      } else {
        float pulse = 1.0f + sinf(GetTime() * 6.0f + i) * 0.12f;
        DrawCircleGradient(p->position.x, p->position.y, p->radius * pulse,
                           ColorAlpha(p->color, 0.28f),
                           ColorAlpha(p->color, 0.0f));
        DrawCircleGradient(p->position.x, p->position.y,
                           p->radius * 0.5f * pulse, ColorAlpha(WHITE, 0.15f),
                           ColorAlpha(p->color, 0.0f));
      }
    } else if (p->type == PROJ_TORNADO) {
      if (gs != NULL && gs->tornadoShaderLoaded && !p->isEnemy) {
        BeginShaderMode(gs->tornadoShader);
        float timeVal = GetTime();
        float heightVal = p->radius * 3.5f;
        SetShaderValue(gs->tornadoShader,
                       GetShaderLocation(gs->tornadoShader, "time"), &timeVal,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(gs->tornadoShader,
                       GetShaderLocation(gs->tornadoShader, "center"),
                       &p->position, SHADER_UNIFORM_VEC2);
        SetShaderValue(gs->tornadoShader,
                       GetShaderLocation(gs->tornadoShader, "radius"),
                       &p->radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(gs->tornadoShader,
                       GetShaderLocation(gs->tornadoShader, "height"),
                       &heightVal, SHADER_UNIFORM_FLOAT);
        DrawRectangle((int)(p->position.x - p->radius * 1.5f),
                      (int)(p->position.y - heightVal * 1.2f),
                      (int)(p->radius * 3.0f), (int)(heightVal * 1.4f), WHITE);
        EndShaderMode();
      } else {
        float tRot = p->rotation * DEG2RAD;
        for (int ring = 0; ring < 6; ring++) {
          float offsetOffset = ring * 9.0f;
          float ringRadX = p->radius * (1.1f - ring * 0.13f);
          float ringRadY = ringRadX * 0.35f;
          Vector2 ringCenter = {p->position.x,
                                p->position.y - offsetOffset + 20.0f};

          float angleShift = tRot + ring * 0.4f;
          Vector2 pt1 = {ringCenter.x + cosf(angleShift) * ringRadX,
                         ringCenter.y + sinf(angleShift) * ringRadY};
          Vector2 pt2 = {ringCenter.x + cosf(angleShift + PI) * ringRadX,
                         ringCenter.y + sinf(angleShift + PI) * ringRadY};

          DrawLineEx(pt1, pt2, 4.0f - ring * 0.5f, ColorAlpha(p->color, 0.45f));
          DrawCircleV(pt1, 2.8f, WHITE);
        }
      }
    } else if (p->type == PROJ_ICE) {
      if (gs != NULL && gs->iceBlastShaderLoaded && !p->isEnemy) {
        BeginShaderMode(gs->iceBlastShader);
        float timeVal = GetTime();
        float rotVal = p->rotation * DEG2RAD;
        SetShaderValue(gs->iceBlastShader,
                       GetShaderLocation(gs->iceBlastShader, "time"), &timeVal,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(gs->iceBlastShader,
                       GetShaderLocation(gs->iceBlastShader, "center"),
                       &p->position, SHADER_UNIFORM_VEC2);
        SetShaderValue(gs->iceBlastShader,
                       GetShaderLocation(gs->iceBlastShader, "radius"),
                       &p->radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(gs->iceBlastShader,
                       GetShaderLocation(gs->iceBlastShader, "rotation"),
                       &rotVal, SHADER_UNIFORM_FLOAT);

        int screenTexLoc =
            GetShaderLocation(gs->iceBlastShader, "screenTexture");
        SetShaderValueTexture(gs->iceBlastShader, screenTexLoc,
                              shieldBackgroundTexture);

        // Sử dụng DrawTexturePro với texture dummy trắng để đưa fragTexCoord
        // (0->1) vào shader
        DrawTexturePro(gs->dummyWhiteTex, (Rectangle){0.0f, 0.0f, 2.0f, 2.0f},
                       (Rectangle){p->position.x, p->position.y,
                                   p->radius * 5.0f, p->radius * 5.0f},
                       (Vector2){p->radius * 2.5f, p->radius * 2.5f},
                       p->rotation, WHITE);
        EndShaderMode();
      } else {
        float rotRad = p->rotation * DEG2RAD;
        Vector2 p1 = {p->position.x + cosf(rotRad) * p->radius * 1.7f,
                      p->position.y + sinf(rotRad) * p->radius * 1.7f};
        Vector2 p2 = {
            p->position.x + cosf(rotRad + PI / 2.0f) * p->radius * 0.5f,
            p->position.y + sinf(rotRad + PI / 2.0f) * p->radius * 0.5f};
        Vector2 p3 = {p->position.x - cosf(rotRad) * p->radius * 0.6f,
                      p->position.y - sinf(rotRad) * p->radius * 0.6f};
        Vector2 p4 = {
            p->position.x - cosf(rotRad + PI / 2.0f) * p->radius * 0.5f,
            p->position.y - sinf(rotRad + PI / 2.0f) * p->radius * 0.5f};
        DrawTriangle(p1, p2, p4, p->color);
        DrawTriangle(p3, p4, p2, p->color);
        DrawTriangle(p1, p4, p2, WHITE);
      }
    } else {
      if (p->type == PROJ_FIREBALL && gs != NULL && gs->fireballShaderLoaded &&
          !p->isEnemy) {
        BeginShaderMode(gs->fireballShader);
        float timeVal = GetTime();
        float intensityVal =
            0.5f + 0.1f * gs->witch.skillLevels[SKILL_FIREBALL];
        SetShaderValue(gs->fireballShader,
                       GetShaderLocation(gs->fireballShader, "time"), &timeVal,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(gs->fireballShader,
                       GetShaderLocation(gs->fireballShader, "center"),
                       &p->position, SHADER_UNIFORM_VEC2);
        SetShaderValue(gs->fireballShader,
                       GetShaderLocation(gs->fireballShader, "radius"),
                       &p->radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(gs->fireballShader,
                       GetShaderLocation(gs->fireballShader, "intensity"),
                       &intensityVal, SHADER_UNIFORM_FLOAT);
        DrawRectangle((int)(p->position.x - p->radius * 11.5f),
                      (int)(p->position.y - p->radius * 3.0f),
                      (int)(p->radius * 15.0f), (int)(p->radius * 6.0f), WHITE);
        EndShaderMode();
      } else {
        DrawCircleV(p->position, p->radius * 1.8f, ColorAlpha(p->color, 0.35f));
        DrawCircleV(p->position, p->radius, p->color);
        DrawCircleV(p->position, p->radius * 0.4f, WHITE);
      }
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

  switch (skill) {
  case SKILL_SHURIKEN: {
    int lvl = gs->witch.skillLevels[SKILL_SHURIKEN];
    if (gs->witch.effectiveKimStars == 0) {
      // Weak normal-like shuriken (single, straight, small, low dmg,
      // non-piercing)
      Vector2 vel = {550.0f, 0.0f};
      int idx = SpawnProjectile(&gs->projectilePool, gs->witch.position, vel,
                                6.0f, 8, false, GOLD, PROJ_SHURIKEN);
      if (idx != -1) {
        gs->projectilePool.projectiles[idx].isWeak = true;
      }

      // Fewer particles
      for (int i = 0; i < 3; i++) {
        float vx = (float)GetRandomValue(-40, 40);
        float vy = (float)GetRandomValue(-40, 40);
        SpawnParticle(&gs->particlePool, gs->witch.position, (Vector2){vx, vy},
                      0.3f, 1.5f, GOLD);
      }
    } else {
      // Hệ KIM: Phi tiêu vàng lấp lánh xuyên thấu
      int count = 1 + (gs->witch.effectiveKimStars / 2) + (lvl - 1);
      if (count > 6)
        count = 6;

      float baseAngle = 0.0f;
      float spread = 15.0f * DEG2RAD;
      for (int j = 0; j < count; j++) {
        float angle = baseAngle + (j - (count - 1) / 2.0f) * spread;
        Vector2 vel = {cosf(angle) * 550.0f, sinf(angle) * 550.0f};
        float radius = (11.0f + 1.5f * gs->witch.effectiveKimStars) *
                       (1.0f + 0.08f * (lvl - 1));
        int dmg =
            (20 + 8 * gs->witch.effectiveKimStars) * (1.0f + 0.15f * (lvl - 1));

        SpawnProjectile(&gs->projectilePool, gs->witch.position, vel, radius,
                        dmg, false, GOLD, PROJ_SHURIKEN);
      }

      // Hiệu ứng hạt đầy đủ
      for (int i = 0; i < 10; i++) {
        float vx = (float)GetRandomValue(-80, 80);
        float vy = (float)GetRandomValue(-80, 80);
        SpawnParticle(&gs->particlePool, gs->witch.position, (Vector2){vx, vy},
                      0.4f, 2.5f, GOLD);
      }
    }
    gs->witch.skillCooldown[SKILL_SHURIKEN] =
        fmaxf(1.0f, 2.0f - 0.15f * (lvl - 1));
    gs->witch.animState = WITCH_STATE_ATTACK_HAND;
    gs->witch.stateTimer = 0.3f;
    if (IsSoundReady(shootSound))
      PlaySound(shootSound);
    break;
  }

  case SKILL_POISON_CLOUD: {
    int lvl = gs->witch.skillLevels[SKILL_POISON_CLOUD];
    if (gs->witch.effectiveMocStars == 0) {
      // Weak poison spit (single, straight, small, low dmg, non-lingering)
      Vector2 vel = {550.0f, 0.0f};
      int idx = SpawnProjectile(&gs->projectilePool, gs->witch.position, vel,
                                6.0f, 8, false, LIME, PROJ_POISON);
      if (idx != -1) {
        gs->projectilePool.projectiles[idx].isWeak = true;
      }
    } else {
      // Hệ MỘC: Làn hơi độc màu xanh lá
      int targetIdx = -1;
      float minDist = 999999.0f;
      for (int e = 0; e < MAX_ENEMIES; e++) {
        if (gs->enemyPool.enemies[e].active) {
          if (gs->enemyPool.enemies[e].type == ENEMY_GHOST &&
              !gs->enemyPool.enemies[e].isVisible)
            continue;
          float dist = Vector2Distance(gs->witch.position,
                                       gs->enemyPool.enemies[e].position);
          if (dist < minDist) {
            minDist = dist;
            targetIdx = e;
          }
        }
      }

      Vector2 spawnPos = {gs->witch.position.x + 180.0f, gs->witch.position.y};
      if (targetIdx != -1) {
        spawnPos = gs->enemyPool.enemies[targetIdx].position;
      }

      float radius = (60.0f + 12.0f * gs->witch.effectiveMocStars) *
                     (1.0f + 0.12f * (lvl - 1));
      int dmg =
          (5 + 2 * gs->witch.effectiveMocStars) * (1.0f + 0.25f * (lvl - 1));

      int idx = SpawnProjectile(&gs->projectilePool, spawnPos,
                                (Vector2){-20.0f, 0.0f}, radius, dmg, false,
                                (Color){0, 230, 100, 255}, PROJ_POISON);
      if (idx != -1) {
        gs->projectilePool.projectiles[idx].timer =
            4.0f + 0.5f * (lvl - 1); // Tồn tại lâu hơn
        gs->projectilePool.projectiles[idx].damageTimer = 0.0f;
      }
    }

    gs->witch.skillCooldown[SKILL_POISON_CLOUD] =
        fmaxf(1.5f, 3.5f - 0.25f * (lvl - 1));
    gs->witch.animState = WITCH_STATE_ATTACK_HAND;
    gs->witch.stateTimer = 0.3f;
    if (IsSoundReady(collectStarSound))
      PlaySound(collectStarSound);
    break;
  }

  case SKILL_ICE_BLAST: {
    int lvl = gs->witch.skillLevels[SKILL_ICE_BLAST];
    if (gs->witch.effectiveThuyStars == 0) {
      // Weak ice spit (single, straight, small, low dmg, non-freezing)
      Vector2 vel = {550.0f, 0.0f};
      int idx = SpawnProjectile(&gs->projectilePool, gs->witch.position, vel,
                                6.0f, 8, false, SKYBLUE, PROJ_ICE);
      if (idx != -1) {
        gs->projectilePool.projectiles[idx].isWeak = true;
      }

      // Fewer particles
      for (int i = 0; i < 3; i++) {
        float vx = (float)GetRandomValue(50, 120);
        float vy = (float)GetRandomValue(-25, 25);
        SpawnParticle(&gs->particlePool, gs->witch.position, (Vector2){vx, vy},
                      0.3f, 1.5f, CYAN);
      }
    } else {
      // Hệ THỦY: Chưởng lực băng đóng băng kẻ địch
      Vector2 vel = {650.0f, 0.0f};
      float radius = (18.0f + 2.0f * gs->witch.effectiveThuyStars) *
                     (1.0f + 0.08f * (lvl - 1));
      int dmg =
          (25 + 10 * gs->witch.effectiveThuyStars) * (1.0f + 0.20f * (lvl - 1));

      SpawnProjectile(&gs->projectilePool, gs->witch.position, vel, radius, dmg,
                      false, SKYBLUE, PROJ_ICE);

      // Hạt sương giá đầy đủ
      for (int i = 0; i < 12; i++) {
        float vx = (float)GetRandomValue(100, 250);
        float vy = (float)GetRandomValue(-50, 50);
        SpawnParticle(&gs->particlePool, gs->witch.position, (Vector2){vx, vy},
                      0.5f, 3.0f, CYAN);
      }
    }

    gs->witch.skillCooldown[SKILL_ICE_BLAST] =
        fmaxf(2.0f, 4.5f - 0.3f * (lvl - 1));
    gs->witch.animState = WITCH_STATE_ATTACK_WEAPON;
    gs->witch.stateTimer = 0.35f;
    if (IsSoundReady(shootSound))
      PlaySound(shootSound);
    break;
  }

  case SKILL_FIREBALL: {
    int lvl = gs->witch.skillLevels[SKILL_FIREBALL];
    if (gs->witch.effectiveHoaStars == 0) {
      // Weak fireball spit (single, straight, small, low dmg, non-explosive)
      Vector2 vel = {550.0f, 0.0f};
      int idx = SpawnProjectile(&gs->projectilePool, gs->witch.position, vel,
                                6.0f, 8, false, ORANGE, PROJ_FIREBALL);
      if (idx != -1) {
        gs->projectilePool.projectiles[idx].isWeak = true;
      }
    } else {
      // Hệ HỎA: Cầu lửa nổ diện rộng (AOE)
      int dmg =
          (20 + 10 * gs->witch.effectiveHoaStars) * (1.0f + 0.20f * (lvl - 1));
      float radius = (10.0f + 1.5f * gs->witch.effectiveHoaStars) *
                     (1.0f + 0.1f * (lvl - 1));

      int targetIdx = -1;
      float minDist = 999999.0f;
      for (int e = 0; e < MAX_ENEMIES; e++) {
        if (gs->enemyPool.enemies[e].active) {
          if (gs->enemyPool.enemies[e].type == ENEMY_GHOST &&
              !gs->enemyPool.enemies[e].isVisible)
            continue;
          float dist = Vector2Distance(gs->witch.position,
                                       gs->enemyPool.enemies[e].position);
          if (dist < minDist) {
            minDist = dist;
            targetIdx = e;
          }
        }
      }

      Vector2 projVel;
      if (targetIdx != -1) {
        Vector2 targetPos = gs->enemyPool.enemies[targetIdx].position;
        Vector2 dir =
            Vector2Normalize(Vector2Subtract(targetPos, gs->witch.position));
        projVel = Vector2Scale(dir, 440.0f);
      } else {
        projVel = (Vector2){440.0f, 0.0f};
      }

      Color ballColor = ORANGE;
      if (gs->witch.effectiveHoaStars >= 3 || lvl >= 3) {
        ballColor = RED;
      }

      SpawnProjectile(&gs->projectilePool, gs->witch.position, projVel, radius,
                      dmg, false, ballColor, PROJ_FIREBALL);
    }
    gs->witch.skillCooldown[SKILL_FIREBALL] =
        fmaxf(0.5f, 1.0f - 0.07f * (lvl - 1));
    gs->witch.animState = WITCH_STATE_ATTACK_WEAPON;
    gs->witch.stateTimer = 0.4f;
    if (IsSoundReady(shootSound))
      PlaySound(shootSound);
    break;
  }

  case SKILL_LIGHTNING: {
    int lvl = gs->witch.skillLevels[SKILL_LIGHTNING];
    if (gs->witch.effectiveThoStars == 0) {
      // Weak electric spark (giữ nguyên, không đổi)
      Vector2 vel = {550.0f, 0.0f};
      int idx = SpawnProjectile(&gs->projectilePool, gs->witch.position, vel,
                                6.0f, 8, false, YELLOW, PROJ_TORNADO);
      if (idx != -1) {
        gs->projectilePool.projectiles[idx].isWeak = true;
      }
    } else {
      // Hệ THỔ: CHỈ Lốc xoáy cuồng phong (đã bỏ Chain Lightning)
      float tRadius = (40.0f + 8.0f * gs->witch.effectiveThoStars) *
                      (1.0f + 0.1f * (lvl - 1));
      int tDmg =
          (15 + 5 * gs->witch.effectiveThoStars) * (1.0f + 0.15f * (lvl - 1));
      Vector2 tVel = {300.0f, 0.0f};
      int tIdx = SpawnProjectile(&gs->projectilePool, gs->witch.position, tVel,
                                 tRadius, tDmg, false, YELLOW, PROJ_TORNADO);
      if (tIdx != -1) {
        gs->projectilePool.projectiles[tIdx].timer = 3.5f + 0.5f * (lvl - 1);
        gs->projectilePool.projectiles[tIdx].damageTimer = 0.0f;
      }

      gs->shakeIntensity = 6.0f;
      gs->shakeDuration = 0.25f;
    }

    gs->witch.skillCooldown[SKILL_LIGHTNING] =
        fmaxf(3.0f, 6.0f - 0.4f * (lvl - 1));
    gs->witch.animState = WITCH_STATE_ATTACK_HAND;
    gs->witch.stateTimer = 0.3f;
    if (IsSoundReady(shootSound))
      PlaySound(shootSound);
    break;
  }

  case SKILL_MAGNET: {
    int lvl = gs->witch.skillLevels[SKILL_MAGNET];
    // Buff Nam châm: kích hoạt lâu hơn theo level, giảm hồi chiêu
    gs->witch.skillActiveTimer[SKILL_MAGNET] = 6.0f + 1.5f * (lvl - 1);
    gs->witch.skillCooldown[SKILL_MAGNET] =
        fmaxf(6.0f, 12.0f - 0.8f * (lvl - 1));
    if (IsSoundReady(collectStarSound))
      PlaySound(collectStarSound);

    // Hiệu ứng hạt hội tụ vào phù thủy
    for (int i = 0; i < 20; i++) {
      float angle = ((float)i / 20.0f) * 2.0f * PI;
      float dist = 120.0f;
      Vector2 startPos = {gs->witch.position.x + cosf(angle) * dist,
                          gs->witch.position.y + sinf(angle) * dist};
      Vector2 vel = {-cosf(angle) * 250.0f, -sinf(angle) * 250.0f};
      SpawnParticle(&gs->particlePool, startPos, vel, 0.45f, 2.0f, GOLD);
    }
    break;
  }

  case SKILL_SHIELD: {
    int lvl = gs->witch.skillLevels[SKILL_SHIELD];
    // Buff Khiên ma pháp chủ động: kích hoạt khiên ma lực, thời gian tăng theo
    // level, giảm hồi chiêu
    gs->witch.manaShieldHealth =
        1.67f +
        0.33f * (lvl -
                 1); // Tăng thời gian (0.33f = 1 giây khi chia 3.0f ở witch.c)
    gs->witch.skillCooldown[SKILL_SHIELD] =
        fmaxf(8.0f, 15.0f - 1.0f * (lvl - 1));

    // Chuyển tư thế DEFENSE
    gs->witch.animState = WITCH_STATE_DEFENSE;
    gs->witch.stateTimer = 0.5f;
    if (IsSoundReady(shootSound))
      PlaySound(shootSound);

    // Hiệu ứng hạt tỏa tròn từ phù thủy
    for (int i = 0; i < 15; i++) {
      float angle = ((float)i / 15.0f) * 2.0f * PI;
      float speed = 160.0f;
      Vector2 vel = {cosf(angle) * speed, sinf(angle) * speed};
      SpawnParticle(&gs->particlePool, gs->witch.position, vel, 0.4f, 2.5f,
                    SKYBLUE);
    }
    break;
  }

  default:
    break;
  }
}

// Cập nhật lan truyền sát thương sét chuỗi Chain Lightning thời gian thực
void UpdateSkills(struct GameState *gs, float deltaTime) {
  if (gs == NULL)
    return;

  // Cập nhật vị trí xích sét (Chain Lightning) thời gian thực theo quái/người
  // chơi
  if (gs->witch.skillActiveTimer[SKILL_LIGHTNING] > 0.0f) {
    gs->lightningPoints[0] = gs->witch.position;

    float elapsed = 0.5f - gs->witch.skillActiveTimer[SKILL_LIGHTNING];

    // Cập nhật vị trí của các quái làm mục tiêu nếu chúng còn sống, nếu chết
    // giữ nguyên vị trí cũ
    for (int i = 0; i < 3; i++) {
      int targetIdx = gs->lightningTargets[i];
      if (targetIdx != -1) {
        if (gs->enemyPool.enemies[targetIdx].active) {
          gs->lightningTargetLastPos[i] =
              gs->enemyPool.enemies[targetIdx].position;
        }
        gs->lightningPoints[i + 1] = gs->lightningTargetLastPos[i];

        // Gây sát thương và nổ hạt theo thứ tự lan truyền (0.1s mỗi bước nhảy)
        float hitTime = 0.1f * (i + 1);
        if (elapsed >= hitTime && !gs->lightningDamageApplied[i]) {
          gs->lightningDamageApplied[i] = true;

          int lvl = gs->witch.skillLevels[SKILL_LIGHTNING];
          int damage = (i == 0) ? 35 : ((i == 1) ? 25 : 20);
          damage = (int)(damage * (1.0f + 0.20f * (lvl - 1)));
          damage +=
              12 *
              gs->witch.effectiveThoStars; // Tăng sát thương sét theo sao Thổ
          bool killed = DamageEnemy(&gs->enemyPool, targetIdx, damage);

          // Sinh hiệu ứng nổ tại mục tiêu
          SpawnExplosion(&gs->particlePool, gs->lightningTargetLastPos[i],
                         8 - i, SKYBLUE);

          // Nếu quái bị hạ gục thì cộng điểm/vàng cho phù thủy
          if (killed) {
            gs->witch.score +=
                (gs->enemyPool.enemies[targetIdx].type >= ENEMY_BOSS_FOREST)
                    ? 200
                    : 25;
            gs->witch.gold +=
                GetEnemyGoldDrop(gs->enemyPool.enemies[targetIdx].type);
            SpawnExplosion(&gs->particlePool, gs->lightningTargetLastPos[i], 14,
                           RED);

            // Nếu là Boss bị hạ gục
            if (gs->enemyPool.enemies[targetIdx].type >= ENEMY_BOSS_FOREST) {
              // Chuyển biome (gọi hàm được khai báo ở main.c thông qua cơ chế
              // quản lý màn chơi) Lưu ý: Logic chuyển biome sẽ được xử lý ở
              // main.c sau checkCollisions, tuy nhiên ta vẫn gọi DamageEnemy và
              // đánh dấu chết ở đây, main.c check trong vòng lặp chính.
            }
          }
        }
      }
    }
  }
}

// Vẽ tia sét xích (Chain Lightning)
void DrawSkills(struct GameState *gs) {
  if (gs == NULL)
    return;

  if (gs->witch.skillActiveTimer[SKILL_LIGHTNING] > 0.0f &&
      gs->lightningPointCount >= 2) {
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
      Vector4 glowCol = {0.45f, 0.35f, 1.0f, 1.0f}; // Màu tím điện neon

      SetShaderValue(gs->lightningShader, timeLoc, &timeVal,
                     SHADER_UNIFORM_FLOAT);
      SetShaderValue(gs->lightningShader, glowColorLoc, &glowCol,
                     SHADER_UNIFORM_VEC4);

      for (int i = 0; i < gs->lightningPointCount - 1; i++) {
        float startT = 0.1f * i;
        float endT = 0.1f * (i + 1);

        if (elapsed < startT)
          continue;

        Vector2 segmentStart = gs->lightningPoints[i];
        Vector2 segmentEnd;

        if (elapsed < endT) {
          float p = (elapsed - startT) / 0.1f;
          segmentEnd = Vector2Lerp(segmentStart, gs->lightningPoints[i + 1], p);
        } else {
          segmentEnd = gs->lightningPoints[i + 1];
        }

        SetShaderValue(gs->lightningShader, startPosLoc, &segmentStart,
                       SHADER_UNIFORM_VEC2);
        SetShaderValue(gs->lightningShader, endPosLoc, &segmentEnd,
                       SHADER_UNIFORM_VEC2);

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

        if (elapsed < startT)
          continue;

        Vector2 segmentStart = gs->lightningPoints[i];
        Vector2 segmentEnd;

        if (elapsed < endT) {
          float p = (elapsed - startT) / 0.1f;
          segmentEnd = Vector2Lerp(segmentStart, gs->lightningPoints[i + 1], p);
        } else {
          segmentEnd = gs->lightningPoints[i + 1];
        }

        // Vẽ tia sét nhánh ziczac phụ họa đơn giản bằng cách tạo điểm ngẫu
        // nhiên ở giữa segment
        Vector2 mid = Vector2Scale(Vector2Add(segmentStart, segmentEnd), 0.5f);
        Vector2 perp = {-(segmentEnd.y - segmentStart.y),
                        segmentEnd.x - segmentStart.x};
        perp = Vector2Normalize(perp);
        float offsetVal = sinf(GetTime() * 45.0f + i) * 15.0f;
        mid = Vector2Add(mid, Vector2Scale(perp, offsetVal));

        // Nét vẽ điện neon dày tím ngoài, lõi trắng mảnh ở giữa
        DrawLineEx(segmentStart, mid, 6.0f, (Color){120, 90, 255, 255});
        DrawLineEx(mid, segmentEnd, 6.0f, (Color){120, 90, 255, 255});

        DrawLineEx(segmentStart, mid, 2.0f, WHITE);
        DrawLineEx(mid, segmentEnd, 2.0f, WHITE);
      }
      EndBlendMode();
    }
  }
}

// Vẽ quầng sáng Chain Lightning lên Lightmap để tạo hiệu ứng ánh sáng Ori
void DrawSkillsLightmap(struct GameState *gs) {
  if (gs == NULL)
    return;

  if (gs->witch.skillActiveTimer[SKILL_LIGHTNING] > 0.0f &&
      gs->lightningPointCount >= 2) {
    float elapsed = 0.5f - gs->witch.skillActiveTimer[SKILL_LIGHTNING];

    // Điểm nguồn phát sáng tại vị trí phù thủy
    DrawCircleGradient(gs->lightningPoints[0].x, gs->lightningPoints[0].y,
                       130.0f, ColorAlpha(VIOLET, 0.35f),
                       ColorAlpha(VIOLET, 0.0f));
    DrawCircleGradient(gs->lightningPoints[0].x, gs->lightningPoints[0].y,
                       45.0f, ColorAlpha(WHITE, 0.55f),
                       ColorAlpha(WHITE, 0.0f));

    for (int i = 0; i < gs->lightningPointCount - 1; i++) {
      float startT = 0.1f * i;
      float endT = 0.1f * (i + 1);

      if (elapsed < startT)
        continue;

      Vector2 segmentStart = gs->lightningPoints[i];
      Vector2 segmentEnd;

      if (elapsed < endT) {
        float p = (elapsed - startT) / 0.1f;
        segmentEnd = Vector2Lerp(segmentStart, gs->lightningPoints[i + 1], p);
      } else {
        segmentEnd = gs->lightningPoints[i + 1];

        // Điểm đích đã đạt tới hoàn toàn tỏa sáng rực rỡ
        DrawCircleGradient(segmentEnd.x, segmentEnd.y, 130.0f,
                           ColorAlpha(VIOLET, 0.35f), ColorAlpha(VIOLET, 0.0f));
        DrawCircleGradient(segmentEnd.x, segmentEnd.y, 45.0f,
                           ColorAlpha(WHITE, 0.55f), ColorAlpha(WHITE, 0.0f));
      }

      // Vẽ quầng sáng mờ dọc theo thân tia sét (trung điểm)
      Vector2 mid = Vector2Scale(Vector2Add(segmentStart, segmentEnd), 0.5f);
      DrawCircleGradient(mid.x, mid.y, 110.0f, ColorAlpha(SKYBLUE, 0.25f),
                         ColorAlpha(SKYBLUE, 0.0f));
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
