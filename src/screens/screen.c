#include "screen.h"
#include "game.h"
#include "biome.h"
#include "star.h"
#include "enemy.h"
#include "skill.h"
#include "particle.h"
#include "trail.h"
#include "witch.h"
#include "lightmap.h"
#include "ui.h"
#include "gameplay.h"
#include <stddef.h>

// Quản lý chuyển cảnh tự động ở cấp độ khung hình
static GameScreen lastScreen = -1;

// ----------------------------------------------------------------
// CÁC HÀM CALLBACK CHO TỪNG MÀN HÌNH
// ----------------------------------------------------------------

// 1. Màn hình Menu
static void UpdateMenuScreen(GameState *gs, float dt) {
    UpdateMenu(gs, dt);
}
static void DrawMenuScreen(GameState *gs, float dt) {
    DrawBackground(gs, dt * 0.2f);
    DrawMenu(gs);
}

// 2. Màn hình Gameplay
static void UpdateGameplayScreen(GameState *gs, float dt) {
    UpdateGameplay(gs, dt);
}
static void DrawGameplayScreen(GameState *gs, float dt) {
    DrawGameplayWorld(gs, dt);
}

// 3. Màn hình chọn Kỹ năng
static void UpdateSkillSelectScreen(GameState *gs, float dt) {
    UpdateSkillSelect(gs);
}
static void DrawSkillSelectScreen(GameState *gs, float dt) {
    DrawGameplayWorld(gs, 0.0f);
    DrawSkillSelect(gs);
}

// 4. Màn hình Cửa hàng nâng cấp
static void UpdateUpgradeShopScreen(GameState *gs, float dt) {
    UpdateUpgradeShop(gs);
}
static void DrawUpgradeShopScreen(GameState *gs, float dt) {
    DrawBackground(gs, dt * 0.2f);
    DrawUpgradeShop(gs);
}

// 5. Màn hình Game Over
static void UpdateGameOverScreen(GameState *gs, float dt) {
    UpdateGameOver(gs);
}
static void DrawGameOverScreen(GameState *gs, float dt) {
    DrawGameplayWorld(gs, dt);
    DrawGameOver(gs);
}

// Registry định nghĩa các màn hình
static ScreenDefinition screenRegistry[5] = {
    [SCREEN_MENU] = {
        .type = SCREEN_MENU,
        .name = "Menu",
        .Init = NULL,
        .Update = UpdateMenuScreen,
        .Draw = DrawMenuScreen,
        .Unload = NULL
    },
    [SCREEN_GAMEPLAY] = {
        .type = SCREEN_GAMEPLAY,
        .name = "Gameplay",
        .Init = NULL,
        .Update = UpdateGameplayScreen,
        .Draw = DrawGameplayScreen,
        .Unload = NULL
    },
    [SCREEN_SKILL_SELECT] = {
        .type = SCREEN_SKILL_SELECT,
        .name = "Skill Select",
        .Init = NULL,
        .Update = UpdateSkillSelectScreen,
        .Draw = DrawSkillSelectScreen,
        .Unload = NULL
    },
    [SCREEN_UPGRADE_SHOP] = {
        .type = SCREEN_UPGRADE_SHOP,
        .name = "Upgrade Shop",
        .Init = NULL,
        .Update = UpdateUpgradeShopScreen,
        .Draw = DrawUpgradeShopScreen,
        .Unload = NULL
    },
    [SCREEN_GAMEOVER] = {
        .type = SCREEN_GAMEOVER,
        .name = "Game Over",
        .Init = NULL,
        .Update = UpdateGameOverScreen,
        .Draw = DrawGameOverScreen,
        .Unload = NULL
    }
};

static const ScreenDefinition* GetScreenDefinition(GameScreen type) {
    if (type >= 0 && type < 5) {
        return &screenRegistry[type];
    }
    return NULL;
}

// ----------------------------------------------------------------
// HÀM QUẢN LÝ MÀN HÌNH CHÍNH (SCREEN MANAGER)
// ----------------------------------------------------------------

void InitScreenManager(struct GameState *gs) {
    lastScreen = -1;
}

void UnloadScreenManager(struct GameState *gs) {
    const ScreenDefinition *def = GetScreenDefinition(gs->currentScreen);
    if (def != NULL && def->Unload != NULL) {
        def->Unload(gs);
    }
    lastScreen = -1;
}

void SetCurrentScreen(struct GameState *gs, GameScreen type) {
    // Để giữ tính tương thích ngược, hàm này cập nhật gs->currentScreen.
    // Việc gọi Init/Unload sẽ được tự động xử lý trong UpdateCurrentScreen ở khung hình tiếp theo.
    gs->currentScreen = type;
}

void UpdateCurrentScreen(struct GameState *gs, float dt) {
    if (gs == NULL) return;

    // Tự động phát hiện chuyển màn hình
    if (gs->currentScreen != lastScreen) {
        const ScreenDefinition *oldDef = GetScreenDefinition(lastScreen);
        if (oldDef != NULL && oldDef->Unload != NULL) {
            oldDef->Unload(gs);
        }

        lastScreen = gs->currentScreen;

        const ScreenDefinition *newDef = GetScreenDefinition(gs->currentScreen);
        if (newDef != NULL && newDef->Init != NULL) {
            newDef->Init(gs);
        }
    }

    const ScreenDefinition *def = GetScreenDefinition(gs->currentScreen);
    if (def != NULL && def->Update != NULL) {
        def->Update(gs, dt);
    }
}

void DrawCurrentScreen(struct GameState *gs, float dt) {
    if (gs == NULL) return;

    const ScreenDefinition *def = GetScreenDefinition(gs->currentScreen);
    if (def != NULL && def->Draw != NULL) {
        def->Draw(gs, dt);
    }
}

// Vẽ toàn bộ thế giới gameplay bao gồm Lightmap và Bloom
void DrawGameplayWorld(struct GameState *gs, float bgDt) {
    if (gs == NULL) return;

    // --- BƯỚC 1: Vẽ thế giới Game bình thường ---
    DrawBackground(gs, bgDt);
    DrawStars(&gs->starPool, gs->witch.position);
    DrawEnemies(&gs->enemyPool);

    // Chụp màn hình hiện tại cho Distortion
    EndTextureMode();
    BeginTextureMode(gs->screenCopyTex);
    ClearBackground(BLACK);
    DrawTextureRec(
        gs->virtualCanvas.texture,
        (Rectangle){0.0f, 0.0f, (float)VIRTUAL_WIDTH, -(float)VIRTUAL_HEIGHT},
        (Vector2){0, 0}, WHITE);
    EndTextureMode();
    
    // Tiếp tục vẽ vào canvas ảo
    BeginTextureMode(gs->virtualCanvas);

    extern Texture2D shieldBackgroundTexture;
    shieldBackgroundTexture = gs->screenCopyTex.texture;

    DrawProjectiles(&gs->projectilePool, gs);
    DrawParticlesEx(&gs->particlePool, gs);
    DrawWitchTrail(&gs->trail);
    DrawWitch(&gs->witch);

    // Vẽ hiệu ứng của kỹ năng chủ động (ví dụ: Chain Lightning)
    DrawSkills(gs);

    DrawForeground(gs, bgDt);

    // Vẽ hiệu ứng chớp sáng trắng Ultimate
    if (gs->ultimateFlashTimer > 0.0f) {
        float flashAlpha = (gs->ultimateFlashTimer / 0.20f) * 0.45f;
        DrawRectangle(0, 0, VIRTUAL_WIDTH, VIRTUAL_HEIGHT, ColorAlpha(WHITE, flashAlpha));
    }

    // --- BƯỚC 2: Vẽ bản đồ ánh sáng 2D (Lightmap) ---
    EndTextureMode();
    RenderLightmap(gs);

    // --- BƯỚC 3: Áp dụng bản đồ ánh sáng (Multiply Blend) ---
    ApplyLightmap(gs);

    // --- BƯỚC 3.5: Áp dụng Bloom Pipeline ---
    ApplyBloomPipeline(gs);

    // --- BƯỚC 4: Vẽ HUD lên trên cùng ---
    DrawHUD(gs);
}
