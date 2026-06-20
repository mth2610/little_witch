#include "raylib.h"
#include <math.h>
#include "fluid_skill.h"
#include "metal_skill.h"
#include "fire_skill.h" 
#include "wood_skill.h" // Nhúng thư viện hệ Mộc
#include "electric_skill.h" // Nhúng thư viện hệ Lôi

// Định nghĩa các loại chiêu thức để dễ quản lý
typedef enum {
    SKILL_WATER = 0,
    SKILL_METAL,
    SKILL_FIRE, 
    SKILL_WOOD,  // Thêm loại skill Mộc vào danh sách
    SKILL_ELECTRIC // Thêm loại skill Lôi vào danh sách
} SkillType;

int main(void) {
    const int screenWidth = 1200;
    const int screenHeight = 700;
    InitWindow(screenWidth, screenHeight, "Avatar: Element Testbed");

    // Khởi tạo các module chiêu thức
    InitFluidSkill(screenWidth, screenHeight);
    InitMetalSkill(screenWidth, screenHeight);
    InitFireSkill(screenWidth, screenHeight); 
    InitWoodSkill(screenWidth, screenHeight); // Khởi tạo Mộc
    InitElectricSkill(screenWidth, screenHeight); // Khởi tạo Lôi

    Vector2 characterPos = { 130, 350 };
    SkillType activeSkill = SKILL_WATER; // Mặc định vào game là hệ Thủy

    // Khai báo biến vị trí Enemy và tỉ lệ dao động
    Vector2 enemyPos = { 900, 350 };
    float enemyOscillationScale = 1.0f;
    int woodBranchCycle = 3;

    // Khung UI của các nút bấm (cách nhau 170px)
    Rectangle btnWater = { 20, 20, 150, 45 };
    Rectangle btnMetal = { 190, 20, 150, 45 };
    Rectangle btnFire  = { 360, 20, 150, 45 }; 
    Rectangle btnWood  = { 530, 20, 150, 45 }; // Khung nút hệ Mộc
    Rectangle btnElectric = { 700, 20, 150, 45 }; // Khung nút hệ Lôi

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 mousePos = GetMousePosition();

        // Cập nhật vị trí Enemy dao động nhẹ nhàng lên xuống
        float time = GetTime();
        if (IsWoodSkillCoiling()) {
            // Giảm biên độ dao động về 0 nhanh chóng (bị trói cứng)
            enemyOscillationScale += (0.0f - enemyOscillationScale) * 9.0f * dt;
        } else {
            // Phục hồi dao động từ từ
            enemyOscillationScale += (1.0f - enemyOscillationScale) * 2.0f * dt;
        }
        enemyPos.x = 900.0f; // Reset x trước khi giật điện rung lắc
        enemyPos.y = 350.0f + sinf(time * 2.5f) * 100.0f * enemyOscillationScale;

        if (IsElectricSkillShocking()) {
            enemyPos.x += (float)GetRandomValue(-6, 6);
            enemyPos.y += (float)GetRandomValue(-6, 6);
        }

        // 1. KIỂM TRA TƯƠNG TÁC UI
        bool hoverWater = CheckCollisionPointRec(mousePos, btnWater);
        bool hoverMetal = CheckCollisionPointRec(mousePos, btnMetal);
        bool hoverFire  = CheckCollisionPointRec(mousePos, btnFire);
        bool hoverWood  = CheckCollisionPointRec(mousePos, btnWood); // Hover hệ Mộc
        bool hoverElectric = CheckCollisionPointRec(mousePos, btnElectric); // Hover hệ Lôi
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
            else if (hoverWood) {
                activeSkill = SKILL_WOOD;
                clickedOnUI = true;
            }
            else if (hoverElectric) {
                activeSkill = SKILL_ELECTRIC;
                clickedOnUI = true;
            }

            // 2. NẾU KHÔNG CLICK VÀO UI -> PHÓNG CHIÊU THỨC
            if (!clickedOnUI) {
                // Nhắm bắn thẳng vào con Enemy đang chuyển động để test hiệu ứng
                Vector2 target = enemyPos;

                if (activeSkill == SKILL_WATER) {
                    // Phóng 1 luồng nước
                    CastFluidSkill(characterPos, target, 0.0f);
                } 
                else if (activeSkill == SKILL_METAL) {
                    // Phóng 3 phi kiếm
                    CastMetalSkill(characterPos, target, 3);
                }
                else if (activeSkill == SKILL_FIRE) {
                    // Phóng 1 con Rồng Lửa
                    CastFireSkill(characterPos, target, 0.0f);
                }
                else if (activeSkill == SKILL_WOOD) {
                    CastWoodSkill(characterPos, target, woodBranchCycle);
                    woodBranchCycle = (woodBranchCycle % 4) + 1;
                }
                else if (activeSkill == SKILL_ELECTRIC) {
                    CastElectricSkill(characterPos, target);
                }
            }
        }

        // Cập nhật vật lý chiêu thức
        UpdateFluidSkill(dt);
        UpdateMetalSkill(dt);
        UpdateFireSkill(dt);
        UpdateWoodSkill(dt); // Cập nhật Mộc
        UpdateElectricSkill(dt); // Cập nhật Lôi

        // --- VẼ MÀN HÌNH ---
        BeginDrawing();
            ClearBackground(GetColor(0x111111FF)); 

            // Vẽ Enemy trước làm nền, để các hiệu ứng chiêu thức (quấn rễ) đè lên trên thân Enemy
            DrawCircleV(enemyPos, 35, GetColor(0x8B2500FF)); // Thân gỗ đỏ sẫm
            DrawCircleLinesV(enemyPos, 35, GetColor(0xCD3700FF)); // Viền đỏ tươi
            DrawCircleV(enemyPos, 8, GetColor(0xFF5500FF)); // Tâm phát sáng
            DrawText("ENEMY", (int)enemyPos.x - 22, (int)enemyPos.y - 6, 12, WHITE);

            // Vẽ các chiêu thức đang bay (Vẽ đè lên trên enemy)
            DrawFluidSkill();
            DrawMetalSkill();
            DrawFireSkill();
            DrawWoodSkill(); // Vẽ Mộc
            DrawElectricSkill(); // Vẽ Lôi

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

            // Vẽ nút Hệ Mộc
            Color colorWood = activeSkill == SKILL_WOOD ? GetColor(0x118822FF) : (hoverWood ? GetColor(0x22AA33FF) : GetColor(0x054410FF));
            DrawRectangleRounded(btnWood, 0.3f, 10, colorWood);
            DrawRectangleRoundedLines(btnWood, 0.3f, 10, WHITE);
            DrawText("WOOD SKILL", (int)btnWood.x + 28, (int)btnWood.y + 15, 15, WHITE);

            // Vẽ nút Hệ Lôi
            Color colorElectric = activeSkill == SKILL_ELECTRIC ? GetColor(0x8B008BFF) : (hoverElectric ? GetColor(0xBA55D3FF) : GetColor(0x4B0082FF));
            DrawRectangleRounded(btnElectric, 0.3f, 10, colorElectric);
            DrawRectangleRoundedLines(btnElectric, 0.3f, 10, WHITE);
            DrawText("ELEC SKILL", (int)btnElectric.x + 30, (int)btnElectric.y + 15, 15, WHITE);
            
            // Text hướng dẫn
            DrawText("Click screen to attack the oscillating Enemy.", 20, 80, 16, LIGHTGRAY);
            DrawText(TextFormat("WOOD SKILL wraps/coils with 1-4 branches (Next cast: %d branches)", woodBranchCycle), 20, 105, 16, GetColor(0x55DD66FF));
            DrawText("ELEC SKILL fires a plasma orb and calls down a lightning strike", 20, 130, 16, GetColor(0xBA55D3FF));

        EndDrawing();
    }

    // Dọn dẹp RAM/VRAM
    UnloadFluidSkill();
    UnloadMetalSkill();
    UnloadFireSkill();
    UnloadWoodSkill(); // Giải phóng Mộc
    UnloadElectricSkill(); // Giải phóng Lôi
    CloseWindow();
    
    return 0;
}