// ============================================================
// witch.c — Triển khai logic điều khiển Phù Thủy (Witch)
// ============================================================

#include "witch.h"
#include "game.h"
#include "raymath.h"
#include <stddef.h>

// Biến texture toàn cục được load ở main.c
extern Texture2D witchFlyTexture;
extern Texture2D witchAttackHandTexture;
extern Texture2D witchAttackWeaponTexture;
extern Texture2D witchDefenseTexture;
extern Texture2D witchSpritesheetTexture;
extern Texture2D fluteTexture;
extern bool witchSpritesheetLoaded;
extern Shader shieldShader;
extern bool shieldShaderLoaded;
extern Texture2D shieldBackgroundTexture;

// ----------------------------------------------------------------
// HÀM CÔNG KHAI
// ----------------------------------------------------------------

// Khởi tạo trạng thái Phù Thủy mới
void InitWitch(Witch *witch, PermanentUpgrades upgrades) {
    if (witch == NULL) return;
    
    witch->position = (Vector2){ VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f };
    witch->targetPosition = witch->position;
    witch->velocity = (Vector2){ 0.0f, 0.0f };
    witch->collisionRadius = 28.0f;
    
    // Mạng ban đầu = 3 + cấp nâng cấp tăng mạng
    witch->lives = 3 + upgrades.level[UPGRADE_MAX_LIVES];
    witch->isInvincible = false;
    witch->invincibleTimer = 0.0f;
    witch->hasRevived = false;
    witch->gold = 0;
    witch->score = 0;
    
    // Bật tất cả các kỹ năng chủ động sẵn sàng từ đầu và đặt cấp ban đầu = 1
    for (int i = 0; i < SKILL_COUNT; i++) {
        witch->activeSkills[i] = true;
        witch->skillLevels[i] = 2;
    }
    
    // Khởi tạo cooldown và active timer
    for (int i = 0; i < SKILL_COUNT; i++) {
        witch->skillCooldown[i] = 0.0f;
        witch->skillActiveTimer[i] = 0.0f;
    }
    witch->manaShieldHealth = 0.0f; // Khiên chưa được kích hoạt lúc đầu
    witch->upgrades = upgrades;
    
    witch->animTimer = 0.0f;
    witch->animFrame = 0;
    witch->facingRight = true;
    witch->swipeBoostTimer = 0.0f;
    witch->animState = WITCH_STATE_FLY;
    witch->stateTimer = 0.0f;
    witch->prevVirtualPos = (Vector2){ 0.0f, 0.0f };
    witch->prevInputPressed = false;
    witch->keyboardAttackCooldown = 0.0f;
    
    // Khởi tạo lịch sử vị trí cho afterimages
    for (int i = 0; i < 4; i++) {
        witch->positionHistory[i] = witch->position;
    }
    
}

// Cập nhật trạng thái Phù Thủy qua mỗi frame
void UpdateWitch(Witch *witch, float deltaTime) {
    if (witch == NULL) return;
    
    // 1. ĐỌC INPUT BÀN PHÍM (LÊN/XUỐNG/TRÁI/PHẢI & W/A/S/D)
    float dx = 0.0f;
    float dy = 0.0f;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) dy -= 1.0f;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) dy += 1.0f;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) dx -= 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dx += 1.0f;
    
    bool kbPressed = (dx != 0.0f || dy != 0.0f);
    
    // 2. ĐỌC INPUT CẢM ỨNG / CHUỘT
    bool inputPressed = false;
    Vector2 rawInputPos = { 0.0f, 0.0f };
    
    if (GetTouchPointCount() > 0) {
        rawInputPos = GetTouchPosition(0);
        inputPressed = true;
    } else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        rawInputPos = GetMousePosition();
        inputPressed = true;
    }
    
    if (inputPressed) {
        Vector2 virtualPos = ScreenToVirtual(rawInputPos);
        
        // Bỏ qua di chuyển nếu nhấn vào vùng phím kỹ năng ở góc dưới phải (phạm vi bố cục RPG vòng cung mới)
        bool touchOnSkillBtn = (virtualPos.x > 900.0f && virtualPos.y > 310.0f);
        
        if (!touchOnSkillBtn) {
            witch->targetPosition = virtualPos;
            
            // Phát hiện vuốt màn hình (Swipe Boost)
            if (witch->prevInputPressed) {
                float delta = Vector2Distance(virtualPos, witch->prevVirtualPos);
                // Nếu khoảng cách vuốt giữa 2 frame lớn hơn ngưỡng
                if (delta > SWIPE_THRESHOLD) {
                    witch->swipeBoostTimer = 0.3f; // Boost tốc độ trong 0.3 giây
                }
            }
        }
        
        witch->prevVirtualPos = virtualPos;
        witch->prevInputPressed = true;
    } else {
        witch->prevInputPressed = false;
    }
    
    // 3. DI CHUYỂN NHÂN VẬT
    if (kbPressed) {
        // Di chuyển bằng bàn phím
        Vector2 kbDir = { dx, dy };
        kbDir = Vector2Normalize(kbDir);
        float speed = WitchGetEffectiveSpeed(witch);
        witch->velocity = Vector2Scale(kbDir, speed);
        witch->position = Vector2Add(witch->position, Vector2Scale(witch->velocity, deltaTime));
        
        // Cập nhật targetPosition theo vị trí hiện tại
        witch->targetPosition = witch->position;
        if (dx < 0.0f) witch->facingRight = false;
        else if (dx > 0.0f) witch->facingRight = true;
    } else {
        // Di chuyển LERP về phía target (chuột/cảm ứng)
        Vector2 direction = Vector2Subtract(witch->targetPosition, witch->position);
        float dist = Vector2Length(direction);
        
        if (dist > 2.0f) {
            float speed = WitchGetEffectiveSpeed(witch);
            witch->velocity = Vector2Scale(Vector2Normalize(direction), speed);
            witch->position = Vector2Add(witch->position, Vector2Scale(witch->velocity, deltaTime));
            
            if (direction.x < 0.0f) witch->facingRight = false;
            else if (direction.x > 0.0f) witch->facingRight = true;
        } else {
            witch->position = witch->targetPosition;
            witch->velocity = (Vector2){ 0.0f, 0.0f };
        }
    }
    
    // 3. GIỚI HẠN BIÊN MÀN HÌNH ẢO
    witch->position.x = Clamp(witch->position.x, witch->collisionRadius, VIRTUAL_WIDTH - witch->collisionRadius);
    witch->position.y = Clamp(witch->position.y, witch->collisionRadius, VIRTUAL_HEIGHT - witch->collisionRadius);
    
    // 4. INVINCIBILITY TIMER
    if (witch->isInvincible) {
        witch->invincibleTimer -= deltaTime;
        if (witch->invincibleTimer <= 0.0f) {
            witch->isInvincible = false;
            witch->invincibleTimer = 0.0f;
        }
    }
    
    // 5. SWIPE BOOST TIMER
    if (witch->swipeBoostTimer > 0.0f) {
        witch->swipeBoostTimer -= deltaTime;
        if (witch->swipeBoostTimer < 0.0f) {
            witch->swipeBoostTimer = 0.0f;
        }
    }
    
    // 6. CẬP NHẬT COOLDOWN VÀ ACTIVE TIMER CỦA KỸ NĂNG CHỦ ĐỘNG
    for (int i = 1; i < SKILL_COUNT; i++) {
        if (witch->skillCooldown[i] > 0.0f) {
            witch->skillCooldown[i] -= deltaTime;
            if (witch->skillCooldown[i] < 0.0f) witch->skillCooldown[i] = 0.0f;
        }
        if (witch->skillActiveTimer[i] > 0.0f) {
            witch->skillActiveTimer[i] -= deltaTime;
            if (witch->skillActiveTimer[i] < 0.0f) {
                witch->skillActiveTimer[i] = 0.0f;
            }
        }
    }
    
    // Cập nhật hồi chiêu đòn đánh thường bàn phím/touch (Space)
    // Lý do: Đồng bộ hóa toàn bộ các biến cooldown của nhân vật trong một hàm Update duy nhất để tránh phân mảnh logic ở main.c
    if (witch->keyboardAttackCooldown > 0.0f) {
        witch->keyboardAttackCooldown -= deltaTime;
        if (witch->keyboardAttackCooldown < 0.0f) {
            witch->keyboardAttackCooldown = 0.0f;
        }
    }
    
    // Cập nhật khiên ma thuật hệ Thủy tự kích hoạt
    if (witch->manaShieldHealth > 0.0f) {
        witch->manaShieldHealth -= deltaTime / 3.0f; // Khiên tồn tại trong 3 giây
        if (witch->manaShieldHealth < 0.0f) {
            witch->manaShieldHealth = 0.0f;
        }
    }
    
    // 8. CẬP NHẬT TIMER TRẠNG THÁI HOẠT HỌA
    if (witch->stateTimer > 0.0f) {
        witch->stateTimer -= deltaTime;
        if (witch->stateTimer <= 0.0f) {
            witch->stateTimer = 0.0f;
            witch->animState = WITCH_STATE_FLY;
        }
    }
    
    // Nếu không có hoạt ảnh hành động đè, tự động kích hoạt tư thế DEFENSE nếu có khiên Mana Shield
    if (witch->stateTimer <= 0.0f) {
        if (witch->manaShieldHealth > 0.0f) {
            witch->animState = WITCH_STATE_DEFENSE;
        } else {
            witch->animState = WITCH_STATE_FLY;
        }
    }
    
    // 9. ANIMATION
    witch->animTimer += deltaTime;
    if (witch->animState == WITCH_STATE_FLY) {
        witch->animFrame = ((int)(witch->animTimer * 24.0f)) % 16; // 16 frame hoạt họa siêu mượt
    } else {
        witch->animFrame = ((int)(witch->animTimer * 8.0f)) % 4;   // 4 frame cho các trạng thái khác
    }
    
    // 10. CẬP NHẬT LỊCH SỬ VỊ TRÍ (Hệ thống Afterimages)
    for (int i = 3; i > 0; i--) {
        witch->positionHistory[i] = witch->positionHistory[i - 1];
    }
    witch->positionHistory[0] = witch->position;
    
}

// Vẽ Phù Thủy lên canvas ảo
void DrawWitch(const Witch *witch) {
    if (witch == NULL) return;
    
    // Vẽ các bóng mờ Afterimages nếu đang có Swipe Boost tốc độ cao
    if (witch->swipeBoostTimer > 0.0f) {
        Texture2D tex = witchFlyTexture;
        if (witch->animState == WITCH_STATE_ATTACK_HAND) tex = witchAttackHandTexture;
        else if (witch->animState == WITCH_STATE_ATTACK_WEAPON) tex = witchAttackWeaponTexture;
        else if (witch->animState == WITCH_STATE_DEFENSE) tex = witchDefenseTexture;
        
        bool hasTexture = witchSpritesheetLoaded || IsTextureReady(tex);
        
        for (int i = 3; i >= 0; i--) {
            // Độ mờ giảm dần theo độ cũ của vị trí
            float alpha = 0.35f * (1.0f - (float)i / 4.0f) * (witch->swipeBoostTimer / 0.3f);
            Color afterColor = ColorAlpha(SKYBLUE, alpha);
            Vector2 histPos = witch->positionHistory[i];
            
            if (hasTexture) {
                if (witchSpritesheetLoaded) {
                    float frameWidth = (float)witchSpritesheetTexture.width / 4.0f;
                    float frameHeight = (float)witchSpritesheetTexture.height / 4.0f;
                    float sourceWidth = witch->facingRight ? frameWidth : -frameWidth;
                    
                    Rectangle sourceRec = { 
                        witch->animFrame * frameWidth, 
                        witch->animState * frameHeight, 
                        sourceWidth, 
                        frameHeight 
                    };
                    
                    float drawSize = 90.0f;
                    Rectangle destRec = { histPos.x, histPos.y, drawSize, drawSize };
                    Vector2 origin = { drawSize / 2.0f, drawSize / 2.0f };
                    
                    DrawTexturePro(witchSpritesheetTexture, sourceRec, destRec, origin, 0.0f, afterColor);
                } else {
                    float frameWidth = (float)tex.width / 4.0f;
                    float frameHeight = (float)tex.height;
                    float sourceWidth = witch->facingRight ? frameWidth : -frameWidth;
                    
                    Rectangle sourceRec = { witch->animFrame * frameWidth, 0.0f, sourceWidth, frameHeight };
                    Rectangle destRec = { histPos.x, histPos.y, frameWidth, frameHeight };
                    Vector2 origin = { frameWidth / 2.0f, frameHeight / 2.0f };
                    
                    DrawTexturePro(tex, sourceRec, destRec, origin, 0.0f, afterColor);
                }
            } else {
                // Vẽ bóng mờ dạng vector bằng vòng tròn đại diện
                DrawCircle(histPos.x, histPos.y, witch->collisionRadius, ColorAlpha(SKYBLUE, alpha * 0.5f));
            }
        }
    }
    
    // Hiệu ứng nhấp nháy khi đang bất tử
    if (witch->isInvincible) {
        // Nhấp nháy ở chu kỳ 10Hz
        if (((int)(witch->animTimer * 15.0f)) % 2 == 0) {
            // Không vẽ frame này để tạo hiệu ứng nhấp nháy
            return;
        }
    }
    
    // Vẽ khiên ma thuật bảo vệ nếu có và còn hoạt động (khi vẽ đè lên sprite)
    bool hasShieldTextureDraw = (witch->manaShieldHealth > 0.0f);
    
    // Xác định texture theo trạng thái hoạt họa
    Texture2D tex = witchFlyTexture;
    if (witch->animState == WITCH_STATE_ATTACK_HAND) {
        tex = witchAttackHandTexture;
    } else if (witch->animState == WITCH_STATE_ATTACK_WEAPON) {
        tex = witchAttackWeaponTexture;
    } else if (witch->animState == WITCH_STATE_DEFENSE) {
        tex = witchDefenseTexture;
    }
    
    bool textureReady = witchSpritesheetLoaded || IsTextureReady(tex);
    
    if (textureReady) {
        if (hasShieldTextureDraw) {
            float timeVal = (float)GetTime();
            float radiusVal = (100.0f + 12.0f * witch->effectiveThuyStars) + sinf(timeVal * 7.0f) * 5.0f; // Bán kính động tăng theo sao Thủy
            
            if (shieldShaderLoaded) {
                BeginShaderMode(shieldShader);
                int timeLoc = GetShaderLocation(shieldShader, "time");
                int glowColorLoc = GetShaderLocation(shieldShader, "glowColor");
                int centerLoc = GetShaderLocation(shieldShader, "center");
                int radiusLoc = GetShaderLocation(shieldShader, "radius");
                
                Vector4 glowCol = { 0.3f, 0.75f, 1.0f, 1.0f }; // Sky blue
                Vector2 centerVal = witch->position;
                
                SetShaderValue(shieldShader, timeLoc, &timeVal, SHADER_UNIFORM_FLOAT);
                SetShaderValue(shieldShader, glowColorLoc, &glowCol, SHADER_UNIFORM_VEC4);
                SetShaderValue(shieldShader, centerLoc, &centerVal, SHADER_UNIFORM_VEC2);
                SetShaderValue(shieldShader, radiusLoc, &radiusVal, SHADER_UNIFORM_FLOAT);
                
                int screenTexLoc = GetShaderLocation(shieldShader, "screenTexture");
                SetShaderValueTexture(shieldShader, screenTexLoc, shieldBackgroundTexture);
                
                DrawRectangleRec((Rectangle){ witch->position.x - radiusVal, witch->position.y - radiusVal, radiusVal * 2.0f, radiusVal * 2.0f }, WHITE);
                EndShaderMode();
            } else {
                // Vẽ khiên chắn vector xoay chuyển động (fallback) rộng rãi và đẹp mắt
                float rot = timeVal * 120.0f;
                DrawCircleGradient(witch->position.x, witch->position.y, radiusVal * 1.1f, ColorAlpha(SKYBLUE, 0.12f), ColorAlpha(SKYBLUE, 0.0f));
                DrawCircleLines(witch->position.x, witch->position.y, radiusVal, ColorAlpha(SKYBLUE, 0.7f));
                DrawCircleLines(witch->position.x, witch->position.y, radiusVal - 1.5f, ColorAlpha(SKYBLUE, 0.3f));
                
                // Vẽ 3 quả cầu hộ mệnh xoay tròn quanh khiên
                for (int j = 0; j < 3; j++) {
                    float a = rot * DEG2RAD + j * (2.0f * PI / 3.0f);
                    float sx = witch->position.x + cosf(a) * radiusVal;
                    float sy = witch->position.y + sinf(a) * radiusVal;
                    DrawCircleGradient(sx, sy, 14.0f, ColorAlpha(WHITE, 0.6f), ColorAlpha(SKYBLUE, 0.0f));
                    DrawCircle(sx, sy, 3.5f, WHITE);
                }
            }
        }
        
        // 1. Tính toán hiệu ứng bay bồng bềnh hình sin và độ nghiêng lướt gió sinh động
        float bobbing = sinf((float)GetTime() * 4.5f) * 4.5f;
        float angle = witch->velocity.y * 0.035f;
        if (angle > 14.0f) angle = 14.0f;
        if (angle < -14.0f) angle = -14.0f;
        
        Vector2 drawPos = { witch->position.x, witch->position.y + bobbing };
        
        // 2. Vẽ CÂY SÁO trước ở ngay dưới lòng bàn chân Pháp sư
        if (IsTextureReady(fluteTexture)) {
            float fluteDrawW = 150.0f;
            float fluteDrawH = fluteDrawW * ((float)fluteTexture.height / (float)fluteTexture.width);
            float fluteSourceW = witch->facingRight ? (float)fluteTexture.width : -(float)fluteTexture.width;
            
            Rectangle fluteSourceRec = { 0.0f, 0.0f, fluteSourceW, (float)fluteTexture.height };
            // Lệch xuống dưới mông/chân Pháp sư
            float fluteYOffset = (witch->animState == WITCH_STATE_FLY) ? 45.0f : 50.0f;
            Rectangle fluteDestRec = { drawPos.x, drawPos.y + fluteYOffset, fluteDrawW, fluteDrawH };
            Vector2 fluteOrigin = { fluteDrawW / 2.0f, fluteDrawH / 2.0f };
            
            DrawTexturePro(fluteTexture, fluteSourceRec, fluteDestRec, fluteOrigin, angle, WHITE);
        }
        
        // 3. Vẽ PHÁP SƯ
        if (witch->animState == WITCH_STATE_FLY && IsTextureReady(witchFlyTexture)) {
            // Sử dụng sprite sheet 16 khung hình mới để có chuyển động siêu mượt
            float frameWidth = (float)witchFlyTexture.width / 16.0f;
            float frameHeight = (float)witchFlyTexture.height;
            float sourceWidth = witch->facingRight ? frameWidth : -frameWidth;
            
            Rectangle sourceRec = { witch->animFrame * frameWidth, 0.0f, sourceWidth, frameHeight };
            
            // Tỷ lệ hiển thị tối ưu
            float drawH = 96.0f;
            float drawW = drawH * (frameWidth / frameHeight);
            
            Rectangle destRec = { drawPos.x, drawPos.y, drawW, drawH };
            Vector2 origin = { drawW / 2.0f, drawH / 2.0f };
            
            DrawTexturePro(witchFlyTexture, sourceRec, destRec, origin, angle, WHITE);
        } else if (witchSpritesheetLoaded) {
            // Trạng thái tấn công/phòng thủ vẽ bằng 4x4 Spritesheet
            float frameWidth = (float)witchSpritesheetTexture.width / 4.0f;
            float frameHeight = (float)witchSpritesheetTexture.height / 4.0f;
            float sourceWidth = witch->facingRight ? frameWidth : -frameWidth;
            
            Rectangle sourceRec = { 
                witch->animFrame * frameWidth, 
                witch->animState * frameHeight, 
                sourceWidth, 
                frameHeight 
            };
            
            float drawSize = 90.0f;
            Rectangle destRec = { drawPos.x, drawPos.y, drawSize, drawSize };
            Vector2 origin = { drawSize / 2.0f, drawSize / 2.0f };
            
            DrawTexturePro(witchSpritesheetTexture, sourceRec, destRec, origin, angle, WHITE);
        } else {
            // Vẽ bằng texture riêng lẻ cũ (4 frame ngang)
            float frameWidth = (float)tex.width / 4.0f;
            float frameHeight = (float)tex.height;
            float sourceWidth = witch->facingRight ? frameWidth : -frameWidth;
            
            Rectangle sourceRec = { witch->animFrame * frameWidth, 0.0f, sourceWidth, frameHeight };
            Rectangle destRec = { drawPos.x, drawPos.y, frameWidth, frameHeight };
            Vector2 origin = { frameWidth / 2.0f, frameHeight / 2.0f };
            
            DrawTexturePro(tex, sourceRec, destRec, origin, angle, WHITE);
        }
    } else {
        // TỰ ĐỘNG VẼ NỮ PHÁP SƯ VIỆT NAM MẶC ÁO DÀI CƯỠI SÁO TRÚC THẦN (VECTOR ART FALLBACK)
        // Tạo hình ảnh động, chi tiết và dũng mãnh đậm chất thần thoại Việt Nam
        
        // 1. Tính toán chuyển động nhấp nhô (hovering oscillation)
        float hover = sinf(witch->animTimer * 6.0f) * 4.0f;
        Vector2 pos = { witch->position.x, witch->position.y + hover };
        
        // 2. Vẽ các hạt lá tre phát sáng (sparks) thoát ra từ sáo trúc (sau lưng)
        for (int p = 0; p < 4; p++) {
            float seed = p * 1.57f;
            float sparkOffset = fmodf(witch->animTimer * 45.0f + p * 15.0f, 40.0f);
            float sparkX = pos.x - 38.0f - sparkOffset;
            float sparkY = pos.y + 10.0f + sinf(witch->animTimer * 10.0f + seed) * 5.0f;
            float sparkRadius = 3.5f * (1.0f - sparkOffset / 40.0f);
            Color sparkColor = ColorAlpha(p % 2 == 0 ? LIME : GOLD, 0.7f * (1.0f - sparkOffset / 40.0f));
            DrawCircle(sparkX, sparkY, sparkRadius, sparkColor);
        }
        
        // 3. Vẽ lá trúc ở đuôi sáo trúc thần (bamboo leaves at tail)
        float tailWiggle = sinf(witch->animTimer * 12.0f) * 3.0f;
        Vector2 tailBase = { pos.x - 30.0f, pos.y + 11.0f };
        Vector2 leafTip1 = { pos.x - 46.0f, pos.y + 2.0f + tailWiggle };
        Vector2 leafTip2 = { pos.x - 44.0f, pos.y + 18.0f + tailWiggle };
        
        DrawTriangle(tailBase, leafTip2, leafTip1, DARKGREEN);
        DrawTriangleLines(tailBase, leafTip2, leafTip1, LIME);
        
        // 4. Vẽ sáo trúc thần (magical jade bamboo flute)
        Vector2 stickStart = { pos.x - 32.0f, pos.y + 11.0f };
        Vector2 stickEnd = { pos.x + 28.0f, pos.y + 3.0f };
        DrawLineEx(stickStart, stickEnd, 5.0f, (Color){ 34, 177, 76, 255 }); // Màu xanh ngọc lục bảo
        
        // Khấc sáo trúc màu vàng kim
        for (int segment = 1; segment <= 4; segment++) {
            float t = (float)segment / 5.0f;
            Vector2 segPos = Vector2Lerp(stickStart, stickEnd, t);
            DrawCircleV(segPos, 3.2f, GOLD);
        }
        
        // 5. Vẽ tà áo dài phía sau bay phấp phới (flowing back flap of Ao Dai)
        float flapWave = cosf(witch->animTimer * 10.0f) * 5.0f;
        Vector2 neck = { pos.x + 3.0f, pos.y - 14.0f };
        Vector2 waist = { pos.x - 5.0f, pos.y - 1.0f };
        Vector2 flapBackTip = { pos.x - 38.0f, pos.y + 14.0f + flapWave };
        
        // Vẽ tà sau áo dài đỏ rực rỡ
        DrawTriangle(neck, waist, flapBackTip, (Color){ 200, 15, 30, 255 });
        DrawTriangle(waist, flapBackTip, (Vector2){ pos.x - 28.0f, pos.y + 20.0f + flapWave }, (Color){ 230, 30, 45, 255 });
        
        // 6. Vẽ thân người và tà áo dài phía trước (Ao Dai bodice and front flap)
        Vector2 hipL = { pos.x - 12.0f, pos.y + 6.0f };
        Vector2 hipR = { pos.x + 6.0f, pos.y + 7.0f };
        DrawTriangle(neck, hipL, hipR, (Color){ 220, 20, 40, 255 }); // Thân áo đỏ chính
        
        // Vẽ tà trước của áo dài bay nhẹ
        Vector2 flapFrontTip = { pos.x - 14.0f, pos.y + 22.0f + flapWave * 0.5f };
        DrawTriangle(waist, hipR, flapFrontTip, (Color){ 200, 15, 30, 255 });
        
        // Khăn đóng hoặc thắt lưng màu vàng truyền thống
        DrawLineEx((Vector2){ pos.x - 6.0f, pos.y - 0.5f }, (Vector2){ pos.x + 4.0f, pos.y - 1.5f }, 3.5f, GOLD);
        
        // 7. Vẽ quần lụa trắng (white satin pants) dưới tà áo dài
        Vector2 legStart1 = { pos.x - 7.0f, pos.y + 7.0f };
        Vector2 legEnd1 = { pos.x - 10.0f, pos.y + 16.0f };
        Vector2 legStart2 = { pos.x + 1.0f, pos.y + 7.0f };
        Vector2 legEnd2 = { pos.x - 1.0f, pos.y + 15.0f };
        DrawLineEx(legStart1, legEnd1, 4.0f, WHITE);
        DrawLineEx(legStart2, legEnd2, 4.0f, WHITE);
        
        // Giày vải đen truyền thống
        DrawCircleV(legEnd1, 2.5f, (Color){ 30, 30, 30, 255 });
        DrawCircleV(legEnd2, 2.5f, (Color){ 30, 30, 30, 255 });
        
        // 8. Vẽ đầu & mặt nữ pháp sư
        Vector2 faceCenter = { pos.x + 5.0f, pos.y - 20.0f };
        DrawCircleV(faceCenter, 8.0f, (Color){ 255, 219, 172, 255 }); // Da sáng tự nhiên
        
        // Mắt đen láy truyền thống
        DrawCircle(pos.x + 9.0f, pos.y - 21.0f, 1.5f, (Color){ 20, 20, 20, 255 });
        
        // Tóc đen dài truyền thống bay trong gió
        Vector2 hairTip1 = { pos.x - 10.0f, pos.y - 12.0f };
        Vector2 hairTip2 = { pos.x - 14.0f, pos.y - 20.0f };
        DrawTriangle(faceCenter, hairTip1, hairTip2, (Color){ 15, 15, 15, 255 });
        DrawTriangle(faceCenter, hairTip2, (Vector2){ pos.x - 5.0f, pos.y - 25.0f }, (Color){ 30, 30, 30, 255 });
        
        // 9. Vẽ Nón Lá truyền thống (Vietnamese Conical Hat) thay cho nón phù thủy
        Vector2 brimLeft = { pos.x - 12.0f, pos.y - 24.0f };
        Vector2 brimRight = { pos.x + 18.0f, pos.y - 26.0f };
        DrawLineEx(brimLeft, brimRight, 2.5f, (Color){ 230, 200, 140, 255 }); // Màu tre lá của nón lá
        
        // Chóp Nón Lá hình tam giác cân
        Vector2 coneBaseL = { pos.x - 6.0f, pos.y - 25.0f };
        Vector2 coneBaseR = { pos.x + 12.0f, pos.y - 25.5f };
        Vector2 coneTip = { pos.x + 3.0f, pos.y - 36.0f };
        DrawTriangle(coneBaseL, coneTip, coneBaseR, (Color){ 245, 222, 179, 255 }); // Wheat color
        DrawTriangleLines(coneBaseL, coneTip, coneBaseR, (Color){ 180, 150, 90, 255 });
        
        // Dải quai nón màu đỏ thắm
        DrawLineEx(faceCenter, (Vector2){ pos.x + 4.0f, pos.y - 14.0f }, 1.5f, RED);
        
        // 10. Vẽ chi tiết đặc thù cho từng trạng thái chiến đấu dũng mãnh
        if (witch->animState == WITCH_STATE_ATTACK_HAND) {
            // Niệm bùa chú bằng tay: Kết ấn lá bùa thần
            Vector2 handStart = { pos.x + 6.0f, pos.y - 10.0f };
            Vector2 handEnd = { pos.x + 22.0f, pos.y - 12.0f };
            DrawLineEx(handStart, handEnd, 3.0f, (Color){ 255, 219, 172, 255 });
            DrawCircleV(handEnd, 2.5f, (Color){ 255, 219, 172, 255 });
            
            // Vẽ lá bùa màu vàng chữ đỏ (Vietnamese Talisman)
            Rectangle talisman = { handEnd.x + 2.0f, handEnd.y - 8.0f, 8.0f, 16.0f };
            DrawRectangleRec(talisman, GOLD);
            DrawRectangleLinesEx(talisman, 1.0f, ORANGE);
            DrawLineEx((Vector2){ talisman.x + 4.0f, talisman.y + 2.0f }, (Vector2){ talisman.x + 4.0f, talisman.y + 14.0f }, 1.0f, RED); // Chữ bùa đỏ
            
            float magicPulse = 10.0f + sinf(witch->animTimer * 20.0f) * 3.0f;
            DrawCircleLines(handEnd.x + 6.0f, handEnd.y, magicPulse, ColorAlpha(YELLOW, 0.5f));
        } 
        else if (witch->animState == WITCH_STATE_ATTACK_WEAPON) {
            // Tấn công bằng Bút Lông Thần hoặc vương trượng bắn đạn
            Vector2 wandStart = { pos.x + 6.0f, pos.y - 8.0f };
            Vector2 wandEnd = { pos.x + 32.0f, pos.y - 14.0f };
            // Vẽ thân bút gỗ
            DrawLineEx(wandStart, wandEnd, 3.0f, BLACK); 
            // Vẽ ngòi bút lông trắng phát sáng phép thuật đỏ/cam
            DrawTriangle(wandEnd, (Vector2){ wandEnd.x + 4.0f, wandEnd.y - 3.0f }, (Vector2){ wandEnd.x + 8.0f, wandEnd.y }, WHITE);
            float pulse = 7.0f + sinf(witch->animTimer * 25.0f) * 2.0f;
            DrawCircle(wandEnd.x + 4.0f, wandEnd.y, pulse, ColorAlpha(ORANGE, 0.7f));
            DrawCircle(wandEnd.x + 4.0f, wandEnd.y, pulse * 0.4f, WHITE);
        } 
        else if (witch->animState == WITCH_STATE_DEFENSE) {
            // Trận pháp hộ thể bảo vệ quanh người
            float timeVal = (float)GetTime();
            float radiusVal = 100.0f + sinf(timeVal * 7.0f) * 5.0f; // Tăng kích thước khiên lên gấp ~1.8 lần
            
            if (shieldShaderLoaded) {
                BeginShaderMode(shieldShader);
                int timeLoc = GetShaderLocation(shieldShader, "time");
                int glowColorLoc = GetShaderLocation(shieldShader, "glowColor");
                int centerLoc = GetShaderLocation(shieldShader, "center");
                int radiusLoc = GetShaderLocation(shieldShader, "radius");
                
                Vector4 glowCol = { 0.3f, 0.75f, 1.0f, 1.0f }; // Sky blue
                Vector2 centerVal = pos;
                
                SetShaderValue(shieldShader, timeLoc, &timeVal, SHADER_UNIFORM_FLOAT);
                SetShaderValue(shieldShader, glowColorLoc, &glowCol, SHADER_UNIFORM_VEC4);
                SetShaderValue(shieldShader, centerLoc, &centerVal, SHADER_UNIFORM_VEC2);
                SetShaderValue(shieldShader, radiusLoc, &radiusVal, SHADER_UNIFORM_FLOAT);
                
                int screenTexLoc = GetShaderLocation(shieldShader, "screenTexture");
                SetShaderValueTexture(shieldShader, screenTexLoc, shieldBackgroundTexture);
                
                DrawRectangleRec((Rectangle){ pos.x - radiusVal, pos.y - radiusVal, radiusVal * 2.0f, radiusVal * 2.0f }, WHITE);
                EndShaderMode();
            } else {
                // Vẽ khiên chắn vector xoay chuyển động (fallback) rộng rãi và đẹp mắt
                float rot = timeVal * 120.0f;
                DrawCircleGradient(pos.x, pos.y, radiusVal * 1.1f, ColorAlpha(SKYBLUE, 0.12f), ColorAlpha(SKYBLUE, 0.0f));
                DrawCircleLines(pos.x, pos.y, radiusVal, ColorAlpha(SKYBLUE, 0.7f));
                DrawCircleLines(pos.x, pos.y, radiusVal - 1.5f, ColorAlpha(SKYBLUE, 0.3f));
                
                // Vẽ 3 quả cầu hộ mệnh xoay tròn quanh khiên
                for (int j = 0; j < 3; j++) {
                    float a = rot * DEG2RAD + j * (2.0f * PI / 3.0f);
                    float sx = pos.x + cosf(a) * radiusVal;
                    float sy = pos.y + sinf(a) * radiusVal;
                    DrawCircleGradient(sx, sy, 14.0f, ColorAlpha(WHITE, 0.6f), ColorAlpha(SKYBLUE, 0.0f));
                    DrawCircle(sx, sy, 3.5f, WHITE);
                }
            }
        }
    }
}

// Phù Thủy nhận sát thương. Trả về true nếu sống sót, false nếu hết mạng (Game Over)
bool WitchTakeDamage(Witch *witch) {
    if (witch == NULL) return true;
    if (witch->isInvincible) return true;
    
    // Nếu khiên ma pháp đang hoạt động, chặn hoàn toàn sát thương
    if (witch->manaShieldHealth > 0.0f) {
        return true;
    }
    
    // Nếu bị trúng đòn mà có sao hệ Thủy, tự động tiêu hao để kích hoạt khiên ma pháp bảo vệ!
    if (witch->effectiveThuyStars > 0) {
        witch->manaShieldHealth = 1.0f + 0.33f * (witch->skillLevels[SKILL_SHIELD] - 1);
        // Kích hoạt hoạt ảnh phòng thủ
        witch->animState = WITCH_STATE_DEFENSE;
        witch->stateTimer = 0.5f;
        return true; // Chặn sát thương đợt này
    }
    
    // Giảm mạng
    witch->lives--;
    if (witch->lives <= 0) {
        return false;
    }
    
    // Kích hoạt bất tử trong 2.0 giây
    witch->isInvincible = true;
    witch->invincibleTimer = 2.0f;
    return true;
}

// Cấp kỹ năng mới hoặc nâng cấp kỹ năng cho Phù Thủy
bool WitchGrantSkill(Witch *witch, SkillType skill) {
    if (witch == NULL) return false;
    if (skill <= SKILL_NONE || skill >= SKILL_COUNT) return false;
    
    witch->activeSkills[skill] = true;
    witch->skillLevels[skill]++; // Tăng cấp độ kỹ năng khi chọn nâng cấp
    return true;
}

// Lấy tốc độ chạy hiện tại sau khi tính toán các nâng cấp nâng cao và boost vuốt
float WitchGetEffectiveSpeed(const Witch *witch) {
    if (witch == NULL) return PLAYER_BASE_SPEED;
    
    // Tốc độ cơ bản tăng thêm 5% mỗi cấp nâng cấp tốc độ trong Shop
    float speedMultiplier = 1.0f + 0.05f * witch->upgrades.level[UPGRADE_SPEED];
    
    // Tăng x1.5 tốc độ nếu đang trong trạng thái boost vuốt màn hình
    if (witch->swipeBoostTimer > 0.0f) {
        speedMultiplier *= 1.5f;
    }
    
    return PLAYER_BASE_SPEED * speedMultiplier;
}
