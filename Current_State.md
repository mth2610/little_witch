# 🧙 CURRENT STATE (HIỆN TRẠNG GAME) - LITTLE WITCH ADVENTURE

Tài liệu này ghi nhận trạng thái kỹ thuật và tính năng hiện tại của dự án trước khi thực hiện nâng cấp lên phiên bản 2.0.

---

## 📌 1. THÔNG TIN CHUNG
*   **Ngôn ngữ:** C thuần (C99), tuyệt đối không dùng C++.
*   **Thư viện đồ họa:** Raylib 5.0 (liên kết tĩnh).
*   **Quản lý bộ nhớ:** Sử dụng **Object Pool** mảng tĩnh, không dùng `malloc`/`free` trong game loop để tránh rò rỉ bộ nhớ.
*   **Màn hình:** Render ảo `1280x720` (Virtual Canvas) sau đó scale letterbox lên màn hình thực tế.
*   **Hỗ trợ nền tảng:** PC (macOS, Windows) và Android (API 21+, CMake + NDK).
*   **Quảng cáo:** Google AdMob SDK (Java) qua JNI Bridge (chỉ chạy trên Android).

---

## ⚙️ 2. TRẠNG THÁI CHI TIẾT CÁC HỆ THỐNG

### 2.1 Màn hình & Vòng lặp (`src/main.c`)
*   Quản lý 5 màn hình (`SCREEN_MENU`, `SCREEN_GAMEPLAY`, `SCREEN_SKILL_SELECT`, `SCREEN_UPGRADE_SHOP`, `SCREEN_GAMEOVER`).
*   Vẽ ánh sáng mờ ảo xung quanh Witch và vật thể phát sáng qua `RenderTexture2D lightmap`.

### 2.2 Nhân vật Phù thủy (`src/witch.c`)
*   Di chuyển bằng cách Lerp vị trí đến `targetPosition` (nhấp touch/chuột).
*   Trạng thái bay: Sprite sheet `witch_fly.png` 32 khung hình độc lập, góc nghiêng tự động $\pm 14^{\circ}$ dựa trên vận tốc dọc Y. Cây sáo thần (`flute.png`) được vẽ riêng biệt dưới chân, nghiêng đồng bộ.
*   Khiên ma thuật (Mana Shield): pulsating bán kính `100.0f`, tự kích hoạt khi mất máu nếu sở hữu sao Thủy (`STAR_THUY`).

### 2.3 Hệ thống Sao / Linh hồn (`src/star.c`)
*   Vẽ hoàn toàn bằng đồ họa vector C dạng gradient (Spirit Orb), không dùng ảnh PNG.
*   Pool giới hạn `MAX_STARS` (8).
*   5 hệ Ngũ Hành: Kim (Trắng bạc), Mộc (Xanh lá), Thủy (Xanh dương), Hỏa (Đỏ/Cam), Thổ (Vàng) và sao Ngũ sắc (Rainbow).
*   Hút bởi Magnet. Va chạm quái gây sát thương và bay quỹ đạo quanh Witch.

### 2.4 Quái vật & Boss (`src/enemy.c`, `src/enemy_normal.c`, `src/enemy_boss.c`)
*   Pool giới hạn `MAX_ENEMIES` (32).
*   **Quái thường hiện tại:** Mang yếu tố công nghệ/Sci-fi (Robot, Drone, UFO, Slime, Bat, Ghost, Alien).
*   **Boss:** 
    *   Forest: Ném đá, Rage chế độ ném đá siêu nhanh khi HP < 50%.
    *   Cave: Laser quét ngang.
    *   City: Gọi Drone.
    *   Space: Hố đen kéo Witch.

### 2.5 Kỹ năng & Đạn (`src/skill.c`)
*   **Bị động:** Magnet hút sao (tốc độ tăng theo nâng cấp shop, số sao Kim, và thẻ nâng cấp).
*   **5 Kỹ năng Chủ động:**
    *   *Kim:* Phi tiêu vàng xuyên thấu địch.
    *   *Mộc:* Hơi độc xanh lá cây gây sát thương DOT.
    *   *Thủy:* Luồng hơi lạnh đóng băng quái (quái đứng im, không bắn đạn).
    *   *Hỏa:* Quả cầu lửa nổ lan gây sát thương diện rộng.
    *   *Thổ:* Sét xích (Chain Lightning) truyền qua nhiều quái và Lốc xoáy điện hút quái.
*   **Ultimate (Elemental Burst):** Kích hoạt bằng phím `F` hoặc nút UI khi đủ 5 hệ sao hoặc có sao Ngũ sắc. Phóng toàn bộ sao thành đạn homing tìm quái, gây chớp màn hình trắng và rung lắc màn hình cực mạnh.
*   *Đòn đánh thường:* Hiện tại **chưa có cơ chế bắn thường yếu** khi người chơi không có ngôi sao nào (khiến người chơi dễ bị bất lực ở đầu game).

### 2.6 Âm thanh (`src/main.c`, `assets/sounds/`)
*   Sử dụng duy nhất một file BGM `bgm_forest.ogg` lặp lại.
*   Chưa có cơ chế âm thanh phân lớp hay crossfade động.

### 2.7 Shaders (`assets/shaders/`)
*   Tách biệt hoàn toàn mã shader: PC dùng OpenGL 3.3 (`assets/shaders/glsl330/`), Android dùng OpenGL ES 2.0 (`assets/shaders/gles/`), nạp tự động qua macro `SHADER_PATH`.
*   **Bloom Pipeline:** Tích hợp bộ lọc Bloom 4 bước (Brightpass Extract + Horizontal Blur + Vertical Blur + Additive Blend) chạy trên canvas độ phân giải thấp `320x180` giúp tối ưu hóa hiệu năng, mang lại hiệu ứng phát sáng Ori-glow lung linh.
*   **Mana Shield & Frost Distortion:** Nâng cấp hai shader này tích hợp hiệu ứng khúc xạ biến dạng không gian (Space Distortion) bằng cách sao chép virtual canvas và truyền làm `screenTexture`.
*   **Fireball & Shaders khác:** Tối ưu hóa số bước lặp raymarching trên GLES và viết lại mã tương thích hoàn toàn GLESv2 (không dùng `tanh()`, array initializers hay `textureSize()`).

---

## 🛠️ 3. QUY TRÌNH BUILD VÀ PHÁT TRIỂN
*   **PC:** Dùng CMake để biên dịch, ra file thực thi tại `build/bin/littlewitch`.
*   **Android:** Dùng Gradle tại thư mục `android/`, biên dịch ra APK tại `android/app/build/outputs/apk/debug/app-debug.apk`.
