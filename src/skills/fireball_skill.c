#include "fireball_skill.h"
#include "game.h"
#include "resource_loader.h"
#include "config.h"
#include "raymath.h"
#include <math.h>
#include <stddef.h>

static Shader fireballShader;
static bool fireballShaderLoaded = false;

void InitFireballSkill(void) {
    if (FileExists(SHADER_PATH("fireball.fs"))) {
        fireballShader = LoadShader(NULL, SHADER_PATH("fireball.fs"));
        fireballShaderLoaded = true;
    } else {
        fireballShaderLoaded = false;
    }
}

void CastFireballSkill(struct GameState *gs) {
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
}

void DrawFireballProjectile(const Projectile *p, const struct GameState *gs) {
    if (fireballShaderLoaded && !p->isEnemy) {
        BeginShaderMode(fireballShader);
        float timeVal = GetTime();
        float intensityVal =
            0.5f + 0.1f * gs->witch.skillLevels[SKILL_FIREBALL];
        SetShaderValue(fireballShader,
                       GetShaderLocation(fireballShader, "time"), &timeVal,
                       SHADER_UNIFORM_FLOAT);
        SetShaderValue(fireballShader,
                       GetShaderLocation(fireballShader, "center"),
                       &p->position, SHADER_UNIFORM_VEC2);
        SetShaderValue(fireballShader,
                       GetShaderLocation(fireballShader, "radius"),
                       &p->radius, SHADER_UNIFORM_FLOAT);
        SetShaderValue(fireballShader,
                       GetShaderLocation(fireballShader, "intensity"),
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

void UnloadFireballSkill(void) {
    if (fireballShaderLoaded) {
        UnloadShader(fireballShader);
        fireballShaderLoaded = false;
    }
}
