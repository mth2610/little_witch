#ifndef FIREBALL_SKILL_H
#define FIREBALL_SKILL_H

#include "raylib.h"
#include "skill.h"

struct GameState;

void InitFireballSkill(void);
void CastFireballSkill(struct GameState *gs);
void DrawFireballProjectile(const Projectile *p, const struct GameState *gs);
void UnloadFireballSkill(void);

#endif // FIREBALL_SKILL_H
