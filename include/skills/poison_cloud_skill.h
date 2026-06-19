#ifndef POISON_CLOUD_SKILL_H
#define POISON_CLOUD_SKILL_H

#include "raylib.h"
#include "skill.h"

struct GameState;

void InitPoisonCloudSkill(void);
void CastPoisonCloudSkill(struct GameState *gs);
void DrawPoisonCloudProjectile(const Projectile *p, const struct GameState *gs, int idx);
void UnloadPoisonCloudSkill(void);

#endif // POISON_CLOUD_SKILL_H
