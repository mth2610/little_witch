#include "raylib.h"
#include "fluid_skill.h"
#include "metal_skill.h"
#include "fire_skill.h" // Nhúng thư viện hệ Hỏa

// Định nghĩa các loại chiêu thức để dễ quản lý
typedef enum {
    SKILL_WATER = 0,
    SKILL_METAL,
    SKILL_FIRE // Thêm loại skill Hỏa vào danh sách
} SkillType;

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 700;
    InitWindow(screenWidth, screenHeight, "Avatar: Element Testbed");

    // Khởi tạo các module chiêu thức
    InitFluidSkill(screenWidth, screenHeight);
    InitMetalSkill(screenWidth, screenHeight);
    InitFireSkill(screenWidth, screenHeight); // Khởi tạo Hỏa

    Vector2 characterPos = { 130, 350 };
    SkillType activeSkill = SKILL_WATER; // Mặc định vào game là hệ Thủy

    // Khung UI của các nút bấm (cách nhau 170px)
    Rectangle btnWater = { 20, 20, 150, 45 };
    Rectangle btnMetal = { 190, 20, 150, 45 };
    Rectangle btnFire  = { 360, 20, 150, 45 }; // Khung nút hệ Hỏa

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mousePos = GetMousePosition();

        // 1. KIỂM TRA TƯƠNG TÁC UI
        bool hoverWater = CheckCollisionPointRec(mousePos, btnWater);
        bool hoverMetal = CheckCollisionPointRec(mousePos, btnMetal);
        bool hoverFire  = CheckCollisionPointRec(mousePos, btnFire);
        bool clickedOnUI = false;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            // Nếu bấm trúng nút UI thì đổi chiêu & đánh dấu là đã click UI
            if (hoverWater) {
                activeSkill = SKILL_WATER;
                clickedOnUI = true;
            } 
            else if (hoverMetal) {
                activeSkill = SKILL_METAL;
                clickedOnUI = true;
            }
            else if (hoverFire) {
                activeSkill = SKILL_FIRE;
                clickedOnUI = true;
            }

            // 2. NẾU KHÔNG CLICK VÀO UI -> PHÓNG CHIÊU THỨC
            if (!clickedOnUI) {
                if (activeSkill == SKILL_WATER) {
                    // Phóng 1 luồng nước
                    CastFluidSkill(characterPos, mousePos, 0.0f);
                } 
                else if (activeSkill == SKILL_METAL) {
                    // Phóng 3 phi kiếm
                    CastMetalSkill(characterPos, mousePos, 3);
                }
                else if (activeSkill == SKILL_FIRE) {
                    // Phóng 1 con Rồng Lửa
                    CastFireSkill(characterPos, mousePos, 0.0f);
                }
            }
        }

        // Cập nhật vật lý chiêu thức
        UpdateFluidSkill(dt);
        UpdateMetalSkill(dt);
        UpdateFireSkill(dt);

        // --- VẼ MÀN HÌNH ---
        BeginDrawing();
            ClearBackground(GetColor(0x111111FF)); 

            // Vẽ các chiêu thức đang bay (Nên vẽ theo thứ tự ưu tiên hiển thị)
            DrawFluidSkill();
            DrawMetalSkill();
            DrawFireSkill();

            // Vẽ nhân vật (cục tròn đại diện)
            DrawCircleV(characterPos, 30, GRAY); 
            // Vẽ thêm 1 chấm đỏ nhỏ ở tâm nhân vật cho giống cái nòng súng
            DrawCircleV(characterPos, 5, MAROON);

            // --- VẼ GIAO DIỆN CHỌN SKILL (UI) ---
            
            // Vẽ nút Hệ Thủy
            Color colorWater = activeSkill == SKILL_WATER ? BLUE : (hoverWater ? SKYBLUE : DARKBLUE);
            DrawRectangleRounded(btnWater, 0.3f, 10, colorWater);
            DrawRectangleRoundedLines(btnWater, 0.3f, 10, WHITE);            
            DrawText("WATER SKILL", (int)btnWater.x + 20, (int)btnWater.y + 15, 15, WHITE);

            // Vẽ nút Hệ Kim
            Color colorMetal = activeSkill == SKILL_METAL ? ORANGE : (hoverMetal ? GOLD : DARKGRAY);
            DrawRectangleRounded(btnMetal, 0.3f, 10, colorMetal);
            DrawRectangleRoundedLines(btnMetal, 0.3f, 10, WHITE);
            DrawText("METAL SKILL", (int)btnMetal.x + 22, (int)btnMetal.y + 15, 15, WHITE);
            
            // Vẽ nút Hệ Hỏa
            Color colorFire = activeSkill == SKILL_FIRE ? RED : (hoverFire ? ORANGE : MAROON);
            DrawRectangleRounded(btnFire, 0.3f, 10, colorFire);
            DrawRectangleRoundedLines(btnFire, 0.3f, 10, WHITE);
            DrawText("FIRE SKILL", (int)btnFire.x + 28, (int)btnFire.y + 15, 15, WHITE);
            
            // Text hướng dẫn
            DrawText("Left Click to shoot. Click UI to switch skills.", 20, 80, 16, LIGHTGRAY);

        EndDrawing();
    }

    // Dọn dẹp RAM/VRAM
    UnloadFluidSkill();
    UnloadMetalSkill();
    UnloadFireSkill();
    CloseWindow();
    
    return 0;
}