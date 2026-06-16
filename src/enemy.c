// ============================================================
// enemy.c — Hệ thống quản lý Pool Quái vật và Trùm (Enemy Core)
// ============================================================

#include "enemy.h"
#include "game.h"
#include "enemy_normal.h"
#include "enemy_boss.h"
#include "raymath.h"
#include <stdlib.h>

// Texture quái vật và trùm (khai báo ở main.c)
extern Texture2D enemySlimeTexture;
extern Texture2D enemyBatTexture;
extern Texture2D enemyGhostTexture;
extern Texture2D enemyRobotTexture;
extern Texture2D enemyDroneTexture;
extern Texture2D enemyAlienTexture;
extern Texture2D enemyUfoTexture;

extern Texture2D bossForestTexture;
extern Texture2D bossCaveTexture;
extern Texture2D bossCityTexture;
extern Texture2D bossSpaceTexture;

// Âm thanh khi quái bị tấn công
extern Sound hitEnemySound;

// ----------------------------------------------------------------
// HÀM CÔNG KHAI
// ----------------------------------------------------------------

// Khởi tạo Pool quản lý quái vật
void InitEnemyPool(EnemyPool *pool) {
    if (pool == NULL) return;
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        pool->enemies[i].active = false;
    }
    pool->spawnTimer = 0.0f;
    pool->spawnInterval = 2.0f;
    pool->bossActive = false;
    pool->bossIndex = -1;
}

// Sinh quái vật mới vào pool
int SpawnEnemy(EnemyPool *pool, EnemyType type, float x, float y) {
    if (pool == NULL) return -1;
    
    // Tìm slot trống trong pool
    int index = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!pool->enemies[i].active) {
            index = i;
            break;
        }
    }
    
    // Nếu pool đầy, bỏ qua
    if (index == -1) return -1;
    
    Enemy *e = &pool->enemies[index];
    e->active = true;
    e->type = type;
    e->position = (Vector2){ x, y };
    e->animTimer = 0.0f;
    e->animFrame = 0;
    e->flashColor = WHITE;
    e->flashTimer = 0.0f;
    e->isVisible = true;
    e->invisTimer = 0.0f;
    e->shootCooldown = 1.0f + ((float)rand() / RAND_MAX) * 1.5f; // Tránh bắn đồng loạt ngay khi xuất hiện
    e->attackTimer = 0.0f;
    e->attackPhase = 0;
    e->rainbowCooldown = 0.0f;
    e->freezeTimer = 0.0f;
    e->knockbackTimer = 0.0f;
    e->knockbackVelocity = (Vector2){ 0.0f, 0.0f };
    
    // Cấu hình thuộc tính theo từng loại quái
    switch (type) {
        case ENEMY_SLIME:
            e->hp = 1;
            e->maxHp = 1;
            e->speed = 80.0f;
            e->collisionRadius = 22.0f;
            e->velocity = (Vector2){ -e->speed, 0.0f };
            break;
            
        case ENEMY_BAT:
            e->hp = 1;
            e->maxHp = 1;
            e->speed = 120.0f;
            e->collisionRadius = 18.0f;
            e->velocity = (Vector2){ -e->speed, 0.0f };
            break;
            
        case ENEMY_GHOST:
            e->hp = 1;
            e->maxHp = 1;
            e->speed = 100.0f;
            e->collisionRadius = 20.0f;
            e->velocity = (Vector2){ -e->speed, 0.0f };
            break;
            
        case ENEMY_ROBOT:
            e->hp = 3;
            e->maxHp = 3;
            e->speed = 90.0f;
            e->collisionRadius = 26.0f;
            e->velocity = (Vector2){ -e->speed, 0.0f };
            e->shootCooldown = 2.5f;
            break;
            
        case ENEMY_DRONE:
            e->hp = 2;
            e->maxHp = 2;
            e->speed = 160.0f;
            e->collisionRadius = 16.0f;
            e->velocity = (Vector2){ 0.0f, 0.0f };
            break;
            
        case ENEMY_ALIEN:
            e->hp = 4;
            e->maxHp = 4;
            e->speed = 150.0f;
            e->collisionRadius = 24.0f;
            e->velocity = (Vector2){ -e->speed, e->speed };
            e->shootCooldown = 0.0f;
            break;
            
        case ENEMY_UFO:
            e->hp = 5;
            e->maxHp = 5;
            e->speed = 80.0f;
            e->collisionRadius = 30.0f;
            e->velocity = (Vector2){ -e->speed, 0.0f };
            e->shootCooldown = 3.0f;
            break;
            
        case ENEMY_BOSS_FOREST:
            e->hp = 150;
            e->maxHp = 150;
            e->speed = 60.0f;
            e->collisionRadius = 55.0f;
            e->velocity = (Vector2){ -e->speed, 0.0f };
            e->shootCooldown = 1.3f;
            break;
            
        case ENEMY_BOSS_CAVE:
            e->hp = 50;
            e->maxHp = 50;
            e->speed = 60.0f;
            e->collisionRadius = 60.0f;
            e->velocity = (Vector2){ -e->speed, 0.0f };
            e->shootCooldown = 2.0f;
            break;
            
        case ENEMY_BOSS_CITY:
            e->hp = 80;
            e->maxHp = 80;
            e->speed = 60.0f;
            e->collisionRadius = 65.0f;
            e->velocity = (Vector2){ -e->speed, 0.0f };
            e->shootCooldown = 4.0f;
            break;
            
        case ENEMY_BOSS_SPACE:
            e->hp = 120;
            e->maxHp = 120;
            e->speed = 60.0f;
            e->collisionRadius = 70.0f;
            e->velocity = (Vector2){ -e->speed, 0.0f };
            e->shootCooldown = 5.0f;
            break;
            
        default:
            break;
    }
    
    return index;
}

// Cập nhật trạng thái di chuyển và tấn công của quái vật trong pool
void UpdateEnemies(struct GameState *gs, float deltaTime) {
    if (gs == NULL) return;
    
    EnemyPool *pool = &gs->enemyPool;
    struct Witch *witch = &gs->witch;
    ProjectilePool *projPool = &gs->projectilePool;
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!pool->enemies[i].active) continue;
        
        Enemy *e = &pool->enemies[i];
        
        // Cập nhật hoạt họa chung & flash timer
        e->animTimer += deltaTime;
        e->animFrame = ((int)(e->animTimer * 6.0f)) % 4;
        
        if (e->flashTimer > 0.0f) {
            e->flashTimer -= deltaTime;
            if (e->flashTimer < 0.0f) e->flashTimer = 0.0f;
        }
        
        if (e->rainbowCooldown > 0.0f) {
            e->rainbowCooldown -= deltaTime;
            if (e->rainbowCooldown < 0.0f) e->rainbowCooldown = 0.0f;
        }
        
        // Xử lý hiệu ứng đẩy lùi (Shield knockback)
        if (e->knockbackTimer > 0.0f) {
            e->position = Vector2Add(e->position, Vector2Scale(e->knockbackVelocity, deltaTime));
            e->knockbackTimer -= deltaTime;
            e->knockbackVelocity = Vector2Scale(e->knockbackVelocity, 1.0f - 6.0f * deltaTime);
            if (e->knockbackTimer < 0.0f) e->knockbackTimer = 0.0f;
            
            // Vẫn chạy hoạt họa
            e->animTimer += deltaTime;
            continue; // Bị đẩy lùi và choáng, bỏ qua di chuyển thông thường và tấn công
        }
        
        // Xử lý hiệu ứng đóng băng hệ Thủy
        if (e->freezeTimer > 0.0f) {
            e->freezeTimer -= deltaTime;
            if (e->freezeTimer < 0.0f) e->freezeTimer = 0.0f;
            continue; // Đứng im hoàn toàn, bỏ qua di chuyển và bắn đạn
        }
        
        bool isBoss = (e->type >= ENEMY_BOSS_FOREST);
        
        if (isBoss) {
            // Logic di chuyển của Trùm: Đi từ bên phải vào điểm giữa màn hình rồi đứng yên bay lơ lửng
            if (e->position.x > VIRTUAL_WIDTH - 220.0f) {
                e->position.x += e->velocity.x * deltaTime;
            } else {
                e->position.x = VIRTUAL_WIDTH - 220.0f;
                // Bay nhẹ nhàng lên xuống hình sin
                e->position.y = VIRTUAL_HEIGHT / 2.0f + sinf(e->animTimer * 1.5f) * 110.0f;
                
                // Thực hiện tấn công Trùm chuyên biệt (từ enemy_boss.c)
                UpdateBossAttack(pool, e, gs, deltaTime);
            }
        } else {
            // Logic di chuyển của quái thường (từ enemy_normal.c)
            UpdateNormalEnemy(e, witch, projPool, deltaTime);
            
            // Xử lý quái thường bay thoát khỏi rìa trái màn hình thì hủy
            if (e->position.x < -100.0f) {
                e->active = false;
            }
        }
    }
}

// Vẽ toàn bộ quái vật và các hiệu ứng tấn công của Trùm
void DrawEnemies(const EnemyPool *pool) {
    if (pool == NULL) return;
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!pool->enemies[i].active) continue;
        
        const Enemy *e = &pool->enemies[i];
        
        // Quái Ghost tàng hình (ẩn hình hoàn toàn)
        if (e->type == ENEMY_GHOST && !e->isVisible) {
            continue;
        }
        
        // Vẽ hiệu ứng đặc biệt của trùm trước khi vẽ thân trùm
        if (e->type == ENEMY_BOSS_CAVE && e->attackPhase >= 1) {
            // Tia laser đỏ của Cave Boss
            float sweepY = (e->attackTimer / 1.5f) * VIRTUAL_HEIGHT;
            DrawLineEx((Vector2){ 0.0f, sweepY }, (Vector2){ e->position.x, sweepY }, 18.0f, ColorAlpha(RED, 0.75f));
            DrawLineEx((Vector2){ 0.0f, sweepY }, (Vector2){ e->position.x, sweepY }, 6.0f, WHITE);
            DrawCircle(e->position.x - 30.0f, sweepY, 20.0f, ColorAlpha(RED, 0.9f));
        }
        
        if (e->type == ENEMY_BOSS_SPACE && e->attackPhase >= 1) {
            // Vòng xoáy hố đen của Space Boss
            Vector2 center = { VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f };
            float spin = e->animTimer * 180.0f;
            DrawCircleSector(center, 140.0f, spin, spin + 120.0f, 16, ColorAlpha(DARKPURPLE, 0.4f));
            DrawCircleSector(center, 140.0f, spin + 180.0f, spin + 300.0f, 16, ColorAlpha(DARKPURPLE, 0.4f));
            DrawCircleV(center, 70.0f + sinf(e->animTimer * 5.0f) * 10.0f, ColorAlpha(BLACK, 0.9f));
            DrawCircleLines(center.x, center.y, 75.0f, PURPLE);
        }
        
        // Chọn texture tương ứng
        Texture2D tex;
        bool hasTexture = false;
        Color fallbackColor = RED;
        
        switch (e->type) {
            case ENEMY_SLIME:
                if (IsTextureReady(enemySlimeTexture)) { tex = enemySlimeTexture; hasTexture = true; }
                fallbackColor = (Color){ 0, 255, 128, 255 }; // Lime neon
                break;
            case ENEMY_BAT:
                if (IsTextureReady(enemyBatTexture)) { tex = enemyBatTexture; hasTexture = true; }
                fallbackColor = (Color){ 255, 80, 0, 255 }; // Bright orange-red
                break;
            case ENEMY_GHOST:
                if (IsTextureReady(enemyGhostTexture)) { tex = enemyGhostTexture; hasTexture = true; }
                fallbackColor = (Color){ 210, 160, 255, 255 }; // Glowing lavender
                break;
            case ENEMY_ROBOT:
                if (IsTextureReady(enemyRobotTexture)) { tex = enemyRobotTexture; hasTexture = true; }
                fallbackColor = (Color){ 0, 191, 255, 255 }; // Neon blue
                break;
            case ENEMY_DRONE:
                if (IsTextureReady(enemyDroneTexture)) { tex = enemyDroneTexture; hasTexture = true; }
                fallbackColor = (Color){ 0, 255, 255, 255 }; // Neon cyan
                break;
            case ENEMY_ALIEN:
                if (IsTextureReady(enemyAlienTexture)) { tex = enemyAlienTexture; hasTexture = true; }
                fallbackColor = (Color){ 140, 255, 10, 255 }; // Neon yellow-green
                break;
            case ENEMY_UFO:
                if (IsTextureReady(enemyUfoTexture)) { tex = enemyUfoTexture; hasTexture = true; }
                fallbackColor = (Color){ 255, 0, 255, 255 }; // Neon magenta
                break;
            case ENEMY_BOSS_FOREST:
                if (IsTextureReady(bossForestTexture)) { tex = bossForestTexture; hasTexture = true; }
                fallbackColor = (Color){ 0, 255, 0, 255 }; // Pure neon green
                break;
            case ENEMY_BOSS_CAVE:
                if (IsTextureReady(bossCaveTexture)) { tex = bossCaveTexture; hasTexture = true; }
                fallbackColor = (Color){ 255, 160, 0, 255 }; // Glowing gold
                break;
            case ENEMY_BOSS_CITY:
                if (IsTextureReady(bossCityTexture)) { tex = bossCityTexture; hasTexture = true; }
                fallbackColor = (Color){ 255, 0, 128, 255 }; // Cyberpunk hot pink
                break;
            case ENEMY_BOSS_SPACE:
                if (IsTextureReady(bossSpaceTexture)) { tex = bossSpaceTexture; hasTexture = true; }
                fallbackColor = (Color){ 128, 0, 255, 255 }; // Swirling purple
                break;
            default:
                break;
        }
        
        // Màu sắc vẽ
        Color drawTint = WHITE;
        if (e->flashTimer > 0.0f) {
            drawTint = e->flashColor;
        } else if (e->freezeTimer > 0.0f) {
            drawTint = (Color){ 100, 200, 255, 255 }; // Phủ sắc xanh băng tuyết nhạt
        }
        Color col = e->flashTimer > 0.0f ? e->flashColor : fallbackColor;
        if (e->freezeTimer > 0.0f && e->flashTimer <= 0.0f) {
            col = (Color){ 0, 180, 255, 255 }; // Quầng sáng xanh băng phát sáng
        }
        
        // Vẽ quầng sáng neon mờ ảo dưới chân quái vật (glowing aura)
        BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleGradient(e->position.x, e->position.y, e->collisionRadius * 2.2f, ColorAlpha(col, 0.35f), ColorAlpha(col, 0.0f));
            DrawCircleGradient(e->position.x, e->position.y, e->collisionRadius * 1.2f, ColorAlpha(WHITE, 0.5f), ColorAlpha(WHITE, 0.0f));
        EndBlendMode();
        
        if (hasTexture) {
            float frameWidth = (float)tex.width / 4.0f; // Sprite sheet có 4 frame
            float frameHeight = (float)tex.height;
            
            Rectangle sourceRec = { e->animFrame * frameWidth, 0.0f, frameWidth, frameHeight };
            Rectangle destRec = { e->position.x, e->position.y, frameWidth, frameHeight };
            Vector2 origin = { frameWidth / 2.0f, frameHeight / 2.0f };
            
            DrawTexturePro(tex, sourceRec, destRec, origin, 0.0f, drawTint);
        } else {
            // Tách biệt việc vẽ vector quái thường và Trùm sang các file phụ tương ứng
            if (e->type >= ENEMY_BOSS_FOREST) {
                DrawProceduralBoss(e, col);
            } else {
                DrawProceduralNormalEnemy(e, col);
            }
            
            // Vẽ thanh máu mini phía trên quái nếu có maxHp > 1
            if (e->maxHp > 1) {
                float barW = e->collisionRadius * 1.5f;
                float barH = 5.0f;
                float hpPct = (float)e->hp / e->maxHp;
                
                DrawRectangle(e->position.x - barW / 2.0f, e->position.y - e->collisionRadius - 10.0f, barW, barH, RED);
                DrawRectangle(e->position.x - barW / 2.0f, e->position.y - e->collisionRadius - 10.0f, barW * hpPct, barH, GREEN);
            }
        }
    }
}

// Giảm HP quái vật, trả về true nếu quái chết, false nếu còn sống
bool DamageEnemy(EnemyPool *pool, int index, int damage) {
    if (pool == NULL || index < 0 || index >= MAX_ENEMIES) return false;
    
    Enemy *e = &pool->enemies[index];
    if (!e->active) return false;
    
    // Giảm máu
    e->hp -= damage;
    
    // Hiệu ứng nhấp nháy đỏ
    e->flashTimer = 0.12f;
    e->flashColor = RED;
    
    // Phát âm thanh khi trúng đòn
    if (IsSoundReady(hitEnemySound)) {
        PlaySound(hitEnemySound);
    }
    
    if (e->hp <= 0) {
        e->active = false;
        
        // Nếu là Trùm chết
        if (e->type >= ENEMY_BOSS_FOREST) {
            pool->bossActive = false;
            pool->bossIndex = -1;
        }
        return true; // Chết
    }
    
    return false; // Chưa chết
}

// Triệu hồi Trùm dựa trên Biome hiện tại ở rìa phải màn hình
void SpawnBoss(EnemyPool *pool, BiomeState biome) {
    if (pool == NULL) return;
    
    EnemyType type = ENEMY_BOSS_FOREST;
    switch (biome) {
        case BIOME_FOREST: type = ENEMY_BOSS_FOREST; break;
        case BIOME_CAVE:   type = ENEMY_BOSS_CAVE;   break;
        case BIOME_CITY:   type = ENEMY_BOSS_CITY;   break;
        case BIOME_SPACE:  type = ENEMY_BOSS_SPACE;  break;
    }
    
    int index = SpawnEnemy(pool, type, VIRTUAL_WIDTH + 150.0f, VIRTUAL_HEIGHT / 2.0f);
    if (index >= 0) {
        pool->bossActive = true;
        pool->bossIndex = index;
    }
}

// Trả về số vàng rơi ra sau khi hạ gục từng loại quái
int GetEnemyGoldDrop(EnemyType type) {
    if (type >= ENEMY_BOSS_FOREST) {
        return 50; // Trùm rơi 50 vàng
    }
    
    switch (type) {
        case ENEMY_SLIME:
        case ENEMY_BAT:
        case ENEMY_GHOST:
            return 1; // Quái cơ bản 1 vàng
            
        case ENEMY_ROBOT:
        case ENEMY_DRONE:
        case ENEMY_ALIEN:
        case ENEMY_UFO:
            return 3; // Quái nâng cao 3 vàng
            
        default:
            return 0;
    }
}
