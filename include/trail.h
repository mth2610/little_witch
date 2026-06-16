#ifndef TRAIL_H
#define TRAIL_H

#include "raylib.h"
#include "witch.h"

#define MAX_WIND_WISPS 16
#define WISP_POINTS_MAX 16

// Cấu trúc đại diện cho một vệt khói/luồng khí khí động học độc lập
typedef struct WindWisp {
    bool    active;
    Vector2 points[WISP_POINTS_MAX];
    int     pointCount;
    float   lifetime;
    float   maxLifetime;
    Color   color;
    float   width;
    float   spawnTimer;
    bool    detached;
    float   yOffset;
} WindWisp;

// Cấu trúc quản lý pool các vệt khói khí động học
typedef struct WitchTrail {
    WindWisp windWisps[MAX_WIND_WISPS];
    float    wispSpawnTimer;
} WitchTrail;

// Các hàm điều khiển vệt khói toàn cục
void InitWitchTrail(WitchTrail *trail);
void UpdateWitchTrail(WitchTrail *trail, const Witch *witch, float deltaTime, float scrollSpeed);
void DrawWitchTrail(const WitchTrail *trail);
Color GetWitchTrailColor(const Witch *witch);

#endif // TRAIL_H
