cmake_minimum_required(VERSION 3.12)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(littlewitch C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Thiết lập chế độ Build mặc định là Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Thư mục xuất file chạy (chỉ áp dụng cho PC)
if(NOT ANDROID)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# Cấu hình FetchContent để tự động tải Raylib
include(FetchContent)

# Tối ưu hóa build Raylib bằng cách tắt các module không cần thiết
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "Build raylib games" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "Customize raylib build features" FORCE)

# Thiết lập nền tảng Android cho Raylib nếu đang cross-compile
if(ANDROID)
    set(PLATFORM "Android" CACHE STRING "Target platform" FORCE)
endif()

# Tải Raylib phiên bản 5.0
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG        5.5
)

FetchContent_MakeAvailable(raylib)

# Include thư mục header
include_directories(
    include
    include/core
    include/enemies
    include/skills
    include/biomes
    include/screens
)

# Tập hợp toàn bộ file nguồn .c trong src/
file(GLOB_RECURSE SRC_FILES src/*.c)

# Khai báo mục tiêu build (Shared Library cho Android, Executable cho PC)
if(ANDROID)
    # Tìm thư mục chứa android_native_app_glue.c từ NDK
    set(NATIVE_APP_GLUE_DIR ${ANDROID_NDK}/sources/android/native_app_glue)
    include_directories(${NATIVE_APP_GLUE_DIR})
    
    # Tạo thư viện liên kết động (.so) cho game
    add_library(littlewitch SHARED 
        ${SRC_FILES}
        ${NATIVE_APP_GLUE_DIR}/android_native_app_glue.c
    )
    
    # Liên kết các thư viện hệ thống bắt buộc của Android & Raylib
    target_link_libraries(littlewitch PRIVATE
        raylib
        android
        log
        EGL
        GLESv2
        m
    )
    
    # Ngăn linker lược bỏ hàm khởi tạo Native Activity ANativeActivity_onCreate
    target_link_options(littlewitch PRIVATE "-Wl,-u,ANativeActivity_onCreate")
else()
    # Build ứng dụng chạy trực tiếp trên máy tính
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
endif()
