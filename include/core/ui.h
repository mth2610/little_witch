#ifndef UI_H
#define UI_H

#include "raylib.h"
#include "config.h"
#include "witch.h"

// Khai báo trước struct GameState
struct GameState;

// HUD trong gameplay (mạng, điểm số, vàng, kĩ năng hoạt động, máu boss)
void DrawHUD(const struct GameState *gs);

// Màn hình Menu chính
void DrawMenu(const struct GameState *gs);
void UpdateMenu(struct GameState *gs, float deltaTime);

// Màn hình chọn Kỹ năng (sau khi diệt Boss)
void DrawSkillSelect(const struct GameState *gs);
void UpdateSkillSelect(struct GameState *gs);

// Màn hình Game Over
void DrawGameOver(const struct GameState *gs);
void UpdateGameOver(struct GameState *gs);

// Màn hình Shop Nâng cấp Vĩnh viễn
void DrawUpgradeShop(const struct GameState *gs);
void UpdateUpgradeShop(struct GameState *gs);

// Vẽ nút bấm tiện ích dùng chung (chỉ thực hiện chức năng vẽ)
void DrawButton(Rectangle bounds, const char *text, Color color);

// Kiểm tra xem nút bấm có được nhấn chuột/touch hay không (logic)
bool IsButtonClicked(Rectangle bounds);

// Lấy tọa độ tâm nút kỹ năng theo bố cục vòng cung RPG Mobile
Vector2 GetSkillButtonCenter(SkillType skill);

// Kiểm tra xem nút bấm dạng hình tròn có được nhấn/chạm hay không
bool IsCircleButtonClicked(Vector2 center, float radius);

// Dịch ngôn ngữ động dựa vào GameState
const char* Tr(const struct GameState *gs, const char *vi, const char *en);

#endif // UI_H
