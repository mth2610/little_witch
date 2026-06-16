#ifndef BIOME_PROCEDURAL_H
#define BIOME_PROCEDURAL_H

#include "raylib.h"

struct GameState;

// Hàm vẽ nền đồ họa vector thủ thuật cho từng vùng đất
void DrawProceduralBackgroundForest(struct GameState *gs, float rawScroll);
void DrawProceduralBackgroundCave(struct GameState *gs, float rawScroll);
void DrawProceduralBackgroundCity(struct GameState *gs, float rawScroll);
void DrawProceduralBackgroundSpace(struct GameState *gs, float rawScroll);

// Hàm vẽ cận cảnh (foreground) đồ họa vector thủ thuật cho từng vùng đất
void DrawProceduralForegroundForest(struct GameState *gs, float rawScroll);
void DrawProceduralForegroundCave(struct GameState *gs, float rawScroll);
void DrawProceduralForegroundCity(struct GameState *gs, float rawScroll);
void DrawProceduralForegroundSpace(struct GameState *gs, float rawScroll);

#endif // BIOME_PROCEDURAL_H
