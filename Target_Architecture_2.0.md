# 🚀 TARGET ARCHITECTURE (KIẾN TRÚC HƯỚNG TỚI - PHIÊN BẢN 2.0)

Tài liệu này vạch ra kiến trúc hệ thống và phân rã công việc chi tiết cho các sub-agents hoạt động độc lập nhằm nâng cấp game Little Witch Adventure lên phiên bản 2.0.

---

## 🎯 1. ĐỊNH HƯỚNG VÀ THIẾT KẾ MỚI
*   **Vibe:** Nhẹ nhàng, thư giãn kết hợp chiến thuật sâu sắc (70% Thư giãn - 20% Chiến thuật - 10% Bùng nổ).
*   **Nghệ thuật:** Linh khí Ngũ Hành, thiên nhiên, thần thoại Á Đông. Thay thế các kẻ địch Sci-fi (Robot, Drone, UFO) bằng các linh linh, yêu linh tương ứng.
*   **Trọng tâm kỹ thuật:** Tăng cường VFX bằng Shaders kết hợp hạt (Particle) và ánh sáng ambient (Lightmap), tối ưu hóa âm thanh phân lớp không bị lệch pha, đảm bảo chạy mượt trên cả PC và Android.

---

## ⚙️ 2. KIẾN TRÚC CHI TIẾT CÁC TÍNH NĂNG MỚI

### 2.1 Cơ chế Tấn công Thường Yếu (Weak Attack)
*   **Điều kiện:** Số lượng sao đang bay quanh Witch (`witch.starCount == 0`).
*   **Logic:** Tự động bắn ra luồng linh khí nhỏ, sát thương rất thấp (ví dụ: 1-2 HP) với tần suất trung bình để người chơi tự vệ ở đầu màn chơi.
*   **Tập tin ảnh hưởng:** `src/witch.c`, `src/skill.c`, `include/witch.h`.

### 2.2 Hệ thống Cân bằng Ngũ Hành (Wu Xing Balance)
*   **Logic:** Theo dõi số lượng sao của từng hệ đang sở hữu. Khi một hệ vượt trội:
    *   Tăng tỉ lệ sinh quái thuộc hệ khắc chế hoặc tương ứng hệ đó.
    *   Thay đổi màu sắc bầu trời (`targetSkyColor`) bằng cách pha trộn màu ambient trên lightmap.
*   **Chỉ báo UI:** Thêm thanh chỉ báo trạng thái cân bằng Ngũ Hành (Kim, Mộc, Thủy, Hỏa, Thổ) trên HUD.
*   **Tập tin ảnh hưởng:** `src/biome.c`, `src/enemy.c`, `src/ui.c`, `include/game.h`.

### 2.3 Trạng thái Tĩnh Tâm (Meditation State)
*   **Điều kiện kích hoạt:** 10 giây liên tục không nhận sát thương, không sử dụng Ultimate, không Dash (lướt).
*   **Hiệu ứng:** 
    *   Tăng mạnh lực hút của Magnet.
    *   Tăng 20% EXP nhận được.
    *   Phát ra luồng linh khí phát sáng xung quanh Witch.
    *   Kích hoạt lớp nhạc Sáo trúc ngân nga êm dịu.
*   **Chỉ báo UI:** Vòng sáng đếm ngược (Charging Ring) chạy quanh chân Witch.
*   **Tập tin ảnh hưởng:** `src/witch.c`, `src/ui.c`, `include/witch.h`.

### 2.4 Thiết kế Boss & Vòng Tương Khắc Ngũ Hành
*   **Mộc Thần:** Yếu trước **Kim** (Nhận thêm 50% sát thương từ kỹ năng phi tiêu). Hồi máu liên tục nếu không bị trúng độc (Mộc).
*   **Thủy Linh:** Yếu trước **Thổ** (Bị làm chậm/sát thương lớn từ Lốc xoáy/Sét xích).
*   **Kim Giáp:** Yếu trước **Hỏa** (Giáp dày, chỉ bị phá hủy nhanh bởi Hỏa cầu nổ lan).
*   **Hỏa Thần:** Yếu trước **Thủy** (Yếu trước Băng).
*   **Thổ Thần:** Yếu trước **Mộc** (Yếu trước hơi độc).
*   **Tập tin ảnh hưởng:** `src/enemy_boss.c`, `src/enemy.c`, `src/skill.c`.

### 2.5 Hệ thống Shader Đa Nền Tảng
*   **Cấu trúc thư mục mới:**
    *   `assets/shaders/glsl330/` (Dành cho PC - OpenGL 3.3).
    *   `assets/shaders/gles/` (Dành cho Android - OpenGL ES 2.0/3.0).
*   **Shader cần tối ưu/bổ sung:** Bloom pipeline, Mana Shield Distortion, Frost Distortion, Chain Lightning, Shockwave Ripple.
*   **Tập tin ảnh hưởng:** `src/main.c`, `src/skill.c`, `assets/shaders/`.

### 2.6 Âm thanh Phân lớp (Layered Audio Crossfade)
*   **Logic:** Mix sẵn các phiên bản nhạc nền theo mức độ kịch tính (Relax, Medium, Epic). Sử dụng cơ chế Crossfade (lerp âm lượng nhỏ dần track hiện tại và lớn dần track mới) để thay đổi không khí mượt mà, tránh hiện tượng lệch pha (Audio Drifting) khi phát nhiều file song song.
*   **Tập tin ảnh hưởng:** `src/main.c`.

---

## 👥 3. PHÂN CHIA NHIỆM VỤ CHO CÁC SUB-AGENTS ĐỘC LẬP

Để phát triển song song hiệu quả, công việc được chia nhỏ cho 5 Sub-agents chuyên trách dưới đây:

### 1. 🛡️ Agent Visuals (VFX & Shader Specialist)
*   **Nhiệm vụ:**
    *   Tái cấu trúc thư mục shader thành hai phiên bản `glsl330` và `gles`.
    *   Nâng cấp shader Mana Shield (tạo hiệu ứng Distortion méo dạng không gian xung quanh).
    *   Viết Shader Bloom Pipeline cơ bản cho Virtual Canvas để tạo hiệu ứng Ori-glow rực rỡ.
    *   Tạo shader cho quả cầu lửa (Fire Noise) và hơi đóng băng (Frost Distortion).
*   **Ràng buộc:** Đảm bảo shader chạy được trên Android giả lập và PC không bị lỗi compile.

### 2. 🎮 Agent Gameplay (Core Logic & Mechanics)
*   **Nhiệm vụ:**
    *   Triển khai đòn đánh thường yếu khi phù thủy có 0 sao (`starCount == 0`).
    *   Xây dựng logic đếm ngược và kích hoạt trạng thái **Tĩnh Tâm** (`Meditation State`).
    *   Xây dựng biến số theo dõi độ mất cân bằng Ngũ Hành và tác động của nó lên tần suất xuất hiện quái, màu nền ambient.
*   **Ràng buộc:** Tích hợp gọn gàng vào mảng trạng thái tĩnh, không dùng cấp phát động.

### 3. 👹 Agent Boss (Enemy & Boss AI Designer)
*   **Nhiệm vụ:**
    *   Refactor thay thế tên và asset các quái vật Sci-fi (Robot, Drone, UFO) sang tên gọi thần thoại Á Đông.
    *   Tái cấu trúc 4 Boss hiện tại và viết thêm Boss thứ 5 (Thổ Thần).
    *   Cài đặt logic tương khắc thuộc tính (nhân sát thương thích hợp dựa trên hệ đạn).
    *   Vẽ thanh HP Boss nghệ thuật hơn dưới chân hoặc trên đầu màn hình.

### 4. 🎵 Agent Sound (Audio Engine Optimizer)
*   **Nhiệm vụ:**
    *   Thiết kế hàm `UpdateBGMState()` thực hiện crossfade mượt mà giữa các bài nhạc nền trộn sẵn khi trạng thái game thay đổi (Bình thường ↔ Tĩnh Tâm ↔ Boss xuất hiện).
    *   Tối ưu hóa bộ nhớ âm thanh trên Android (sử dụng OGG nén chất lượng cao).

### 5. 📱 Agent UI (Controls & Interface Developer)
*   **Nhiệm vụ:**
    *   Vẽ vòng sáng đếm ngược (Charging Ring) dưới chân Witch khi đang tích lũy Tĩnh Tâm.
    *   Thiết kế thanh chỉ báo cân bằng Ngũ Hành (Wu Xing indicator) trên góc màn hình.
    *   Cải thiện layout của màn hình Chọn kỹ năng (`SCREEN_SKILL_SELECT`) và cửa hàng nâng cấp (`SCREEN_UPGRADE_SHOP`) cho trực quan hơn.
    *   Tối ưu nút cảm ứng di chuyển và phím dash trên Android.

---

> ⚠️ **LUẬT DÀNH CHO SUB-AGENTS:**
> *   Trước khi sửa đổi cấu trúc dữ liệu chung (`config.h` hoặc `game.h`), bắt buộc phải đồng bộ hóa với Agent Gameplay để tránh xung đột biên dịch (Compilation Error).
> *   Luôn tuân thủ nguyên tắc C99 thuần, static memory allocation (không dùng `malloc`/`free`).
