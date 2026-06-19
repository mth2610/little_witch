#ifndef LIGHTNING_SKILL_H
#define LIGHTNING_SKILL_H

#include "raylib.h"
#include "skill.h"

struct GameState;

void InitLightningSkill(void);
void CastLightningSkill(struct GameState *gs);
void UpdateLightningSkill(struct GameState *gs, float dt);
void DrawLightningSkill(struct GameState *gs);
void DrawLightningSkillLightmap(struct GameState *gs);
void DrawTornadoProjectile(const Projectile *p, const struct GameState *gs);
void UnloadLightningSkill(void);

#endif // LIGHTNING_SKILL_H
