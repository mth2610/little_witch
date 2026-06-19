#ifndef FLUID_SKILL_H
#define FLUID_SKILL_H

#include "raylib.h"

struct GameState;

void InitFluidSkill(void);
void CastFluidSkill(struct GameState *gs);
void UpdateFluidSkill(struct GameState *gs, float dt);
void DrawFluidSkill(struct GameState *gs);
void DrawFluidSkillLightmap(struct GameState *gs);
void UnloadFluidSkill(void);

#endif // FLUID_SKILL_H
