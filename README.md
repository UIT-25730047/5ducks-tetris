# 🎮 Game Tetris - Phiên Bản Terminal

[![C++](https://img.shields.io/badge/C++-11-blue.svg)](https://en.cppreference.com/w/cpp/11)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](https://github.com/UIT-25730047/5ducks-tetris)

Game Tetris cổ điển được lập trình bằng C++ chạy trực tiếp trên terminal! Được phát triển bởi **Nhóm 5 Ducks** trong khuôn khổ đồ án môn Kỹ Năng Nghề Nghiệp tại UIT (Trường Đại học Công nghệ Thông tin).

## 📋 Mục Lục

- [Giới Thiệu](#-giới-thiệu)
- [Tính Năng](#-tính-năng)
- [Cấu Trúc Dự Án](#-cấu-trúc-dự-án)
- [Yêu Cầu Hệ Thống](#-yêu-cầu-hệ-thống)
- [Hướng Dẫn Cài Đặt](#-hướng-dẫn-cài-đặt)
- [Cách Chơi](#-cách-chơi)
- [Phím Điều Khiển](#️-phím-điều-khiển)
- [Hệ Thống Tính Điểm](#-hệ-thống-tính-điểm)
- [Kiến Trúc Kỹ Thuật](#-kiến-trúc-kỹ-thuật)
- [Thành Viên Nhóm](#-thành-viên-nhóm)
- [Đóng Góp](#-đóng-góp)

## 🎯 Giới Thiệu

Dự án này tái hiện trải nghiệm chơi game Tetris cổ điển trực tiếp trên terminal sử dụng C++ với thư viện hệ thống POSIX. Game được phát triển theo phong cách **Object-Oriented Programming (OOP)** với kiến trúc modular, dễ mở rộng và bảo trì.

### 🎨 Đặc điểm nổi bật

- **Kiến trúc OOP**: Sử dụng class encapsulation, separation of concerns
- **Cross-platform sound**: Hỗ trợ cả macOS (afplay) và Linux (aplay/mpg123)
- **Unicode rendering**: Box-drawing characters (╔═╗║╚╝) cho giao diện đẹp mắt
- **ANSI colors**: 7 màu sắc cho 7 loại Tetromino
- **Terminal I/O**: POSIX APIs (termios, fcntl) cho non-blocking input
- **Persistent storage**: Lưu top 10 high scores vào file

## ✨ Tính Năng

- 🎨 **Gameplay Tetris Cổ Điển**: Đầy đủ 7 mảnh Tetromino truyền thống (I, O, T, S, Z, J, L)
- ⌨️ **Điều Khiển Trực Quan**: Phím điều khiển với WASD hoặc phím mũi tên
- 📊 **Hệ Thống Tính Điểm**: Điểm dựa trên số hàng xóa, có hệ số nhân theo cấp độ
- 🎵 **Hiệu Ứng Âm Thanh**: Nhạc nền Tetris cổ điển và hiệu ứng âm thanh (soft drop, hard drop, line clear, level up, game over)
- 📈 **Độ Khó Tăng Dần**: Hệ thống cấp độ động tăng tốc độ (configurable qua `LINES_PER_LEVEL` constant)
- 👻 **Ghost Piece**: Hiển thị preview vị trí khối sẽ rơi (toggle bằng phím G)
- 📋 **Hiển Thị Thống Kê**: Theo dõi điểm số, cấp độ, số hàng đã xóa và khối tiếp theo
- ⏸️ **Tính Năng Tạm Dừng**: Tạm dừng và tiếp tục bất cứ lúc nào (phím P)
- 🏆 **Theo Dõi Điểm Cao**: Lưu trữ top 10 điểm cao nhất vào file `highscores.txt`

## 📁 Cấu Trúc Dự Án

```
5ducks-tetris/
├── main.cpp              # Entry point của game
├── TetrisGame.h          # Class chính - game loop & logic
├── TetrisGame.cpp        # Implementation của TetrisGame
├── Board.h               # Class quản lý bảng chơi
├── Board.cpp             # Rendering & line clearing
├── Piece.h               # Class Piece và struct Position
├── GameState.h           # Class quản lý game state
├── BlockTemplate.h       # Class static cho 7 tetromino templates
├── BlockTemplate.cpp     # Rotation logic
├── SoundManager.h        # Class static cho audio system
├── SoundManager.cpp      # Platform-aware sound playback
├── sounds/               # Thư mục chứa các file âm thanh (.wav)
│   ├── background_sound_01.wav
│   ├── soft_drop_2.wav
│   ├── hard_drop.wav
│   ├── lock_piece.wav
│   ├── line_clear.wav
│   ├── 4lines_clear.wav
│   ├── level_up.wav
│   └── game_over.wav
├── highscores.txt        # File lưu top 10 điểm cao (tự động tạo)
└── README.md             # File này
```

## 💻 Yêu Cầu Hệ Thống

### Khuyến nghị: Linux
- **Hệ điều hành**: Linux (Ubuntu 20.04+, Fedora 30+, Debian 10+, Arch Linux)
- **Terminal**: Hỗ trợ ANSI escape codes và UTF-8 encoding
- **Compiler**: GCC 7.0+ hoặc Clang 5.0+ với hỗ trợ C++11
- **Audio**: `aplay` (ALSA), `mpg123`, hoặc `ffplay` cho sound effects
- **CPU**: Intel Core i3 hoặc tương đương
- **RAM**: 2GB trở lên
- **Dung lượng**: 50MB dung lượng trống

### Tương thích nền tảng

✅ **Linux**: Hỗ trợ đầy đủ, khuyến nghị sử dụng
- Game chạy tốt nhất trên Linux do sử dụng POSIX APIs và Unicode box-drawing characters

⚠️ **macOS**: Có thể compile nhưng **không khuyến nghị**
- Vấn đề hiển thị box-drawing characters trên macOS terminal
- Một số Unicode symbols có thể hiển thị không chính xác
- Game có thể bị đứng hoặc render sai

❌ **Windows**: Chưa hỗ trợ
- Người dùng Windows có thể sử dụng **WSL2** (Windows Subsystem for Linux) để chạy game

## 🚀 Hướng Dẫn Cài Đặt

### 1. Cài đặt dependencies (Linux)

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential g++ alsa-utils mpg123 pulseaudio
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ alsa-utils mpg123 pulseaudio
```

**Arch Linux:**
```bash
sudo pacman -S base-devel alsa-utils mpg123 pulseaudio
```

### 2. Clone repository

```bash
git clone https://github.com/UIT-25730047/5ducks-tetris.git
cd 5ducks-tetris
```

### 3. Compile game

**QUAN TRỌNG**: Bạn phải compile **tất cả file .cpp** cùng nhau:

```bash
g++ -std=c++11 *.cpp -o tetris
```


### 4. Chuẩn bị terminal

Đảm bảo terminal của bạn:
- Hỗ trợ UTF-8 encoding (check: `echo $LANG` - nên là `en_US.UTF-8` hoặc tương tự)
- Kích thước tối thiểu: 80 cột × 24 hàng
- Hiển thị được box-drawing characters (╔═╗║╚╝)

### 5. Chạy game

```bash
./tetris
```

### Troubleshooting

**Lỗi compile:**
```bash
# Check compiler version
g++ --version  # Cần >= 7.0

# Nếu thiếu C++11 support
sudo apt-get install gcc-7 g++-7
```

**Box-drawing characters bị vỡ:**
```bash
# Set UTF-8 encoding
export LANG=en_US.UTF-8
```

**Không có âm thanh:**
```bash
# Cài đặt ALSA (Ubuntu/Debian)
sudo apt-get install alsa-utils

# Hoặc mpg123 (cho MP3)
sudo apt-get install mpg123

# Hoặc ffplay (fallback)
sudo apt-get install ffmpeg

# Hoặc pulseaudio (fallback)
sudo apt-get install pulseaudio
```

**Game bị đứng trên macOS:**
- Đây là vấn đề đã biết
- Khuyến nghị: Chạy trên Linux hoặc WSL2

**Phím không phản hồi:**
- Đảm bảo terminal ở chế độ interactive (không pipe input/output)
- Thử khởi động lại terminal

## 🎮 Cách Chơi

### Mục Tiêu
Sắp xếp các mảnh Tetromino rơi xuống để tạo thành các hàng ngang hoàn chỉnh. Khi một hàng được hoàn thành, nó sẽ biến mất và bạn nhận được điểm. Game kết thúc khi các mảnh chồng lên đến đỉnh màn hình.

### Bảy Mảnh Tetromino

| Mảnh | Hình Dạng | Màu Sắc | Chiến Thuật |
|------|-----------|---------|-------------|
| **Khối I** | ████ | Xanh dương (Cyan) | Để dành xóa 4 hàng cùng lúc (Tetris!) |
| **Khối O** | ██<br>██ | Vàng (Yellow) | Mảnh duy nhất không xoay được, lấp khoảng trống lớn |
| **Khối T** | ▀█▀ | Tím (Purple) | Rất linh hoạt, có thể thực hiện T-Spin |
| **Khối S** | ▄█▀ | Xanh lá (Green) | Tạo các đường zigzag |
| **Khối Z** | ▀█▄ | Đỏ (Red) | Đối xứng với khối S |
| **Khối J** | ▄██ | Xanh đậm (Blue) | Tốt để lấp các góc |
| **Khối L** | ██▄ | Cam (Orange) | Đối xứng với khối J |

## ⌨️ Phím Điều Khiển

| Phím | Chức Năng |
|------|-----------|
| `A` hoặc `←` | Di chuyển mảnh sang trái |
| `D` hoặc `→` | Di chuyển mảnh sang phải |
| `S` hoặc `↓` | Rơi nhanh (soft drop) |
| `W` hoặc `↑` | Xoay mảnh theo chiều kim đồng hồ |
| `Space` | Rơi ngay lập tức (hard drop) |
| `G` | Bật/tắt Ghost Piece (bóng ma) |
| `P` | Tạm dừng/Tiếp tục game |
| `Q` | Thoát game |

> **Mẹo**: Giữ phím di chuyển để di chuyển liên tục!

## 📊 Hệ Thống Tính Điểm

| Hành Động | Số Hàng Xóa | Điểm Cơ Bản |
|-----------|-------------|-------------|
| Single | 1 hàng | 100 điểm |
| Double | 2 hàng | 300 điểm |
| Triple | 3 hàng | 500 điểm |
| **Tetris** | **4 hàng** | **800 điểm** |

### Hệ Số Nhân Theo Cấp Độ
Điểm số của bạn được nhân với cấp độ hiện tại!
- **Công thức**: `score += base_points × level`
- **Ví dụ**: Xóa 4 hàng ở Cấp độ 5 = 800 × 5 = **4,000 điểm**

### Tiến Độ Cấp Độ

Mỗi `LINES_PER_LEVEL` hàng xóa (mặc định: **10 hàng**) → tăng 1 level

**Công thức**: `level = 1 + (linesCleared / LINES_PER_LEVEL)`

**Tốc độ rơi theo level**:
- Level 1-3: 0.50s (chậm)
- Level 4-6: 0.30s (trung bình)
- Level 7-9: 0.15s (nhanh)
- Level 10+: 0.08s (rất nhanh!)

> **Lưu ý**: Bạn có thể điều chỉnh độ khó bằng cách thay đổi constant `LINES_PER_LEVEL` trong file `TetrisGame.h:20`

## 🏗️ Kiến Trúc Kỹ Thuật

### Object-Oriented Design

Game được thiết kế theo mô hình OOP với các class chính:

**Core Classes:**
- `TetrisGame`: Orchestrate game loop, logic và state
- `Board`: Quản lý playfield (20×15 grid), rendering, line clearing
- `Piece`: Đại diện cho một Tetromino piece
- `GameState`: Lưu trữ game state (score, level, lines cleared, high scores)
- `BlockTemplate`: Static templates cho 7 loại Tetromino với rotation logic
- `SoundManager`: Platform-aware audio playback system

**Supporting Structures:**
- `Position`: Simple POD struct cho 2D coordinates

### Technical Features

**Terminal I/O:**
- POSIX `termios` cho raw mode (no echo, no buffering)
- POSIX `fcntl` cho non-blocking input
- ANSI escape sequences cho colors và cursor control
- Unicode box-drawing characters cho UI borders

**Rendering:**
- Double-buffering approach để giảm screen flickering
- ANSI 256-color codes cho 7 piece colors
- Cache next piece preview để tránh regenerate mỗi frame

**Sound System:**
- Platform detection: macOS (`__APPLE__`) vs Linux
- macOS: `afplay` cho audio playback
- Linux: Fallback chain (`aplay` → `mpg123` → `ffplay`)
- Background music loop với `pkill` cleanup
- Non-blocking sound effects với `system()` calls

**Game Mechanics:**
- Collision detection: O(16) algorithm (4×4 bounding box)
- Rotation: 90° clockwise transformation `(row, col) → (col, 3 - row)`
- Wall kick: Thử 7 vị trí offset khi rotate
- Ghost piece: Simulate hard drop để preview landing position
- Line clearing: O(n) scan + shift algorithm

### Customization

**Adjustable Constants** (TetrisGame.h):
```cpp
constexpr long BASE_DROP_SPEED_US  = 500000;  // Base tick duration
constexpr int  DROP_INTERVAL_TICKS = 5;       // Ticks per drop
constexpr int  LINES_PER_LEVEL     = 10;      // Lines to level up
constexpr int  ANIM_DELAY_US       = 15000;   // Game over animation delay
constexpr int  LINES_PER_LEVEL     = 10;      // Lines to level up
```

**Board Dimensions** (Board.h):
```cpp
constexpr int BOARD_HEIGHT = 20;
constexpr int BOARD_WIDTH  = 15;
```

## 👥 Thành Viên Nhóm

**Môn học**: Kỹ Năng Nghề Nghiệp

**Giảng viên**: ThS. Nguyễn Văn Toàn

**Trường**: Trường Đại học Công nghệ Thông tin (UIT)

**Nhóm 5 Ducks** - CN1.K2025.1.CNTT

| Họ và Tên | MSSV | Vai Trò |
|-----------|------|---------|
| Lê Quang Nhật | 25730047 | Trưởng nhóm, UI/UX, Performance optimization, Tài liệu |
| Dương Hoà Long | 25730040 | Input handling, Collision detection, Testing |
| Lê Hữu Nhị | 25730048 | Rotation logic, Wall kick, Code refactoring |
| Nguyễn Duy Thanh | 25730068 | Ghost piece, Sound system, Platform compatibility |
| Kiều Quang Việt | 25730093 | High scores, Pause/Resume, Game over animation |

## 🔧 Công Cụ Phát Triển

- **Quản lý công việc**: [GitHub Projects](https://github.com/users/UIT-25730047/projects/1)
- **Quản lý mã nguồn**: [GitHub Repository](https://github.com/UIT-25730047/5ducks-tetris)
- **Giao tiếp**: [Slack Workspace](https://app.slack.com/client/T09M5KGA799/C0A0AR9KJ4X)
- **Soạn thảo tài liệu**: [Overleaf](https://www.overleaf.com/read/jnjfgkqtvpsh#9f751d)

## 🤝 Đóng Góp

Chúng tôi hoan nghênh các đóng góp! Đây là cách bạn có thể giúp đỡ:

1. Fork repository
2. Tạo feature branch (`git checkout -b feature/TinhNangTuyetVoi`)
3. Commit các thay đổi (`git commit -m 'Add: Thêm tính năng tuyệt vời'`)
4. Push lên branch (`git push origin feature/TinhNangTuyetVoi`)
5. Mở Pull Request

### Hướng Dẫn Phát Triển

- Tuân theo coding conventions:
  - Class names: `PascalCase` (VD: `TetrisGame`, `SoundManager`)
  - Function/variable names: `camelCase` (VD: `handleInput`, `currentPiece`)
  - Constants: `UPPER_SNAKE_CASE` (VD: `BOARD_WIDTH`, `LINES_PER_LEVEL`)
- Viết commit message rõ ràng (format: `<type>: <message>`)
- Thêm comment cho logic phức tạp
- Test kỹ lưỡng trước khi submit PR
- Cập nhật tài liệu khi cần thiết

## 📝 Câu Hỏi Thường Gặp

**H: Game có chạy trên Windows không?**
Đ: Chưa hỗ trợ trực tiếp. Người dùng Windows nên sử dụng WSL2 (Windows Subsystem for Linux) để chạy game.

**H: Game có chạy trên macOS không?**
Đ: Có thể compile nhưng **không khuyến nghị** do vấn đề hiển thị Unicode characters. Game chạy tốt nhất trên Linux.

**H: Tại sao terminal không hiển thị màu đúng?**
Đ: Đảm bảo terminal hỗ trợ ANSI escape codes. Hầu hết terminal hiện đại (GNOME Terminal, Konsole, iTerm2) đều hỗ trợ.

**H: Làm sao để thay đổi độ khó game?**
Đ: Thay đổi constant `LINES_PER_LEVEL` trong file `TetrisGame.h:20`. Giảm giá trị (VD: 5) để game khó hơn, tăng giá trị (VD: 20) để dễ hơn.

**H: File âm thanh nằm ở đâu?**
Đ: Tất cả file âm thanh (.wav) nằm trong thư mục `sounds/` cùng thư mục với executable.

**H: Làm sao để tắt âm thanh?**
Đ: Hiện tại chưa có option trong game. Bạn có thể comment out các dòng `SoundManager::play*()` trong source code và recompile.

**H: Game bị giật hoặc phím không phản hồi?**
Đ: Thử:
- Đóng các ứng dụng terminal khác
- Tăng buffer size của terminal
- Khởi động lại terminal
- Đảm bảo terminal không bị lag do quá nhiều processes

## 📞 Hỗ Trợ & Liên Hệ

- **GitHub Issues**: [Báo lỗi hoặc yêu cầu tính năng](https://github.com/UIT-25730047/5ducks-tetris/issues)
- **Slack Community**: [Tham gia workspace](https://app.slack.com/client/T09M5KGA799/C0A0AR9KJ4X)

## 📚 Tài Liệu Tham Khảo

- **Tetris Original**: [Alexey Pajitnov (1985)](https://en.wikipedia.org/wiki/Tetris)
- **Tetris Wiki**: [Gameplay & Strategy](https://tetris.wiki/Gameplay)
- **POSIX termios**: [Terminal I/O Manual](https://man7.org/linux/man-pages/man3/termios.3.html)
- **C++ Reference**: [C++11 Standard](https://en.cppreference.com/w/cpp/11)
- **GitHub Flow**: [Branching Strategy](https://docs.github.com/en/get-started/quickstart/github-flow)

## 🙏 Ghi Nhận

- **Alexey Pajitnov** - Người sáng tạo Tetris gốc (1985)
- **ThS. Nguyễn Văn Toàn** - Giảng viên hướng dẫn
- **UIT (Trường Đại học Công nghệ Thông tin)** - Hỗ trợ học thuật
- Tất cả những người đóng góp và tester

---

<div align="center">

**🎮 Được làm với ❤️ bởi Nhóm 5 Ducks**

*"Trong Tetris như trong cuộc sống, những thành tựu (achievements) biến mất, còn những sai lầm (mistakes) thì tích lũy lại."*

**[⭐ Star this repo](https://github.com/UIT-25730047/5ducks-tetris) | [📝 Report Issues](https://github.com/UIT-25730047/5ducks-tetris/issues) | [🤝 Contribute](https://github.com/UIT-25730047/5ducks-tetris/pulls)**

</div>
