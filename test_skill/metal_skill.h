#ifndef METAL_SKILL_H
#define METAL_SKILL_H

#include "raylib.h"

void InitMetalSkill(int screenWidth, int screenHeight);

// Gọi chiêu: Bắn 'count' luồng kiếm khí từ startPos về phía target
void CastMetalSkill(Vector2 startPos, Vector2 target, int count);

void UpdateMetalSkill(float dt);
void DrawMetalSkill(void);
void UnloadMetalSkill(void);

#endif // METAL_SKILL_H
