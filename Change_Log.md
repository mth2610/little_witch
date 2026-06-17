# 📝 CHANGE LOG (NHẬT KÝ THAY ĐỔI) - LITTLE WITCH ADVENTURE 2.0

Tài liệu này ghi lại toàn bộ các thay đổi về cấu trúc dữ liệu, hàm mới, hoặc chỉnh sửa hệ thống được thực hiện bởi các Agent trong quá trình nâng cấp game.

---

## 📌 HƯỚNG DẪN DÀNH CHO CÁC AGENT:
1. **ĐỌC TRƯỚC KHI LÀM:** Luôn đọc file này để xem có sự thay đổi nào từ các Agent khác ảnh hưởng đến phần việc của mình không.
2. **CẬP NHẬT KHI XONG:** Ngay sau khi sửa code hoặc thêm cấu trúc dữ liệu mới, bắt buộc ghi nhận vào đây để các Agent khác biết cách sử dụng.
3. **KHÔNG XÓA LỊCH SỬ:** Chỉ thêm mới vào phần trên cùng.

---

## 🔄 LỊCH SỬ CẬP NHẬT

### [2026-06-17] Visuals Agent (Chuyên viên Đồ họa & Shader)
*   **Tái cấu trúc thư mục Shader:** Tạo thư mục `assets/shaders/glsl330/` (PC) và `assets/shaders/gles/` (Android). Dùng macro `SHADER_PATH` trong `src/main.c` để tự động chuyển đổi đường dẫn shader theo nền tảng.
*   **Bloom Post-Processing Pipeline:** Thêm hai shader `bloom_extract.fs` và `bloom_blur.fs` để trích xuất và làm mờ các vùng phát sáng trên Virtual Canvas độ phân giải thấp `320x180`. Cộng chập Bloom (Additive Blending) lại canvas chính trước khi vẽ HUD, tạo ra hiệu ứng Ori-glow rực rỡ, huyền ảo.
*   **Nâng cấp Mana Shield & Frost Distortion:** Thiết lập cơ chế copy texture `gs.screenCopyTex` từ canvas ảo hiện thời ngay trước khi vẽ đạn và witch, truyền làm `screenTexture` cho `shield.fs` và `ice_blast.fs` để thực hiện biến dạng không gian (Space Distortion) bằng khúc xạ ánh sáng động.
*   **Sửa lỗi viền cứng (Hard Edges) của hệ Băng:** Tính toán mặt nạ nhòe viền (Alpha Mask/Vignette) từ tọa độ UV (`fragTexCoord`) trực tiếp ở đầu hàm `main()` trong `ice_blast.fs` (cả bản glsl330 và gles). Loại bỏ mảnh pixel sớm bằng `discard` nếu độ mờ viền $< 0.001$ giúp nâng cao hiệu năng và làm hiệu ứng biến mất mượt mà 100% vào nền.
*   **Tối ưu hóa Shader GLES:** Viết lại toàn bộ 9 shader cho GLESv2, hạ số bước raymarching của quả cầu lửa (`fireball.fs`) từ 35 xuống 20, lốc xoáy (`tornado.fs`) từ 22 xuống 15, mô phỏng hàm `tanh()` thủ công để tránh crash trên các thiết bị Android cũ.
*   **Cleanup:** Xóa bỏ các file shader cũ nằm ở thư mục gốc `assets/shaders/`.
