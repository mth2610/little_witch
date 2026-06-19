#ifndef LIGHTMAP_H
#define LIGHTMAP_H

#include "raylib.h"

struct GameState;

// Vẽ bản đồ ánh sáng 2D (Lightmap) phong cách Ori
void RenderLightmap(struct GameState *gs);

// Áp dụng lightmap lên virtual canvas (Multiply Blend)
void ApplyLightmap(struct GameState *gs);

// Áp dụng Bloom Pipeline (extract → blur H → blur V → additive composite)
void ApplyBloomPipeline(struct GameState *gs);

// Scale và vẽ virtual canvas ra màn hình thật (Letterboxing + Screen Shake)
void DrawVirtualCanvasToScreen(struct GameState *gs);

// Chuyển đổi tọa độ màn hình thật sang màn hình ảo 1280x720
Vector2 ScreenToVirtual(Vector2 screenPos);

#endif // LIGHTMAP_H
