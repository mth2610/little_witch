#include "trail.h"
#include <stddef.h>

// Lấy màu sắc vệt sáng khí động học dựa trên hệ ngũ hành mạnh nhất của Pháp sư
Color GetWitchTrailColor(const Witch *witch) {
    if (witch == NULL) return WHITE;
    
    Color baseColor = CYAN; // Mặc định: Xanh Cyan sáng bóng khí động
    
    int maxStars = 0;
    // Tìm hệ sao ngũ hành có số lượng hiệu dụng cao nhất để đổi màu vệt sáng tương ứng
    if (witch->effectiveHoaStars > maxStars) { baseColor = RED; maxStars = witch->effectiveHoaStars; }
    if (witch->effectiveThuyStars > maxStars) { baseColor = CYAN; maxStars = witch->effectiveThuyStars; }
    if (witch->effectiveMocStars > maxStars) { baseColor = LIME; maxStars = witch->effectiveMocStars; }
    if (witch->effectiveKimStars > maxStars) { baseColor = GOLD; maxStars = witch->effectiveKimStars; }
    if (witch->effectiveThoStars > maxStars) { baseColor = PURPLE; maxStars = witch->effectiveThoStars; }
    
    return baseColor;
}

// Khởi tạo trạng thái ban đầu cho hệ thống vệt khói
void InitWitchTrail(WitchTrail *trail) {
    if (trail == NULL) return;
    
    for (int i = 0; i < MAX_WIND_WISPS; i++) {
        trail->windWisps[i].active = false;
    }
    trail->wispSpawnTimer = 0.0f;
}

// Cập nhật tọa độ và trạng thái các vệt khói theo thời gian thực
void UpdateWitchTrail(WitchTrail *trail, const Witch *witch, float deltaTime, float scrollSpeed) {
    if (trail == NULL || witch == NULL) return;
    
    // Cooldown sinh vệt khói mới
    trail->wispSpawnTimer -= deltaTime;
    if (trail->wispSpawnTimer <= 0.0f) {
        int wispIndex = -1;
        for (int i = 0; i < MAX_WIND_WISPS; i++) {
            if (!trail->windWisps[i].active) {
                wispIndex = i;
                break;
            }
        }
        
        if (wispIndex != -1) {
            WindWisp *wisp = &trail->windWisps[wispIndex];
            wisp->active = true;
            
            // Tăng thời gian sống để vệt khói bay mượt mà lâu hơn
            float baseLife = 0.8f + (float)GetRandomValue(0, 4) / 10.0f; // 0.8s - 1.2s
            wisp->lifetime = baseLife;
            if (witch->swipeBoostTimer > 0.0f) {
                wisp->lifetime *= 1.3f;
            }
            wisp->maxLifetime = wisp->lifetime;
            
            wisp->width = 2.0f + (float)GetRandomValue(0, 10) / 10.0f; // 2.0px - 3.0px
            if (witch->swipeBoostTimer > 0.0f) {
                wisp->width *= 1.4f;
            }
            
            wisp->color = GetWitchTrailColor(witch);
            wisp->detached = false;
            wisp->pointCount = 0;
            wisp->yOffset = (float)GetRandomValue(-25, 22);
            wisp->spawnTimer = 0.0f;
            
            // Giảm tần suất sinh vệt khói để tránh chồng chéo rối mắt
            float spawnCooldown = 0.15f + (float)GetRandomValue(0, 10) / 100.0f; // 0.15s - 0.25s
            if (witch->swipeBoostTimer > 0.0f) {
                spawnCooldown *= 0.6f;
            }
            trail->wispSpawnTimer = spawnCooldown;
        }
    }
    
    // Cập nhật các vệt khói đang hoạt động
    for (int i = 0; i < MAX_WIND_WISPS; i++) {
        if (!trail->windWisps[i].active) continue;
        
        WindWisp *wisp = &trail->windWisps[i];
        wisp->lifetime -= deltaTime;
        if (wisp->lifetime <= 0.0f) {
            wisp->active = false;
            continue;
        }
        
        if (wisp->detached) {
            // Khi đã tách rời, toàn bộ vệt khói trôi theo gió cực kỳ mượt mà
            for (int j = 0; j < wisp->pointCount; j++) {
                wisp->points[j].x -= scrollSpeed * deltaTime;
            }
        } 
        else {
            // Trôi phần đuôi của vệt khói theo gió
            for (int j = 0; j < wisp->pointCount; j++) {
                wisp->points[j].x -= scrollSpeed * deltaTime;
            }
            
            // Thêm điểm mới theo thời gian định kỳ để kéo dài vệt khói mượt mà
            wisp->spawnTimer += deltaTime;
            if (wisp->spawnTimer >= 0.04f) { // 0.04s ~ 2.5 frames
                wisp->spawnTimer = 0.0f;
                
                for (int j = wisp->pointCount; j > 0; j--) {
                    if (j < WISP_POINTS_MAX) {
                        wisp->points[j] = wisp->points[j - 1];
                    }
                }
                
                if (wisp->pointCount < WISP_POINTS_MAX) {
                    wisp->pointCount++;
                } else {
                    wisp->detached = true; // Đạt chiều dài tối đa -> tách rời và bay tự do
                }
            }
            
            // Điểm đầu luôn bám chặt vào vị trí hiện tại của Pháp sư
            float xOffset = witch->facingRight ? -18.0f : 18.0f;
            wisp->points[0] = (Vector2){ witch->position.x + xOffset, witch->position.y + wisp->yOffset };
        }
    }
}

// Vẽ các vệt khói khí động học (Wind Wisps) bay tự do theo gió
void DrawWitchTrail(const WitchTrail *trail) {
    if (trail == NULL) return;
    
    // Vẽ từng vệt khói đang hoạt động
    for (int i = 0; i < MAX_WIND_WISPS; i++) {
        if (!trail->windWisps[i].active || trail->windWisps[i].pointCount < 2) continue;
        
        const WindWisp *wisp = &trail->windWisps[i];
        float lifePct = wisp->lifetime / wisp->maxLifetime;
        
        for (int j = 0; j < wisp->pointCount - 1; j++) {
            float pct1 = (float)j / WISP_POINTS_MAX;
            float pct2 = (float)(j + 1) / WISP_POINTS_MAX;
            
            // Vệt khói phai dần từ đầu đến cuối đuôi và phai dần theo thời gian sống
            float alpha1 = (1.0f - pct1) * lifePct * 0.20f;
            float alpha2 = (1.0f - pct2) * lifePct * 0.20f;
            
            Vector2 p1 = wisp->points[j];
            Vector2 p2 = wisp->points[j + 1];
            
            // Độ dày của vệt khói nhỏ dần về phía đuôi
            float w1 = (1.0f - pct1) * wisp->width;
            
            // Vẽ 2 lượt: 1 lượt quầng sáng mờ rộng (Glow), 1 lượt lõi sáng (Core)
            DrawLineEx(p1, p2, w1 * 4.5f, ColorAlpha(wisp->color, alpha1 * 0.30f)); // Glow
            DrawLineEx(p1, p2, w1, ColorAlpha(WHITE, alpha1 * 0.80f));             // Core
        }
    }
}
