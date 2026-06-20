#include "fire_skill.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_PARTICLES 2500
#define MAX_EMITTERS 10

// ---- Hành trình theo thời gian ----
#define FIRE_TRAVEL_SPEED 0.9f
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

static float GetAmplitudeEnvelope(float distFromHead, float totalLength) {
  if (totalLength <= 0.0f) return 0.0f;
  float norm = distFromHead / totalLength;
  if (norm < 0.15f) {
    float t = norm / 0.15f;
    return Math_Mix(0.25f, 1.0f, t);
  } else if (norm < 0.8f) {
    return 1.0f;
  } else {
    float t = (norm - 0.8f) / 0.2f;
    return Math_Mix(1.0f, 0.0f, t);
  }
}

// Lấy các điểm cách đều nhau một khoảng cố định dọc theo đường đi (path)
static int SamplePath(const Vector2* path, int pathCount, float spacing, Vector2* outSegments, int maxSegments) {
  if (pathCount == 0 || maxSegments <= 0) return 0;

  outSegments[0] = path[0];
  int segmentIndex = 1;
  float targetDist = spacing;
  float accumulatedDist = 0.0f;

  for (int i = 0; i < pathCount - 1; i++) {
    Vector2 p1 = path[i];
    Vector2 p2 = path[i + 1];
    float d = Vector2Distance(p1, p2);

    while (accumulatedDist + d >= targetDist) {
      float needed = targetDist - accumulatedDist;
      float t = (d > 0.0f) ? (needed / d) : 0.0f;
      Vector2 interpPos = Vector2Lerp(p1, p2, t);

      outSegments[segmentIndex] = interpPos;
      segmentIndex++;
      if (segmentIndex >= maxSegments) {
        return segmentIndex;
      }
      targetDist += spacing;
    }
    accumulatedDist += d;
  }

  return segmentIndex;
}

// =============================================================================
// Struct dữ liệu
// =============================================================================

#define EMITTER_PATH_MAX 500
#define MAX_SAMPLED_SEGMENTS 150

typedef struct {
  bool active;
  Vector2 startPos;
  Vector2 targetPos;
  Vector2 p1, p2;
  float headProgress;
  float twistPhase;
  Vector2 path[EMITTER_PATH_MAX];
  int pathCount;
  Vector2 sampledPath[MAX_SAMPLED_SEGMENTS];
  int sampledCount;
} FireEmitter;

#define PARTICLE_HISTORY_COUNT 6

typedef struct {
  bool active;
  Vector2 position;
  Vector2 velocity;
  float radius;
  float lifetime;
  float maxLifetime;
  int type; // 1: Wind Trails, 2: Spark Bursts, 3: Floating Embers, 4: Fire Vortex, 5: Fire Breath, 6: Shockwave Ring, 7: Ambient Light Flash
  Vector2 history[PARTICLE_HISTORY_COUNT];
  int historyCount;
  float angle;
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

static Vector2 GetTexturePointInWorld(Vector2 headPos, Vector2 tangent, float scaleX, float scaleY, Vector2 origin, Vector2 texPt, float flipY) {
  float lx = texPt.x * DRAGON_HEAD_BASE_SCALE - origin.x;
  float ly = (texPt.y * scaleY - origin.y) * flipY;
  Vector2 perp = {-tangent.y, tangent.x};
  Vector2 worldPos = {
    headPos.x + tangent.x * lx + perp.x * ly,
    headPos.y + tangent.y * lx + perp.y * ly
  };
  return worldPos;
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
      firePool[index].historyCount = 0;
      for (int h = 0; h < PARTICLE_HISTORY_COUNT; h++) {
        firePool[index].history[h] = pos;
      }
      firePool[index].angle = Random01() * 3.14159265f * 2.0f;
      lastUsedParticle = (index + 1) % MAX_PARTICLES;
      break;
    }
  }
}

static void TriggerFireImpact(Vector2 pos) {
  // 1. Tia lửa phát tán nhanh (Radial Spark Burst)
  int sparkCount = GetRandomValue(30, 45);
  for (int s = 0; s < sparkCount; s++) {
    float angle = Random01() * 3.14159265f * 2.0f;
    float speed = Math_Mix(250.0f, 650.0f, Random01());
    Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed };
    float rad = Math_Mix(2.0f, 5.5f, Random01());
    float life = Math_Mix(0.3f, 0.7f, Random01());
    SpawnParticle(2, pos, vel, rad, life);
  }

  // 2. Hạt bụi lửa tán ra rồi bốc lên theo gió (Dispersing Noise Particles)
  int disperseCount = GetRandomValue(40, 50);
  for (int v = 0; v < disperseCount; v++) {
    float angle = Random01() * 3.14159265f * 2.0f;
    float speed = Math_Mix(120.0f, 400.0f, Random01());
    Vector2 vel = { cosf(angle) * speed, sinf(angle) * speed - 120.0f }; // Tán rộng ra và hơi bay lên
    float rad = Math_Mix(6.0f, 15.0f, Random01());
    float life = Math_Mix(0.6f, 1.3f, Random01());
    SpawnParticle(4, pos, vel, rad, life);
  }

  // 3. Vòng sóng kích nổ (Fiery Shockwave Rings - Type 6)
  SpawnParticle(6, pos, (Vector2){0, 0}, 130.0f, 0.40f); // Vòng ngoài
  SpawnParticle(6, pos, (Vector2){0, 0}, 80.0f, 0.25f);  // Vòng trong

  // 4. Ánh chớp phát sáng môi trường (Ambient Light Flash - Type 7)
  SpawnParticle(7, pos, (Vector2){0, 0}, 260.0f, 0.25f);
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
  float phases[2] = { twistPhase, twistPhase + PI };
  for (int p = 0; p < 2; p++) {
    for (int i = 0; i < MAX_EMITTERS; i++) {
      if (!emitters[i].active) {
        emitters[i].active = true;
        emitters[i].startPos = startPos;
        emitters[i].targetPos = target;
        emitters[i].headProgress = 0.0f;
        emitters[i].twistPhase = phases[p];
        emitters[i].pathCount = 1;
        emitters[i].path[0] = startPos;

        float curveSign = (phases[p] == 0.0f || phases[p] == 2.0f * PI) ? -1.0f : 1.0f;
        if (p == 1) curveSign = -curveSign;
        
        emitters[i].p1 = (Vector2){startPos.x + (target.x - startPos.x) * 0.5f +
                                       curveSign * BEZIER_CONTROL1_X_OFFSET,
                                   startPos.y - BEZIER_CONTROL1_Y_OFFSET};
        emitters[i].p2 = (Vector2){target.x, target.y + BEZIER_CONTROL2_Y_OFFSET};
        break;
      }
    }
  }

  // Casting heat flash (Type 7)
  SpawnParticle(7, startPos, (Vector2){0, 0}, 150.0f, 0.30f);

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
    float targetProgress = prevProgress + dt * FIRE_TRAVEL_SPEED;
    if (targetProgress >= FIRE_PROGRESS_MAX) {
      targetProgress = FIRE_PROGRESS_MAX;
    }

    // Sub-stepping to record a smooth straight path
    float step = 0.005f;
    float currentProgress = prevProgress;
    while (currentProgress < targetProgress) {
      currentProgress += step;
      if (currentProgress > targetProgress) currentProgress = targetProgress;

      Vector2 pos = GetDragonPathPos(emitters[e].startPos, emitters[e].p1, emitters[e].p2,
                                     emitters[e].targetPos, currentProgress);

      float dist = 0.0f;
      if (emitters[e].pathCount > 0) {
        dist = Vector2Distance(pos, emitters[e].path[0]);
      } else {
        dist = Vector2Distance(pos, emitters[e].startPos);
      }

      if (dist > 1.5f || emitters[e].pathCount == 0) {
        for (int h = EMITTER_PATH_MAX - 1; h > 0; h--) {
          emitters[e].path[h] = emitters[e].path[h - 1];
        }
        emitters[e].path[0] = pos;
        if (emitters[e].pathCount < EMITTER_PATH_MAX) {
          emitters[e].pathCount++;
        }
      }
    }
    emitters[e].headProgress = targetProgress;

    // If progress exceeded max, deactivate
    if (emitters[e].headProgress >= FIRE_PROGRESS_MAX) {
      emitters[e].active = false;
      continue;
    }

    // Sample path at 6.0f spacing for drawing and particle spawning
    emitters[e].sampledCount = SamplePath(emitters[e].path, emitters[e].pathCount,
                                          6.0f, emitters[e].sampledPath, MAX_SAMPLED_SEGMENTS);

    if (prevProgress <= 1.0f && emitters[e].headProgress > 1.0f) {
      TriggerFireImpact(emitters[e].targetPos);
    }

    // Determine current head properties for visuals
    Vector2 headPos = (emitters[e].pathCount > 0) ? emitters[e].path[0] : emitters[e].startPos;
    Vector2 tangent = GetDragonPathTangent(emitters[e].startPos, emitters[e].p1, emitters[e].p2,
                                           emitters[e].targetPos, emitters[e].headProgress);
    Vector2 perp = {-tangent.y, tangent.x};

    // Calculate visual head position (head wiggle envelope is 0.25)
    float headWaveVal = sinf(time * 16.0f + emitters[e].twistPhase) * (22.0f * 0.25f);
    Vector2 visualHeadPos = Vector2Add(headPos, Vector2Scale(perp, headWaveVal));

    if (emitters[e].headProgress <= 1.0f) {
      float jawOsc = sinf(time * DRAGON_JAW_OSCILLATION_SPEED) * DRAGON_JAW_OSCILLATION_AMP;
      float scaleY = DRAGON_HEAD_BASE_SCALE * (1.0f + jawOsc);
      float flipY = (tangent.x < 0.0f) ? -1.0f : 1.0f;
      Vector2 origin = { dragonHeadTex.width * DRAGON_HEAD_BASE_SCALE * DRAGON_HEAD_ORIGIN_X_FACTOR,
                        dragonHeadTex.height * scaleY * 0.5f };
      Vector2 mouthTexPt = { 460.0f, 260.0f };
      Vector2 mouthWorldPos = GetTexturePointInWorld(visualHeadPos, tangent, DRAGON_HEAD_BASE_SCALE, scaleY, origin, mouthTexPt, flipY);

      int breathCount = GetRandomValue(2, 4);
      for (int b = 0; b < breathCount; b++) {
        float angleOffset = (float)GetRandomValue(-20, 20) * DEG2RAD;
        Vector2 dir = {
            tangent.x * cosf(angleOffset) - tangent.y * sinf(angleOffset),
            tangent.x * sinf(angleOffset) + tangent.y * cosf(angleOffset)
        };
        float speed = (float)GetRandomValue(450, 850);
        Vector2 vel = Vector2Scale(dir, speed);
        float rad = Math_Mix(2.0f, 5.5f, Random01());
        float life = Math_Mix(0.15f, 0.45f, Random01());
        SpawnParticle(5, mouthWorldPos, vel, rad, life);
      }
    }

    // Spawn wind trails and embers along the sampled path
    if (emitters[e].sampledCount > 1) {
      // Wisps (Type 1) - Spiraling around the body
      int wispsToSpawn = GetRandomValue(25, 45);
      for (int k = 0; k < wispsToSpawn; k++) {
        int idx = GetRandomValue(0, emitters[e].sampledCount - 1);
        Vector2 purePos = emitters[e].sampledPath[idx];

        Vector2 pureTangent;
        if (idx < emitters[e].sampledCount - 1)
          pureTangent = Vector2Normalize(Vector2Subtract(purePos, emitters[e].sampledPath[idx + 1]));
        else
          pureTangent = Vector2Normalize(Vector2Subtract(emitters[e].sampledPath[idx - 1], purePos));

        Vector2 purePerp = {-pureTangent.y, pureTangent.x};
        float distFromHead = idx * 6.0f;

        // Core radius at this segment (approximate matching of the core size)
        float normDist = (emitters[e].sampledCount > 1) ? ((float)idx / (float)(emitters[e].sampledCount - 1)) : 0.0f;
        float taper = powf(1.0f - normDist, 0.5f);
        float coreRad = Math_Mix(2.0f, 10.0f, taper);

        // Spiral phase matching the single helix ribbon but with random offset/variety
        float spiralPhase = time * 18.0f - distFromHead * 0.05f + emitters[e].twistPhase + Random01() * 0.2f;
        float coilRad = coreRad * 1.8f;

        Vector2 spawnPos = {
          purePos.x + purePerp.x * cosf(spiralPhase) * coilRad,
          purePos.y + purePerp.y * cosf(spiralPhase) * coilRad
        };

        // Add minor randomness
        spawnPos.x += Math_Mix(-5.0f, 5.0f, Random01());
        spawnPos.y += Math_Mix(-5.0f, 5.0f, Random01());

        // Velocity vector: drift back, orbit, and expand slowly
        float cosVal = cosf(spiralPhase);
        float sinVal = sinf(spiralPhase);
        Vector2 radDir = { purePerp.x * cosVal, purePerp.y * cosVal };
        Vector2 rotDir = { -purePerp.x * sinVal, -purePerp.y * sinVal };

        float forwardSpeed = Math_Mix(200.0f, 400.0f, Random01());
        float orbitSpeed = 180.0f;
        float expandSpeed = 25.0f;

        Vector2 windVel = {
          -pureTangent.x * forwardSpeed + rotDir.x * orbitSpeed + radDir.x * expandSpeed,
          -pureTangent.y * forwardSpeed + rotDir.y * orbitSpeed + radDir.y * expandSpeed
        };

        float rad = Math_Mix(2.0f, 4.5f, Random01());
        float life = Math_Mix(0.3f, 0.6f, Random01());
        SpawnParticle(1, spawnPos, windVel, rad, life);
      }

      // Embers (Type 3) - Also spiraling but slightly wider and slower
      int embersToSpawn = GetRandomValue(3, 5);
      for (int k = 0; k < embersToSpawn; k++) {
        int idx = GetRandomValue(0, emitters[e].sampledCount - 1);
        Vector2 purePos = emitters[e].sampledPath[idx];

        Vector2 pureTangent;
        if (idx < emitters[e].sampledCount - 1)
          pureTangent = Vector2Normalize(Vector2Subtract(purePos, emitters[e].sampledPath[idx + 1]));
        else
          pureTangent = Vector2Normalize(Vector2Subtract(emitters[e].sampledPath[idx - 1], purePos));

        Vector2 purePerp = {-pureTangent.y, pureTangent.x};
        float distFromHead = idx * 6.0f;
        float normDist = (emitters[e].sampledCount > 1) ? ((float)idx / (float)(emitters[e].sampledCount - 1)) : 0.0f;
        float taper = powf(1.0f - normDist, 0.5f);
        float coreRad = Math_Mix(2.0f, 10.0f, taper);

        float spiralPhase = time * 18.0f - distFromHead * 0.05f + emitters[e].twistPhase + Random01() * PI;
        float coilRad = coreRad * 2.2f; // slightly wider than Type 1

        Vector2 spawnPos = {
          purePos.x + purePerp.x * cosf(spiralPhase) * coilRad,
          purePos.y + purePerp.y * cosf(spiralPhase) * coilRad
        };

        // Add minor randomness
        spawnPos.x += Math_Mix(-6.0f, 6.0f, Random01());
        spawnPos.y += Math_Mix(-6.0f, 6.0f, Random01());

        float cosVal = cosf(spiralPhase);
        float sinVal = sinf(spiralPhase);
        Vector2 radDir = { purePerp.x * cosVal, purePerp.y * cosVal };
        Vector2 rotDir = { -purePerp.x * sinVal, -purePerp.y * sinVal };

        float forwardSpeed = Math_Mix(120.0f, 250.0f, Random01());
        float orbitSpeed = 120.0f;
        float expandSpeed = 35.0f;

        Vector2 vel = {
          -pureTangent.x * forwardSpeed + rotDir.x * orbitSpeed + radDir.x * expandSpeed,
          -pureTangent.y * forwardSpeed + rotDir.y * orbitSpeed + radDir.y * expandSpeed
        };
        float rad = Math_Mix(3.0f, 7.5f, Random01());
        float life = Math_Mix(0.7f, 1.4f, Random01());
        SpawnParticle(3, spawnPos, vel, rad, life);
      }
    }
  }

  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!firePool[i].active)
      continue;

    firePool[i].lifetime -= dt;
    if (firePool[i].lifetime <= 0.0f) {
      firePool[i].active = false;
      continue;
    }

    for (int h = PARTICLE_HISTORY_COUNT - 1; h > 0; h--) {
      firePool[i].history[h] = firePool[i].history[h - 1];
    }
    firePool[i].history[0] = firePool[i].position;
    if (firePool[i].historyCount < PARTICLE_HISTORY_COUNT) {
      firePool[i].historyCount++;
    }

    firePool[i].velocity.x *= (1.0f - 2.5f * dt);
    firePool[i].velocity.y *= (1.0f - 2.5f * dt);

    firePool[i].velocity.y -= 180.0f * dt;

    if (firePool[i].type == 1) {
      float pX = firePool[i].position.x * 0.015f;
      float pY = firePool[i].position.y * 0.015f;
      firePool[i].velocity.x += sinf(pY + time * 8.0f) * 160.0f * dt;
      firePool[i].velocity.y += cosf(pX + time * 8.0f) * 160.0f * dt;
    }
    else if (firePool[i].type == 3) {
      firePool[i].angle += dt * 6.0f;
      firePool[i].velocity.x += sinf(firePool[i].angle) * 90.0f * dt;
      firePool[i].velocity.y -= 120.0f * dt;
    }
    else if (firePool[i].type == 4) {
      firePool[i].velocity.x *= (1.0f - 3.5f * dt);
      firePool[i].velocity.y *= (1.0f - 2.0f * dt);
      firePool[i].velocity.y -= 260.0f * dt;
      float pX = firePool[i].position.x * 0.02f;
      float pY = firePool[i].position.y * 0.02f;
      firePool[i].velocity.x += sinf(pY + time * 10.0f) * 180.0f * dt;
      firePool[i].velocity.y += cosf(pX + time * 10.0f) * 180.0f * dt;
    }
    else if (firePool[i].type == 5) {
      firePool[i].velocity.x *= (1.0f - 4.5f * dt);
      firePool[i].velocity.y *= (1.0f - 4.5f * dt);
      firePool[i].velocity.y -= 90.0f * dt;
    }
    else if (firePool[i].type == 7) {
      firePool[i].velocity = (Vector2){0, 0};
    }

    firePool[i].position.x += firePool[i].velocity.x * dt;
    firePool[i].position.y += firePool[i].velocity.y * dt;
  }
}

void DrawFireSkill(void) {
  float time = GetTime();

  BeginTextureMode(canvasTexture);
  ClearBackground(BLANK);

  // =======================================================
  // 1. VẼ KHÓI MỜ BỌC ÔM SÁT THÂN RỒNG (BLEND_ALPHA)
  // =======================================================
  for (int e = 0; e < MAX_EMITTERS; e++) {
    if (!emitters[e].active)
      continue;

    int bodySegments = emitters[e].sampledCount;
    if (bodySegments == 0) continue;

    BeginBlendMode(BLEND_ALPHA);
    for (int i = 0; i < bodySegments; i++) {
      Vector2 purePos_i = emitters[e].sampledPath[i];
      Vector2 pureTangent;
      if (i < bodySegments - 1)
        pureTangent = Vector2Normalize(Vector2Subtract(purePos_i, emitters[e].sampledPath[i + 1]));
      else if (i > 0)
        pureTangent = Vector2Normalize(Vector2Subtract(emitters[e].sampledPath[i - 1], purePos_i));
      else
        pureTangent = (Vector2){1, 0};

      Vector2 purePerp = {-pureTangent.y, pureTangent.x};
      float distFromHead = (float)i * 6.0f;
      float envelope = GetAmplitudeEnvelope(distFromHead, bodySegments * 6.0f);
      float waveVal = sinf(time * 16.0f - distFromHead * 0.018f + emitters[e].twistPhase) * (22.0f * envelope);
      Vector2 drawPos = Vector2Add(purePos_i, Vector2Scale(purePerp, waveVal));

      float normDist = (bodySegments > 1) ? ((float)i / (float)(bodySegments - 1)) : 0.0f;
      float taper = powf(1.0f - normDist, 0.5f);

      float smokeRad = Math_Mix(12.0f, 30.0f, taper);
      unsigned char smokeAlpha = (unsigned char)(255.0f * taper * 0.35f);
      DrawCircleGradient((int)drawPos.x, (int)drawPos.y, smokeRad,
                         (Color){70, 30, 30, smokeAlpha}, BLANK);
    }
    EndBlendMode();
  }

  // =======================================================
  // 2. VẼ VỆT SỢI XÉ GIÓ VÀ CÁC HẠT (BLEND_ADDITIVE)
  // =======================================================
  BeginBlendMode(BLEND_ADDITIVE);
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!firePool[i].active)
      continue;

    float lifeRatio = firePool[i].lifetime / firePool[i].maxLifetime;
    Color edgeCol = {0, 0, 0, 0};

    if (firePool[i].type == 1) {
      float rad = firePool[i].radius * (0.5f + 0.5f * lifeRatio);
      unsigned char intensity = (unsigned char)(255.0f * lifeRatio * 0.45f);
      Color wispCol = {intensity, (unsigned char)(intensity * 0.5f), (unsigned char)(intensity * 0.15f), 255};
      DrawCircleGradient((int)firePool[i].position.x, (int)firePool[i].position.y, rad * 2.5f, wispCol, edgeCol);
    } else if (firePool[i].type == 2) {
      float rad = firePool[i].radius * lifeRatio;
      unsigned char intensity = (unsigned char)(255.0f * lifeRatio * 0.9f);
      Color sparkColor = {intensity, intensity, intensity, 255};
      DrawCircleGradient((int)firePool[i].position.x,
                         (int)firePool[i].position.y, rad * 2.0f, sparkColor, edgeCol);
    } else if (firePool[i].type == 3) {
      float rad = firePool[i].radius * (0.4f + 0.6f * lifeRatio);
      unsigned char intensity = (unsigned char)(255.0f * lifeRatio * 0.8f);
      Color emberCol = {intensity, (unsigned char)(intensity * 0.9f), (unsigned char)(intensity * 0.3f), 255};
      DrawCircleGradient((int)firePool[i].position.x, (int)firePool[i].position.y, rad * 2.5f, emberCol, edgeCol);
      DrawCircle((int)firePool[i].position.x, (int)firePool[i].position.y, rad * 0.5f, (Color){intensity, intensity, intensity, 255});
    } else if (firePool[i].type == 4) {
      float rad = firePool[i].radius * (1.1f - 0.3f * lifeRatio) * lifeRatio;
      unsigned char intensity = (unsigned char)(255.0f * lifeRatio * 0.85f);
      Color vortexCol = {intensity, (unsigned char)(intensity * 0.4f), (unsigned char)(intensity * 0.1f), 255};
      DrawCircleGradient((int)firePool[i].position.x, (int)firePool[i].position.y, rad * 2.8f, vortexCol, edgeCol);
      DrawCircle((int)firePool[i].position.x, (int)firePool[i].position.y, rad * 0.6f, (Color){intensity, (unsigned char)(intensity * 0.8f), (unsigned char)(intensity * 0.5f), 255});
    } else if (firePool[i].type == 5) {
      float rad = firePool[i].radius * lifeRatio;
      unsigned char intensity = (unsigned char)(255.0f * lifeRatio * 0.95f);
      Color breathCol = {intensity, (unsigned char)(intensity * 0.6f), (unsigned char)(intensity * 0.2f), 255};
      DrawCircleGradient((int)firePool[i].position.x, (int)firePool[i].position.y, rad * 2.2f, breathCol, edgeCol);
    } else if (firePool[i].type == 6) {
      float progress = 1.0f - lifeRatio;
      float outerRad = firePool[i].radius * progress;
      float innerRad = outerRad * 0.70f;
      unsigned char intensity = (unsigned char)(255.0f * lifeRatio * 0.85f);
      Color ringCol = {intensity, intensity, intensity, 255};
      DrawRing(firePool[i].position, innerRad, outerRad, 0.0f, 360.0f, 32, ringCol);
    } else if (firePool[i].type == 7) {
      float progress = 1.0f - lifeRatio;
      float rad = firePool[i].radius * (0.6f + 0.4f * progress);
      unsigned char intensity = (unsigned char)(255.0f * lifeRatio * 0.45f);
      Color flashCol = {intensity, intensity, intensity, 255};
      DrawCircleGradient((int)firePool[i].position.x, (int)firePool[i].position.y, rad, flashCol, edgeCol);
    }
  }
  EndBlendMode();

  // =======================================================
  // 3. VẼ LÕI RỒNG, HELIX RIBBONS, VẢY RỒNG, ĐẦU VÀ MẮT (BLEND_ADDITIVE)
  // =======================================================
  for (int e = 0; e < MAX_EMITTERS; e++) {
    if (!emitters[e].active)
      continue;

    int bodySegments = emitters[e].sampledCount;
    if (bodySegments == 0) continue;

    BeginBlendMode(BLEND_ADDITIVE);
    Vector2 prevDrawPos = {0};
    Vector2 prevH1 = {0};
    Vector2 headDrawPos = {0};
    Vector2 headPureTangent = {1, 0};

    for (int i = 0; i < bodySegments; i++) {
      Vector2 purePos_i = emitters[e].sampledPath[i];
      Vector2 pureTangent;
      if (i < bodySegments - 1)
        pureTangent = Vector2Normalize(Vector2Subtract(purePos_i, emitters[e].sampledPath[i + 1]));
      else if (i > 0)
        pureTangent = Vector2Normalize(Vector2Subtract(emitters[e].sampledPath[i - 1], purePos_i));
      else
        pureTangent = (Vector2){1, 0};

      Vector2 purePerp = {-pureTangent.y, pureTangent.x};
      float distFromHead = (float)i * 6.0f;
      float envelope = GetAmplitudeEnvelope(distFromHead, bodySegments * 6.0f);
      float waveVal = sinf(time * 16.0f - distFromHead * 0.018f + emitters[e].twistPhase) * (22.0f * envelope);
      Vector2 drawPos = Vector2Add(purePos_i, Vector2Scale(purePerp, waveVal));

      if (i == 0) {
        headDrawPos = drawPos;
        headPureTangent = pureTangent;
      }

      float normDist = (bodySegments > 1) ? ((float)i / (float)(bodySegments - 1)) : 0.0f;
      float taper = powf(1.0f - normDist, 0.5f);

      float coreRad = Math_Mix(2.0f, 10.0f, taper);
      float auraRad = Math_Mix(8.0f, 26.0f, taper);
      float brightness = taper * 0.06f;

      unsigned char coreV = (unsigned char)(255.0f * brightness * 3.0f);
      unsigned char auraV = (unsigned char)(255.0f * brightness * 1.0f);
      Color coreColor = {coreV, coreV, coreV, 255};
      Color auraColor = {auraV, auraV, auraV, 255};

      if (i > 0) {
        DrawLineEx(prevDrawPos, drawPos, auraRad * 2.0f, auraColor);
        DrawLineEx(prevDrawPos, drawPos, coreRad * 2.0f, coreColor);
      }
      DrawCircleGradient((int)drawPos.x, (int)drawPos.y, auraRad, auraColor, BLANK);
      DrawCircleGradient((int)drawPos.x, (int)drawPos.y, coreRad, coreColor, BLANK);

      // LỬA CUỘN (Helix): Giờ là làn khói lửa cuộn tròn mềm mại mờ ảo (1 sợi lụa)
      float spiralPhase1 = time * 15.0f - distFromHead * 0.05f;
      float coilRad = coreRad * 1.8f;

      Vector2 h1 = {drawPos.x + purePerp.x * cosf(spiralPhase1) * coilRad,
                    drawPos.y + purePerp.y * cosf(spiralPhase1) * coilRad};

      unsigned char hV = (unsigned char)(255.0f * brightness * 1.6f);
      Color hColor = {hV, hV, hV, 255};

      if (i > 0) {
        float ribbonThickness = coreRad * 1.8f; // Dải lụa dày dặn lả lướt
        DrawLineEx(prevH1, h1, ribbonThickness, hColor);
      }

      DrawCircleGradient((int)h1.x, (int)h1.y, coreRad * 2.0f, hColor, BLANK);

      // VẢY RỒNG: Sắc nhọn và vươn dài ra ngoài (tăng độ sáng đáng kể để phát sáng rực rỡ)
      if (i % 15 == 0 && i > 0 && i < bodySegments - 4) {
        float sideSign = (i % 30 == 0) ? 1.0f : -1.0f;
        Vector2 tip = {drawPos.x + purePerp.x * coreRad * 3.5f * sideSign,
                       drawPos.y + purePerp.y * coreRad * 3.5f * sideSign};
        Vector2 base1 = {drawPos.x - pureTangent.x * coreRad * 1.0f,
                         drawPos.y - pureTangent.y * coreRad * 1.0f};
        Vector2 base2 = {drawPos.x + pureTangent.x * coreRad * 1.0f,
                         drawPos.y + pureTangent.y * coreRad * 1.0f};
        unsigned char scaleV = (unsigned char)fminf(255.0f * brightness * 8.5f, 255.0f);
        DrawTriangle(base1, base2, tip, (Color){scaleV, scaleV, scaleV, 255});
      }
      prevDrawPos = drawPos;
      prevH1 = h1;
    }

    // VẼ ĐUÔI RỒNG TRIDENT
    if (bodySegments > 5) {
      Vector2 tailPos = prevDrawPos;
      Vector2 tailTangent;
      if (bodySegments > 1) {
        tailTangent = Vector2Normalize(Vector2Subtract(emitters[e].sampledPath[bodySegments - 2], emitters[e].sampledPath[bodySegments - 1]));
      } else {
        tailTangent = headPureTangent;
      }
      Vector2 tailPerp = {-tailTangent.y, tailTangent.x};

      float finLength = 40.0f;
      float finWidth = 14.0f;

      // Center tail fin tip
      Vector2 centerTip = {tailPos.x - tailTangent.x * finLength,
                           tailPos.y - tailTangent.y * finLength};
      // Left tail fin tip
      Vector2 leftTip = {tailPos.x - tailTangent.x * finLength * 0.8f + tailPerp.x * finWidth,
                         tailPos.y - tailTangent.y * finLength * 0.8f + tailPerp.y * finWidth};
      // Right tail fin tip
      Vector2 rightTip = {tailPos.x - tailTangent.x * finLength * 0.8f - tailPerp.x * finWidth,
                          tailPos.y - tailTangent.y * finLength * 0.8f - tailPerp.y * finWidth};

      Vector2 base1 = {tailPos.x - tailPerp.x * 5.0f, tailPos.y - tailPerp.y * 5.0f};
      Vector2 base2 = {tailPos.x + tailPerp.x * 5.0f, tailPos.y + tailPerp.y * 5.0f};

      unsigned char tailV = (unsigned char)(255.0f * 0.06f * 4.0f);
      Color tailCol = {tailV, tailV, tailV, 255};

      DrawTriangle(base1, base2, centerTip, tailCol);
      DrawTriangle(base1, base2, leftTip, tailCol);
      DrawTriangle(base1, base2, rightTip, tailCol);
    }

    if (emitters[e].headProgress < FIRE_PROGRESS_MAX && bodySegments > 1) {
      Vector2 headPerp = {-headPureTangent.y, headPureTangent.x};
      float rotation = atan2f(headPureTangent.y, headPureTangent.x) * RAD2DEG;
      float flipY = (headPureTangent.x < 0.0f) ? -1.0f : 1.0f;
      Rectangle sourceRec = {0.0f, 0.0f, (float)dragonHeadTex.width,
                             (float)dragonHeadTex.height * flipY};

      float jawOscillation = sinf(time * DRAGON_JAW_OSCILLATION_SPEED) *
                             DRAGON_JAW_OSCILLATION_AMP;
      float scaleY = DRAGON_HEAD_BASE_SCALE * (1.0f + jawOscillation);
      Rectangle destRec = {headDrawPos.x, headDrawPos.y,
                           dragonHeadTex.width * DRAGON_HEAD_BASE_SCALE,
                           dragonHeadTex.height * scaleY};
      Vector2 origin = {destRec.width * DRAGON_HEAD_ORIGIN_X_FACTOR,
                        destRec.height * 0.5f};

      float headAlpha = (emitters[e].headProgress > FIRE_PROGRESS_MAX - 0.4f)
                            ? Clamp(1.0f - (emitters[e].headProgress -
                                            (FIRE_PROGRESS_MAX - 0.4f)) /
                                               0.4f,
                                    0.0f, 1.0f)
                            : 1.0f;

      unsigned char jointV = (unsigned char)(255.0f * headAlpha * 0.35f);
      DrawCircleGradient((int)headDrawPos.x, (int)headDrawPos.y, 28.0f * scaleY,
                         (Color){jointV, jointV, jointV, 255}, BLANK);

      DrawTexturePro(dragonHeadTex, sourceRec, destRec, origin, rotation,
                     ColorAlpha(WHITE, headAlpha));

      Vector2 whiskerBaseTop = GetTexturePointInWorld(
          headDrawPos, headPureTangent, DRAGON_HEAD_BASE_SCALE, scaleY, origin,
          (Vector2){320.0f, 180.0f}, flipY);
      Vector2 whiskerBaseBottom = GetTexturePointInWorld(
          headDrawPos, headPureTangent, DRAGON_HEAD_BASE_SCALE, scaleY, origin,
          (Vector2){320.0f, 260.0f}, flipY);

      Vector2 prevPtTop = whiskerBaseTop, prevPtBottom = whiskerBaseBottom;
      for (int s = 1; s <= 12; s++) {
        float tFactor = (float)s / 12.0f;
        float segmentDist = (float)s * 7.5f;

        float waveTop = sinf(time * 16.0f - (float)s * 0.7f) * 10.0f * tFactor;
        float waveBottom =
            cosf(time * 16.0f - (float)s * 0.7f) * 10.0f * tFactor;

        Vector2 ptTop = {whiskerBaseTop.x - headPureTangent.x * segmentDist +
                             headPerp.x * waveTop,
                         whiskerBaseTop.y - headPureTangent.y * segmentDist +
                             headPerp.y * waveTop};
        Vector2 ptBottom = {
            whiskerBaseBottom.x - headPureTangent.x * segmentDist +
                headPerp.x * waveBottom,
            whiskerBaseBottom.y - headPureTangent.y * segmentDist +
                headPerp.y * waveBottom};

        float thickness = Math_Mix(4.0f, 1.0f, tFactor) * scaleY;
        unsigned char whiskerV =
            (unsigned char)(255.0f * headAlpha * (1.0f - tFactor * 0.4f) *
                            0.75f);
        Color whiskerColor = {whiskerV, whiskerV, whiskerV, 255};

        DrawLineEx(prevPtTop, ptTop, thickness, whiskerColor);
        DrawLineEx(prevPtBottom, ptBottom, thickness, whiskerColor);
        prevPtTop = ptTop;
        prevPtBottom = ptBottom;
      }

      Vector2 eyePos = GetTexturePointInWorld(
          headDrawPos, headPureTangent, DRAGON_HEAD_BASE_SCALE, scaleY, origin,
          (Vector2){320.0f, 180.0f}, flipY);
      float eyeRad = 9.0f * scaleY;
      unsigned char eyeV = (unsigned char)(255.0f * headAlpha);
      DrawCircleGradient((int)eyePos.x, (int)eyePos.y, eyeRad * 2.2f,
                         (Color){eyeV, (unsigned char)(eyeV * 0.65f),
                                 (unsigned char)(eyeV * 0.3f), 255},
                         BLANK);
      DrawCircle((int)eyePos.x, (int)eyePos.y, eyeRad * 0.4f,
                 (Color){255, 255, 255, 255});
    }
    EndBlendMode();
  }

  EndTextureMode();

  // Set shader & draw canvas
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

// Projectile Collision Interfaces
int GetFireSkillProjectiles(SkillProjectile* outProjectiles, int maxProjectiles) {
  int count = 0;
  for (int i = 0; i < MAX_EMITTERS; i++) {
    if (emitters[i].active && count < maxProjectiles) {
      Vector2 headPos = (emitters[i].pathCount > 0) ? emitters[i].path[0] : emitters[i].startPos;
      if (emitters[i].pathCount > 1) {
        Vector2 tangent = GetDragonPathTangent(emitters[i].startPos, emitters[i].p1, emitters[i].p2,
                                               emitters[i].targetPos, emitters[i].headProgress);
        Vector2 perp = {-tangent.y, tangent.x};
        float time = GetTime();
        float headWaveVal = sinf(time * 16.0f + emitters[i].twistPhase) * (22.0f * 0.25f);
        headPos = Vector2Add(headPos, Vector2Scale(perp, headWaveVal));
      }
      outProjectiles[count].position = headPos;
      outProjectiles[count].radius = 35.0f; // Head / body collision radius
      outProjectiles[count].active = true;
      count++;
    }
  }
  return count;
}

void DeactivateFireProjectile(int index) {
  int count = 0;
  for (int i = 0; i < MAX_EMITTERS; i++) {
    if (emitters[i].active) {
      if (count == index) {
        emitters[i].active = false;
        Vector2 headPos = (emitters[i].pathCount > 0) ? emitters[i].path[0] : emitters[i].startPos;
        if (emitters[i].pathCount > 1) {
          Vector2 tangent = GetDragonPathTangent(emitters[i].startPos, emitters[i].p1, emitters[i].p2,
                                                 emitters[i].targetPos, emitters[i].headProgress);
          Vector2 perp = {-tangent.y, tangent.x};
          float time = GetTime();
          float headWaveVal = sinf(time * 16.0f + emitters[i].twistPhase) * (22.0f * 0.25f);
          headPos = Vector2Add(headPos, Vector2Scale(perp, headWaveVal));
        }
        TriggerFireImpact(headPos);
        break;
      }
      count++;
    }
  }
}