// ============================================================
// enemy.c — Hệ thống quản lý Pool Quái vật và Trùm (Enemy Core)
// ============================================================

#include "enemy.h"
#include "game.h"
#include "raymath.h"
#include <stdlib.h>
#include <math.h>

// Âm thanh khi quái bị tấn công
extern Sound hitEnemySound;

// Khai báo các EnemyDefinition từ các file độc lập
extern EnemyDefinition slime_definition;
extern EnemyDefinition bat_definition;
extern EnemyDefinition ghost_definition;
extern EnemyDefinition robot_definition;
extern EnemyDefinition drone_definition;
extern EnemyDefinition alien_definition;
extern EnemyDefinition ufo_definition;
extern EnemyDefinition boss_forest_definition;
extern EnemyDefinition boss_cave_definition;
extern EnemyDefinition boss_city_definition;
extern EnemyDefinition boss_space_definition;

// Registry chứa các định nghĩa quái vật
static const EnemyDefinition* enemyRegistry[ENEMY_COUNT] = {
    [ENEMY_SLIME]       = &slime_definition,
    [ENEMY_BAT]         = &bat_definition,
    [ENEMY_GHOST]       = &ghost_definition,
    [ENEMY_ROBOT]       = &robot_definition,
    [ENEMY_DRONE]       = &drone_definition,
    [ENEMY_ALIEN]       = &alien_definition,
    [ENEMY_UFO]         = &ufo_definition,
    [ENEMY_BOSS_FOREST] = &boss_forest_definition,
    [ENEMY_BOSS_CAVE]   = &boss_cave_definition,
    [ENEMY_BOSS_CITY]   = &boss_city_definition,
    [ENEMY_BOSS_SPACE]  = &boss_space_definition,
};

// ----------------------------------------------------------------
// HÀM TRỢ GIÚP ĐĂNG KÝ VÀ DYNAMIC DISPATCH
// ----------------------------------------------------------------

const EnemyDefinition* GetEnemyDefinition(EnemyType type) {
    if (type >= 0 && type < ENEMY_COUNT) {
        return enemyRegistry[type];
    }
    return NULL;
}

// Logic di chuyển chung của Trùm: Đi vào từ bên phải rồi bay lên xuống hình sin
void UpdateBossMovement(Enemy *e, float dt) {
    if (e->position.x > VIRTUAL_WIDTH - 220.0f) {
        e->position.x += e->velocity.x * dt;
    } else {
        e->position.x = VIRTUAL_WIDTH - 220.0f;
        e->position.y = VIRTUAL_HEIGHT / 2.0f + sinf(e->animTimer * 1.5f) * 110.0f;
    }
}

// Vẽ mặc định (có texture hoặc dùng procedural)
void DrawEnemyDefault(const Enemy *e, Color drawTint, Color fallbackColor) {
    const EnemyDefinition *def = GetEnemyDefinition(e->type);
    if (def == NULL) return;

    if (def->texture != NULL && IsTextureReady(*(def->texture))) {
        float frameWidth = (float)def->texture->width / 4.0f; // Sprite sheet có 4 frame
        float frameHeight = (float)def->texture->height;
        
        Rectangle sourceRec = { e->animFrame * frameWidth, 0.0f, frameWidth, frameHeight };
        Rectangle destRec = { e->position.x, e->position.y, frameWidth, frameHeight };
        Vector2 origin = { frameWidth / 2.0f, frameHeight / 2.0f };
        
        DrawTexturePro(*(def->texture), sourceRec, destRec, origin, 0.0f, drawTint);
    } else {
        if (def->DrawProcedural != NULL) {
            def->DrawProcedural(e, fallbackColor);
        }
        
        // Vẽ thanh máu mini phía trên quái nếu có maxHp > 1 (trong bản vẽ vector)
        if (e->maxHp > 1) {
            float barW = e->collisionRadius * 1.5f;
            float barH = 5.0f;
            float hpPct = (float)e->hp / e->maxHp;
            
            DrawRectangle(e->position.x - barW / 2.0f, e->position.y - e->collisionRadius - 10.0f, barW, barH, RED);
            DrawRectangle(e->position.x - barW / 2.0f, e->position.y - e->collisionRadius - 10.0f, barW * hpPct, barH, GREEN);
        }
    }
}

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
    e->attackTimer = 0.0f;
    e->attackPhase = 0;
    e->rainbowCooldown = 0.0f;
    e->freezeTimer = 0.0f;
    e->knockbackTimer = 0.0f;
    e->knockbackVelocity = (Vector2){ 0.0f, 0.0f };
    
    // Cấu hình thuộc tính từ EnemyRegistry
    const EnemyDefinition *def = GetEnemyDefinition(type);
    if (def != NULL) {
        e->hp = def->maxHp;
        e->maxHp = def->maxHp;
        e->speed = def->speed;
        e->collisionRadius = def->collisionRadius;
        e->velocity = (Vector2){ -e->speed, 0.0f };
        e->shootCooldown = def->defaultShootCooldown;
        if (def->InitState != NULL) {
            def->InitState(e);
        }
    }
    
    return index;
}

// Cập nhật trạng thái di chuyển và tấn công của quái vật trong pool
void UpdateEnemies(struct GameState *gs, float deltaTime) {
    if (gs == NULL) return;
    
    EnemyPool *pool = &gs->enemyPool;
    
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
        
        // Gọi hàm cập nhật từ Registry
        const EnemyDefinition *def = GetEnemyDefinition(e->type);
        if (def != NULL && def->Update != NULL) {
            def->Update(e, gs, deltaTime);
        }
        
        // Xử lý quái thường bay thoát khỏi rìa trái màn hình thì hủy
        if (e->type < ENEMY_BOSS_FOREST && e->position.x < -100.0f) {
            e->active = false;
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
        
        const EnemyDefinition *def = GetEnemyDefinition(e->type);
        if (def == NULL) continue;
        
        // Màu sắc vẽ
        Color drawTint = WHITE;
        if (e->flashTimer > 0.0f) {
            drawTint = e->flashColor;
        } else if (e->freezeTimer > 0.0f) {
            drawTint = (Color){ 100, 200, 255, 255 }; // Phủ sắc xanh băng tuyết nhạt
        }
        
        Color col = e->flashTimer > 0.0f ? e->flashColor : def->fallbackColor;
        if (e->freezeTimer > 0.0f && e->flashTimer <= 0.0f) {
            col = (Color){ 0, 180, 255, 255 }; // Quầng sáng xanh băng phát sáng
        }
        
        // Vẽ quầng sáng neon mờ ảo dưới chân quái vật (glowing aura)
        BeginBlendMode(BLEND_ADDITIVE);
            DrawCircleGradient(e->position.x, e->position.y, e->collisionRadius * 2.2f, ColorAlpha(col, 0.35f), ColorAlpha(col, 0.0f));
            DrawCircleGradient(e->position.x, e->position.y, e->collisionRadius * 1.2f, ColorAlpha(WHITE, 0.5f), ColorAlpha(WHITE, 0.0f));
        EndBlendMode();
        
        // Gọi hàm vẽ từ registry (nếu có vẽ đặc biệt như laser/hố đen của boss)
        if (def->Draw != NULL) {
            def->Draw(e, drawTint);
        } else {
            DrawEnemyDefault(e, drawTint, col);
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
