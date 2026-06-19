#ifndef SCREEN_H
#define SCREEN_H

#include "config.h"

struct GameState;

typedef struct ScreenDefinition {
    GameScreen type;
    const char *name;
    void (*Init)(struct GameState *gs);
    void (*Update)(struct GameState *gs, float dt);
    void (*Draw)(struct GameState *gs, float dt);
    void (*Unload)(struct GameState *gs);
} ScreenDefinition;

void InitScreenManager(struct GameState *gs);
void UnloadScreenManager(struct GameState *gs);
void SetCurrentScreen(struct GameState *gs, GameScreen type);
void UpdateCurrentScreen(struct GameState *gs, float dt);
void DrawCurrentScreen(struct GameState *gs, float dt);

// Helper to draw the gameplay world
void DrawGameplayWorld(struct GameState *gs, float bgDt);

#endif // SCREEN_H
