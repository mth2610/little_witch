// ============================================================
// enemy_normal.c — Logic di chuyển và vẽ vector cho quái thường
// ============================================================

#include "enemy_normal.h"
#include "witch.h"
#include "skill.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>

// Texture quái vật (khai báo ở main.c)
extern Texture2D enemySlimeTexture;
extern Texture2D enemyBatTexture;
extern Texture2D enemyGhostTexture;
extern Texture2D enemyRobotTexture;
extern Texture2D enemyDroneTexture;
extern Texture2D enemyAlienTexture;
extern Texture2D enemyUfoTexture;

// Cập nhật di chuyển và đòn tấn công của quái thường
void UpdateNormalEnemy(Enemy *e, struct Witch *witch, ProjectilePool *projPool, float deltaTime) {
    if (e == NULL || witch == NULL || projPool == NULL) return;

    switch (e->type) {
        case ENEMY_SLIME:
            e->position.x += e->velocity.x * deltaTime;
            break;
            
        case ENEMY_BAT:
            e->position.x += e->velocity.x * deltaTime;
            // Di chuyển hình sin theo trục Y
            e->position.y += sinf(e->animTimer * 5.0f) * 140.0f * deltaTime;
            break;
            
        case ENEMY_GHOST:
            e->position.x += e->velocity.x * deltaTime;
            e->invisTimer += deltaTime;
            // Ẩn/hiện định kỳ mỗi 1.5s
            if (e->invisTimer >= 1.5f) {
                e->isVisible = !e->isVisible;
                e->invisTimer = 0.0f;
            }
            break;
            
        case ENEMY_ROBOT:
            e->position.x += e->velocity.x * deltaTime;
            e->shootCooldown -= deltaTime;
            // Bắn đạn thẳng nhắm về phía witch mỗi 2.5s
            if (e->shootCooldown <= 0.0f) {
                Vector2 dir = Vector2Normalize(Vector2Subtract(witch->position, e->position));
                Vector2 projVel = Vector2Scale(dir, 240.0f);
                SpawnProjectile(projPool, e->position, projVel, 7.0f, 1, true, ORANGE, PROJ_ENEMY_NORMAL);
                e->shootCooldown = 2.5f;
            }
            break;
            
        case ENEMY_DRONE: {
            // Bay trực tiếp dí theo witch
            Vector2 toWitch = Vector2Subtract(witch->position, e->position);
            Vector2 dir = Vector2Normalize(toWitch);
            e->velocity = Vector2Scale(dir, e->speed);
            e->position = Vector2Add(e->position, Vector2Scale(e->velocity, deltaTime));
            break;
        }
        
        case ENEMY_ALIEN:
            // Đi chéo, đảo hướng Y mỗi 0.8s để tạo đường zigzag
            e->shootCooldown += deltaTime;
            if (e->shootCooldown >= 0.8f) {
                e->velocity.y = -e->velocity.y;
                e->shootCooldown = 0.0f;
            }
            e->position.x += e->velocity.x * deltaTime;
            e->position.y += e->velocity.y * deltaTime;
            break;
            
        case ENEMY_UFO:
            e->position.x += e->velocity.x * deltaTime;
            e->shootCooldown -= deltaTime;
            // Bắn đạn chùm 3 tia tỏa ra nhắm về witch
            if (e->shootCooldown <= 0.0f) {
                Vector2 toWitch = Vector2Normalize(Vector2Subtract(witch->position, e->position));
                float baseAngle = atan2f(toWitch.y, toWitch.x);
                
                // 3 tia lệch góc nhau 20 độ (~0.35 rad)
                float angles[3] = { baseAngle - 0.35f, baseAngle, baseAngle + 0.35f };
                for (int a = 0; a < 3; a++) {
                    Vector2 projVel = { cosf(angles[a]) * 200.0f, sinf(angles[a]) * 200.0f };
                    SpawnProjectile(projPool, e->position, projVel, 8.0f, 1, true, RED, PROJ_ENEMY_SPECIAL);
                }
                e->shootCooldown = 3.0f;
            }
            break;
            
        default:
            break;
    }
}

// Vẽ vector/thủ thuật cho quái thường khi không load được texture
void DrawProceduralNormalEnemy(const Enemy *e, Color col) {
    if (e == NULL) return;

    switch (e->type) {
        case ENEMY_SLIME: {
            // Slime xanh nảy phồng theo nhịp hình sin
            float pulseY = sinf(e->animTimer * 8.0f) * 3.0f;
            float rx = e->collisionRadius * 1.1f;
            float ry = e->collisionRadius * 0.9f + pulseY;
            
            DrawEllipse(e->position.x, e->position.y + pulseY, rx, ry, col);
            DrawEllipseLines(e->position.x, e->position.y + pulseY, rx, ry, DARKGREEN);
            
            // Vẽ 2 mắt nhỏ tinh nghịch
            Vector2 eyeL = { e->position.x - 5.0f, e->position.y - 2.0f + pulseY };
            Vector2 eyeR = { e->position.x + 5.0f, e->position.y - 2.0f + pulseY };
            DrawCircleV(eyeL, 2.5f, BLACK);
            DrawCircleV(eyeR, 2.5f, BLACK);
            DrawCircle(eyeL.x - 0.8f, eyeL.y - 0.8f, 0.8f, WHITE);
            DrawCircle(eyeR.x - 0.8f, eyeR.y - 0.8f, 0.8f, WHITE);
            break;
        }
        
        case ENEMY_BAT: {
            // Dơi vỗ cánh góc tam giác sinh động
            float wingFlap = sinf(e->animTimer * 18.0f) * 10.0f;
            Vector2 wingTipL = { e->position.x - e->collisionRadius * 2.0f, e->position.y - wingFlap };
            Vector2 wingTipR = { e->position.x + e->collisionRadius * 2.0f, e->position.y - wingFlap };
            
            DrawTriangle(e->position, (Vector2){ e->position.x - e->collisionRadius * 0.4f, e->position.y + 4.0f }, wingTipL, col);
            DrawTriangle(e->position, wingTipR, (Vector2){ e->position.x + e->collisionRadius * 0.4f, e->position.y + 4.0f }, col);
            
            DrawCircleV(e->position, e->collisionRadius * 0.8f, col);
            DrawCircleLines(e->position.x, e->position.y, e->collisionRadius * 0.8f, BLACK);
            
            // Mắt dơi phát sáng đỏ neon
            BeginBlendMode(BLEND_ADDITIVE);
            DrawCircle(e->position.x - 3.5f, e->position.y - 1.0f, 4.0f, ColorAlpha(RED, 0.6f));
            DrawCircle(e->position.x + 3.5f, e->position.y - 1.0f, 4.0f, ColorAlpha(RED, 0.6f));
            DrawCircle(e->position.x - 3.5f, e->position.y - 1.0f, 1.5f, WHITE);
            DrawCircle(e->position.x + 3.5f, e->position.y - 1.0f, 1.5f, WHITE);
            EndBlendMode();
            break;
        }
        
        case ENEMY_GHOST: {
            // Bóng ma trôi nổi tà áo lượn sóng
            float floatY = sinf(e->animTimer * 6.0f) * 4.0f;
            Vector2 gPos = { e->position.x, e->position.y + floatY };
            
            DrawCircleV(gPos, e->collisionRadius * 0.9f, col);
            DrawTriangle(
                (Vector2){ gPos.x - e->collisionRadius * 0.9f, gPos.y },
                (Vector2){ gPos.x + e->collisionRadius * 0.9f, gPos.y },
                (Vector2){ gPos.x - 8.0f - sinf(e->animTimer * 12.0f) * 3.0f, gPos.y + e->collisionRadius * 1.4f },
                col
            );
            
            // 2 mắt rỗng tối tăm của linh hồn
            DrawCircle(gPos.x - 4.5f, gPos.y - 3.0f, 2.5f, (Color){ 30, 30, 50, 255 });
            DrawCircle(gPos.x + 4.5f, gPos.y - 3.0f, 2.5f, (Color){ 30, 30, 50, 255 });
            DrawCircle(gPos.x, gPos.y + 3.0f, 1.8f, (Color){ 30, 30, 50, 255 });
            break;
        }
        
        case ENEMY_ROBOT: {
            // Robot khối kim loại bay lơ lửng
            float hoverY = sinf(e->animTimer * 10.0f) * 2.0f;
            Vector2 rPos = { e->position.x, e->position.y + hoverY };
            
            Rectangle rect = { rPos.x - e->collisionRadius * 0.9f, rPos.y - e->collisionRadius * 0.9f, e->collisionRadius * 1.8f, e->collisionRadius * 1.8f };
            DrawRectangleRec(rect, col);
            DrawRectangleLinesEx(rect, 2.0f, BLACK);
            
            // Kính chắn (Visor) robot phát sáng xanh
            BeginBlendMode(BLEND_ADDITIVE);
            DrawRectangle(rPos.x - e->collisionRadius * 0.6f, rPos.y - 6.0f, e->collisionRadius * 1.2f, 5.0f, ColorAlpha(SKYBLUE, 0.4f));
            DrawRectangle(rPos.x - e->collisionRadius * 0.4f, rPos.y - 5.0f, e->collisionRadius * 0.8f, 3.0f, WHITE);
            EndBlendMode();
            break;
        }
        
        case ENEMY_DRONE: {
            // Drone cánh quạt đôi quay chéo
            float angle = e->animTimer * 720.0f;
            Vector2 propL = { e->position.x - e->collisionRadius * 1.3f, e->position.y - 6.0f };
            Vector2 propR = { e->position.x + e->collisionRadius * 1.3f, e->position.y - 6.0f };
            
            DrawLineEx(e->position, propL, 2.0f, BLACK);
            DrawLineEx(e->position, propR, 2.0f, BLACK);
            DrawLineEx(propL, (Vector2){ propL.x + cosf(angle * DEG2RAD) * 12.0f, propL.y + sinf(angle * DEG2RAD) * 4.0f }, 2.0f, BLACK);
            DrawLineEx(propR, (Vector2){ propR.x - cosf(angle * DEG2RAD) * 12.0f, propR.y - sinf(angle * DEG2RAD) * 4.0f }, 2.0f, BLACK);
            
            DrawCircleV(e->position, e->collisionRadius * 0.8f, col);
            DrawCircleLines(e->position.x, e->position.y, e->collisionRadius * 0.8f, BLACK);
            DrawCircle(e->position.x + 3.0f, e->position.y, 2.5f, LIME);
            break;
        }
        
        case ENEMY_ALIEN: {
            // Người ngoài hành tinh đầu to lắc lư
            float wiggleX = sinf(e->animTimer * 12.0f) * 2.0f;
            Vector2 aPos = { e->position.x + wiggleX, e->position.y };
            
            DrawCircle(aPos.x, aPos.y - 3.0f, e->collisionRadius * 0.8f, col);
            DrawTriangle(
                (Vector2){ aPos.x - e->collisionRadius * 0.8f, aPos.y - 3.0f },
                (Vector2){ aPos.x + e->collisionRadius * 0.8f, aPos.y - 3.0f },
                (Vector2){ aPos.x, aPos.y + e->collisionRadius * 0.7f },
                col
            );
            DrawEllipse(aPos.x - 5.0f, aPos.y - 5.0f, 4.0f, 6.0f, BLACK);
            DrawEllipse(aPos.x + 5.0f, aPos.y - 5.0f, 4.0f, 6.0f, BLACK);
            break;
        }
        
        case ENEMY_UFO: {
            // Đĩa bay UFO phát sóng nhẹ
            float ufoPulse = sinf(e->animTimer * 10.0f) * 3.0f;
            Vector2 uPos = { e->position.x, e->position.y + ufoPulse };
            
            DrawEllipse(uPos.x, uPos.y, e->collisionRadius * 1.2f, e->collisionRadius * 0.5f, col);
            DrawEllipseLines(uPos.x, uPos.y, e->collisionRadius * 1.2f, e->collisionRadius * 0.5f, BLACK);
            DrawCircleSector((Vector2){ uPos.x, uPos.y - 2.0f }, e->collisionRadius * 0.6f, 180.0f, 360.0f, 16, ColorAlpha(SKYBLUE, 0.6f));
            break;
        }
        
        default: {
            DrawCircleV(e->position, e->collisionRadius, col);
            DrawCircleLinesV(e->position, e->collisionRadius + 2.0f, BLACK);
            break;
        }
    }
}
