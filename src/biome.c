// ============================================================
// biome.c — Triển khai hệ thống Vùng Đất & Cuộn Nền (Biomes Core)
// ============================================================

#include "biome.h"
#include "game.h"
#include "raymath.h"
#include <stddef.h>

// Các texture nền được load từ main.c
extern Texture2D bgForestTexture;
extern Texture2D bgCaveTexture;
extern Texture2D bgCityTexture;
extern Texture2D bgSpaceTexture;

// Lấy thông tin cấu hình của biome hiện tại
BiomeConfig GetBiomeConfig(BiomeState biome) {
    BiomeConfig cfg = {0};
    
    switch (biome) {
        case BIOME_FOREST:
            cfg.name = "Rung Xanh Ky Bi (Forest)";
            cfg.backgroundColor = (Color){ 12, 38, 20, 255 };
            cfg.scrollSpeed = 80.0f;
            cfg.enemySpawnRate = 0.5f;
            cfg.enemyTypes[0] = ENEMY_SLIME;
            cfg.enemyTypes[1] = ENEMY_BAT;
            cfg.enemyTypeCount = 2;
            break;
            
        case BIOME_CAVE:
            cfg.name = "Hang Dong Tinh The (Cave)";
            cfg.backgroundColor = (Color){ 25, 18, 12, 255 };
            cfg.scrollSpeed = 100.0f;
            cfg.enemySpawnRate = 0.65f;
            cfg.enemyTypes[0] = ENEMY_SLIME;
            cfg.enemyTypes[1] = ENEMY_BAT;
            cfg.enemyTypes[2] = ENEMY_GHOST;
            cfg.enemyTypeCount = 3;
            break;
            
        case BIOME_CITY:
            cfg.name = "Thanh Pho Cong Nghe (City)";
            cfg.backgroundColor = (Color){ 10, 15, 28, 255 };
            cfg.scrollSpeed = 130.0f;
            cfg.enemySpawnRate = 0.8f;
            cfg.enemyTypes[0] = ENEMY_ROBOT;
            cfg.enemyTypes[1] = ENEMY_DRONE;
            cfg.enemyTypeCount = 2;
            break;
            
        case BIOME_SPACE:
            cfg.name = "Vu Tru Vo Tan (Space)";
            cfg.backgroundColor = (Color){ 4, 4, 12, 255 };
            cfg.scrollSpeed = 170.0f;
            cfg.enemySpawnRate = 1.0f;
            cfg.enemyTypes[0] = ENEMY_ALIEN;
            cfg.enemyTypes[1] = ENEMY_UFO;
            cfg.enemyTypeCount = 2;
            break;
    }
    
    return cfg;
}

// Trả về biome tiếp theo
BiomeState NextBiome(BiomeState current) {
    if (current == BIOME_FOREST) return BIOME_CAVE;
    if (current == BIOME_CAVE)   return BIOME_CITY;
    if (current == BIOME_CITY)   return BIOME_SPACE;
    return BIOME_SPACE;
}

// Vẽ và cập nhật background cuộn vô tận
void DrawBackground(struct GameState *gs, float deltaTime) {
    if (gs == NULL) return;
    
    BiomeConfig cfg = GetBiomeConfig(gs->currentBiome);
    
    // Cập nhật vị trí cuộn nền liên tục
    gs->bgScrollOffset += cfg.scrollSpeed * deltaTime;
    if (gs->bgScrollOffset > VIRTUAL_WIDTH * 100.0f) {
        gs->bgScrollOffset = fmodf(gs->bgScrollOffset, VIRTUAL_WIDTH * 100.0f);
    }
    
    // Xác định texture nền tương ứng
    Texture2D tex;
    bool hasTexture = false;
    
    switch (gs->currentBiome) {
        case BIOME_FOREST:
            if (IsTextureReady(bgForestTexture)) { tex = bgForestTexture; hasTexture = true; }
            break;
        case BIOME_CAVE:
            if (IsTextureReady(bgCaveTexture)) { tex = bgCaveTexture; hasTexture = true; }
            break;
        case BIOME_CITY:
            if (IsTextureReady(bgCityTexture)) { tex = bgCityTexture; hasTexture = true; }
            break;
        case BIOME_SPACE:
            if (IsTextureReady(bgSpaceTexture)) { tex = bgSpaceTexture; hasTexture = true; }
            break;
    }
    
    if (hasTexture) {
        // Vẽ cuộn nền vô tận sử dụng 2 ảnh kề nhau tịnh tiến sang trái
        float scrollX = fmodf(gs->bgScrollOffset, VIRTUAL_WIDTH);
        
        Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
        Rectangle destRec1 = { -scrollX, 0.0f, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
        Rectangle destRec2 = { VIRTUAL_WIDTH - scrollX, 0.0f, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
        
        DrawTexturePro(tex, sourceRec, destRec1, (Vector2){0,0}, 0.0f, WHITE);
        DrawTexturePro(tex, sourceRec, destRec2, (Vector2){0,0}, 0.0f, WHITE);
    } else {
        // Tách rời toàn bộ logic vẽ thủ thuật sang biome_procedural.c
        float rawScroll = gs->bgScrollOffset;
        
        switch (gs->currentBiome) {
            case BIOME_FOREST:
                DrawProceduralBackgroundForest(gs, rawScroll);
                break;
            case BIOME_CAVE:
                DrawProceduralBackgroundCave(gs, rawScroll);
                break;
            case BIOME_CITY:
                DrawProceduralBackgroundCity(gs, rawScroll);
                break;
            case BIOME_SPACE:
                DrawProceduralBackgroundSpace(gs, rawScroll);
                break;
        }
    }
}

// Vẽ cận cảnh (Foreground) đè lên nhân vật và quái vật
void DrawForeground(struct GameState *gs, float deltaTime) {
    if (gs == NULL) return;
    
    float rawScroll = gs->bgScrollOffset;
    
    switch (gs->currentBiome) {
        case BIOME_FOREST:
            DrawProceduralForegroundForest(gs, rawScroll);
            break;
        case BIOME_CAVE:
            DrawProceduralForegroundCave(gs, rawScroll);
            break;
        case BIOME_CITY:
            DrawProceduralForegroundCity(gs, rawScroll);
            break;
        case BIOME_SPACE:
            DrawProceduralForegroundSpace(gs, rawScroll);
            break;
    }
}
