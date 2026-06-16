// ============================================================
// biome_procedural.c — Triển khai vẽ nền vector thủ thuật (Procedural Art)
// ============================================================

#include "biome_procedural.h"
#include "game.h"
#include "raymath.h"
#include <math.h>

#define INDIGO (Color){ 75, 0, 130, 255 }

// --- FOREST PROCEDURAL BACKGROUND ---
void DrawProceduralBackgroundForest(struct GameState *gs, float rawScroll) {
    // Gradient bầu trời tối huyền ảo
    Color skyTop = (Color){ 15, 32, 42, 255 };      // Teal tối trung tính
    Color skyBottom = (Color){ 20, 55, 42, 255 };  // Xanh lục rừng huyền bí
    DrawRectangleGradientV(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, skyTop, skyBottom);
    
    // Vẽ các dải nắng/God Rays xiên nhẹ từ trên trái xuống (Pulsing & Swaying phong cách Ori)
    float rayTime = gs->frameCount * 0.015f;
    float rayPulse = 0.7f + 0.3f * sinf(rayTime);
    
    // Ray 1: Cyan-Blue ray
    float sway1 = sinf(rayTime * 0.5f) * 40.0f;
    DrawTriangle(
        (Vector2){ -150.0f + sway1, -50 },
        (Vector2){ 350.0f + sway1, -50 },
        (Vector2){ 100.0f + sway1 * 1.5f, VIRTUAL_HEIGHT + 50 },
        ColorAlpha(SKYBLUE, 0.06f * rayPulse)
    );
    
    // Ray 2: Magic Green/Lime ray
    float sway2 = cosf(rayTime * 0.7f) * 30.0f;
    DrawTriangle(
        (Vector2){ 100.0f + sway2, -50 },
        (Vector2){ 600.0f + sway2, -50 },
        (Vector2){ 400.0f + sway2 * 1.3f, VIRTUAL_HEIGHT + 50 },
        ColorAlpha(LIME, 0.05f * rayPulse)
    );
    
    // Ray 3: Soft Gold ray
    float sway3 = sinf(rayTime * 0.9f + 1.0f) * 25.0f;
    DrawTriangle(
        (Vector2){ 400.0f + sway3, -50 },
        (Vector2){ 850.0f + sway3, -50 },
        (Vector2){ 650.0f + sway3 * 1.2f, VIRTUAL_HEIGHT + 50 },
        ColorAlpha(GOLD, 0.04f * rayPulse)
    );
    
    // Lớp 1: Hậu cảnh xa (Far Mountains & Giant Trees - Cuộn x0.15)
    float farSpeed = 0.15f;
    float farSpacing = 320.0f;
    for (int i = 0; i < 6; i++) {
        float x = fmodf(i * farSpacing - rawScroll * farSpeed, VIRTUAL_WIDTH + farSpacing);
        if (x < -farSpacing) x += (VIRTUAL_WIDTH + farSpacing);
        
        DrawTriangle(
            (Vector2){ x + 160.0f, VIRTUAL_HEIGHT - 450.0f },
            (Vector2){ x - 40.0f, VIRTUAL_HEIGHT },
            (Vector2){ x + 360.0f, VIRTUAL_HEIGHT },
            ColorAlpha((Color){ 25, 75, 65, 255 }, 0.28f)
        );
    }
    
    // Sương mù mờ ảo lớp 1 (Volumetric Mist Layer 1 - Cuộn x0.25)
    float mistScroll1 = fmodf(rawScroll * 0.25f, VIRTUAL_WIDTH + 450.0f);
    for (int m = 0; m < 4; m++) {
        float mx = m * 450.0f - mistScroll1;
        if (mx < -250.0f) mx += VIRTUAL_WIDTH + 450.0f;
        float my = VIRTUAL_HEIGHT - 120.0f + sinf(gs->frameCount * 0.01f + m) * 15.0f;
        DrawCircleGradient(mx, my, 220.0f, ColorAlpha(SKYBLUE, 0.05f), ColorAlpha(SKYBLUE, 0.0f));
    }
    
    // Lớp 2: Hậu cảnh trung (Mid Trees - Cuộn x0.4)
    float midSpeed = 0.4f;
    float midSpacing = 240.0f;
    for (int i = 0; i < 7; i++) {
        float x = fmodf(i * midSpacing - rawScroll * midSpeed, VIRTUAL_WIDTH + midSpacing);
        if (x < -midSpacing) x += (VIRTUAL_WIDTH + midSpacing);
        
        DrawTriangle(
            (Vector2){ x + 120.0f, VIRTUAL_HEIGHT - 320.0f },
            (Vector2){ x - 10.0f, VIRTUAL_HEIGHT },
            (Vector2){ x + 250.0f, VIRTUAL_HEIGHT },
            ColorAlpha((Color){ 20, 70, 50, 255 }, 0.55f)
        );
        DrawCircle(x + 120.0f, VIRTUAL_HEIGHT - 320.0f, 35.0f, ColorAlpha((Color){ 24, 78, 56, 255 }, 0.55f));
    }
    
    // Sương mù mờ ảo lớp 2 (Volumetric Mist Layer 2 - Cuộn x0.6)
    float mistScroll2 = fmodf(rawScroll * 0.6f, VIRTUAL_WIDTH + 400.0f);
    for (int m = 0; m < 5; m++) {
        float mx = m * 350.0f - mistScroll2;
        if (mx < -200.0f) mx += VIRTUAL_WIDTH + 350.0f;
        float my = VIRTUAL_HEIGHT - 70.0f + cosf(gs->frameCount * 0.013f + m) * 10.0f;
        DrawCircleGradient(mx, my, 180.0f, ColorAlpha(LIME, 0.04f), ColorAlpha(LIME, 0.0f));
    }
    
    // Lớp 3: Lớp Gameplay nền (Near Roots/Bushes - Cuộn x0.8)
    float nearSpeed = 0.8f;
    float nearSpacing = 200.0f;
    for (int i = 0; i < 8; i++) {
        float x = fmodf(i * nearSpacing - rawScroll * nearSpeed, VIRTUAL_WIDTH + nearSpacing);
        if (x < -nearSpacing) x += (VIRTUAL_WIDTH + nearSpacing);
        
        DrawTriangle(
            (Vector2){ x + 100.0f, VIRTUAL_HEIGHT - 210.0f },
            (Vector2){ x + 10.0f, VIRTUAL_HEIGHT },
            (Vector2){ x + 190.0f, VIRTUAL_HEIGHT },
            ColorAlpha((Color){ 12, 50, 35, 255 }, 0.82f)
        );
        DrawCircle(x + 100.0f, VIRTUAL_HEIGHT - 210.0f, 25.0f, ColorAlpha((Color){ 15, 60, 42, 255 }, 0.85f));
        
        // Rễ cây
        DrawLineEx((Vector2){ x, VIRTUAL_HEIGHT }, (Vector2){ x + 60.0f, VIRTUAL_HEIGHT - 80.0f }, 12.0f, (Color){ 10, 35, 20, 255 });
        
        // Nụ hoa rừng thần tiên phát sáng xanh cyan
        Vector2 budPos = { x + 60.0f, VIRTUAL_HEIGHT - 80.0f };
        float budPulse = 1.0f + sinf(gs->frameCount * 0.07f + i) * 0.2f;
        DrawCircleV(budPos, 4.0f * budPulse, CYAN);
        DrawCircleV(budPos, 10.0f * budPulse, ColorAlpha(CYAN, 0.35f));
    }
    
    // Vẽ đom đóm bay lấp lánh (glowing fireflies)
    for (int f = 0; f < 12; f++) {
        float fireflyX = fmodf(f * 113.0f - rawScroll * 0.3f, VIRTUAL_WIDTH);
        if (fireflyX < 0) fireflyX += VIRTUAL_WIDTH;
        float fireflyY = fmodf(f * 89.0f + sinf(gs->frameCount * 0.03f + f) * 35.0f, VIRTUAL_HEIGHT);
        float pulse = 1.2f + sinf(gs->frameCount * 0.08f + f) * 0.5f;
        
        DrawCircle(fireflyX, fireflyY, pulse * 8.0f, ColorAlpha(LIME, 0.15f));  // Hào quang ngoài
        DrawCircle(fireflyX, fireflyY, pulse * 4.0f, ColorAlpha(YELLOW, 0.4f)); // Hào quang trong
        DrawCircle(fireflyX, fireflyY, pulse * 1.5f, WHITE);                   // Lõi sáng trắng
    }
}

// --- CAVE PROCEDURAL BACKGROUND ---
void DrawProceduralBackgroundCave(struct GameState *gs, float rawScroll) {
    // Bầu trời hang động tím sẫm (Cave depth gradient)
    Color caveTop = (Color){ 12, 8, 18, 255 };
    Color caveBottom = (Color){ 25, 15, 20, 255 };
    DrawRectangleGradientV(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, caveTop, caveBottom);
    
    // Vẽ chùm sáng chiếu lên từ các tinh thể pha lê ở đáy hang (Crystal Light Shafts)
    float caveRayPulse = 0.6f + 0.4f * sinf(gs->frameCount * 0.025f);
    for (int i = 0; i < 4; i++) {
        float cx = fmodf(i * 380.0f - rawScroll * 0.5f, VIRTUAL_WIDTH + 150.0f) - 75.0f;
        if (cx < -150.0f) cx += VIRTUAL_WIDTH + 150.0f;
        
        float sway = sinf(gs->frameCount * 0.01f + i) * 20.0f;
        DrawTriangle(
            (Vector2){ cx + 50.0f, VIRTUAL_HEIGHT },
            (Vector2){ cx + sway - 30.0f, 0.0f },
            (Vector2){ cx + sway + 100.0f, 0.0f },
            ColorAlpha(SKYBLUE, 0.035f * caveRayPulse)
        );
    }
    
    // Lớp 1: Hậu cảnh xa (Vòm hang lớn - Cuộn x0.2)
    float farSpeed = 0.2f;
    float farSpacing = 400.0f;
    for (int i = 0; i < 5; i++) {
        float x = fmodf(i * farSpacing - rawScroll * farSpeed, VIRTUAL_WIDTH + farSpacing);
        if (x < -farSpacing) x += (VIRTUAL_WIDTH + farSpacing);
        
        DrawTriangle(
            (Vector2){ x, 0.0f },
            (Vector2){ x + 400.0f, 0.0f },
            (Vector2){ x + 200.0f, 280.0f },
            ColorAlpha((Color){ 18, 12, 24, 255 }, 0.4f)
        );
    }
    
    // Sương mù ánh tím dày đặc bò trên nền hang (Cave rolling violet mist)
    float caveMistScroll = fmodf(rawScroll * 0.4f, VIRTUAL_WIDTH + 300.0f);
    for (int m = 0; m < 5; m++) {
        float mx = m * 350.0f - caveMistScroll;
        if (mx < -150.0f) mx += VIRTUAL_WIDTH + 350.0f;
        float my = VIRTUAL_HEIGHT - 30.0f + sinf(gs->frameCount * 0.015f + m) * 12.0f;
        DrawCircleGradient(mx, my, 160.0f, ColorAlpha(VIOLET, 0.06f), ColorAlpha(VIOLET, 0.0f));
    }
    
    // Lớp 2: Hậu cảnh trung (Nhũ đá trung cảnh - Cuộn x0.5)
    float midSpeed = 0.5f;
    float midSpacing = 220.0f;
    for (int i = 0; i < 7; i++) {
        float x = fmodf(i * midSpacing - rawScroll * midSpeed, VIRTUAL_WIDTH + midSpacing);
        if (x < -midSpacing) x += (VIRTUAL_WIDTH + midSpacing);
        
        // Nhũ đá sàn trung cảnh
        DrawTriangle(
            (Vector2){ x + 110.0f, VIRTUAL_HEIGHT - 260.0f },
            (Vector2){ x + 10.0f, VIRTUAL_HEIGHT },
            (Vector2){ x + 210.0f, VIRTUAL_HEIGHT },
            ColorAlpha((Color){ 30, 18, 22, 255 }, 0.65f)
        );
        // Nhũ đá trần trung cảnh
        DrawTriangle(
            (Vector2){ x + 50.0f, 0.0f },
            (Vector2){ x + 170.0f, 0.0f },
            (Vector2){ x + 110.0f, 180.0f },
            ColorAlpha((Color){ 34, 20, 26, 255 }, 0.65f)
        );
    }
    
    // Lớp 3: Cận gameplay (Nhũ đá sắc cạnh - Cuộn x0.9)
    float nearSpeed = 0.9f;
    float nearSpacing = 160.0f;
    for (int i = 0; i < 9; i++) {
        float x = fmodf(i * nearSpacing - rawScroll * nearSpeed, VIRTUAL_WIDTH + nearSpacing);
        if (x < -nearSpacing) x += (VIRTUAL_WIDTH + nearSpacing);
        
        // Nhũ đá sàn gần sắc cạnh màu tối
        DrawTriangle(
            (Vector2){ x + 80.0f, VIRTUAL_HEIGHT - 120.0f },
            (Vector2){ x + 20.0f, VIRTUAL_HEIGHT },
            (Vector2){ x + 140.0f, VIRTUAL_HEIGHT },
            (Color){ 20, 12, 16, 255 }
        );
        // Nhũ đá trần gần
        DrawTriangle(
            (Vector2){ x + 40.0f, 0.0f },
            (Vector2){ x + 120.0f, 0.0f },
            (Vector2){ x + 80.0f, 95.0f },
            (Color){ 24, 14, 20, 255 }
        );
    }
    
    // Vẽ tinh thể pha lê phát sáng nhấp nháy (glowing crystals)
    for (int i = 0; i < 6; i++) {
        float cx = fmodf(i * 240.0f - rawScroll, VIRTUAL_WIDTH + 100.0f) - 50.0f;
        if (cx < -100.0f) cx += VIRTUAL_WIDTH + 100.0f;
        
        float pulse = 0.5f + 0.5f * sinf(gs->frameCount * 0.06f + i);
        Color crystalColor = ColorAlpha(SKYBLUE, 0.3f + 0.6f * pulse);
        
        // Tinh thể sàn phát sáng
        Vector2 pS = { cx + 40, VIRTUAL_HEIGHT - 50 };
        DrawCircle(pS.x, pS.y + 20, 30.0f, ColorAlpha(SKYBLUE, 0.1f * pulse));
        DrawTriangle(pS, (Vector2){ cx + 25, VIRTUAL_HEIGHT }, (Vector2){ cx + 55, VIRTUAL_HEIGHT }, crystalColor);
        DrawTriangleLines(pS, (Vector2){ cx + 25, VIRTUAL_HEIGHT }, (Vector2){ cx + 55, VIRTUAL_HEIGHT }, WHITE);
    }
    
    // Bào tử pha lê phát sáng bay lơ lửng hướng lên trên
    for (int s = 0; s < 15; s++) {
        float sx = fmodf(s * 93.0f - rawScroll * 0.2f, VIRTUAL_WIDTH);
        if (sx < 0) sx += VIRTUAL_WIDTH;
        float sy = fmodf(s * 71.0f - gs->frameCount * 0.6f, VIRTUAL_HEIGHT + 50.0f);
        if (sy < -20.0f) sy += VIRTUAL_HEIGHT + 50.0f;
        
        float pulse = 1.0f + sinf(gs->frameCount * 0.07f + s) * 0.4f;
        Color sporeCol = (s % 2 == 0) ? SKYBLUE : VIOLET;
        DrawCircle(sx, sy, pulse * 5.0f, ColorAlpha(sporeCol, 0.15f));
        DrawCircle(sx, sy, pulse * 1.5f, WHITE);
    }
}

// --- CITY PROCEDURAL BACKGROUND ---
void DrawProceduralBackgroundCity(struct GameState *gs, float rawScroll) {
    // Bầu trời thành phố đêm
    Color cityTop = (Color){ 6, 8, 20, 255 };
    Color cityBottom = (Color){ 15, 20, 38, 255 };
    DrawRectangleGradientV(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, cityTop, cityBottom);
    
    // Xe bay cyberpunk lướt qua trong đêm
    for (int t = 0; t < 8; t++) {
        float tx = fmodf(t * 193.0f - gs->frameCount * 1.5f, VIRTUAL_WIDTH + 100.0f) - 50.0f;
        if (tx < -100.0f) tx += VIRTUAL_WIDTH + 100.0f;
        float ty = 80.0f + fmodf(t * 73.0f, 220.0f);
        
        Color trafficCol = (t % 2 == 0) ? CYAN : ORANGE;
        DrawCircle(tx, ty, 3.0f, trafficCol);
        DrawLineEx((Vector2){ tx, ty }, (Vector2){ tx + 12.0f, ty }, 1.5f, ColorAlpha(trafficCol, 0.35f));
    }
    
    // Lớp 1: Hậu cảnh xa (Building chọc trời mờ - Cuộn x0.15)
    float farSpeed = 0.15f;
    float farSpacing = 280.0f;
    for (int i = 0; i < 6; i++) {
        float x = fmodf(i * farSpacing - rawScroll * farSpeed, VIRTUAL_WIDTH + farSpacing);
        if (x < -farSpacing) x += (VIRTUAL_WIDTH + farSpacing);
        
        float buildingH = 350.0f + sinf(i * 1.5f) * 120.0f;
        DrawRectangle(x, VIRTUAL_HEIGHT - buildingH, 200.0f, buildingH, ColorAlpha((Color){ 20, 25, 45, 255 }, 0.3f));
    }
    
    // Khói công nghiệp ánh hồng/xanh sương mù cuộn bên dưới
    float citySmogScroll = fmodf(rawScroll * 0.3f, VIRTUAL_WIDTH + 300.0f);
    for (int m = 0; m < 5; m++) {
        float mx = m * 330.0f - citySmogScroll;
        if (mx < -150.0f) mx += VIRTUAL_WIDTH + 330.0f;
        float my = VIRTUAL_HEIGHT - 60.0f + sinf(gs->frameCount * 0.012f + m) * 10.0f;
        Color smogCol = (m % 2 == 0) ? MAGENTA : INDIGO;
        DrawCircleGradient(mx, my, 190.0f, ColorAlpha(smogCol, 0.05f), ColorAlpha(smogCol, 0.0f));
    }
    
    // Lớp 2: Hậu cảnh trung (Building cửa sổ sáng - Cuộn x0.45)
    float midSpeed = 0.45f;
    float midSpacing = 220.0f;
    for (int i = 0; i < 7; i++) {
        float x = fmodf(i * midSpacing - rawScroll * midSpeed, VIRTUAL_WIDTH + midSpacing);
        if (x < -midSpacing) x += (VIRTUAL_WIDTH + midSpacing);
        
        float buildingW = 150.0f;
        float buildingH = 250.0f + sinf(i * 3.3f) * 90.0f;
        
        DrawRectangle(x, VIRTUAL_HEIGHT - buildingH, buildingW, buildingH, ColorAlpha((Color){ 25, 32, 58, 255 }, 0.6f));
        DrawRectangleLines(x, VIRTUAL_HEIGHT - buildingH, buildingW, buildingH, ColorAlpha(CYAN, 0.2f));
        
        // Bảng quảng cáo hologram rực rỡ neon (Cyberpunk Billboard)
        if (i % 3 == 0) {
            float pulse = 0.8f + 0.2f * sinf(gs->frameCount * 0.05f + i);
            Color holoColor = (i % 2 == 0) ? MAGENTA : CYAN;
            
            DrawRectangleLinesEx((Rectangle){ x + 10.0f, VIRTUAL_HEIGHT - buildingH + 50.0f, 60.0f, 40.0f }, 2.0f, ColorAlpha(holoColor, pulse));
            DrawRectangleRec((Rectangle){ x + 12.0f, VIRTUAL_HEIGHT - buildingH + 52.0f, 56.0f, 36.0f }, ColorAlpha(holoColor, 0.1f * pulse));
            DrawCircle(x + 40.0f, VIRTUAL_HEIGHT - buildingH + 70.0f, 8.0f * pulse, ColorAlpha(holoColor, 0.3f));
        }
        
        // Dây cáp điện neon nối giữa các tòa nhà võng xuống
        if (i < 6) {
            float nextX = fmodf((i + 1) * midSpacing - rawScroll * midSpeed, VIRTUAL_WIDTH + midSpacing);
            if (nextX < -midSpacing) nextX += (VIRTUAL_WIDTH + midSpacing);
            
            Vector2 pStart = { x + buildingW, VIRTUAL_HEIGHT - buildingH + 30.0f };
            float nextBuildingH = 250.0f + sinf((i + 1) * 3.3f) * 90.0f;
            Vector2 pEnd = { nextX, VIRTUAL_HEIGHT - nextBuildingH + 30.0f };
            
            Vector2 midPoint = { (pStart.x + pEnd.x) * 0.5f, ((pStart.y + pEnd.y) * 0.5f) + 30.0f };
            Color cableCol = (i % 2 == 0) ? CYAN : MAGENTA;
            DrawLineEx(pStart, midPoint, 2.0f, ColorAlpha(cableCol, 0.35f));
            DrawLineEx(midPoint, pEnd, 2.0f, ColorAlpha(cableCol, 0.35f));
        }
        
        // Ô cửa sổ nhỏ lấp lánh
        for (int w = 0; w < 3; w++) {
            for (int h = 0; h < (int)(buildingH / 40); h++) {
                if ((w + h + i) % 4 == 0) {
                    Color winCol = ((w + i) % 2 == 0) ? GOLD : SKYBLUE;
                    DrawRectangle(x + 20.0f + w * 40.0f, VIRTUAL_HEIGHT - buildingH + 25.0f + h * 40.0f, 12.0f, 15.0f, ColorAlpha(winCol, 0.25f));
                }
            }
        }
    }
    
    // Lớp 3: Cận gameplay (Cuộn x0.8)
    float nearSpeed = 0.8f;
    float nearSpacing = 300.0f;
    for (int i = 0; i < 5; i++) {
        float x = fmodf(i * nearSpacing - rawScroll * nearSpeed, VIRTUAL_WIDTH + nearSpacing);
        if (x < -nearSpacing) x += (VIRTUAL_WIDTH + nearSpacing);
        
        // Cột sắt và dây điện
        DrawRectangle(x + 40, VIRTUAL_HEIGHT - 180.0f, 16.0f, 180.0f, (Color){ 18, 22, 35, 255 });
        DrawLineEx((Vector2){ x, VIRTUAL_HEIGHT - 130.0f }, (Vector2){ x + 300.0f, VIRTUAL_HEIGHT - 145.0f }, 1.5f, ColorAlpha(DARKGRAY, 0.7f));
    }
}

// --- SPACE PROCEDURAL BACKGROUND ---
void DrawProceduralBackgroundSpace(struct GameState *gs, float rawScroll) {
    Color spaceColor = (Color){ 4, 3, 10, 255 };
    ClearBackground(spaceColor);
    
    // 0. Siêu tân tinh / Lõi tinh vân xoáy khổng lồ ở trung tâm nền (Cosmic Nebula Core)
    Vector2 coreCenter = { VIRTUAL_WIDTH * 0.5f, VIRTUAL_HEIGHT * 0.45f };
    float coreTime = gs->frameCount * 0.005f;
    
    for (int layer = 0; layer < 4; layer++) {
        float scale = 1.0f - layer * 0.2f;
        float rotSpeed = coreTime * (1.0f + layer * 0.5f);
        Color coreColor = (layer % 2 == 0) ? MAGENTA : CYAN;
        
        DrawCircleGradient(coreCenter.x, coreCenter.y, 180.0f * scale, ColorAlpha(coreColor, 0.06f), ColorAlpha(coreColor, 0.0f));
        
        for (int arm = 0; arm < 3; arm++) {
            float angleOffset = arm * 2.094f; // 120 độ
            for (int dot = 1; dot <= 8; dot++) {
                float dist = dot * 18.0f * scale;
                float angle = rotSpeed + angleOffset + (dot * 0.22f);
                float dx = cosf(angle) * dist;
                float dy = sinf(angle) * dist;
                float dotPulse = 0.5f + 0.5f * sinf(gs->frameCount * 0.1f + dot);
                
                DrawCircle(coreCenter.x + dx, coreCenter.y + dy, (4.0f - dot * 0.3f) * dotPulse, ColorAlpha(coreColor, 0.5f));
            }
        }
    }
    DrawCircleGradient(coreCenter.x, coreCenter.y, 25.0f, ColorAlpha(WHITE, 0.7f), ColorAlpha(WHITE, 0.0f));
    
    // 1. Tinh vân khí rực rỡ (Cosmic Nebula Clouds - Cuộn x0.08)
    float nebSpeed = 0.08f;
    for (int n = 0; n < 3; n++) {
        float x = fmodf(n * 500.0f - rawScroll * nebSpeed, VIRTUAL_WIDTH + 400.0f) - 200.0f;
        Vector2 center = { x, VIRTUAL_HEIGHT * 0.4f + n * 70.0f };
        Color nebColor = (n == 0) ? PURPLE : (n == 1 ? DARKBLUE : MAGENTA);
        DrawCircleGradient(center.x, center.y, 250.0f, ColorAlpha(nebColor, 0.09f), ColorAlpha(nebColor, 0.0f));
    }
    
    // 2. Vì sao lấp lánh ở vô tận (Twinkling Stars - Cuộn x0.12)
    float starSpeed = 0.12f;
    for (int s = 0; s < 30; s++) {
        float starX = fmodf(s * 73.0f - rawScroll * starSpeed, VIRTUAL_WIDTH);
        if (starX < 0) starX += VIRTUAL_WIDTH;
        float starY = fmodf(s * 151.0f, VIRTUAL_HEIGHT);
        
        float size = 1.0f + (s % 3);
        Color color = (s % 5 == 0) ? SKYBLUE : (s % 7 == 0 ? GOLD : WHITE);
        float alpha = 0.2f + 0.8f * sinf(gs->frameCount * 0.08f + s);
        DrawCircle(starX, starY, size, ColorAlpha(color, alpha));
    }
    
    // Hạt bụi tinh vân phát sáng lướt qua (Cosmic Stardust)
    for (int s = 0; s < 15; s++) {
        float sx = fmodf(s * 87.0f - rawScroll * 0.4f, VIRTUAL_WIDTH + 50.0f) - 25.0f;
        if (sx < -50.0f) sx += VIRTUAL_WIDTH + 50.0f;
        float sy = fmodf(s * 93.0f + sinf(gs->frameCount * 0.02f + s) * 50.0f, VIRTUAL_HEIGHT);
        float pulse = 0.8f + 0.5f * sinf(gs->frameCount * 0.05f + s);
        Color dustCol = (s % 2 == 0) ? MAGENTA : SKYBLUE;
        
        DrawCircle(sx, sy, 3.0f * pulse, ColorAlpha(dustCol, 0.2f * pulse));
        DrawCircle(sx, sy, 1.0f * pulse, ColorAlpha(WHITE, 0.6f));
    }
    
    // 3. Lớp 1: Thiên thạch rất xa nhỏ (Far Asteroids - Cuộn x0.25)
    float astSpeed = 0.25f;
    float astSpacing = 350.0f;
    for (int i = 0; i < 5; i++) {
        float x = fmodf(i * astSpacing - rawScroll * astSpeed, VIRTUAL_WIDTH + astSpacing);
        if (x < -astSpacing) x += (VIRTUAL_WIDTH + astSpacing);
        float y = 100.0f + sinf(i) * 150.0f;
        
        DrawCircle(x, y, 10.0f, ColorAlpha(DARKGRAY, 0.4f));
        DrawCircleLines(x, y, 10.0f, ColorAlpha(GRAY, 0.2f));
    }
    
    // 4. Lớp 2: Thiên thạch vừa trung cảnh (Mid Asteroids - Cuộn x0.5)
    float astMidSpeed = 0.5f;
    float astMidSpacing = 280.0f;
    for (int i = 0; i < 6; i++) {
        float x = fmodf(i * astMidSpacing - rawScroll * astMidSpeed, VIRTUAL_WIDTH + astMidSpacing);
        if (x < -astMidSpacing) x += (VIRTUAL_WIDTH + astMidSpacing);
        float y = 200.0f + cosf(i * 2.0f) * 180.0f;
        
        DrawCircle(x, y, 22.0f, ColorAlpha((Color){ 45, 40, 50, 255 }, 0.7f));
        DrawCircleLines(x, y, 22.0f, ColorAlpha(GRAY, 0.4f));
    }
}

// --- FOREST FOREGROUND ---
void DrawProceduralForegroundForest(struct GameState *gs, float rawScroll) {
    float fgSpeed = 1.6f;
    float fgSpacing = 400.0f;
    
    for (int i = 0; i < 5; i++) {
        float x = fmodf(i * fgSpacing - rawScroll * fgSpeed, VIRTUAL_WIDTH + fgSpacing);
        if (x < -fgSpacing) x += (VIRTUAL_WIDTH + fgSpacing);
        
        // Lá to che góc trên
        DrawCircle(x, -20.0f, 90.0f, ColorAlpha((Color){ 3, 15, 8, 255 }, 0.88f));
        DrawCircle(x + 60.0f, -40.0f, 75.0f, ColorAlpha((Color){ 2, 12, 6, 255 }, 0.9f));
        
        // Dây leo thòng xuống
        DrawLineEx((Vector2){ x + 40, 0 }, (Vector2){ x + 10, 150.0f }, 3.5f, ColorAlpha((Color){ 4, 18, 10, 255 }, 0.9f));
        DrawCircle(x + 10, 150.0f, 6.0f, ColorAlpha(DARKGREEN, 0.9f));
        
        // Bụi cây to dưới đất lướt qua nhanh sát camera
        DrawCircle(x + 200.0f, VIRTUAL_HEIGHT + 20.0f, 100.0f, ColorAlpha((Color){ 3, 15, 8, 255 }, 0.88f));
        DrawCircle(x + 270.0f, VIRTUAL_HEIGHT + 40.0f, 85.0f, ColorAlpha((Color){ 2, 12, 6, 255 }, 0.9f));
    }
}

// --- CAVE FOREGROUND ---
void DrawProceduralForegroundCave(struct GameState *gs, float rawScroll) {
    float fgSpeed = 1.5f;
    float fgSpacing = 450.0f;
    
    for (int i = 0; i < 4; i++) {
        float x = fmodf(i * fgSpacing - rawScroll * fgSpeed, VIRTUAL_WIDTH + fgSpacing);
        if (x < -fgSpacing) x += (VIRTUAL_WIDTH + fgSpacing);
        
        // Nhũ đá to đen kịt rơi từ trần sát camera
        DrawTriangle(
            (Vector2){ x, 0.0f },
            (Vector2){ x + 160.0f, 0.0f },
            (Vector2){ x + 80.0f, 220.0f },
            ColorAlpha((Color){ 12, 7, 10, 255 }, 0.95f)
        );
        
        // Măng đá to đâm từ đất lên sát camera
        DrawTriangle(
            (Vector2){ x + 250.0f, VIRTUAL_HEIGHT - 240.0f },
            (Vector2){ x + 170.0f, VIRTUAL_HEIGHT },
            (Vector2){ x + 330.0f, VIRTUAL_HEIGHT },
            ColorAlpha((Color){ 10, 6, 8, 255 }, 0.95f)
        );
    }
}

// --- CITY FOREGROUND ---
void DrawProceduralForegroundCity(struct GameState *gs, float rawScroll) {
    float fgSpeed = 1.7f;
    float fgSpacing = 500.0f;
    
    for (int i = 0; i < 4; i++) {
        float x = fmodf(i * fgSpacing - rawScroll * fgSpeed, VIRTUAL_WIDTH + fgSpacing);
        if (x < -fgSpacing) x += (VIRTUAL_WIDTH + fgSpacing);
        
        // Dầm dọc to lướt qua
        DrawRectangle(x, 0, 45.0f, VIRTUAL_HEIGHT, ColorAlpha((Color){ 8, 10, 18, 255 }, 0.9f));
        
        // Đường dầm chéo
        DrawLineEx(
            (Vector2){ x - 100.0f, -50.0f },
            (Vector2){ x + 400.0f, VIRTUAL_HEIGHT + 50.0f },
            18.0f,
            ColorAlpha((Color){ 10, 12, 22, 255 }, 0.92f)
        );
    }
}

// --- SPACE FOREGROUND ---
void DrawProceduralForegroundSpace(struct GameState *gs, float rawScroll) {
    float fgSpeed = 1.6f;
    float fgSpacing = 600.0f;
    
    for (int i = 0; i < 3; i++) {
        float x = fmodf(i * fgSpacing - rawScroll * fgSpeed, VIRTUAL_WIDTH + fgSpacing);
        if (x < -fgSpacing) x += (VIRTUAL_WIDTH + fgSpacing);
        float y = VIRTUAL_HEIGHT * 0.7f + cosf(i) * 120.0f;
        
        // Thiên thạch khổng lồ mờ tối lướt qua dưới góc màn hình
        DrawCircle(x, y, 90.0f, ColorAlpha((Color){ 25, 20, 30, 255 }, 0.88f));
        DrawCircleLines(x, y, 90.0f, ColorAlpha(GRAY, 0.3f));
        
        // Thiên thạch nhỏ hơn trôi nhanh ở phía trên
        DrawCircle(x + 200.0f, y - 400.0f, 50.0f, ColorAlpha((Color){ 20, 18, 25, 255 }, 0.85f));
    }
}
