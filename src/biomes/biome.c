// ============================================================
// biome.c — Triển khai hệ thống Vùng Đất & Cuộn Nền (Biomes Core)
// ============================================================

#include "biome.h"
#include "game.h"
#include "raymath.h"
#include <stddef.h>

// Các texture nền được load từ main.c / resource_loader
extern Texture2D bgForestTexture;
extern Texture2D bgCaveTexture;
extern Texture2D bgCityTexture;
extern Texture2D bgSpaceTexture;

// Registry chứa cấu hình và hàm vẽ cho 4 Biome
static const BiomeDefinition biomeRegistry[4] = {
    [BIOME_FOREST] = {
        .type = BIOME_FOREST,
        .name = "Rung Xanh Ky Bi (Forest)",
        .backgroundColor = (Color){ 12, 38, 20, 255 },
        .scrollSpeed = 80.0f,
        .enemySpawnRate = 0.5f,
        .enemyTypes = { ENEMY_SLIME, ENEMY_BAT },
        .enemyTypeCount = 2,
        .texture = &bgForestTexture,
        .DrawProceduralBackground = DrawProceduralBackgroundForest,
        .DrawProceduralForeground = DrawProceduralForegroundForest
    },
    [BIOME_CAVE] = {
        .type = BIOME_CAVE,
        .name = "Hang Dong Tinh The (Cave)",
        .backgroundColor = (Color){ 25, 18, 12, 255 },
        .scrollSpeed = 100.0f,
        .enemySpawnRate = 0.65f,
        .enemyTypes = { ENEMY_SLIME, ENEMY_BAT, ENEMY_GHOST },
        .enemyTypeCount = 3,
        .texture = &bgCaveTexture,
        .DrawProceduralBackground = DrawProceduralBackgroundCave,
        .DrawProceduralForeground = DrawProceduralForegroundCave
    },
    [BIOME_CITY] = {
        .type = BIOME_CITY,
        .name = "Thanh Pho Cong Nghe (City)",
        .backgroundColor = (Color){ 10, 15, 28, 255 },
        .scrollSpeed = 130.0f,
        .enemySpawnRate = 0.8f,
        .enemyTypes = { ENEMY_ROBOT, ENEMY_DRONE },
        .enemyTypeCount = 2,
        .texture = &bgCityTexture,
        .DrawProceduralBackground = DrawProceduralBackgroundCity,
        .DrawProceduralForeground = DrawProceduralForegroundCity
    },
    [BIOME_SPACE] = {
        .type = BIOME_SPACE,
        .name = "Vu Tru Vo Tan (Space)",
        .backgroundColor = (Color){ 4, 4, 12, 255 },
        .scrollSpeed = 170.0f,
        .enemySpawnRate = 1.0f,
        .enemyTypes = { ENEMY_ALIEN, ENEMY_UFO },
        .enemyTypeCount = 2,
        .texture = &bgSpaceTexture,
        .DrawProceduralBackground = DrawProceduralBackgroundSpace,
        .DrawProceduralForeground = DrawProceduralForegroundSpace
    }
};

const BiomeDefinition* GetBiomeDefinition(BiomeState biome) {
    if (biome >= 0 && biome < 4) {
        return &biomeRegistry[biome];
    }
    return NULL;
}

// Lấy thông tin cấu hình của biome hiện tại
BiomeConfig GetBiomeConfig(BiomeState biome) {
    BiomeConfig cfg = {0};
    const BiomeDefinition *def = GetBiomeDefinition(biome);
    if (def != NULL) {
        cfg.name = def->name;
        cfg.backgroundColor = def->backgroundColor;
        cfg.scrollSpeed = def->scrollSpeed;
        cfg.enemySpawnRate = def->enemySpawnRate;
        cfg.enemyTypeCount = def->enemyTypeCount;
        for (int i = 0; i < def->enemyTypeCount; i++) {
            cfg.enemyTypes[i] = def->enemyTypes[i];
        }
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
    
    const BiomeDefinition *def = GetBiomeDefinition(gs->currentBiome);
    if (def == NULL) return;
    
    // Cập nhật vị trí cuộn nền liên tục
    gs->bgScrollOffset += def->scrollSpeed * deltaTime;
    if (gs->bgScrollOffset > VIRTUAL_WIDTH * 100.0f) {
        gs->bgScrollOffset = fmodf(gs->bgScrollOffset, VIRTUAL_WIDTH * 100.0f);
    }
    
    if (def->texture != NULL && IsTextureReady(*(def->texture))) {
        Texture2D tex = *(def->texture);
        // Vẽ cuộn nền vô tận sử dụng 2 ảnh kề nhau tịnh tiến sang trái
        float scrollX = fmodf(gs->bgScrollOffset, VIRTUAL_WIDTH);
        
        Rectangle sourceRec = { 0.0f, 0.0f, (float)tex.width, (float)tex.height };
        Rectangle destRec1 = { -scrollX, 0.0f, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
        Rectangle destRec2 = { VIRTUAL_WIDTH - scrollX, 0.0f, VIRTUAL_WIDTH, VIRTUAL_HEIGHT };
        
        DrawTexturePro(tex, sourceRec, destRec1, (Vector2){0,0}, 0.0f, WHITE);
        DrawTexturePro(tex, sourceRec, destRec2, (Vector2){0,0}, 0.0f, WHITE);
    } else {
        if (def->DrawProceduralBackground != NULL) {
            def->DrawProceduralBackground(gs, gs->bgScrollOffset);
        }
    }
}

// Vẽ cận cảnh (Foreground) đè lên nhân vật và quái vật
void DrawForeground(struct GameState *gs, float deltaTime) {
    if (gs == NULL) return;
    
    const BiomeDefinition *def = GetBiomeDefinition(gs->currentBiome);
    if (def != NULL && def->DrawProceduralForeground != NULL) {
        def->DrawProceduralForeground(gs, gs->bgScrollOffset);
    }
}
