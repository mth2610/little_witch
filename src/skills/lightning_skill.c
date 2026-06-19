#include "lightning_skill.h"
#include "game.h"
#include "resource_loader.h"
#include "config.h"
#include "raymath.h"
#include "enemy.h"
#include "particle.h"
#include <math.h>
#include <stddef.h>

static Shader tornadoShader;
static bool tornadoShaderLoaded = false;
static Shader lightningShader;
static bool lightningShaderLoaded = false;

void InitLightningSkill(void) {
    if (FileExists(SHADER_PATH("tornado.fs"))) {
        tornadoShader = LoadShader(NULL, SHADER_PATH("tornado.fs"));
        tornadoShaderLoaded = true;
    } else {
        tornadoShaderLoaded = false;
    }

    if (FileExists(SHADER_PATH("lightning.fs"))) {
        lightningShader = LoadShader(NULL, SHADER_PATH("lightning.fs"));
        lightningShaderLoaded = true;
    } else {
        lightningShaderLoaded = false;
    }
}

void CastLightningSkill(struct GameState *gs) {
    int lvl = gs->witch.skillLevels[SKILL_LIGHTNING];
    if (gs->witch.effectiveThoStars == 0) {
      // Weak electric spark
      Vector2 vel = {550.0f, 0.0f};
      int idx = SpawnProjectile(&gs->projectilePool, gs->witch.position, vel,
                                6.0f, 8, false, YELLOW, PROJ_TORNADO);
      if (idx != -1) {
        gs->projectilePool.projectiles[idx].isWeak = true;
      }
    } else {
      // Hệ THỔ: CHỈ Lốc xoáy cuồng phong (đã bỏ Chain Lightning ở CastActiveSkill)
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
}

void UpdateLightningSkill(struct GameState *gs, float dt) {
  if (gs == NULL)
    return;

  // Cập nhật vị trí xích sét (Chain Lightning) thời gian thực theo quái/người chơi
  if (gs->witch.skillActiveTimer[SKILL_LIGHTNING] > 0.0f) {
    gs->lightningPoints[0] = gs->witch.position;

    float elapsed = 0.5f - gs->witch.skillActiveTimer[SKILL_LIGHTNING];

    // Cập nhật vị trí của các quái làm mục tiêu nếu chúng còn sống, nếu chết giữ nguyên vị trí cũ
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
          }
        }
      }
    }
  }
}

void DrawLightningSkill(struct GameState *gs) {
  if (gs == NULL)
    return;

  if (gs->witch.skillActiveTimer[SKILL_LIGHTNING] > 0.0f &&
      gs->lightningPointCount >= 2) {
    float elapsed = 0.5f - gs->witch.skillActiveTimer[SKILL_LIGHTNING];

    if (lightningShaderLoaded) {
      // VẼ TIA SÉT BẰNG SHADER CHUYÊN BIỆT (LIGHTNING SHADER)
      BeginBlendMode(BLEND_ADDITIVE);
      BeginShaderMode(lightningShader);

      int startPosLoc = GetShaderLocation(lightningShader, "startPos");
      int endPosLoc = GetShaderLocation(lightningShader, "endPos");
      int timeLoc = GetShaderLocation(lightningShader, "time");
      int glowColorLoc = GetShaderLocation(lightningShader, "glowColor");

      float timeVal = GetTime();
      Vector4 glowCol = {0.45f, 0.35f, 1.0f, 1.0f}; // Màu tím điện neon

      SetShaderValue(lightningShader, timeLoc, &timeVal,
                     SHADER_UNIFORM_FLOAT);
      SetShaderValue(lightningShader, glowColorLoc, &glowCol,
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

        SetShaderValue(lightningShader, startPosLoc, &segmentStart,
                       SHADER_UNIFORM_VEC2);
        SetShaderValue(lightningShader, endPosLoc, &segmentEnd,
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

        // Vẽ tia sét nhánh ziczac phụ họa đơn giản bằng cách tạo điểm ngẫu nhiên ở giữa segment
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

void DrawLightningSkillLightmap(struct GameState *gs) {
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

void DrawTornadoProjectile(const Projectile *p, const struct GameState *gs) {
    if (tornadoShaderLoaded && !p->isEnemy) {
        BeginShaderMode(tornadoShader);
        float timeVal = GetTime();
        float heightVal = p->radius * 3.5f;
        SetShaderValue(tornadoShader,
                       GetShaderLocation(tornadoShader, "time"), &timeVal,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(tornadoShader,
                       GetShaderLocation(tornadoShader, "center"),
                       &p->position, SHADER_UNIFORM_VEC2);
        SetShaderValue(tornadoShader,
                       GetShaderLocation(tornadoShader, "radius"),
                       &p->radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(tornadoShader,
                       GetShaderLocation(tornadoShader, "height"),
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
}

void UnloadLightningSkill(void) {
    if (tornadoShaderLoaded) {
        UnloadShader(tornadoShader);
        tornadoShaderLoaded = false;
    }
    if (lightningShaderLoaded) {
        UnloadShader(lightningShader);
        lightningShaderLoaded = false;
    }
}
