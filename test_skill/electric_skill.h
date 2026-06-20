#ifndef ELECTRIC_SKILL_H
#define ELECTRIC_SKILL_H

#include "raylib.h"

void InitElectricSkill(int screenWidth, int screenHeight);
void CastElectricSkill(Vector2 startPos, Vector2 target);
void UpdateElectricSkill(float dt);
void DrawElectricSkill(void);
void UnloadElectricSkill(void);
bool IsElectricSkillShocking(void);

#endif // ELECTRIC_SKILL_H
