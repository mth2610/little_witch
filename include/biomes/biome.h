#ifndef BIOME_H
#define BIOME_H

#include "raylib.h"
#include "config.h"
#include "enemy.h"
#include "biome_procedural.h"

// Khai báo trước struct GameState
struct GameState;

typedef struct BiomeConfig {
    const char  *name;
    Color        backgroundColor;
    float        scrollSpeed;       // Tốc độ cuộn background (px/s)
    float        enemySpawnRate;    // Tần suất sinh quái (số quái/giây)
    EnemyType    enemyTypes[4];     // Các loại quái xuất hiện trong biome này
    int          enemyTypeCount;
} BiomeConfig;

typedef struct BiomeDefinition {
    BiomeState   type;
    const char  *name;
    Color        backgroundColor;
    float        scrollSpeed;
    float        enemySpawnRate;
    EnemyType    enemyTypes[4];
    int          enemyTypeCount;
    Texture2D   *texture;
    void        (*DrawProceduralBackground)(struct GameState *gs, float rawScroll);
    void        (*DrawProceduralForeground)(struct GameState *gs, float rawScroll);
} BiomeDefinition;

// Lấy thông tin cấu hình của biome hiện tại
BiomeConfig GetBiomeConfig(BiomeState biome);

// Lấy thông tin định nghĩa đầy đủ của biome
const BiomeDefinition* GetBiomeDefinition(BiomeState biome);

// Trả về biome tiếp theo
BiomeState NextBiome(BiomeState current);

// Vẽ và cập nhật background cuộn vô tận
void DrawBackground(struct GameState *gs, float deltaTime);

// Vẽ cận cảnh (Foreground) đè lên nhân vật và quái vật
void DrawForeground(struct GameState *gs, float deltaTime);

#endif // BIOME_H
