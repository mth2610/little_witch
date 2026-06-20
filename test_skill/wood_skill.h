#ifndef WOOD_SKILL_H
#define WOOD_SKILL_H

#include "raylib.h"

void InitWoodSkill(int screenWidth, int screenHeight);
void CastWoodSkill(Vector2 startPos, Vector2 target, int branchCount);
void UpdateWoodSkill(float dt);
void DrawWoodSkill(void);
void UnloadWoodSkill(void);
bool IsWoodSkillCoiling(void);


#endif // WOOD_SKILL_H
