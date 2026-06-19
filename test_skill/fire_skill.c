#include "fire_skill.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_PARTICLES 2500
#define MAX_EMITTERS 10

// ---- Hành trình theo thời gian ----
#define FIRE_TRAVEL_SPEED 1.5f
#define FIRE_CAST_DURATION 0.45f
#define FIRE_PROGRESS_MAX 2.5f // Bay vút lên trời

// ---- Đường cong Bezier ----
#define BEZIER_CONTROL1_X_OFFSET 300.0f
#define BEZIER_CONTROL1_Y_OFFSET 200.0f
#define BEZIER_CONTROL2_Y_OFFSET 400.0f

// ---- Tia lửa & Va chạm ----
#define CAST_BURST_COUNT_MIN 20
#define CAST_BURST_COUNT_MAX 35
#define CAST_BURST_VEL_X_MIN -200.0f
#define CAST_BURST_VEL_X_MAX 300.0f
#define CAST_BURST_VEL_Y_MIN -400.0f
#define CAST_BURST_VEL_Y_MAX 100.0f
#define CAST_BURST_RADIUS_MIN 4.0f
#define CAST_BURST_RADIUS_MAX 10.0f
#define CAST_BURST_LIFETIME_MIN 0.3f
#define CAST_BURST_LIFETIME_MAX 0.8f

#define SPLASH_BURST_COUNT_MIN 4
#define SPLASH_BURST_COUNT_MAX 8
#define SPLASH_VEL_X_MIN -400.0f
#define SPLASH_VEL_X_MAX 400.0f
#define SPLASH_VEL_Y_MIN -400.0f
#define SPLASH_VEL_Y_MAX 100.0f
#define SPLASH_RADIUS_MIN 4.0f
#define SPLASH_RADIUS_MAX 12.0f
#define SPLASH_LIFETIME_MIN 0.4f
#define SPLASH_LIFETIME_MAX 1.2f

// ---- Đầu rồng ----
#define DRAGON_HEAD_BASE_SCALE 0.25f
#define DRAGON_JAW_OSCILLATION_SPEED 30.0f
#define DRAGON_JAW_OSCILLATION_AMP 0.15f
#define DRAGON_HEAD_ORIGIN_X_FACTOR 0.2f

static float Math_Mix(float x, float y, float a) {
  return x * (1.0f - a) + y * a;
}
static float Random01(void) {
  return (float)GetRandomValue(0, 10000) / 10000.0f;
}

// =============================================================================
// Struct dữ liệu
// =============================================================================

typedef struct {
  bool active;
  Vector2 startPos;
  Vector2 targetPos;
  Vector2 p1, p2;
  float headProgress;
  float twistPhase;
} FireEmitter;

typedef struct {
  bool active;
  Vector2 position;
  Vector2 velocity;
  float radius;
  float lifetime;
  float maxLifetime;
  int type; // 1: Vệt sợi xé gió (Wind Trails), 2: Tia lửa va chạm (Burst)
} FireParticle;

static FireParticle firePool[MAX_PARTICLES];
static FireEmitter emitters[MAX_EMITTERS];
static int lastUsedParticle = 0;

static RenderTexture2D canvasTexture;
static Shader fireShader;
static int timeLoc;
static Texture2D dragonHeadTex;

// =============================================================================
// Toán học: Đường bay Thăng Long
// =============================================================================

static Vector2 GetBezierPoint(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3,
                              float t) {
  float u = 1.0f - t;
  float tt = t * t;
  float uu = u * u;
  float uuu = uu * u;
  float ttt = tt * t;
  return Vector2Add(Vector2Add(Vector2Add(Vector2Scale(p0, uuu),
                                          Vector2Scale(p1, 3.0f * uu * t)),
                               Vector2Scale(p2, 3.0f * u * tt)),
                    Vector2Scale(p3, ttt));
}

static Vector2 GetBezierTangent(Vector2 p0, Vector2 p1, Vector2 p2,
                                Vector2 target, float t) {
  float u = 1.0f - t;
  Vector2 tangent = Vector2Add(
      Vector2Add(Vector2Scale(Vector2Subtract(p1, p0), 3.0f * u * u),
                 Vector2Scale(Vector2Subtract(p2, p1), 6.0f * u * t)),
      Vector2Scale(Vector2Subtract(target, p2), 3.0f * t * t));
  if (tangent.x == 0 && tangent.y == 0)
    return (Vector2){1.0f, 0.0f};
  return Vector2Normalize(tangent);
}

static Vector2 GetDragonPathPos(Vector2 p0, Vector2 p1, Vector2 p2,
                                Vector2 target, float t) {
  if (t <= 1.0f)
    return GetBezierPoint(p0, p1, p2, target, t);

  float over = t - 1.0f;
  Vector2 vIn = Vector2Scale(Vector2Subtract(target, p2), 3.0f);
  if (Vector2Length(vIn) > 800.0f)
    vIn = Vector2Scale(Vector2Normalize(vIn), 800.0f);

  float upwardSpeed = 1600.0f;
  float waveFreq = 12.0f;
  float waveAmp = 200.0f;

  Vector2 idealUpPos = {target.x + sinf(over * waveFreq) * waveAmp,
                        target.y - upwardSpeed * over};
  Vector2 inertiaPos = {target.x + vIn.x * over, target.y + vIn.y * over};

  float blend = fminf(over * 3.5f, 1.0f);
  blend = blend * blend * (3.0f - 2.0f * blend);

  return (Vector2){Math_Mix(inertiaPos.x, idealUpPos.x, blend),
                   Math_Mix(inertiaPos.y, idealUpPos.y, blend)};
}

static Vector2 GetDragonPathTangent(Vector2 p0, Vector2 p1, Vector2 p2,
                                    Vector2 target, float t) {
  if (t <= 1.0f)
    return GetBezierTangent(p0, p1, p2, target, t);
  float delta = 0.01f;
  Vector2 tangent =
      Vector2Subtract(GetDragonPathPos(p0, p1, p2, target, t + delta),
                      GetDragonPathPos(p0, p1, p2, target, t));
  if (tangent.x == 0 && tangent.y == 0)
    return (Vector2){0.0f, -1.0f};
  return Vector2Normalize(tangent);
}

static void SpawnParticle(int type, Vector2 pos, Vector2 vel, float baseRadius,
                          float maxLife) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    int index = (lastUsedParticle + i) % MAX_PARTICLES;
    if (!firePool[index].active) {
      firePool[index].type = type;
      firePool[index].position = pos;
      firePool[index].velocity = vel;
      firePool[index].radius = baseRadius;
      firePool[index].maxLifetime = maxLife;
      firePool[index].lifetime = maxLife;
      firePool[index].active = true;
      lastUsedParticle = (index + 1) % MAX_PARTICLES;
      break;
    }
  }
}

// =============================================================================
// Core API
// =============================================================================

void InitFireSkill(int screenWidth, int screenHeight) {
  canvasTexture = LoadRenderTexture(screenWidth, screenHeight);
  fireShader = LoadShader(0, "fire.fs");
  timeLoc = GetShaderLocation(fireShader, "u_time");
  dragonHeadTex = LoadTexture("dragon_head.png");

  for (int i = 0; i < MAX_PARTICLES; i++)
    firePool[i].active = false;
  for (int i = 0; i < MAX_EMITTERS; i++)
    emitters[i].active = false;
}

void CastFireSkill(Vector2 startPos, Vector2 target, float twistPhase) {
  for (int i = 0; i < MAX_EMITTERS; i++) {
    if (!emitters[i].active) {
      emitters[i].active = true;
      emitters[i].startPos = startPos;
      emitters[i].targetPos = target;
      emitters[i].headProgress = 0.0f;
      emitters[i].twistPhase = twistPhase;

      float curveSign = (twistPhase == 0.0f) ? -1.0f : 1.0f;
      emitters[i].p1 = (Vector2){startPos.x + (target.x - startPos.x) * 0.5f +
                                     curveSign * BEZIER_CONTROL1_X_OFFSET,
                                 startPos.y - BEZIER_CONTROL1_Y_OFFSET};
      emitters[i].p2 = (Vector2){target.x, target.y + BEZIER_CONTROL2_Y_OFFSET};
      break;
    }
  }

  int burstCount = GetRandomValue(CAST_BURST_COUNT_MIN, CAST_BURST_COUNT_MAX);
  for (int s = 0; s < burstCount; s++) {
    Vector2 vel = {(float)GetRandomValue((int)CAST_BURST_VEL_X_MIN,
                                         (int)CAST_BURST_VEL_X_MAX),
                   (float)GetRandomValue((int)CAST_BURST_VEL_Y_MIN,
                                         (int)CAST_BURST_VEL_Y_MAX)};
    float rad =
        Math_Mix(CAST_BURST_RADIUS_MIN, CAST_BURST_RADIUS_MAX, Random01());
    float life =
        Math_Mix(CAST_BURST_LIFETIME_MIN, CAST_BURST_LIFETIME_MAX, Random01());
    SpawnParticle(2, startPos, vel, rad, life);
  }
}

void UpdateFireSkill(float dt) {
  float time = GetTime();

  for (int e = 0; e < MAX_EMITTERS; e++) {
    if (!emitters[e].active)
      continue;

    float prevProgress = emitters[e].headProgress;
    emitters[e].headProgress += dt * FIRE_TRAVEL_SPEED;

    if (emitters[e].headProgress >= FIRE_PROGRESS_MAX) {
      emitters[e].active = false;
      continue;
    }

    if (prevProgress <= 1.0f && emitters[e].headProgress > 1.0f) {
      int splashCount = GetRandomValue(15, 25);
      for (int s = 0; s < splashCount; s++) {
        Vector2 vel = {
            (float)GetRandomValue((int)SPLASH_VEL_X_MIN, (int)SPLASH_VEL_X_MAX),
            (float)GetRandomValue((int)SPLASH_VEL_Y_MIN,
                                  (int)SPLASH_VEL_Y_MAX)};
        float rad = Math_Mix(SPLASH_RADIUS_MIN, SPLASH_RADIUS_MAX, Random01());
        float life =
            Math_Mix(SPLASH_LIFETIME_MIN, SPLASH_LIFETIME_MAX, Random01());
        SpawnParticle(2, emitters[e].targetPos, vel, rad, life);
      }
    }

    // ===============================================================
    // MA THUẬT: SPAWN CÁC SỢI KHÍ XÉ GIÓ (WIND TRAILS)
    // ===============================================================
    float tailProgress =
        fmaxf(0.0f, emitters[e].headProgress - FIRE_CAST_DURATION);

    // Spawn lượng lớn hạt nhưng sẽ vẽ bằng Dòng Kẻ (Line) mảnh
    int wispsToSpawn = GetRandomValue(35, 65);
    for (int k = 0; k < wispsToSpawn; k++) {
      float t = Math_Mix(tailProgress, emitters[e].headProgress, Random01());

      Vector2 basePos =
          GetDragonPathPos(emitters[e].startPos, emitters[e].p1, emitters[e].p2,
                           emitters[e].targetPos, t);
      Vector2 tangent =
          GetDragonPathTangent(emitters[e].startPos, emitters[e].p1,
                               emitters[e].p2, emitters[e].targetPos, t);
      Vector2 perp = {-tangent.y, tangent.x};

      float wave =
          sinf(t * 18.0f - time * 12.0f + emitters[e].twistPhase) * 20.0f;

      // Lệch ra ngoài rìa thân rồng một chút
      float offset = Math_Mix(-25.0f, 25.0f, Random01());
      Vector2 spawnPos = {basePos.x + perp.x * (wave + offset),
                          basePos.y + perp.y * (wave + offset)};

      // Gió tạt siêu mạnh về phía sau để kéo giãn sợi khói
      Vector2 windVel = {-tangent.x * GetRandomValue(300, 800) +
                             perp.x * GetRandomValue(-150, 150),
                         -tangent.y * GetRandomValue(300, 800) +
                             perp.y * GetRandomValue(-150, 150)};

      // Bán kính giờ đây chính là ĐỘ DÀY CỦA SỢI KHÓI (rất mỏng)
      float rad = Math_Mix(1.0f, 4.0f, Random01());
      float life = Math_Mix(0.2f, 0.6f, Random01());
      SpawnParticle(1, spawnPos, windVel, rad, life);
    }
  }

  // CẬP NHẬT VẬT LÝ
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!firePool[i].active)
      continue;

    firePool[i].lifetime -= dt;
    if (firePool[i].lifetime <= 0.0f) {
      firePool[i].active = false;
      continue;
    }

    firePool[i].velocity.x *= (1.0f - 2.5f * dt);
    firePool[i].velocity.y *= (1.0f - 2.5f * dt);

    // Khói luôn có xu hướng bốc lên trên
    firePool[i].velocity.y -= 180.0f * dt;

    // Turbulance uốn lượn nhẹ
    if (firePool[i].type == 1) {
      float pX = firePool[i].position.x * 0.015f;
      float pY = firePool[i].position.y * 0.015f;
      firePool[i].velocity.x += sinf(pY + time * 6.0f) * 120.0f * dt;
      firePool[i].velocity.y += cosf(pX + time * 6.0f) * 120.0f * dt;
    }

    firePool[i].position.x += firePool[i].velocity.x * dt;
    firePool[i].position.y += firePool[i].velocity.y * dt;
  }
}

void DrawFireSkill(void) {
  float time = GetTime();

  BeginTextureMode(canvasTexture);
  ClearBackground(BLANK);

  BeginBlendMode(BLEND_ADDITIVE);

  // =======================================================
  // 1. VẼ THÂN RỒNG (Trả lại dạng Lửa tự nhiên, bớt khối đặc)
  // =======================================================
  for (int e = 0; e < MAX_EMITTERS; e++) {
    if (!emitters[e].active)
      continue;

    float headT = emitters[e].headProgress;
    float tailT = fmaxf(0.0f, headT - FIRE_CAST_DURATION);

    // Trải 200 điểm dọc thân. Nếu đứt vài chỗ càng giống lửa bùng cháy.
    int bodySegments = 200;
    for (int i = 0; i < bodySegments; i++) {
      float t = Math_Mix(tailT, headT, (float)i / (bodySegments - 1));

      Vector2 basePos =
          GetDragonPathPos(emitters[e].startPos, emitters[e].p1, emitters[e].p2,
                           emitters[e].targetPos, t);
      Vector2 tangent =
          GetDragonPathTangent(emitters[e].startPos, emitters[e].p1,
                               emitters[e].p2, emitters[e].targetPos, t);
      Vector2 perp = {-tangent.y, tangent.x};

      float normDist = Clamp((headT - t) / FIRE_CAST_DURATION, 0.0f, 1.0f);
      float taper = powf(1.0f - normDist, 0.5f);

      float wave = sinf(t * 18.0f - time * 12.0f + emitters[e].twistPhase) *
                   22.0f * taper;

      // Jitter xáo trộn nhẹ để thân rồng không bị thẳng đuột
      float jitterX = sinf(t * 532.1f + time * 20.0f) * 12.0f * taper;
      float jitterY = cosf(t * 921.4f - time * 20.0f) * 12.0f * taper;

      Vector2 drawPos = {basePos.x + perp.x * wave + jitterX,
                         basePos.y + perp.y * wave + jitterY};

      float coreRad = Math_Mix(4.0f, 15.0f, taper);
      float auraRad = Math_Mix(14.0f, 35.0f, taper);

      // Khóa Alpha ở 255. Dùng độ sáng kênh RGB để Shader cộng dồn.
      // Điều này tránh triệt để lỗi "tàng hình" do Shader discard!
      float brightness = (1.0f - normDist) * 0.15f;
      unsigned char coreV = (unsigned char)(255.0f * brightness * 2.0f);
      unsigned char auraV = (unsigned char)(255.0f * brightness * 0.5f);

      Color coreColor = {coreV, coreV, coreV, 255};
      Color auraColor = {auraV, auraV, auraV, 255};
      Color edgeCol = {0, 0, 0, 0}; // Viền đen trong suốt để fade mượt

      DrawCircleGradient((int)drawPos.x, (int)drawPos.y, auraRad, auraColor,
                         edgeCol);
      DrawCircleGradient((int)drawPos.x, (int)drawPos.y, coreRad, coreColor,
                         edgeCol);
    }
  }

  // =======================================================
  // 2. VẼ VỆT SỢI XÉ GIÓ (STREAK LINES)
  // =======================================================
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!firePool[i].active)
      continue;

    float lifeRatio = firePool[i].lifetime / firePool[i].maxLifetime;

    if (firePool[i].type == 1) {
      // MA THUẬT STREAKS: Vẽ đường thẳng (Line) giật lùi theo vận tốc
      Vector2 p1 = firePool[i].position;
      // Stretch factor (0.04f) quyết định độ dài của sợi khí
      Vector2 p2 = {p1.x - firePool[i].velocity.x * 0.04f,
                    p1.y - firePool[i].velocity.y * 0.04f};

      // Tỏa sáng mạnh ở đầu, mờ ở đuôi
      unsigned char intensity = (unsigned char)(255.0f * lifeRatio * 0.8f);
      Color streakColor = {intensity, intensity, intensity, 255};

      DrawLineEx(p1, p2, firePool[i].radius, streakColor);
    } else {
      // Tia lửa nổ
      float rad = firePool[i].radius * lifeRatio;
      unsigned char intensity = (unsigned char)(255.0f * lifeRatio * 0.9f);
      Color sparkColor = {intensity, intensity, intensity, 255};
      Color edgeCol = {0, 0, 0, 0};
      DrawCircleGradient((int)firePool[i].position.x,
                         (int)firePool[i].position.y, rad, sparkColor, edgeCol);
    }
  }

  // =======================================================
  // 3. VẼ ĐẦU RỒNG & KHỚP CỔ
  // =======================================================
  for (int e = 0; e < MAX_EMITTERS; e++) {
    if (!emitters[e].active || emitters[e].headProgress >= FIRE_PROGRESS_MAX)
      continue;

    Vector2 headPos =
        GetDragonPathPos(emitters[e].startPos, emitters[e].p1, emitters[e].p2,
                         emitters[e].targetPos, emitters[e].headProgress);
    Vector2 tangent = GetDragonPathTangent(
        emitters[e].startPos, emitters[e].p1, emitters[e].p2,
        emitters[e].targetPos, emitters[e].headProgress);
    Vector2 perp = {-tangent.y, tangent.x};

    float wave = sinf(emitters[e].headProgress * 18.0f - time * 12.0f +
                      emitters[e].twistPhase) *
                 22.0f;
    headPos = Vector2Add(headPos, Vector2Scale(perp, wave));

    float rotation = atan2f(tangent.y, tangent.x) * RAD2DEG;
    float flipY = (tangent.x < 0.0f) ? -1.0f : 1.0f;
    Rectangle sourceRec = {0.0f, 0.0f, (float)dragonHeadTex.width,
                           (float)dragonHeadTex.height * flipY};

    float jawOscillation =
        sinf(time * DRAGON_JAW_OSCILLATION_SPEED) * DRAGON_JAW_OSCILLATION_AMP;
    float scaleY = DRAGON_HEAD_BASE_SCALE * (1.0f + jawOscillation);

    Rectangle destRec = {headPos.x, headPos.y,
                         dragonHeadTex.width * DRAGON_HEAD_BASE_SCALE,
                         dragonHeadTex.height * scaleY};
    Vector2 origin = {destRec.width * DRAGON_HEAD_ORIGIN_X_FACTOR,
                      destRec.height * 0.5f};

    float headAlpha = 1.0f;
    float fadeStart = FIRE_PROGRESS_MAX - 0.4f;
    if (emitters[e].headProgress > fadeStart) {
      headAlpha = Clamp(1.0f - (emitters[e].headProgress - fadeStart) / 0.4f,
                        0.0f, 1.0f);
    }

    // Cục năng lượng khớp cổ (Nối mạch đầu với thân)
    unsigned char jointV = (unsigned char)(255.0f * headAlpha);
    Color jointCol = {jointV, jointV, jointV, 255};
    Color edgeCol = {0, 0, 0, 0};
    DrawCircleGradient((int)headPos.x, (int)headPos.y, 45.0f * scaleY, jointCol,
                       edgeCol);

    // Vẽ Đầu Rồng trực tiếp bằng ADDITIVE (Lửa ăn khớp mượt mà)
    DrawTexturePro(dragonHeadTex, sourceRec, destRec, origin, rotation,
                   ColorAlpha(WHITE, headAlpha));
  }

  EndBlendMode();
  EndTextureMode();

  SetShaderValue(fireShader, timeLoc, &time, SHADER_UNIFORM_FLOAT);
  BeginShaderMode(fireShader);
  DrawTextureRec(canvasTexture.texture,
                 (Rectangle){0, 0, (float)canvasTexture.texture.width,
                             (float)-canvasTexture.texture.height},
                 (Vector2){0, 0}, WHITE);
  EndShaderMode();
}

void UnloadFireSkill(void) {
  UnloadShader(fireShader);
  UnloadTexture(dragonHeadTex);
  UnloadRenderTexture(canvasTexture);
}