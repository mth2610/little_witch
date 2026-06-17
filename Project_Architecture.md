# 🧙 Little Witch Adventure - AI Developer Specification

> ⚠️ **LUẬT BẮT BUỘC CHO AI:**
> 1. **KHI MỞ CONVERSATION:** Đọc file này đầu tiên để nắm toàn bộ bối cảnh dự án.
> 2. **KHI HOÀN THÀNH CHỨC NĂNG:** Hỏi ý kiến người dùng để cập nhật các thay đổi vào file này.

---

## 📌 1. RÀNG BUỘC CỨNG & THÔNG SỐ KỸ THUẬT
- **Ngôn ngữ:** C thuần (chuẩn C99). Tuyệt đối không dùng C++.
- **Thư viện:** Raylib 5.0 (link tĩnh).
- **Bộ nhớ:** Không dùng `malloc`/`free` trong game loop. Bắt buộc dùng **Object Pool** với mảng tĩnh.
- **Màn hình:** Virtual Canvas `1280x720` vẽ trước, sau đó scale letterbox lên màn hình thật (xử lý chuột/touch qua `ScreenToVirtual`).
- **Nền tảng:** Android (API 21+, CMake + NDK) & PC (macOS/Windows).
- **Quảng cáo:** Google AdMob SDK (Java) trên Android giao tiếp qua JNI Bridge. Trên PC dùng Stub/Mock.
- **Quy ước Code:** Hàm công khai PascalCase ở `.h`, hàm nội bộ camelCase có `static` ở `.c`. Chú thích bằng tiếng Việt giải thích **LÝ DO (Why)** hơn là **HÀNH ĐỘNG (What)**.

---

## 📂 2. CẤU TRÚC THƯ MỤC CHÍNH
- `include/`:
  - [config.h](file:///Users/mth2610/Desktop/c_games/little_witch/include/config.h): Các hằng số toàn cục (FPS, giới hạn pool, chỉ số cân bằng game) và enum (`GameScreen`, `BiomeState`, `AdState`).
  - [game.h](file:///Users/mth2610/Desktop/c_games/little_witch/include/game.h): Cấu trúc `GameState` toàn cục duy nhất liên kết tất cả các sub-system pools.
  - [witch.h](file:///Users/mth2610/Desktop/c_games/little_witch/include/witch.h), [star.h](file:///Users/mth2610/Desktop/c_games/little_witch/include/star.h), [enemy.h](file:///Users/mth2610/Desktop/c_games/little_witch/include/enemy.h), [particle.h](file:///Users/mth2610/Desktop/c_games/little_witch/include/particle.h), [skill.h](file:///Users/mth2610/Desktop/c_games/little_witch/include/skill.h), [ui.h](file:///Users/mth2610/Desktop/c_games/little_witch/include/ui.h), [biome.h](file:///Users/mth2610/Desktop/c_games/little_witch/include/biome.h), [save.h](file:///Users/mth2610/Desktop/c_games/little_witch/include/save.h), [admob_bridge.h](file:///Users/mth2610/Desktop/c_games/little_witch/include/admob_bridge.h).
- `src/`: Các file `.c` tương ứng triển khai logic.
- `assets/`: Âm thanh (`sounds/`), phông chữ (`fonts/`), hình ảnh (`textures/`), shaders (`shaders/`).

---

## ⚙️ 3. KIẾN TRÚC & HOẠT ĐỘNG CÁC HỆ THỐNG

### 3.1 Vòng lặp & Luồng Màn Hình (`main.c`)
- Quản lý 5 màn hình (`GameScreen`): `SCREEN_MENU`, `SCREEN_GAMEPLAY`, `SCREEN_SKILL_SELECT`, `SCREEN_UPGRADE_SHOP`, `SCREEN_GAMEOVER`.
- Quản lý `RenderTexture2D lightmap` phục vụ vẽ ánh sáng ambient xung quanh nhân vật và các vật thể phát sáng (phong cách Ori).

### 3.2 Phù Thủy (`witch.c`)
- Di chuyển: Lerp vị trí hiện tại theo `targetPosition` (nhấp touch/chuột), bị giới hạn trong vùng canvas 1280x720.
- Hoạt họa chạy/tấn công/phòng thủ dựa trên `animState`.
  - **Tư thế Ngự Sáo Bay (WITCH_STATE_FLY):** Sử dụng sprite sheet 32 khung hình độc lập `witch_fly.png` với góc nghiêng tự động (lên tới $\pm 14^{\circ}$) dựa theo vận tốc dọc $Y$ và hiệu ứng bồng bềnh hình sin.
  - **Cây sáo thần (flute.png):** Được vẽ riêng biệt ngay dưới chân Pháp sư ở tất cả các trạng thái, nghiêng góc đồng bộ và chuyển động cùng nhịp với cơ thể nhân vật.
- **Mana Shield (Khiên ma lực):**
  - Khi kích hoạt, vẽ khiên tròn pulsating bán kính `100.0f` bao quanh Witch cùng quầng sáng radial glow `260px` trên lightmap.
  - Khi khiên hoạt động: người chơi bất tử tuyệt đối (không mất HP khi chạm quái). Nhận sát thương chỉ làm tắt khiên và kích hoạt thời gian sạc lại (8.0s).
  - **Cơ chế tự kích hoạt:** Khi nhận sát thương nguy hiểm, khiên tự động kích hoạt nếu người chơi có sao Thủy (`STAR_THUY`). Bán kính của khiên được tăng theo số sao Thủy.

### 3.3 Linh Hồn Sao / Spirit Orbs (`star.c`)
- Được vẽ hoàn toàn bằng C vector dạng quả cầu gradient phát sáng mờ ảo (Spirit Orb) thay vì dùng ảnh PNG.
- Pool giới hạn `MAX_STARS` (8). Tự do nhấp nháy phát sáng trước khi thu thập.
- Bao gồm 5 hệ Ngũ hành: Kim (Trắng bạc), Mộc (Xanh lá), Thủy (Xanh dương), Hỏa (Đỏ/Cam), Thổ (Vàng) và sao Ngũ sắc (Rainbow).
- Hút bởi kỹ năng Magnet. Khi chạm người chơi, chuyển sang quỹ đạo xoay quanh Witch. Số lượng sao của từng hệ đang xoay quanh sẽ cường hóa trực tiếp sức mạnh các kỹ năng chủ động tương ứng.
- Va chạm quái gây sát thương: Sao ngũ hành biến mất khi chạm quái, riêng sao Ngũ sắc (Rainbow) có khả năng xuyên thấu và hồi sát thương 0.5s trên mỗi quái.

### 3.4 Quái Vật & Boss (`enemy.c`)
- Pool giới hạn `MAX_ENEMIES` (32).
- Quái thường: Slime (đi ngang), Bat (sin-wave), Ghost (tự động ẩn/hiện), Robot (bắn đạn), Drone (dí theo Witch), Alien (zigzag dọc), UFO (bắn đạn tỏa 3 hướng).
- Boss xuất hiện khi đạt điểm số quy định cho mỗi Biome.
- 4 Boss tương ứng: Forest (ném đá, tăng lên 150 HP, có trạng thái Rage phẫn nộ dưới 50% HP ném đá siêu nhanh kèm rung lắc màn hình), Cave (laser quét ngang), City (gọi drone), Space (hố đen kéo Witch). Hiển thị thanh máu (HP bar) động.

### 3.5 Đạn & Hiệu ứng Hạt (`skill.c` / `particle.c`)
- `ProjectilePool` (`MAX_PROJECTILES` = 48) quản lý đạn bắn của Witch (Phi tiêu, hỏa cầu, hơi độc, băng, lốc xoáy) và của quái/boss. Đạn có hiệu ứng vệt sáng hạt phía sau (Sparkle trail) và góc quay tự xoay.
- `ParticlePool` (`MAX_PARTICLES` = 128) xử lý các hạt phát nổ khi quái chết hoặc đạn trúng đích để tạo độ mượt mà.

### 3.6 Kỹ năng & Xích Sét Chain Lightning (`skill.c` / `main.c`)
- **Hệ thống Nâng cấp Cấp độ Kỹ năng (Skill Levels):** Toàn bộ 7 kỹ năng chủ động/bị động được theo dõi cấp độ qua mảng `skillLevels` trong struct `Witch`. Khi người chơi vượt ải và diệt Boss, màn hình `SCREEN_SKILL_SELECT` sẽ cho phép chọn 1 trong 3 thẻ ngẫu nhiên để tăng cấp kỹ năng, trực tiếp cường hóa sát thương, bán kính ảnh hưởng, số tia hoặc giảm thời gian hồi chiêu vĩnh viễn trong run đấu.
- **Kỹ năng Bị động:** Magnet (Nam châm hút sao tự động vĩnh viễn, lực hút tăng theo cấp shop, số lượng sao Kim sở hữu, và cấp độ nâng cấp thẻ kỹ năng).
- **5 Kỹ năng Chủ động Ngũ hành:**
  - **Kim (Golden Shuriken):** Phóng phi tiêu xuyên thấu địch. Sát thương/số lượng phi tiêu tăng theo sao Kim và cấp độ kỹ năng.
  - **Mộc (Poison Cloud):** Triệu hồi hơi độc màu xanh lá cây gây sát thương duy trì (DOT) bán kính lớn theo sao Mộc và cấp kỹ năng.
  - **Thủy (Ice Blast):** Phóng luồng khí lạnh gây sát thương băng và đóng băng quái hoàn toàn (quái đứng im, không bắn đạn) trong thời gian tăng theo sao Thủy và cấp kỹ năng.
  - **Hỏa (Explosive Fireball):** Quả cầu lửa nổ lan gây sát thương diện rộng (AOE) tăng theo sao Hỏa và cấp kỹ năng (bao gồm cả sát thương nổ splash).
  - **Thổ (Lightning & Tornado):** Sét xích (Chain Lightning) truyền qua nhiều quái cùng một cơn Lốc xoáy điện cuồng phong di chuyển lượn sóng hút quái vào tâm, sát thương/phạm vi tăng theo sao Thổ và cấp kỹ năng.
- **Chiêu cuối Ultimate (Elemental Burst):** Kích hoạt bằng phím `F` hoặc nút UI khi đủ 5 hệ sao đang orbit quanh người hoặc có sao Ngũ sắc. Phóng toàn bộ sao thành đạn lớn tự động bẻ lái (homing steering) tìm mục tiêu, gây nổ AOE cực đại (100 sát thương), tạo chớp màn hình trắng và rung lắc màn hình cực mạnh.

### 3.7 Vùng Đất & Phân Giai Đoạn Màn Chơi (`biome.c` / `main.c`)
- 4 Biome: Rừng (Forest), Hang động (Cave), Thành phố (City), Vũ trụ (Space).
- **Phân chia Giai đoạn Màn 1 (Biome Forest):**
  - Màn 1 kéo dài đến 600 điểm, chia làm 3 Phase sinh quái (Phase 1: Slime thong thả 2.5s; Phase 2: Dơi + Slime và sinh đôi 1.8s; Phase 3: Bão quái tốc độ cao 1.2s).
  - Đạt 500+ điểm, môi trường xung quanh sẽ tối dần một cách kịch tính (ambient dimming) kết hợp màn hình rung lắc nhẹ cảnh báo Boss sắp xuất hiện.
  - Tiêu diệt Boss sẽ không qua màn ngay mà chuyển tiếp sang màn hình chọn thẻ kỹ năng `SCREEN_SKILL_SELECT`.
- Mỗi Biome chứa cấu hình màu nền ambient riêng, tốc độ cuộn background vô tận (2 bản copy dịch chuyển song song) và tỉ lệ sinh quái tăng dần.

### 3.8 Lưu Trữ & Shop Nâng Cấp (`save.c` / `ui.c`)
- Đọc/ghi SaveData nhị phân qua `LWA_save.dat`. Lưu cấp độ nâng cấp vĩnh viễn (Mạng, Tốc độ, May mắn, Nam châm), tổng vàng và kỷ lục điểm.
- Shop cho phép dùng vàng kiếm được để nâng cấp vĩnh viễn các chỉ số trên.

### 3.9 Android AdMob JNI Bridge (`admob_bridge.c` / `AdMobBridge.java`)
- Cho phép hồi sinh 1 lần duy nhất bằng cách xem quảng cáo Rewarded Video ở màn hình GameOver.
- Java-side (`AdMobBridge.java`) gọi JNI callbacks (`onRewardedAdCompleted`, `onRewardedAdFailed`) để cập nhật trạng thái `AdState` về phía C.
- C-side (`admob_bridge.c`) tự động lưu JVM pointer tại `JNI_OnLoad` và cache class reference toàn cục để JNI hoạt động đa luồng. Cung cấp hàm cầu nối (`callStaticVoidMethod`) tự động `Attach`/`Detach` thread JVM của Game Loop để gọi Java (`loadRewardedAd`, `showRewardedAd`).

---

## 🛠️ 4. HƯỚNG DẪN BUILD NHANH
- **Yêu cầu:** CMake 3.12+, GCC/Clang hỗ trợ C99, Android SDK/NDK r27+ (nếu build Android).
- **Lệnh biên dịch trên PC:**
  ```bash
  cmake -B build
  cmake --build build
  ```
  File thực thi xuất ra tại `build/bin/littlewitch`.

- **Lệnh biên dịch cho Android (APK):**
  Yêu cầu cài đặt Gradle (hoặc có sẵn cache Wrapper). Cấu hình file `android/local.properties` chỉ đúng đường dẫn SDK và NDK, sau đó chạy lệnh ở thư mục `android/`:
  ```bash
  cd android
  ./gradlew assembleDebug
  ```
  File APK đầu ra nằm tại `android/app/build/outputs/apk/debug/app-debug.apk`.

---

## ⚠️ 5. CÁC ĐIỂM LƯU Ý ĐẶC BIỆT TRÁNH BỊ LỖI LẠI (REGRESSION)
1. **Trong suốt hình ảnh (Transparent Sprite):** Khi nạp spritesheet `assets/little_witch.png`, hàm load texture tự động xử lý tách màu đen tuyệt đối (#000000) của ảnh thành transparent để nhân vật không bị loang lổ. Tránh sửa phần logic chuyển màu đen này trong loader.
2. **Mana Shield:** Trạng thái bất tử của khiên hoạt động theo cơ chế chặn hoàn toàn sát thương mà không làm giảm máu của khiên trong suốt thời gian hoạt động.
3. **Sao (Stars):** Không cố load file texture của sao từ thư mục assets vì game đã chuyển hẳn sang vẽ vector "Spirit Orbs" gradient mịn để đồng nhất phong cách nghệ thuật giống game Ori.
4. **Rung lắc màn hình (Screen Shake):** Hiệu ứng rung màn hình chỉ được áp dụng lên Virtual Canvas khi màn hình đang ở trạng thái Gameplay (`SCREEN_GAMEPLAY`). Các biến rung màn hình (`shakeDuration`, `shakeIntensity`) phải được reset về `0.0f` ngay lập tức khi chuyển sang màn hình GameOver (`SCREEN_GAMEOVER`) để đảm bảo bảng điều khiển hồi sinh/nâng cấp không bị rung lắc.
