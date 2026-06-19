// ============================================================
// star.c — Hệ thống Ngôi Sao (Stars & Orbiting Stars)
// ============================================================

#include "star.h"
#include "witch.h"
#include "particle.h"
#include "game.h"
#include "raymath.h"
#include <stdlib.h>

// Các texture được load từ main.c
extern Texture2D starRainbowTexture;

// Âm thanh thu thập được load từ main.c
extern Sound collectStarSound;

// ----------------------------------------------------------------
// HÀM CÔNG KHAI
// ----------------------------------------------------------------

// Khởi tạo pool quản lý Ngôi Sao
void InitStarPool(StarPool *pool) {
    if (pool == NULL) return;
    
    for (int i = 0; i < MAX_STARS; i++) {
        pool->stars[i].active = false;
        pool->stars[i].isOrbiting = false;
    }
    pool->orbitingCount = 0;
    pool->spawnTimer = 0.0f;
    pool->spawnInterval = 3.0f; // Mặc định 3 giây spawn 1 lần
}

// Sinh một Ngôi Sao mới tại tọa độ x, y
int SpawnStar(StarPool *pool, float x, float y, StarType type) {
    if (pool == NULL) return -1;
    
    // Tìm slot trống trong pool
    int index = -1;
    for (int i = 0; i < MAX_STARS; i++) {
        if (!pool->stars[i].active) {
            index = i;
            break;
        }
    }
    
    // Pool đầy, bỏ qua
    if (index == -1) return -1;
    
    Star *s = &pool->stars[index];
    s->active = true;
    s->isOrbiting = false;
    s->type = type;
    s->position = (Vector2){ x, y };
    s->spawnPosition = s->position;
    s->angle = 0.0f;
    s->orbitRadius = STAR_ORBIT_RADIUS;
    s->orbitSpeed = STAR_ORBIT_SPEED;
    s->bobTimer = ((float)rand() / RAND_MAX) * 10.0f; // Bắt đầu lệch pha nhịp lắc lư
    s->bobOffset = 0.0f;
    s->isBeingAttracted = false;
    s->animTimer = 0.0f;
    s->scale = 1.0f;
    s->orbitTiltAlpha = 0.0f;
    s->orbitTiltBeta = 0.0f;
    s->orbitDepth = 0.0f;
    
    // Đặt màu mặc định theo loại sao ngũ hành
    if (type == STAR_KIM) {
        s->tintColor = (Color){ 230, 230, 250, 255 }; // Trắng bạc lấp lánh
    } else if (type == STAR_MOC) {
        s->tintColor = (Color){ 0, 230, 100, 255 };   // Xanh lá cây tự nhiên
    } else if (type == STAR_THUY) {
        s->tintColor = (Color){ 0, 180, 255, 255 };   // Xanh dương tươi mát
    } else if (type == STAR_HOA) {
        s->tintColor = (Color){ 255, 80, 0, 255 };    // Đỏ lửa / Cam rực rỡ
    } else if (type == STAR_THO) {
        s->tintColor = (Color){ 240, 180, 50, 255 };  // Vàng sáng rực
    } else if (type == STAR_RAINBOW) {
        s->tintColor = PINK; // Nhấp nháy cầu vồng khi vẽ
    }
    
    return index;
}

// Cập nhật tọa độ, kiểm tra va chạm thu thập và cập nhật quỹ đạo xoay
void UpdateStars(StarPool *pool, Witch *witch,
                  float deltaTime, bool hasMagnet, float scrollSpeed, ParticlePool *partPool) {
    if (pool == NULL || witch == NULL) return;
    
    Vector2 witchPos = witch->position;
    float witchRadius = witch->collisionRadius;
    
    for (int i = 0; i < MAX_STARS; i++) {
        if (!pool->stars[i].active) continue;
        
        Star *s = &pool->stars[i];
        s->animTimer += deltaTime;
        
        // Sinh hạt bụi ma thuật lấp lánh (star dust sparkles) bay nhẹ nhàng quanh sao làm vệt sáng (trail)
        if (partPool != NULL && GetRandomValue(0, 100) < 25) { // Tăng lên 25% để trail dày dặn
            float size = 1.2f + ((float)rand() / RAND_MAX) * 2.0f;
            Vector2 vel = { 0 };
            if (!s->isOrbiting) {
                // Sao tự do trôi: hạt trail trôi chậm lại theo nền
                vel.x = -scrollSpeed * 0.4f;
            } else {
                // Sao xoay quỹ đạo: hạt trail đứng yên tại chỗ và tự phân rã khi sao bay đi
                vel = (Vector2){ 0.0f, 0.0f };
            }
            float lifetime = 0.3f + ((float)rand() / RAND_MAX) * 0.3f;
            SpawnParticle(partPool, s->position, vel, lifetime, size, s->tintColor);
        }
        
        if (!s->isOrbiting) {
            // SAO TỰ DO (CHƯA THU THẬP)
            
            // Nam châm hút sao bị động hệ Kim: Luôn có lực hút cơ bản, tăng theo nâng cấp shop và sao Kim
            float magnetRadius = 85.0f;
            float shopBonus = witch->upgrades.level[UPGRADE_MAGNET_BASE] * 35.0f;
            float kimBonus = witch->effectiveKimStars * 25.0f;
            magnetRadius += shopBonus + kimBonus;
            
            float pullSpeed = 320.0f + witch->effectiveKimStars * 40.0f;
            
            // Nam châm chủ động cực mạnh (khi kích hoạt active skill SKILL_MAGNET)
            if (witch->activeSkills[SKILL_MAGNET] && witch->skillActiveTimer[SKILL_MAGNET] > 0.0f) {
                magnetRadius = 900.0f; // Hút toàn màn hình
                pullSpeed = 750.0f + witch->effectiveKimStars * 50.0f;
            }
            
            if (magnetRadius > 0.0f) {
                float dist = Vector2Distance(s->position, witchPos);
                if (dist < magnetRadius) {
                    s->isBeingAttracted = true;
                }
            }
            
            if (s->isBeingAttracted) {
                Vector2 dir = Vector2Normalize(Vector2Subtract(witchPos, s->position));
                s->position = Vector2Add(s->position, Vector2Scale(dir, pullSpeed * deltaTime));
            } else {
                // Cuộn từ phải sang trái theo tốc độ nền cuộn (scrollSpeed)
                s->position.x -= scrollSpeed * deltaTime;
                s->spawnPosition.x -= scrollSpeed * deltaTime;
                
                // Hiệu ứng lắc lư (bobbing) nhẹ nhàng lên xuống khi đứng yên
                s->bobTimer += deltaTime;
                s->position.y = s->spawnPosition.y + sinf(s->bobTimer * 3.0f) * 6.0f;
            }
            
            // Tự động hủy nếu trôi ra rìa ngoài bên trái màn hình ảo
            if (s->position.x < -60.0f) {
                s->active = false;
                continue;
            }
            
            // Kiểm tra va chạm để thu thập sao
            float distToWitch = Vector2Distance(s->position, witchPos);
            if (distToWitch < witchRadius + STAR_COLLISION_RADIUS) {
                s->isOrbiting = true;
                s->isBeingAttracted = false;
                
                // Đặt góc ban đầu trong quỹ đạo dựa trên số sao hiện có
                s->angle = (2.0f * PI / (pool->orbitingCount + 1)) * pool->orbitingCount;
                
                // Tự do hóa góc nghiêng và bán kính để tạo chiều sâu 3D tuyệt đẹp
                // Nghiêng so với màn hình (X-rotation) từ 30 đến 60 độ
                s->orbitTiltAlpha = (30.0f + ((float)rand() / RAND_MAX) * 30.0f) * DEG2RAD;
                // Xoay nghiêng trên màn hình (Z-rotation) từ 0 đến 360 độ
                s->orbitTiltBeta = (((float)rand() / RAND_MAX) * 360.0f) * DEG2RAD;
                
                float baseRadius = STAR_ORBIT_RADIUS;
                float baseSpeed = STAR_ORBIT_SPEED;
                
                // Nếu người chơi có sao hệ Mộc (Mộc sinh bão sao xoay rộng hơn)
                if (witch->effectiveMocStars > 0) {
                    baseRadius *= (1.0f + 0.12f * witch->effectiveMocStars);
                    baseSpeed *= (1.0f + 0.15f * witch->effectiveMocStars);
                }
                
                // Bán kính ngẫu nhiên phân tán từ 75% đến 120% của bán kính cơ sở
                s->orbitRadius = baseRadius * (0.75f + ((float)rand() / RAND_MAX) * 0.45f);
                
                // Tốc độ xoay ngẫu nhiên từ 80% đến 120%
                s->orbitSpeed = baseSpeed * (0.8f + ((float)rand() / RAND_MAX) * 0.4f);
                if (((float)rand() / RAND_MAX) > 0.5f) {
                    s->orbitSpeed = -s->orbitSpeed; // Có thể xoay ngược chiều
                }
                s->orbitDepth = 0.0f;
                
                pool->orbitingCount++;
                
                // Kích hoạt hoạt ảnh tấn công tay không/nhặt sao
                witch->animState = WITCH_STATE_ATTACK_HAND;
                witch->stateTimer = 0.3f;
                
                // Phát âm thanh thu thập sao
                if (IsSoundReady(collectStarSound)) {
                    PlaySound(collectStarSound);
                }
                
                // Sắp xếp lại góc của toàn bộ các sao đang quỹ đạo để chia đều khoảng cách ban đầu
                int orbitIdx = 0;
                for (int j = 0; j < MAX_STARS; j++) {
                    if (pool->stars[j].active && pool->stars[j].isOrbiting) {
                        pool->stars[j].angle = (2.0f * PI / pool->orbitingCount) * orbitIdx;
                        orbitIdx++;
                    }
                }
            }
        } else {
            // SAO XOAY QUỸ ĐẠO BẢO VỆ PHÙ THỦY (DỰA TRÊN QUỸ ĐẠO 3D CHIẾU 2D)
            // Tốc độ xoay và bán kính tăng nhẹ theo số lượng sao Mộc
            float speedMult = 1.0f;
            float radiusMult = 1.0f;
            if (witch->effectiveMocStars > 0) {
                speedMult = 1.0f + 0.15f * witch->effectiveMocStars;
                radiusMult = 1.0f + 0.12f * witch->effectiveMocStars;
            }
            
            s->angle += s->orbitSpeed * speedMult * deltaTime;
            if (s->angle >= 2.0f * PI) s->angle -= 2.0f * PI;
            if (s->angle < 0.0f) s->angle += 2.0f * PI;
            
            // Tọa độ phẳng 2D gốc trên mặt phẳng quay
            float x0 = cosf(s->angle) * (s->orbitRadius * radiusMult);
            float y0 = sinf(s->angle) * (s->orbitRadius * radiusMult);
            
            // Xoay nghiêng theo trục X (độ phẳng dẹt quỹ đạo)
            float x1 = x0;
            float y1 = y0 * cosf(s->orbitTiltAlpha);
            float z1 = y0 * sinf(s->orbitTiltAlpha); // Tọa độ chiều sâu Z
            
            // Xoay theo trục Z (góc nghiêng chéo trên màn hình)
            float x2 = x1 * cosf(s->orbitTiltBeta) - y1 * sinf(s->orbitTiltBeta);
            float y2 = x1 * sinf(s->orbitTiltBeta) + y1 * cosf(s->orbitTiltBeta);
            
            s->position.x = witchPos.x + x2;
            s->position.y = witchPos.y + y2;
            s->orbitDepth = z1; // Lưu lại chiều sâu phục vụ vẽ xa gần
        }
    }
    
    // Cập nhật số sao ngũ hành hiệu dụng vào struct Witch
    UpdateEffectiveStarCounts(pool, witch);
}

// Hàm vẽ ngôi sao dạng Quả cầu ánh sáng linh hồn tối giản, siêu mượt (Ori-Style Spirit Light Orb)
static void DrawVectorStar(Vector2 position, float outerRadius, float innerRadius, float rotation, Color color) {
    float alphaFactor = (float)color.a / 255.0f;
    float coreAlpha = 0.35f + 0.65f * alphaFactor; // Giới hạn alpha tối thiểu để sao ở xa không bị tối xỉn
    
    // Vẽ quả cầu ánh sáng đa lớp bằng Blend Additive và Gradient tròn mềm (Ori-style Spirit Orb)
    BeginBlendMode(BLEND_ADDITIVE);
        // Lớp 1: Quầng sáng rộng ngoài cùng (Màu của ngôi sao: Vàng/Hồng/Trắng)
        DrawCircleGradient(position.x, position.y, outerRadius * 2.8f, ColorAlpha(color, 0.35f * alphaFactor), ColorAlpha(color, 0.0f));
        
        // Lớp 2: Vầng sáng trung tâm đậm đặc hơn
        DrawCircleGradient(position.x, position.y, outerRadius * 1.5f, ColorAlpha(color, 0.65f * alphaFactor), ColorAlpha(color, 0.0f));
        
        // Lớp 3: Lõi sáng trắng rực rỡ ở tâm
        DrawCircleGradient(position.x, position.y, outerRadius * 0.75f, ColorAlpha(WHITE, 0.85f * coreAlpha), ColorAlpha(WHITE, 0.0f));
        
        // Lớp 4: Điểm tâm sáng trắng đặc cực mạnh
        DrawCircleV(position, outerRadius * 0.32f, ColorAlpha(WHITE, coreAlpha));
    EndBlendMode();
}

// Vẽ toàn bộ ngôi sao
void DrawStars(const StarPool *pool, Vector2 witchPos) {
    if (pool == NULL) return;
    
    for (int i = 0; i < MAX_STARS; i++) {
        if (!pool->stars[i].active) continue;
        
        const Star *s = &pool->stars[i];
        Texture2D tex;
        bool hasTexture = false;
        
        if (s->type == STAR_RAINBOW && IsTextureReady(starRainbowTexture)) {
            tex = starRainbowTexture;
            hasTexture = true;
        }
        
        if (hasTexture) {
            // Vẽ texture ngôi sao xoay vòng
            Vector2 origin = { (float)tex.width / 2.0f, (float)tex.height / 2.0f };
            Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
            
            float depthScale = 1.0f;
            float alpha = 1.0f;
            if (s->isOrbiting) {
                // Điều chỉnh tỉ lệ và độ mờ dựa trên chiều sâu Z trong 3D
                depthScale = 1.0f + 0.35f * (s->orbitDepth / s->orbitRadius);
                alpha = 0.5f + 0.5f * ((s->orbitDepth + s->orbitRadius) / (2.0f * s->orbitRadius));
            }
            
            Rectangle destRec = { s->position.x, s->position.y, (float)tex.width * s->scale * depthScale, (float)tex.height * s->scale * depthScale };
            
            // Xoay hình ảnh ngôi sao theo thời gian hoạt họa
            float rotation = s->animTimer * 120.0f; 
            
            DrawTexturePro(tex, sourceRec, destRec, origin, rotation, Fade(WHITE, alpha));
        } else {
            // Vẽ ngôi sao dạng Vector với hiệu ứng chiều sâu 3D và kích thước tinh chỉnh gọn đẹp
            Color color = s->tintColor;
            if (s->type == STAR_RAINBOW) {
                // Nhấp nháy cầu vồng bằng cách thay đổi màu theo thời gian
                float t = s->animTimer * 5.0f;
                color = ColorFromHSV(fmodf(t * 60.0f, 360.0f), 0.8f, 1.0f);
            }
            
            float depthScale = 1.0f;
            float alpha = 1.0f;
            
            if (s->isOrbiting) {
                // Ngôi sao xoay quanh nhân vật: Scale nhỏ/lớn và mờ/tỏ theo trục Z
                depthScale = 1.0f + 0.32f * (s->orbitDepth / s->orbitRadius);
                alpha = 0.45f + 0.55f * ((s->orbitDepth + s->orbitRadius) / (2.0f * s->orbitRadius));
            }
            
            // Vẽ ngôi sao 5 cánh lấp lánh tự động xoay theo animTimer
            float rot = s->animTimer * 100.0f;
            
            // Bán kính cơ sở nhỏ hơn theo ý kiến đóng góp của người chơi (gọn gàng, tinh tế)
            float baseOuter = s->isOrbiting ? (STAR_COLLISION_RADIUS * 0.65f) : (STAR_COLLISION_RADIUS * 0.7f);
            float baseInner = s->isOrbiting ? (STAR_COLLISION_RADIUS * 0.22f) : (STAR_COLLISION_RADIUS * 0.25f);
            
            float outerR = baseOuter * depthScale;
            float innerR = baseInner * depthScale;
            
            DrawVectorStar(s->position, outerR, innerR, rot, Fade(color, alpha));
        }
    }
}

// Áp dụng kỹ năng Star Storm (Nhân đôi bán kính hiện tại, x2.5 tốc độ xoay hiện tại)
void ApplyStarStormEffect(StarPool *pool) {
    if (pool == NULL) return;
    
    for (int i = 0; i < MAX_STARS; i++) {
        if (pool->stars[i].active && pool->stars[i].isOrbiting) {
            pool->stars[i].orbitRadius *= 2.0f;
            pool->stars[i].orbitSpeed *= 2.5f;
        }
    }
}

// Lấy con trỏ đến ngôi sao đang xoay theo index thực tế
Star* GetOrbitingStar(StarPool *pool, int orbitIndex) {
    if (pool == NULL) return NULL;
    
    int count = 0;
    for (int i = 0; i < MAX_STARS; i++) {
        if (pool->stars[i].active && pool->stars[i].isOrbiting) {
            if (count == orbitIndex) {
                return &pool->stars[i];
            }
            count++;
        }
    }
    return NULL;
}

// Vô hiệu hóa ngôi sao (khi va chạm quái và biến mất)
void DeactivateStar(StarPool *pool, int starIndex) {
    if (pool == NULL || starIndex < 0 || starIndex >= MAX_STARS) return;
    
    if (pool->stars[starIndex].active) {
        bool wasOrbiting = pool->stars[starIndex].isOrbiting;
        pool->stars[starIndex].active = false;
        pool->stars[starIndex].isOrbiting = false;
        
        if (wasOrbiting) {
            pool->orbitingCount--;
            if (pool->orbitingCount < 0) pool->orbitingCount = 0;
            
            // Sắp xếp phân bố lại góc của các sao đang xoay còn lại
            if (pool->orbitingCount > 0) {
                int orbitIdx = 0;
                for (int i = 0; i < MAX_STARS; i++) {
                    if (pool->stars[i].active && pool->stars[i].isOrbiting) {
                        pool->stars[i].angle = (2.0f * PI / pool->orbitingCount) * orbitIdx;
                        orbitIdx++;
                    }
                }
            }
        }
    }
}

// Cập nhật số sao ngũ hành hiệu dụng vào struct Witch mỗi frame
void UpdateEffectiveStarCounts(StarPool *pool, Witch *witch) {
    if (pool == NULL || witch == NULL) return;
    
    int kim = 0;
    int moc = 0;
    int thuy = 0;
    int hoa = 0;
    int tho = 0;
    int rainbow = 0;
    
    for (int i = 0; i < MAX_STARS; i++) {
        if (pool->stars[i].active && pool->stars[i].isOrbiting) {
            if (pool->stars[i].type == STAR_KIM) kim++;
            else if (pool->stars[i].type == STAR_MOC) moc++;
            else if (pool->stars[i].type == STAR_THUY) thuy++;
            else if (pool->stars[i].type == STAR_HOA) hoa++;
            else if (pool->stars[i].type == STAR_THO) tho++;
            else if (pool->stars[i].type == STAR_RAINBOW) rainbow++;
        }
    }
    
    // Sao ngũ sắc (Rainbow) được tính cộng thêm vào cho tất cả các hệ
    // Thêm +2 sao cơ sở cho tất cả các hệ để test ngay các chiêu thức mạnh cấp 2 từ đầu (yêu cầu của user)
    witch->effectiveKimStars = kim + rainbow + 2;
    witch->effectiveMocStars = moc + rainbow + 2;
    witch->effectiveThuyStars = thuy + rainbow + 2;
    witch->effectiveHoaStars = hoa + rainbow + 2;
    witch->effectiveThoStars = tho + rainbow + 2;
}
