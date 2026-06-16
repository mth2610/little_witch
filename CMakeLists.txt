cmake_minimum_required(VERSION 3.12)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(littlewitch C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Thiết lập chế độ Build mặc định là Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Thư mục xuất file chạy
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# cấu hình FetchContent để tự động tải Raylib
include(FetchContent)

# Tối ưu hóa build Raylib bằng cách tắt các module không cần thiết
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "Build raylib games" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "Customize raylib build features" FORCE)

# Tải Raylib phiên bản 5.0
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG        5.0
)

FetchContent_MakeAvailable(raylib)

# Include thư mục header
include_directories(include)

# Tập hợp toàn bộ file nguồn .c trong src/
file(GLOB_RECURSE SRC_FILES src/*.c)

# Khai báo file thực thi
add_executable(littlewitch ${SRC_FILES})

# Liên kết tĩnh với thư viện Raylib
target_link_libraries(littlewitch PRIVATE raylib)

# Cấu hình các frameworks bắt buộc cho macOS
if(APPLE)
    target_link_libraries(littlewitch PRIVATE
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()
