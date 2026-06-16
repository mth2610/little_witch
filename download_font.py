import os
import urllib.request

FONT_DIR = "assets/fonts"
FONT_PATH = os.path.join(FONT_DIR, "clean_font.ttf")
# URL liên kết tải font Roboto-Regular từ thư mục src/hinted của kho roboto-2 trên GitHub
FONT_URL = "https://raw.githubusercontent.com/googlefonts/roboto-2/main/src/hinted/Roboto-Regular.ttf"

if not os.path.exists(FONT_DIR):
    os.makedirs(FONT_DIR)

print(f"Dang tai phông chu tu {FONT_URL} den {FONT_PATH}...")
try:
    urllib.request.urlretrieve(FONT_URL, FONT_PATH)
    print("Tai phông chu thanh cong!")
except Exception as e:
    print(f"Tai phông chu that bai: {e}")
