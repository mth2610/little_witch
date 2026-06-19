#ifndef FLUID_SKILL_H
#define FLUID_SKILL_H

#include "raylib.h"

// Khởi tạo hệ thống
void InitFluidSkill(int screenWidth, int screenHeight);

// Gọi 1 lần cho 1 tia nước độc lập.
// twistPhase: Độ lệch pha (0.0f cho tia 1, PI cho tia 2 để chúng xoắn chéo nhau)
void CastFluidSkill(Vector2 startPos, Vector2 target, float twistPhase);

// Cập nhật hệ thống (Không cần truyền toạ độ tay vào đây nữa)
void UpdateFluidSkill(float dt);

// Vẽ ra màn hình
void DrawFluidSkill(void);

// Dọn dẹp
void UnloadFluidSkill(void);

#endif // FLUID_SKILL_H
