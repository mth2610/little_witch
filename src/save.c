// ============================================================
// save.c — Hệ thống Lưu Trữ và Đọc Save Game (Save System)
// ============================================================

#include "save.h"
#include <stdio.h>

// ----------------------------------------------------------------
// HÀM CÔNG KHAI
// ----------------------------------------------------------------

// Kiểm tra tính hợp lệ của gói dữ liệu (Magic key và Version)
bool ValidateSaveData(const SaveData *data) {
    return (data != NULL && 
            data->magic == SAVE_MAGIC && 
            data->version == SAVE_VERSION);
}

// Tải dữ liệu lưu trữ từ file nhị phân. Nếu không có file sẽ trả về mặc định
SaveData LoadSaveData(void) {
    SaveData data = {0};
    
    // Đặt cấu hình mặc định ban đầu phòng khi load thất bại
    data.magic = SAVE_MAGIC;
    data.version = SAVE_VERSION;
    data.totalGoldEarned = 0;
    data.highScore = 0;
    data.firstLaunch = true;
    data.language = LANG_VI; // Mặc định chạy lần đầu là tiếng Việt
    for (int i = 0; i < UPGRADE_COUNT; i++) {
        data.upgrades.level[i] = 0;
    }
    
    FILE *file = fopen(SAVE_FILE_PATH, "rb");
    if (file == NULL) {
        // File không tồn tại (chạy lần đầu)
        return data;
    }
    
    // Đọc dữ liệu từ file nhị phân
    size_t readCount = fread(&data, sizeof(SaveData), 1, file);
    fclose(file);
    
    if (readCount != 1 || !ValidateSaveData(&data)) {
        // Dữ liệu lỗi hoặc sai cấu trúc phiên bản
        // Reset về mặc định
        data.magic = SAVE_MAGIC;
        data.version = SAVE_VERSION;
        data.totalGoldEarned = 0;
        data.highScore = 0;
        data.firstLaunch = true;
        data.language = LANG_VI;
        for (int i = 0; i < UPGRADE_COUNT; i++) {
            data.upgrades.level[i] = 0;
        }
        return data;
    }
    
    // Load thành công
    data.firstLaunch = false;
    return data;
}

// Ghi dữ liệu hiện tại vào file nhị phân
void WriteSaveData(const SaveData *data) {
    if (data == NULL) return;
    
    FILE *file = fopen(SAVE_FILE_PATH, "wb");
    if (file == NULL) {
        // Không thể mở file ghi (ví dụ không có quyền ghi)
        return;
    }
    
    fwrite(data, sizeof(SaveData), 1, file);
    fclose(file);
}

// Xóa file save vật lý trên đĩa
void ResetSaveData(void) {
    remove(SAVE_FILE_PATH);
}
