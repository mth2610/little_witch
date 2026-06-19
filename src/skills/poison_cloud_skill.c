#include "poison_cloud_skill.h"
#include "game.h"
#include "resource_loader.h"
#include "config.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static Shader poisonCloudShader;
static bool poisonCloudShaderLoaded = false;

void InitPoisonCloudSkill(void) {
    if (FileExists(SHADER_PATH("poison_cloud.fs"))) {
        poisonCloudShader = LoadShader(NULL, SHADER_PATH("poison_cloud.fs"));
        poisonCloudShaderLoaded = true;
    } else {
        poisonCloudShaderLoaded = false;
    }
}

void CastPoisonCloudSkill(struct GameState *gs) {
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
}

void DrawPoisonCloudProjectile(const Projectile *p, const struct GameState *gs, int idx) {
    if (poisonCloudShaderLoaded && !p->isEnemy) {
        BeginShaderMode(poisonCloudShader);
        float timeVal = GetTime();
        SetShaderValue(poisonCloudShader,
                       GetShaderLocation(poisonCloudShader, "time"),
                       &timeVal, SHADER_UNIFORM_FLOAT);
        SetShaderValue(poisonCloudShader,
                       GetShaderLocation(poisonCloudShader, "center"),
                       &p->position, SHADER_UNIFORM_VEC2);
        SetShaderValue(poisonCloudShader,
                       GetShaderLocation(poisonCloudShader, "radius"),
                       &p->radius, SHADER_UNIFORM_FLOAT);
        DrawRectangle((int)(p->position.x - p->radius * 2.0f),
                      (int)(p->position.y - p->radius * 2.0f),
                      (int)(p->radius * 4.0f), (int)(p->radius * 4.0f), WHITE);
        EndShaderMode();
    } else {
        float pulse = 1.0f + sinf(GetTime() * 6.0f + idx) * 0.12f;
        DrawCircleGradient(p->position.x, p->position.y, p->radius * pulse,
                           ColorAlpha(p->color, 0.28f),
                           ColorAlpha(p->color, 0.0f));
        DrawCircleGradient(p->position.x, p->position.y,
                           p->radius * 0.5f * pulse, ColorAlpha(WHITE, 0.15f),
                           ColorAlpha(p->color, 0.0f));
    }
}

void UnloadPoisonCloudSkill(void) {
    if (poisonCloudShaderLoaded) {
        UnloadShader(poisonCloudShader);
        poisonCloudShaderLoaded = false;
    }
}
