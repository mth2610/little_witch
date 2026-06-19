#ifndef SHURIKEN_SKILL_H
#define SHURIKEN_SKILL_H

#include "raylib.h"
#include "skill.h"

struct GameState;

void InitShurikenSkill(void);
void CastShurikenSkill(struct GameState *gs);
void DrawShurikenProjectile(const Projectile *p, const struct GameState *gs);
void UnloadShurikenSkill(void);

#endif // SHURIKEN_SKILL_H
