#ifndef SAVE_H
#define SAVE_H

#include "config.h"
#include "witch.h"

#define SAVE_FILE_PATH  "LWA_save.dat"
#define SAVE_MAGIC      0x4C574100  // "LWA\0" làm dấu nhận dạng file save
#define SAVE_VERSION    1

typedef struct SaveData {
    int             magic;
    int             version;
    PermanentUpgrades upgrades;
    int             totalGoldEarned;
    int             highScore;
    bool            firstLaunch;
    Language        language;
} SaveData;

// Tải dữ liệu lưu trữ từ file
SaveData LoadSaveData(void);

// Ghi đè dữ liệu lưu trữ vào file
void     WriteSaveData(const SaveData *data);

// Xóa file save và reset dữ liệu mặc định
void     ResetSaveData(void);

// Kiểm tra tính hợp lệ của file save (magic header + version)
bool     ValidateSaveData(const SaveData *data);

#endif // SAVE_H
