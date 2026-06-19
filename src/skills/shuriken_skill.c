#include "shuriken_skill.h"
#include "game.h"
#include "resource_loader.h"
#include "config.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static Shader shurikenShader;
static bool shurikenShaderLoaded = false;

void InitShurikenSkill(void) {
    if (FileExists(SHADER_PATH("shuriken.fs"))) {
        shurikenShader = LoadShader(NULL, SHADER_PATH("shuriken.fs"));
        shurikenShaderLoaded = true;
    } else {
        shurikenShaderLoaded = false;
    }
}

void CastShurikenSkill(struct GameState *gs) {
    int lvl = gs->witch.skillLevels[SKILL_SHURIKEN];
    if (gs->witch.effectiveKimStars == 0) {
      // Weak normal-like shuriken (single, straight, small, low dmg, non-piercing)
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
}

void DrawShurikenProjectile(const Projectile *p, const struct GameState *gs) {
    if (shurikenShaderLoaded && !p->isEnemy) {
        BeginShaderMode(shurikenShader);
        float timeVal = GetTime();
        float rotVal = p->rotation * DEG2RAD;
        SetShaderValue(shurikenShader,
                       GetShaderLocation(shurikenShader, "time"), &timeVal,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(shurikenShader,
                       GetShaderLocation(shurikenShader, "center"),
                       &p->position, SHADER_UNIFORM_VEC2);
        SetShaderValue(shurikenShader,
                       GetShaderLocation(shurikenShader, "radius"),
                       &p->radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shurikenShader,
                       GetShaderLocation(shurikenShader, "rotation"),
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
          side2 =
              (Vector2){p->position.x + cosf(angle - 0.25f) * p->radius * 0.4f,
                        p->position.y + sinf(angle - 0.25f) * p->radius * 0.4f};
          DrawTriangle(tip, side1, side2, p->color);
          DrawTriangle(tip, side2, side1, p->color);
        }
        DrawCircleV(p->position, p->radius * 0.35f, WHITE);
        DrawCircleLines(p->position.x, p->position.y, p->radius * 0.4f, YELLOW);
    }
}

void UnloadShurikenSkill(void) {
    if (shurikenShaderLoaded) {
        UnloadShader(shurikenShader);
        shurikenShaderLoaded = false;
    }
}
