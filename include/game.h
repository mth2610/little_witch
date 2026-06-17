#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "config.h"
#include "witch.h"
#include "star.h"
#include "enemy.h"
#include "particle.h"
#include "skill.h"
#include "save.h"
#include "trail.h"

// Cấu trúc GameState toàn cục duy nhất quản lý trạng thái trò chơi
typedef struct GameState {
    GameScreen          currentScreen;
    BiomeState          currentBiome;
    AdState             adState;
    Witch               witch;
    WitchTrail          trail;          // Hệ thống vệt khói khí động học
    StarPool            starPool;
    EnemyPool           enemyPool;
    ParticlePool        particlePool;
    ProjectilePool      projectilePool; // Object Pool cho đạn của phù thủy và quái
    RenderTexture2D     virtualCanvas;  // Canvas ảo 1280x720 cho letterboxing
    RenderTexture2D     lightmap;       // Canvas ánh sáng 2D phong cách Ori
    PermanentUpgrades   savedUpgrades;
    float               bgScrollOffset; // Vị trí cuộn background của biome hiện tại
    int                 frameCount;
    
    // Hệ thống chọn thẻ kỹ năng sau diệt Boss
    SkillType           skillChoices[3];
    bool                bossSpawned; // Đã sinh boss ở Biome hiện tại chưa
    Language            language;    // Ngôn ngữ hiện tại
    
    // Shader cầu vồng (Giai đoạn 5)
    Shader              rainbowShader;
    bool                rainbowLoaded;
    
    // Hệ thống xích sét (Chain Lightning) bằng Shader
    Shader              lightningShader;
    bool                lightningShaderLoaded;
    Vector2             lightningPoints[4];
    int                 lightningPointCount;
    int                 lightningTargets[3];       // Chỉ mục quái bị trúng sét (hoặc -1)
    Vector2             lightningTargetLastPos[3];  // Vị trí cuối cùng đã biết của các quái mục tiêu
    bool                lightningDamageApplied[3]; // Đã kích hoạt sát thương và hiệu ứng nổ chưa
    
    // Hiệu ứng Screen Shake (Giai đoạn 5)
    float               shakeIntensity;
    float               shakeDuration;
    
    // Shader hiệu ứng kỹ năng Ngũ hành (5 hệ)
    Shader              fireballShader;
    bool                fireballShaderLoaded;
    Shader              iceBlastShader;
    bool                iceBlastShaderLoaded;
    Shader              poisonCloudShader;
    bool                poisonCloudShaderLoaded;
    Shader              shurikenShader;
    bool                shurikenShaderLoaded;
    Shader              tornadoShader;
    bool                tornadoShaderLoaded;
    
    // Hệ thống Bloom Pipeline & Screen copy cho Distortion
    RenderTexture2D     screenCopyTex;
    RenderTexture2D     bloomCanvas;
    RenderTexture2D     blurCanvas;
    Shader              bloomExtractShader;
    bool                bloomExtractShaderLoaded;
    Shader              bloomBlurShader;
    bool                bloomBlurShaderLoaded;
    
    // Shader hạt sương giá và texture ảo 2x2 để map tọa độ UV cho các hiệu ứng
    Shader              vaporShader;
    bool                vaporShaderLoaded;
    Texture2D           dummyWhiteTex;
    
    // Cache dữ liệu save tránh đọc file I/O mỗi frame trong hàm render
    SaveData            cachedSave;
    // Điểm số khi bắt đầu biome hiện tại (tính boss threshold tránh boss sinh tức thì)
    int                 biomeStartScore;
    float               ultimateFlashTimer; // Thời gian còn lại của hiệu ứng lóe sáng Ultimate
} GameState;

// Phông chữ vector chất lượng cao toàn cục
extern Font gameFont;

// Hàm chuyển đổi tọa độ chuột/touch thực sang canvas ảo
Vector2 ScreenToVirtual(Vector2 screenPos);

#endif // GAME_H
