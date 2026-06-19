#ifndef FIRE_SKILL_H
#define FIRE_SKILL_H

#include "raylib.h"

void InitFireSkill(int screenWidth, int screenHeight);
void CastFireSkill(Vector2 startPos, Vector2 target, float twistPhase);
void UpdateFireSkill(float dt);
void DrawFireSkill(void);
void UnloadFireSkill(void);

#endif // FIRE_SKILL_H
