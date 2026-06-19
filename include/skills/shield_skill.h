#ifndef SHIELD_SKILL_H
#define SHIELD_SKILL_H

#include "raylib.h"
#include "skill.h"

struct GameState;

void InitShieldSkill(void);
void CastShieldSkill(struct GameState *gs);
void DrawShieldEffect(Vector2 position, float radiusVal);
void UnloadShieldSkill(void);

#endif // SHIELD_SKILL_H
