// ============================================================
// ui.c — Triển khai Giao diện Người dùng (HUD, Menu, Shop, GameOver)
// ============================================================

#include "ui.h"
#include "game.h"
#include "save.h"
#include "admob_bridge.h"
#include "biome.h"
#include "raymath.h"
#include "trail.h"
#include <stdio.h>

// Phông chữ vector toàn cục khai báo từ main.c
extern Font gameFont;

// ----------------------------------------------------------------
// HÀM NỘI BỘ (static — tiện ích vẽ nội bộ)
// ----------------------------------------------------------------

#define Min(a, b) ((a) < (b) ? (a) : (b))

// Vẽ chữ vector sắc nét, mịn màng chống răng cưa
static void DrawCrispText(const char *text, float x, float y, float fontSize, Color color) {
    DrawTextEx(gameFont, text, (Vector2){ x, y }, fontSize, 1.0f, color);
}

// Vẽ chữ vector căn giữa trục X màn hình ảo
static void DrawCrispTextCenter(const char *text, float centerY, float fontSize, Color color) {
    Vector2 size = MeasureTextEx(gameFont, text, fontSize, 1.0f);
    float x = (VIRTUAL_WIDTH - size.x) / 2.0f;
    DrawTextEx(gameFont, text, (Vector2){ x, centerY }, fontSize, 1.0f, color);
}

// Lấy tên nâng cấp bằng tiếng Anh hoặc tiếng Việt
static const char* getUpgradeName(UpgradeType type, Language lang) {
    if (lang == LANG_VI) {
        switch (type) {
            case UPGRADE_MAX_LIVES:   return "Tang Them Mang (Max Lives)";
            case UPGRADE_SPEED:       return "Tang Toc Do Bay (Speed Boost)";
            case UPGRADE_LUCK:        return "May Man Nhan Doi (Extra Luck)";
            case UPGRADE_MAGNET_BASE: return "Nam Cham Nen (Base Magnet)";
            default:                  return "Nang Cap";
        }
    } else {
        switch (type) {
            case UPGRADE_MAX_LIVES:   return "Increase Lives (Max Lives)";
            case UPGRADE_SPEED:       return "Flight Speed (Speed Boost)";
            case UPGRADE_LUCK:        return "Fortune (Extra Luck)";
            case UPGRADE_MAGNET_BASE: return "Base Magnet (Base Magnet)";
            default:                  return "Upgrade";
        }
    }
}

// Lấy mô tả chi tiết của nâng cấp theo ngôn ngữ
static const char* getUpgradeDesc(UpgradeType type, Language lang) {
    if (lang == LANG_VI) {
        switch (type) {
            case UPGRADE_MAX_LIVES:   return "+1 mang bat dau moi cap (Toi da 3)";
            case UPGRADE_SPEED:       return "+5% toc do di chuyen moi cap (Toi da 5)";
            case UPGRADE_LUCK:        return "+10% ti le xuat hien sao Vang va Cau Vong (Toi da 3)";
            case UPGRADE_MAGNET_BASE: return "Luon tu dong hut sao trong pham vi nho 80px (Toi da 1)";
            default:                  return "";
        }
    } else {
        switch (type) {
            case UPGRADE_MAX_LIVES:   return "+1 starting life per level (Max 3)";
            case UPGRADE_SPEED:       return "+5% movement speed per level (Max 5)";
            case UPGRADE_LUCK:        return "+10% spawn chance for Gold & Rainbow stars (Max 3)";
            case UPGRADE_MAGNET_BASE: return "Permanently attract stars within an 80px radius (Max 1)";
            default:                  return "";
        }
    }
}

// Lấy giới hạn cấp tối đa của từng loại nâng cấp
static int getUpgradeMaxLevel(UpgradeType type) {
    switch (type) {
        case UPGRADE_MAX_LIVES:   return 3;
        case UPGRADE_SPEED:       return 5;
        case UPGRADE_LUCK:        return 3;
        case UPGRADE_MAGNET_BASE: return 1;
        default:                  return 0;
    }
}

// Lấy giá vàng nâng cấp theo cấp hiện tại
static int getUpgradeCost(UpgradeType type, int currentLevel) {
    int maxLevel = getUpgradeMaxLevel(type);
    if (currentLevel >= maxLevel) return -1; // Đã đạt cấp tối đa
    
    switch (type) {
        case UPGRADE_MAX_LIVES: {
            int costs[3] = { 50, 100, 200 };
            return costs[currentLevel];
        }
        case UPGRADE_SPEED: {
            int costs[5] = { 30, 60, 100, 150, 200 };
            return costs[currentLevel];
        }
        case UPGRADE_LUCK: {
            int costs[3] = { 80, 160, 300 };
            return costs[currentLevel];
        }
        case UPGRADE_MAGNET_BASE: {
            return 120;
        }
        default:
            return 9999;
    }
}

// ----------------------------------------------------------------
// HÀM CÔNG KHAI
// ----------------------------------------------------------------

// Dịch ngôn ngữ động
const char* Tr(const struct GameState *gs, const char *vi, const char *en) {
    if (gs == NULL) return vi;
    return (gs->language == LANG_VI) ? vi : en;
}

// Chỉ thực hiện việc vẽ nút bấm (Không xử lý logic click trong render)
void DrawButton(Rectangle bounds, const char *text, Color color) {
    Vector2 mousePos = ScreenToVirtual(GetMousePosition());
    bool hovered = CheckCollisionPointRec(mousePos, bounds);
    
    Color drawColor = color;
    if (hovered) {
        // Làm sáng màu lên khi hover chuột
        drawColor.r = (unsigned char)Min(drawColor.r + 30, 255);
        drawColor.g = (unsigned char)Min(drawColor.g + 30, 255);
        drawColor.b = (unsigned char)Min(drawColor.b + 30, 255);
    }
    
    // Vẽ nền nút mịn màng
    DrawRectangleRec(bounds, drawColor);
    DrawRectangleLinesEx(bounds, 3.0f, hovered ? WHITE : DARKGRAY);
    
    // Căn giữa dòng chữ vector chất lượng cao
    float fontSize = 20.0f;
    Vector2 textSize = MeasureTextEx(gameFont, text, fontSize, 1.0f);
    float textX = bounds.x + (bounds.width - textSize.x) / 2.0f;
    float textY = bounds.y + (bounds.height - textSize.y) / 2.0f;
    
    DrawTextEx(gameFont, text, (Vector2){ (int)textX, (int)textY }, fontSize, 1.0f, WHITE);
}

// Kiểm tra nút bấm có được click chuột / chạm tay không (Chỉ gọi trong Update)
bool IsButtonClicked(Rectangle bounds) {
    bool inputPressed = false;
    Vector2 rawPos = { 0.0f, 0.0f };
    
    if (GetTouchPointCount() > 0) {
        rawPos = GetTouchPosition(0);
        inputPressed = true;
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        rawPos = GetMousePosition();
        inputPressed = true;
    }
    
    if (inputPressed) {
        Vector2 virtualPos = ScreenToVirtual(rawPos);
        return CheckCollisionPointRec(virtualPos, bounds);
    }
    return false;
}

// Lấy tọa độ tâm nút kỹ năng theo bố cục vòng cung RPG Mobile
Vector2 GetSkillButtonCenter(SkillType skill) {
    Vector2 baseCenter = { 1150.0f, 600.0f };
    switch (skill) {
        case SKILL_SHURIKEN:      return (Vector2){ 1150.0f, 470.0f }; // Kim (Golden Shuriken) - Cung trong, R=130, 90°
        case SKILL_POISON_CLOUD:  return (Vector2){ 977.0f, 500.0f };  // Mộc (Poison Cloud) - Cung ngoài, R=200, 150°
        case SKILL_ICE_BLAST:     return (Vector2){ 1037.0f, 535.0f }; // Thủy (Ice Blast) - Cung trong, R=130, 150°
        case SKILL_FIREBALL:      return (Vector2){ 1020.0f, 600.0f }; // Hỏa (Fireball) - Cung trong, R=130, 180°
        case SKILL_LIGHTNING:     return (Vector2){ 1085.0f, 487.0f }; // Thổ (Lightning) - Cung trong, R=130, 120°
        case SKILL_MAGNET:        return (Vector2){ 950.0f, 600.0f };  // Nam châm - Cung ngoài, R=200, 180°
        case SKILL_SHIELD:        return (Vector2){ 1050.0f, 427.0f }; // Khiên - Cung ngoài, R=200, 120°
        default:                  return baseCenter;
    }
}

// Kiểm tra xem nút bấm dạng hình tròn có được nhấn/chạm hay không (Chỉ gọi trong Update)
bool IsCircleButtonClicked(Vector2 center, float radius) {
    bool inputPressed = false;
    Vector2 rawPos = { 0.0f, 0.0f };
    
    if (GetTouchPointCount() > 0) {
        rawPos = GetTouchPosition(0);
        inputPressed = true;
    } else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        rawPos = GetMousePosition();
        inputPressed = true;
    }
    
    if (inputPressed) {
        Vector2 virtualPos = ScreenToVirtual(rawPos);
        return CheckCollisionPointCircle(virtualPos, center, radius);
    }
    return false;
}

// Vẽ HUD hiển thị thông số màn chơi (Mạng, Điểm số, Vàng, Kỹ năng, Boss HP)
void DrawHUD(const struct GameState *gs) {
    if (gs == NULL) return;
    
    // 1. Vẽ mạng (Tim đỏ) góc trái trên
    DrawCrispText(Tr(gs, "MANG:", "LIVES:"), 20, 22, 20, WHITE);
    for (int i = 0; i < gs->witch.lives; i++) {
        float hx = 100.0f + i * 30.0f;
        float hy = 32.0f;
        DrawCircle(hx - 6, hy - 6, 7, RED);
        DrawCircle(hx + 6, hy - 6, 7, RED);
        DrawTriangle(
            (Vector2){ hx - 13.5f, hy - 4.5f },
            (Vector2){ hx + 13.5f, hy - 4.5f },
            (Vector2){ hx, hy + 11.0f },
            RED
        );
    }
    
    // 2. Vẽ thông tin Score và Gold góc phải trên
    char scoreStr[32];
    sprintf(scoreStr, "%s: %05d", Tr(gs, "DIEM", "SCORE"), gs->witch.score);
    Vector2 scoreSize = MeasureTextEx(gameFont, scoreStr, 22, 1.0f);
    DrawCrispText(scoreStr, VIRTUAL_WIDTH - scoreSize.x - 180, 22, 22, WHITE);
    
    char goldStr[32];
    sprintf(goldStr, "%s: %d", Tr(gs, "VANG", "GOLD"), gs->witch.gold);
    DrawCircle(VIRTUAL_WIDTH - 130, 32, 10, GOLD);
    DrawCircleLines(VIRTUAL_WIDTH - 130, 32, 11, YELLOW);
    DrawCrispText(goldStr, VIRTUAL_WIDTH - 110, 22, 22, YELLOW);
    
    // 3. Vẽ các nút kỹ năng chủ động sắp xếp dạng RPG Mobile (vòng cung)
    float btnRadius = 28.0f;
    
    // 3.1 Vẽ Nút Đánh thường tròn lớn ở góc dưới phải (Spacebar)
    Vector2 attackCenter = { 1150.0f, 600.0f };
    float attackRadius = 48.0f;
    DrawCircleV(attackCenter, attackRadius, ColorAlpha(DARKBLUE, 0.45f));
    DrawCircleLines(attackCenter.x, attackCenter.y, attackRadius, WHITE);
    DrawCircleLines(attackCenter.x, attackCenter.y, attackRadius - 1.0f, ColorAlpha(WHITE, 0.4f));
    
    // Vẽ icon cây sáo phép / bùa chú đánh thường
    DrawLineEx((Vector2){attackCenter.x - 12, attackCenter.y + 12}, (Vector2){attackCenter.x + 10, attackCenter.y - 10}, 3.5f, BROWN);
    DrawCircle(attackCenter.x + 12, attackCenter.y - 12, 4.5f, YELLOW);
    DrawCircle(attackCenter.x + 6, attackCenter.y - 18, 1.8f, GOLD);
    DrawCircle(attackCenter.x + 18, attackCenter.y - 6, 1.8f, GOLD);
    
    // Cooldown Đánh thường
    if (gs->witch.keyboardAttackCooldown > 0.0f) {
        float pct = gs->witch.keyboardAttackCooldown / 0.35f;
        if (pct > 1.0f) pct = 1.0f;
        DrawCircleSector(attackCenter, attackRadius, -90.0f, -90.0f + 360.0f * pct, 24, ColorAlpha(BLACK, 0.65f));
    }
    // Phím tắt đánh thường
    Vector2 spaceS = MeasureTextEx(gameFont, "SPACE", 10, 1.0f);
    DrawCrispText("SPACE", attackCenter.x - spaceS.x / 2.0f, attackCenter.y + attackRadius - 13.0f, 10, LIGHTGRAY);

    // 3.2 Vẽ 7 nút kỹ năng chủ động (Kim, Mộc, Thủy, Hỏa, Thổ, Nam châm, Khiên)
    for (int s = 1; s < SKILL_COUNT; s++) {
        if (!gs->witch.activeSkills[s]) continue;
        
        Vector2 center = GetSkillButtonCenter(s);
        
        // Xác định màu sắc theo chủ đề của kỹ năng
        Color ringColor = SKYBLUE;
        const char* keyStr = "";
        
        if (s == SKILL_SHURIKEN) { ringColor = GOLD; keyStr = "1"; }
        else if (s == SKILL_POISON_CLOUD) { ringColor = LIME; keyStr = "2"; }
        else if (s == SKILL_ICE_BLAST) { ringColor = SKYBLUE; keyStr = "3"; }
        else if (s == SKILL_FIREBALL) { ringColor = ORANGE; keyStr = "4"; }
        else if (s == SKILL_LIGHTNING) { ringColor = (Color){ 240, 180, 50, 255 }; keyStr = "5"; }
        else if (s == SKILL_MAGNET) { ringColor = GOLD; keyStr = "E"; }
        else if (s == SKILL_SHIELD) { ringColor = CYAN; keyStr = "Q"; }
        
        // Vẽ vòng tròn nền và viền
        DrawCircleV(center, btnRadius, ColorAlpha(DARKBLUE, 0.4f));
        DrawCircleLines(center.x, center.y, btnRadius, ringColor);
        DrawCircleLines(center.x, center.y, btnRadius - 1.0f, ColorAlpha(ringColor, 0.5f));
        
        // Vẽ icon biểu tượng kỹ năng
        if (s == SKILL_SHURIKEN) {
            DrawLineEx((Vector2){center.x - 7, center.y - 7}, (Vector2){center.x + 7, center.y + 7}, 2.5f, GOLD);
            DrawLineEx((Vector2){center.x - 7, center.y + 7}, (Vector2){center.x + 7, center.y - 7}, 2.5f, GOLD);
            DrawCircle(center.x, center.y, 3.0f, WHITE);
        } 
        else if (s == SKILL_POISON_CLOUD) {
            DrawCircle(center.x - 4, center.y + 3, 7.0f, ColorAlpha(LIME, 0.5f));
            DrawCircle(center.x + 4, center.y + 3, 7.0f, ColorAlpha(LIME, 0.5f));
            DrawCircle(center.x, center.y - 4, 8.5f, ColorAlpha(GREEN, 0.6f));
        } 
        else if (s == SKILL_ICE_BLAST) {
            DrawLineEx((Vector2){center.x, center.y - 10}, (Vector2){center.x, center.y + 10}, 2.0f, SKYBLUE);
            DrawLineEx((Vector2){center.x - 8, center.y}, (Vector2){center.x + 8, center.y}, 2.0f, SKYBLUE);
            DrawCircle(center.x, center.y, 3.5f, WHITE);
        } 
        else if (s == SKILL_FIREBALL) {
            DrawCircle(center.x, center.y, 9, ORANGE);
            DrawCircle(center.x, center.y, 5, YELLOW);
        } 
        else if (s == SKILL_LIGHTNING) {
            Vector2 p1 = { center.x + 4.0f, center.y - 12.0f };
            Vector2 p2 = { center.x - 6.0f, center.y + 2.0f };
            Vector2 p3 = { center.x + 4.0f, center.y - 1.0f };
            Vector2 p4 = { center.x - 4.0f, center.y + 13.0f };
            
            DrawLineEx(p1, p2, 2.5f, ringColor);
            DrawLineEx(p2, p3, 2.5f, ringColor);
            DrawLineEx(p3, p4, 2.5f, ringColor);
            
            DrawLineEx(p1, p2, 1.0f, WHITE);
            DrawLineEx(p2, p3, 1.0f, WHITE);
            DrawLineEx(p3, p4, 1.0f, WHITE);
        }
        else if (s == SKILL_MAGNET) {
            // Vẽ biểu tượng nam châm hình chữ U
            DrawLineEx((Vector2){center.x - 6, center.y - 8}, (Vector2){center.x - 6, center.y + 2}, 3.0f, RED);
            DrawLineEx((Vector2){center.x + 6, center.y - 8}, (Vector2){center.x + 6, center.y + 2}, 3.0f, BLUE);
            DrawLineEx((Vector2){center.x - 7.5f, center.y + 2}, (Vector2){center.x + 7.5f, center.y + 2}, 3.0f, LIGHTGRAY);
        }
        else if (s == SKILL_SHIELD) {
            // Vẽ biểu tượng khiên
            DrawLineEx((Vector2){center.x - 9, center.y - 8}, (Vector2){center.x + 9, center.y - 8}, 2.0f, CYAN);
            DrawLineEx((Vector2){center.x - 9, center.y - 8}, (Vector2){center.x - 9, center.y}, 2.0f, CYAN);
            DrawLineEx((Vector2){center.x + 9, center.y - 8}, (Vector2){center.x + 9, center.y}, 2.0f, CYAN);
            DrawTriangle((Vector2){center.x - 9.5f, center.y}, (Vector2){center.x + 9.5f, center.y}, (Vector2){center.x, center.y + 10.0f}, CYAN);
        }
        
        // Vẽ viền phát sáng màu vàng kim nhấp nháy khi kỹ năng đang kích hoạt (active timer)
        if (gs->witch.skillActiveTimer[s] > 0.0f) {
            float pulse = 2.0f + sinf(gs->frameCount * 0.15f) * 2.0f;
            DrawCircleLines(center.x, center.y, btnRadius + pulse, GOLD);
        }
        
        // Vẽ overlay hồi chiêu
        if (gs->witch.skillCooldown[s] > 0.0f) {
            float maxCD = 1.0f;
            if (s == SKILL_SHURIKEN) maxCD = 2.0f;
            else if (s == SKILL_POISON_CLOUD) maxCD = 3.5f;
            else if (s == SKILL_ICE_BLAST) maxCD = 4.5f;
            else if (s == SKILL_FIREBALL) maxCD = 1.0f;
            else if (s == SKILL_LIGHTNING) maxCD = 6.0f;
            else if (s == SKILL_MAGNET) maxCD = 12.0f;
            else if (s == SKILL_SHIELD) maxCD = 15.0f;
            
            float pct = gs->witch.skillCooldown[s] / maxCD;
            if (pct > 1.0f) pct = 1.0f;
            
            float startAngle = -90.0f;
            float sweepAngle = 360.0f * pct;
            DrawCircleSector(center, btnRadius, startAngle, startAngle + sweepAngle, 24, ColorAlpha(BLACK, 0.65f));
            
            char cdStr[8];
            snprintf(cdStr, sizeof(cdStr), "%.1f", gs->witch.skillCooldown[s]);
            Vector2 textS = MeasureTextEx(gameFont, cdStr, 13, 1.0f);
            DrawCrispText(cdStr, center.x - textS.x / 2.0f, center.y - textS.y / 2.0f, 13, WHITE);
        }
        
        // Vẽ nhãn phím tắt tương tương ứng ở dưới nút
        Vector2 keyS = MeasureTextEx(gameFont, keyStr, 10, 1.0f);
        DrawCrispText(keyStr, center.x - keyS.x / 2.0f, center.y + btnRadius - 10.0f, 10, LIGHTGRAY);
    }
    
    // 3.3 Vẽ nút Ultimate hệ ngũ sắc khi sẵn sàng kích hoạt ở vị trí trên cùng vòng cung
    int rainbowCount = 0;
    int kim = 0, moc = 0, thuy = 0, hoa = 0, tho = 0;
    for (int i = 0; i < MAX_STARS; i++) {
        if (gs->starPool.stars[i].active && gs->starPool.stars[i].isOrbiting) {
            if (gs->starPool.stars[i].type == STAR_RAINBOW) rainbowCount++;
            else if (gs->starPool.stars[i].type == STAR_KIM) kim++;
            else if (gs->starPool.stars[i].type == STAR_MOC) moc++;
            else if (gs->starPool.stars[i].type == STAR_THUY) thuy++;
            else if (gs->starPool.stars[i].type == STAR_HOA) hoa++;
            else if (gs->starPool.stars[i].type == STAR_THO) tho++;
        }
    }
    bool isUltReady = (rainbowCount > 0) || (kim > 0 && moc > 0 && thuy > 0 && hoa > 0 && tho > 0);
    
    Vector2 ultCenter = { 1150.0f, 360.0f };
    float ultRadius = 36.0f;
    
    if (isUltReady) {
        float pulse = 2.0f + sinf(gs->frameCount * 0.20f) * 3.0f;
        float t = GetTime() * 8.0f;
        Color ultColor = ColorFromHSV(fmodf(t * 45.0f, 360.0f), 0.85f, 1.0f);
        
        DrawCircleV(ultCenter, ultRadius, ColorAlpha(DARKBLUE, 0.4f));
        DrawCircleLines(ultCenter.x, ultCenter.y, ultRadius, ultColor);
        DrawCircleLines(ultCenter.x, ultCenter.y, ultRadius - 1.0f, ColorAlpha(ultColor, 0.5f));
        DrawCircleLines(ultCenter.x, ultCenter.y, ultRadius + pulse, ColorAlpha(ultColor, 0.6f));
        
        Vector2 ultS = MeasureTextEx(gameFont, "ULTI", 14, 1.0f);
        DrawCrispText("ULTI", ultCenter.x - ultS.x / 2.0f, ultCenter.y - ultS.y / 2.0f - 2.0f, 14, WHITE);
        
        Vector2 fS = MeasureTextEx(gameFont, "[F]", 11, 1.0f);
        DrawCrispText("[F]", ultCenter.x - fS.x / 2.0f, ultCenter.y + ultRadius - 12.0f, 11, ultColor);
    } else {
        DrawCircleV(ultCenter, ultRadius, ColorAlpha(GRAY, 0.15f));
        DrawCircleLines(ultCenter.x, ultCenter.y, ultRadius, ColorAlpha(GRAY, 0.35f));
        
        Vector2 ultS = MeasureTextEx(gameFont, "ULTI", 13, 1.0f);
        DrawCrispText("ULTI", ultCenter.x - ultS.x / 2.0f, ultCenter.y - ultS.y / 2.0f, 13, ColorAlpha(GRAY, 0.6f));
    }
    
    // 4. Vẽ thanh máu của Trùm (Boss HP Bar) nếu Trùm đang hoạt động
    if (gs->enemyPool.bossActive && gs->enemyPool.bossIndex != -1) {
        const Enemy *boss = &gs->enemyPool.enemies[gs->enemyPool.bossIndex];
        if (boss->active) {
            float barW = 600.0f;
            float barH = 20.0f;
            float barX = (VIRTUAL_WIDTH - barW) / 2.0f;
            float barY = 70.0f;
            
            float hpPct = (float)boss->hp / boss->maxHp;
            if (hpPct < 0.0f) hpPct = 0.0f;
            
            const char* bossName = Tr(gs, "BOSS: QUAI THU RUNG XANH", "BOSS: FOREST BEAST");
            if (boss->type == ENEMY_BOSS_CAVE)  bossName = Tr(gs, "BOSS: QUAI CO HANG DONG", "BOSS: CAVE COLOSSUS");
            if (boss->type == ENEMY_BOSS_CITY)  bossName = Tr(gs, "BOSS: PHAO DAI DO THI", "BOSS: CYBER FORTRESS");
            if (boss->type == ENEMY_BOSS_SPACE) bossName = Tr(gs, "BOSS: HO DEN TINH HA", "BOSS: GALACTIC VOID");
            
            Vector2 textS = MeasureTextEx(gameFont, bossName, 18, 1.0f);
            DrawCrispText(bossName, (VIRTUAL_WIDTH - textS.x) / 2, barY - 22.0f, 18, RED);
            
            DrawRectangle(barX, barY, barW, barH, BLACK);
            DrawRectangle(barX + 2, barY + 2, (barW - 4.0f) * hpPct, barH - 4.0f, RED);
            DrawRectangleLines(barX, barY, barW, barH, GRAY);
        }
    }
    
    // 5. Cảnh báo Trùm rừng chuẩn bị xuất hiện (phát sáng nhấp nháy đỏ)
    if (gs->currentBiome == BIOME_FOREST && !gs->enemyPool.bossActive && !gs->bossSpawned) {
        int relativeScore = gs->witch.score - gs->biomeStartScore;
        if (relativeScore >= 500) {
            if ((gs->frameCount / 15) % 2 == 0) {
                const char *warnText = Tr(gs, "!!! CANH BAO: THU HU KINH PHONG !!!", "!!! WARNING: BEAST APPROACHING !!!");
                Vector2 warnSize = MeasureTextEx(gameFont, warnText, 24, 1.0f);
                DrawCrispText(warnText, (VIRTUAL_WIDTH - warnSize.x) / 2.0f, 150, 24, RED);
            }
        }
    }
}

// Vẽ Giao diện Menu chính
void DrawMenu(const struct GameState *gs) {
    if (gs == NULL) return;
    
    // Tiêu đề game vector sắc nét
    DrawCrispTextCenter(Tr(gs, "PHAP SU AO DAI", "VIET MAGE ADVENTURE"), 130, 48, VIOLET);
    DrawCrispTextCenter(Tr(gs, "PHAP SU AO DAI", "VIET MAGE ADVENTURE"), 128, 48, WHITE);
    
    DrawCrispTextCenter(
        Tr(gs, "Hay giup Nu Phap Su thu thap sao va danh bai quai vat!", "Help the Viet Mage collect stars and defeat monsters!"),
        210, 20, LIGHTGRAY
    );
    
    // Điểm cao nhất từ cache (không đọc file mỗi frame)
    char highStr[64];
    sprintf(highStr, "%s: %05d", Tr(gs, "DIEM CAO NHAT (HIGH SCORE)", "HIGH SCORE"), gs->cachedSave.highScore);
    DrawCrispTextCenter(highStr, 270, 24, GOLD);
    
    // Vẽ các nút bấm menu
    DrawButton((Rectangle){ VIRTUAL_WIDTH / 2.0f - 130.0f, 350.0f, 260.0f, 50.0f }, Tr(gs, "Bat dau Choi", "Play Game"), GREEN);
    DrawButton((Rectangle){ VIRTUAL_WIDTH / 2.0f - 130.0f, 420.0f, 260.0f, 50.0f }, Tr(gs, "Cua Hang Nang Cap", "Upgrade Shop"), BLUE);
    DrawButton((Rectangle){ VIRTUAL_WIDTH / 2.0f - 130.0f, 490.0f, 260.0f, 50.0f }, Tr(gs, "Xoa Save Progress", "Reset Progress"), DARKGRAY);
    
    // Nút chuyển đổi Ngôn ngữ EN/VI
    Rectangle langRect = { VIRTUAL_WIDTH - 220.0f, 20.0f, 200.0f, 40.0f };
    DrawButton(langRect, Tr(gs, "EN / VI (Tieng Viet)", "EN / VI (English)"), ORANGE);
}

// Cập nhật logic các nút bấm menu chính
void UpdateMenu(struct GameState *gs, float deltaTime) {
    if (gs == NULL) return;
    
    Rectangle playRect = { VIRTUAL_WIDTH / 2.0f - 130.0f, 350.0f, 260.0f, 50.0f };
    if (IsButtonClicked(playRect)) {
        InitWitch(&gs->witch, gs->savedUpgrades);
        InitWitchTrail(&gs->trail);
        InitStarPool(&gs->starPool);
        InitEnemyPool(&gs->enemyPool);
        InitProjectilePool(&gs->projectilePool);
        InitParticlePool(&gs->particlePool);
        gs->bgScrollOffset = 0.0f;
        gs->bossSpawned = false;
        gs->biomeStartScore = 0; // Biome đầu tiên bắt đầu từ 0 điểm
        gs->currentScreen = SCREEN_GAMEPLAY;
        gs->currentBiome = BIOME_FOREST;
    }
    
    Rectangle shopRect = { VIRTUAL_WIDTH / 2.0f - 130.0f, 420.0f, 260.0f, 50.0f };
    if (IsButtonClicked(shopRect)) {
        gs->currentScreen = SCREEN_UPGRADE_SHOP;
    }
    
    Rectangle resetRect = { VIRTUAL_WIDTH / 2.0f - 130.0f, 490.0f, 260.0f, 50.0f };
    if (IsButtonClicked(resetRect)) {
        ResetSaveData();
        SaveData empty = LoadSaveData();
        gs->cachedSave = empty; // Cập nhật cache sau khi reset
        gs->savedUpgrades = empty.upgrades;
        gs->language = empty.language;
    }
    
    // Xử lý click nút đổi Ngôn ngữ
    Rectangle langRect = { VIRTUAL_WIDTH - 220.0f, 20.0f, 200.0f, 40.0f };
    if (IsButtonClicked(langRect)) {
        gs->language = (gs->language == LANG_VI) ? LANG_EN : LANG_VI;
        // Ghi lại cấu hình ngôn ngữ vào file save và cập nhật cache
        gs->cachedSave.language = gs->language;
        WriteSaveData(&gs->cachedSave);
    }
}

// Vẽ màn hình chọn Kỹ năng sau diệt Boss
void DrawSkillSelect(const struct GameState *gs) {
    if (gs == NULL) return;
    
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, ColorAlpha(BLACK, 0.75f));
    
    const char* title = Tr(gs, "CHON 1 THE KY NANG DE TIEP TUC", "CHOOSE 1 SKILL TO CONTINUE");
    DrawCrispTextCenter(title, 80, 36, SKYBLUE);
    
    float cardW = 260.0f;
    float cardH = 370.0f;
    float startX = (VIRTUAL_WIDTH - (cardW * 3 + 60.0f)) / 2.0f;
    float cardY = 180.0f;
    
    Vector2 mousePos = ScreenToVirtual(GetMousePosition());
    
    for (int i = 0; i < 3; i++) {
        Rectangle cardRect = { startX + i * (cardW + 30.0f), cardY, cardW, cardH };
        bool hovered = CheckCollisionPointRec(mousePos, cardRect);
        
        DrawRectangleRec(cardRect, hovered ? (Color){ 30, 45, 80, 255 } : (Color){ 20, 25, 45, 255 });
        DrawRectangleLinesEx(cardRect, 4.0f, hovered ? SKYBLUE : GRAY);
        
        SkillType skill = gs->skillChoices[i];
        
        const char *name = "";
        const char *desc = "";
        char lvlDesc[64] = "";
        Color iconColor = GRAY;
        
        int currentLvl = gs->witch.skillLevels[skill];
        int nextLvl = currentLvl + 1;
        
        switch (skill) {
            case SKILL_SHURIKEN:
                name = Tr(gs, "PHI TIEU VANG", "GOLD SHURIKEN");
                sprintf(lvlDesc, "%s %d -> %d: Dmg +15%%, +1 Proj", Tr(gs, "Nang cap Lv.", "Upgrade Lv."), currentLvl, nextLvl);
                desc = Tr(gs, "Phong phi tieu vang xoay tron luyen la bay xuyen qua moi ke dich.", 
                          "Launch spinning gold shurikens piercing all enemies.");
                iconColor = GOLD;
                break;
            case SKILL_FIREBALL:
                name = Tr(gs, "HOA CAU NO", "EXPLOSIVE FIREBALL");
                sprintf(lvlDesc, "%s %d -> %d: Dmg +20%%, +10%% AOE", Tr(gs, "Nang cap Lv.", "Upgrade Lv."), currentLvl, nextLvl);
                desc = Tr(gs, "Ban cau lua lon, khi cham quai se phat no gay sat thuong lan.", 
                          "Fires fireballs that explode and deal AOE damage.");
                iconColor = ORANGE;
                break;
            case SKILL_POISON_CLOUD:
                name = Tr(gs, "HOI DOC XANH", "POISON CLOUD");
                sprintf(lvlDesc, "%s %d -> %d: Dmg +25%%, +0.5s Time", Tr(gs, "Nang cap Lv.", "Upgrade Lv."), currentLvl, nextLvl);
                desc = Tr(gs, "Tao dam may doc rut mau moi ke dich dung trong vung anh huong.", 
                          "Spawn a toxic mist dealing damage over time.");
                iconColor = LIME;
                break;
            case SKILL_ICE_BLAST:
                name = Tr(gs, "CHUONG BANG GIA", "ICE BLAST");
                sprintf(lvlDesc, "%s %d -> %d: Dmg +20%%, +0.5s Freeze", Tr(gs, "Nang cap Lv.", "Upgrade Lv."), currentLvl, nextLvl);
                desc = Tr(gs, "Phong chong luc bang gay sat thuong va dong bang ke dich.", 
                          "Shoot ice blast that damages and freezes enemies.");
                iconColor = SKYBLUE;
                break;
            case SKILL_LIGHTNING:
                name = Tr(gs, "SAM SET & LOC XOAY", "LIGHTNING & TORNADO");
                sprintf(lvlDesc, "%s %d -> %d: Dmg +20%%, +50px Range", Tr(gs, "Nang cap Lv.", "Upgrade Lv."), currentLvl, nextLvl);
                desc = Tr(gs, "Set xich giat lien hoan kem loc xoay dien cuon hut va gay sat thuong.", 
                          "Chain lightning combined with tornado that pulls enemies.");
                iconColor = (Color){ 240, 180, 50, 255 };
                break;
            case SKILL_MAGNET:
                name = Tr(gs, "NAM CHAM KICH HOAT", "ACTIVE MAGNET");
                sprintf(lvlDesc, "%s %d -> %d: +1.5s Time, -0.8s CD", Tr(gs, "Nang cap Lv.", "Upgrade Lv."), currentLvl, nextLvl);
                desc = Tr(gs, "Treo nam cham hut sao pham vi toan man hinh va tang toc do hut.",
                          "Attracts all stars on screen and increases attract speed.");
                iconColor = PINK;
                break;
            case SKILL_SHIELD:
                name = Tr(gs, "KHIEN CHU DONG", "ACTIVE SHIELD");
                sprintf(lvlDesc, "%s %d -> %d: +1.0s Shield, -1.0s CD", Tr(gs, "Nang cap Lv.", "Upgrade Lv."), currentLvl, nextLvl);
                desc = Tr(gs, "Kich hoat khien bat tu chan hoan toan sat thuong trong thoi gian ngan.",
                          "Activate an invincible shield that blocks all incoming damage.");
                iconColor = CYAN;
                break;
            default:
                break;
        }
        
        // Vẽ tên kỹ năng lớn
        float nameSize = 22.0f;
        Vector2 nameS = MeasureTextEx(gameFont, name, nameSize, 1.0f);
        DrawCrispText(name, cardRect.x + (cardW - nameS.x) / 2.0f, cardRect.y + 25, nameSize, hovered ? WHITE : LIGHTGRAY);
        
        // Vẽ biểu tượng giữa thẻ bài
        float iconX = cardRect.x + cardW / 2.0f;
        float iconY = cardRect.y + 130.0f;
        DrawCircle(iconX, iconY, 35.0f, ColorAlpha(iconColor, 0.2f));
        DrawCircleLines(iconX, iconY, 36.0f, iconColor);
        
        if (skill == SKILL_SHURIKEN) {
            DrawLineEx((Vector2){iconX - 10, iconY - 10}, (Vector2){iconX + 10, iconY + 10}, 3.0f, GOLD);
            DrawLineEx((Vector2){iconX - 10, iconY + 10}, (Vector2){iconX + 10, iconY - 10}, 3.0f, GOLD);
            DrawCircle(iconX, iconY, 4.0f, WHITE);
        } else if (skill == SKILL_FIREBALL) {
            DrawCircle(iconX, iconY, 15.0f, ORANGE);
            DrawCircle(iconX, iconY, 8.0f, YELLOW);
        } else if (skill == SKILL_POISON_CLOUD) {
            DrawCircle(iconX - 6, iconY + 4, 10.0f, ColorAlpha(LIME, 0.6f));
            DrawCircle(iconX + 6, iconY + 4, 10.0f, ColorAlpha(LIME, 0.6f));
            DrawCircle(iconX, iconY - 6, 12.0f, ColorAlpha(GREEN, 0.7f));
        } else if (skill == SKILL_ICE_BLAST) {
            DrawLineEx((Vector2){iconX, iconY - 16}, (Vector2){iconX, iconY + 16}, 2.5f, SKYBLUE);
            DrawLineEx((Vector2){iconX - 12, iconY}, (Vector2){iconX + 12, iconY}, 2.5f, SKYBLUE);
            DrawCircle(iconX, iconY, 5.0f, WHITE);
        } else if (skill == SKILL_LIGHTNING) {
            Vector2 p1 = { iconX + 4.0f, iconY - 12.0f };
            Vector2 p2 = { iconX - 6.0f, iconY + 2.0f };
            Vector2 p3 = { iconX + 4.0f, iconY - 1.0f };
            Vector2 p4 = { iconX - 4.0f, iconY + 13.0f };
            DrawLineEx(p1, p2, 2.5f, iconColor);
            DrawLineEx(p2, p3, 2.5f, iconColor);
            DrawLineEx(p3, p4, 2.5f, iconColor);
        }
        
        // Tự động phân tách dòng mô tả ngắn thẻ
        char descLine[64];
        int startPos = 0;
        int lineIdx = 0;
        
        for (int c = 0; desc[c] != '\0'; c++) {
            if (c - startPos >= 22 && desc[c] == ' ') {
                int len = c - startPos;
                snprintf(descLine, len + 1, "%s", desc + startPos);
                Vector2 lineS = MeasureTextEx(gameFont, descLine, 14, 1.0f);
                DrawCrispText(descLine, cardRect.x + (cardW - lineS.x) / 2.0f, cardRect.y + 205 + lineIdx * 20, 14, LIGHTGRAY);
                startPos = c + 1;
                lineIdx++;
            }
        }
        snprintf(descLine, sizeof(descLine), "%s", desc + startPos);
        Vector2 lineS = MeasureTextEx(gameFont, descLine, 14, 1.0f);
        DrawCrispText(descLine, cardRect.x + (cardW - lineS.x) / 2.0f, cardRect.y + 205 + lineIdx * 20, 14, LIGHTGRAY);
        
        // Vẽ thông tin nâng cấp cấp độ ở cuối thẻ bài
        Vector2 lvlS = MeasureTextEx(gameFont, lvlDesc, 13, 1.0f);
        DrawCrispText(lvlDesc, cardRect.x + (cardW - lvlS.x) / 2.0f, cardRect.y + 315, 13, hovered ? YELLOW : GOLD);
    }
}

// Xử lý logic chọn thẻ kỹ năng trong Update
void UpdateSkillSelect(struct GameState *gs) {
    if (gs == NULL) return;
    
    float cardW = 260.0f;
    float cardH = 370.0f;
    float startX = (VIRTUAL_WIDTH - (cardW * 3 + 60.0f)) / 2.0f;
    float cardY = 180.0f;
    
    for (int i = 0; i < 3; i++) {
        Rectangle cardRect = { startX + i * (cardW + 30.0f), cardY, cardW, cardH };
        
        if (IsButtonClicked(cardRect)) {
            SkillType skillSelected = gs->skillChoices[i];
            WitchGrantSkill(&gs->witch, skillSelected);
            
            // Bỏ logic bão sao cũ vì đã chuyển sang cơ chế sao Mộc bị động
            
            InitEnemyPool(&gs->enemyPool);
            InitProjectilePool(&gs->projectilePool);
            InitParticlePool(&gs->particlePool);
            InitWitchTrail(&gs->trail);
            
            gs->currentBiome = NextBiome(gs->currentBiome);
            gs->bossSpawned = false; // Reset cờ sinh boss cho biome mới
            gs->biomeStartScore = gs->witch.score; // Ghi nhớ điểm khi bắt đầu biome mới
            
            // Cập nhật high score vào cache và ghi file
            if (gs->witch.score > gs->cachedSave.highScore) {
                gs->cachedSave.highScore = gs->witch.score;
            }
            WriteSaveData(&gs->cachedSave);
            
            gs->currentScreen = SCREEN_GAMEPLAY;
            break;
        }
    }
}

// Vẽ Giao diện GameOver
void DrawGameOver(const struct GameState *gs) {
    if (gs == NULL) return;
    
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, ColorAlpha(MAROON, 0.45f));
    DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, ColorAlpha(BLACK, 0.6f));
    
    DrawCrispTextCenter("GAME OVER", 130, 64, RED);
    DrawCrispTextCenter("GAME OVER", 127, 64, WHITE);
    
    char scoreStr[64];
    sprintf(scoreStr, "%s: %d", Tr(gs, "Diem dat duoc", "Final Score"), gs->witch.score);
    DrawCrispTextCenter(scoreStr, 225, 26, WHITE);
    
    char goldStr[64];
    sprintf(goldStr, "%s: %d", Tr(gs, "Vang thu thap", "Gold Collected"), gs->witch.gold);
    Vector2 goldS = MeasureTextEx(gameFont, goldStr, 24, 1.0f);
    float goldX = (VIRTUAL_WIDTH - goldS.x) / 2.0f;
    DrawCircle(goldX - 20, 292, 9, GOLD);
    DrawCrispText(goldStr, goldX, 280, 24, YELLOW);
    
    // Nút "Hồi Sinh" (Được vẽ ở Draw)
    Rectangle reviveRect = { VIRTUAL_WIDTH / 2.0f - 200.0f, 380.0f, 400.0f, 55.0f };
    if (!gs->witch.hasRevived) {
        const char *btnText = Tr(gs, "Hoi Sinh Ngay (Xem Ad)", "Revive Now (Watch Ad)");
        Color btnColor = GREEN;
        
        AdState adState = AdMob_GetState();
        if (adState == AD_STATE_LOADING) {
            btnText = Tr(gs, "Dang tai Ad... (Cho chut)", "Loading Ad... (Please wait)");
            btnColor = DARKGRAY;
        } else if (adState == AD_STATE_SHOWING) {
            btnText = Tr(gs, "Dang trinh chieu...", "Ad showing...");
            btnColor = DARKGRAY;
        }
        DrawButton(reviveRect, btnText, btnColor);
    } else {
        // Trạng thái đã hồi sinh
        DrawRectangleRec(reviveRect, ColorAlpha(GRAY, 0.4f));
        DrawRectangleLinesEx(reviveRect, 2.0f, DARKGRAY);
        const char *limitText = Tr(gs, "Chi duoc hoi sinh 1 lan", "Only 1 revive per run");
        Vector2 textS = MeasureTextEx(gameFont, limitText, 22, 1.0f);
        DrawCrispText(limitText, reviveRect.x + (reviveRect.width - textS.x)/2.0f, reviveRect.y + 16, 22, LIGHTGRAY);
    }
    
    // Nút "Trở về" (Được vẽ ở Draw)
    Rectangle returnRect = { VIRTUAL_WIDTH / 2.0f - 200.0f, 460.0f, 400.0f, 55.0f };
    DrawButton(returnRect, Tr(gs, "Luu Ket Qua & Cua Hang", "Save & Open Shop"), BLUE);
}

// Xử lý logic GameOver trong Update
void UpdateGameOver(struct GameState *gs) {
    if (gs == NULL) return;
    
    AdState adState = AdMob_GetState();
    
    if (adState == AD_STATE_REWARDED) {
        gs->witch.lives = 1;
        gs->witch.hasRevived = true;
        gs->witch.isInvincible = true;
        gs->witch.invincibleTimer = 3.0f;
        gs->currentScreen = SCREEN_GAMEPLAY;
        AdMob_ResetState();
        AdMob_LoadRewardedAd();
        return;
    }
    else if (adState == AD_STATE_FAILED) {
        AdMob_ResetState();
    }
    
    // Hồi sinh nếu click
    Rectangle reviveRect = { VIRTUAL_WIDTH / 2.0f - 200.0f, 380.0f, 400.0f, 55.0f };
    if (!gs->witch.hasRevived && (adState != AD_STATE_LOADING && adState != AD_STATE_SHOWING)) {
        if (IsButtonClicked(reviveRect)) {
            AdMob_ShowRewardedAd();
        }
    }
    
    // Trở về shop nếu click
    Rectangle returnRect = { VIRTUAL_WIDTH / 2.0f - 200.0f, 460.0f, 400.0f, 55.0f };
    if (IsButtonClicked(returnRect)) {
        gs->cachedSave.totalGoldEarned += gs->witch.gold;
        if (gs->witch.score > gs->cachedSave.highScore) {
            gs->cachedSave.highScore = gs->witch.score;
        }
        WriteSaveData(&gs->cachedSave);
        gs->savedUpgrades = gs->cachedSave.upgrades;
        gs->currentScreen = SCREEN_UPGRADE_SHOP;
    }
}

// Vẽ Giao diện Cửa hàng Shop (Gồm cả nút Trở về Menu)
void DrawUpgradeShop(const struct GameState *gs) {
    if (gs == NULL) return;
    
    // Dùng cache thay vì đọc file mỗi frame
    DrawCrispTextCenter(Tr(gs, "CUA HANG NANG CAP VINH VIEN", "PERMANENT UPGRADE SHOP"), 50, 38, GOLD);
    
    char goldStr[64];
    sprintf(goldStr, "%s: %d", Tr(gs, "TONG VANG SO HUU", "TOTAL GOLD OWNED"), gs->cachedSave.totalGoldEarned);
    Vector2 goldS = MeasureTextEx(gameFont, goldStr, 24, 1.0f);
    float goldX = (VIRTUAL_WIDTH - goldS.x) / 2.0f;
    DrawCircle(goldX - 20, 117, 10, GOLD);
    DrawCrispText(goldStr, goldX, 105, 24, YELLOW);
    
    float startY = 160.0f;
    float rowH = 90.0f;
    
    for (int i = 0; i < UPGRADE_COUNT; i++) {
        float y = startY + i * (rowH + 15.0f);
        
        Rectangle rowBg = { 100.0f, y, VIRTUAL_WIDTH - 200.0f, rowH };
        DrawRectangleRec(rowBg, ColorAlpha(DARKBLUE, 0.15f));
        DrawRectangleLinesEx(rowBg, 2.0f, ColorAlpha(SKYBLUE, 0.4f));
        
        int lv = gs->cachedSave.upgrades.level[i];
        int maxLv = getUpgradeMaxLevel(i);
        
        char nameStr[128];
        sprintf(nameStr, "%s  (Lv %d/%d)", getUpgradeName(i, gs->language), lv, maxLv);
        DrawCrispText(nameStr, 130, y + 16.0f, 22, WHITE);
        DrawCrispText(getUpgradeDesc(i, gs->language), 130, y + 48.0f, 15, LIGHTGRAY);
        
        // Vẽ nút Buy
        Rectangle buyRect = { VIRTUAL_WIDTH - 280.0f, y + 18.0f, 150.0f, 54.0f };
        int cost = getUpgradeCost(i, lv);
        
        if (lv >= maxLv) {
            DrawRectangleRec(buyRect, DARKGRAY);
            DrawRectangleLinesEx(buyRect, 2.0f, GRAY);
            const char *maxText = Tr(gs, "DAT MAX", "MAX LEVEL");
            Vector2 textS = MeasureTextEx(gameFont, maxText, 18, 1.0f);
            DrawCrispText(maxText, buyRect.x + (buyRect.width - textS.x)/2.0f, buyRect.y + 18.0f, 18, GRAY);
        } else {
            char buyText[64];
            sprintf(buyText, "%s: %d G", Tr(gs, "MUA", "BUY"), cost);
            Color btnCol = (gs->cachedSave.totalGoldEarned >= cost) ? GREEN : MAROON;
            DrawButton(buyRect, buyText, btnCol);
        }
    }
    
    // VẼ NÚT "TRỞ VỀ MENU" NGAY TRONG CONTEXT RENDER
    Rectangle backRect = { VIRTUAL_WIDTH / 2.0f - 130.0f, 600.0f, 260.0f, 50.0f };
    DrawButton(backRect, Tr(gs, "Tro ve Menu", "Back to Menu"), RED);
}

// Xử lý click mua hàng và click nút Trở về trong Update
void UpdateUpgradeShop(struct GameState *gs) {
    if (gs == NULL) return;
    
    // Dùng cache thay vì đọc file mỗi frame
    
    // 1. Kiểm tra click mua nâng cấp
    float startY = 160.0f;
    float rowH = 90.0f;
    
    for (int i = 0; i < UPGRADE_COUNT; i++) {
        float y = startY + i * (rowH + 15.0f);
        Rectangle buyRect = { VIRTUAL_WIDTH - 280.0f, y + 18.0f, 150.0f, 54.0f };
        int lv = gs->cachedSave.upgrades.level[i];
        int maxLv = getUpgradeMaxLevel(i);
        int cost = getUpgradeCost(i, lv);
        
        if (lv < maxLv && IsButtonClicked(buyRect)) {
            if (gs->cachedSave.totalGoldEarned >= cost) {
                gs->cachedSave.totalGoldEarned -= cost;
                gs->cachedSave.upgrades.level[i]++;
                WriteSaveData(&gs->cachedSave);
                gs->savedUpgrades = gs->cachedSave.upgrades;
            }
        }
    }
    
    // 2. Kiểm tra click nút Trở về
    Rectangle backRect = { VIRTUAL_WIDTH / 2.0f - 130.0f, 600.0f, 260.0f, 50.0f };
    if (IsButtonClicked(backRect)) {
        gs->savedUpgrades = gs->cachedSave.upgrades;
        gs->currentScreen = SCREEN_MENU;
    }
}
