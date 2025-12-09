# 🎮 Game Tetris - Phiên Bản Terminal

[![C++](https://img.shields.io/badge/C++-11-blue.svg)](https://en.cppreference.com/w/cpp/11)
[![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey.svg)](https://github.com/lqnhat/5ducks-tetris)

Game Tetris cổ điển được lập trình bằng C++ chạy trực tiếp trên terminal! Được phát triển bởi **Nhóm 5 Ducks** trong khuôn khổ đồ án môn Kỹ Năng Nghề Nghiệp tại UIT (Trường Đại học Công nghệ Thông tin).

## 📋 Mục Lục

- [Giới Thiệu](#giới-thiệu)
- [Tính Năng](#tính-năng)
- [Cấu Trúc Dự Án](#cấu-trúc-dự-án)
- [Yêu Cầu Hệ Thống](#yêu-cầu-hệ-thống)
- [Hướng Dẫn Cài Đặt](#hướng-dẫn-cài-đặt)
- [Cách Chơi](#cách-chơi)
- [Phím Điều Khiển](#phím-điều-khiển)
- [Hệ Thống Tính Điểm](#hệ-thống-tính-điểm)
- [Phát Triển](#phát-triển)
- [Thành Viên Nhóm](#thành-viên-nhóm)
- [Đóng Góp](#đóng-góp)
- [Giấy Phép](#giấy-phép)

## 🎯 Giới Thiệu

Dự án này tái hiện trải nghiệm chơi game Tetris cổ điển trực tiếp trên terminal sử dụng C++ thuần túy với thư viện hệ thống POSIX. Điểm đặc biệt của dự án là chúng tôi phát triển **hai phiên bản song song** để minh họa các cách tiếp cận lập trình khác nhau:

### 🏗️ Hai Cách Triển Khai

1. **Phiên Bản Struct** (`tetris_struct/`)
   - Sử dụng phương pháp Lập trình Thủ tục (Procedural Programming)
   - Tổ chức với struct và các hàm độc lập
   - Phù hợp để học phong cách lập trình C++ truyền thống

2. **Phiên Bản Class** (`tetris_class/`)
   - Sử dụng Lập trình Hướng Đối Tượng (OOP)
   - Triển khai class, encapsulation và inheritance
   - Lý tưởng để nghiên cứu các mẫu thiết kế phần mềm và nguyên lý OOP

Cả hai phiên bản đều có **gameplay giống hệt nhau** - điểm khác biệt duy nhất là cách tổ chức code và kiến trúc!

## ✨ Tính Năng

- 🎨 **Gameplay Tetris Cổ Điển**: Đầy đủ 7 mảnh Tetromino truyền thống (I, O, T, S, Z, J, L)
- ⌨️ **Điều Khiển Trực Quan**: Phím điều khiển với WASD hoặc phím mũi tên
- 📊 **Hệ Thống Tính Điểm**: Điểm dựa trên số hàng xóa, có hệ số nhân theo cấp độ
- 🎵 **Hiệu Ứng Âm Thanh**: Nhạc nền Tetris cổ điển và hiệu ứng âm thanh
- 📈 **Độ Khó Tăng Dần**: Hệ thống cấp độ động tăng tốc độ
- 📋 **Hiển Thị Thống Kê**: Theo dõi điểm số, cấp độ và số hàng đã xóa
- ⏸️ **Tính Năng Tạm Dừng**: Tạm dừng và tiếp tục bất cứ lúc nào
- 🏆 **Theo Dõi Điểm Cao**: Ghi nhớ thành tích tốt nhất của bạn

## 📁 Cấu Trúc Dự Án

```
5ducks-tetris/
├── tetris_struct/          # Phiên bản Lập trình Thủ tục
│   ├── main.cpp           # Entry point cho phiên bản struct
│   └── ...
├── tetris_class/          # Phiên bản Lập trình Hướng Đối Tượng
│   ├── main.cpp           # Entry point cho phiên bản class
│   └── ...
├── README.md             # File này
```

## 💻 Yêu Cầu Hệ Thống

- **Hệ điều hành**: macOS 10.14+ hoặc Linux (Ubuntu 20.04+, Fedora 30+, Debian 10+)
- **CPU**: Intel Core i3 hoặc tương đương
- **RAM**: 2GB trở lên
- **Dung lượng**: 50MB dung lượng trống
- **Terminal**: Phải hỗ trợ ANSI escape codes
- **Compiler**: GCC 7.0+ hoặc Clang 5.0+ với hỗ trợ C++11

> **Lưu ý**: Hiện tại chỉ hỗ trợ hệ thống Unix (macOS và Linux). Hỗ trợ Windows đang được lên kế hoạch cho phiên bản tương lai. Người dùng Windows có thể sử dụng WSL (Windows Subsystem for Linux) để chạy game.

## 🚀 Hướng Dẫn Cài Đặt

### Bắt Đầu Nhanh

1. **Clone repository**
   ```bash
   git clone https://github.com/lqnhat/5ducks-tetris.git
   cd 5ducks-tetris
   ```

2. **Chọn phiên bản của bạn** (Struct hoặc Class)

   **Lựa chọn A: Phiên Bản Thủ Tục (Struct)**
   ```bash
   cd tetris_struct
   g++ -std=c++11 main.cpp -o tetris
   ./tetris
   ```

   **Lựa chọn B: Phiên Bản Hướng Đối Tượng (Class)**
   ```bash
   cd tetris_class
   g++ -std=c++11 main.cpp -o tetris
   ./tetris
   ```

   *Cách khác*: Nếu có Makefile, bạn có thể dùng:
   ```bash
   make
   ./tetris
   ```

3. **Đảm bảo terminal đủ lớn** (tối thiểu 80×24 ký tự)

4. **Bắt đầu chơi!**

### Cài Đặt Compiler (nếu cần)

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential
```

**macOS:**
```bash
xcode-select --install
```

## 🎮 Cách Chơi

### Mục Tiêu
Sắp xếp các mảnh Tetromino rơi xuống để tạo thành các hàng ngang hoàn chỉnh. Khi một hàng được hoàn thành, nó sẽ biến mất và bạn nhận được điểm. Game kết thúc khi các mảnh chồng lên đến đỉnh màn hình.

### Bảy Mảnh Tetromino

| Mảnh | Hình Dạng | Màu Sắc | Chiến Thuật |
|------|-----------|---------|-------------|
| **Khối I** | ████ | Xanh dương | Để dành xóa 4 hàng cùng lúc (Tetris!) |
| **Khối O** | ██<br>██ | Vàng | Mảnh duy nhất không xoay được, lấp khoảng trống lớn |
| **Khối T** | ▀█▀ | Tím | Rất linh hoạt, có thể thực hiện T-Spin |
| **Khối S** | ▄█▀ | Xanh lá | Tạo các đường zigzag |
| **Khối Z** | ▀█▄ | Đỏ | Đối xứng với khối S |
| **Khối J** | ▄██ | Xanh đậm | Tốt để lấp các góc |
| **Khối L** | ██▄ | Cam | Đối xứng với khối J |

## ⌨️ Phím Điều Khiển

| Phím | Chức Năng |
|------|-----------|
| `A` hoặc `←` | Di chuyển mảnh sang trái |
| `D` hoặc `→` | Di chuyển mảnh sang phải |
| `S` hoặc `↓` | Rơi nhanh (soft drop) |
| `W` hoặc `↑` | Xoay mảnh theo chiều kim đồng hồ |
| `Space` | Rơi ngay lập tức (hard drop) |
| `P` | Tạm dừng/Tiếp tục game |
| `Q` hoặc `ESC` | Thoát game |

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
- Ví dụ: Xóa 4 hàng ở Cấp độ 5 = 800 × 5 = **4,000 điểm**

### Điểm Thưởng Combo
Xóa các hàng liên tiếp trong các lượt kế tiếp nhau để nhận điểm thưởng combo. Combo càng dài, điểm thưởng càng cao!

## 🎓 Phát Triển

### So Sánh Các Mô Hình Lập Trình

Dự án này minh họa hai cách tiếp cận lập trình cơ bản:

**Thủ Tục (Phiên Bản Struct):**
- Dữ liệu và hàm tách biệt
- Các hàm thao tác trên cấu trúc dữ liệu
- Luồng đơn giản, trực tiếp hơn
- Tuyệt vời cho người mới học C++

**Hướng Đối Tượng (Phiên Bản Class):**
- Dữ liệu và phương thức được đóng gói trong class
- Sử dụng inheritance và polymorphism
- Module hóa và dễ bảo trì hơn
- Cách tiếp cận chuẩn công nghiệp cho dự án lớn

### Công Nghệ Sử Dụng

- **Ngôn ngữ**: C++ (chuẩn C++11)
- **Thư viện**: POSIX (`termios`, `fcntl`) để điều khiển terminal
- **Đồ họa**: ANSI escape codes để render trên terminal
- **Hệ thống Build**: g++ compiler, Makefile tùy chọn

## 👥 Thành Viên Nhóm

**Nhóm 5 Ducks** - CN1.K2025.1.CNTT

| Họ và Tên | MSSV | Vai Trò |
|-----------|------|---------|
| Lê Quang Nhật | 25730047 | Trưởng nhóm, Hệ thống âm thanh, Kiểm thử, Tài liệu |
| Dương Hoà Long | 25730040 | Xử lý đầu vào, Thao tác Tetromino, Kiểm thử |
| Lê Hữu Nhị | 25730048 | Xử lý đầu vào, Thao tác Tetromino, Kiểm thử |
| Nguyễn Duy Thanh | 25730068 | Hệ thống điểm số, Hệ thống âm thanh, Kiểm thử |
| Kiều Quang Việt | 25730093 | Thao tác Tetromino, Hệ thống điểm số, Kiểm thử |

**Môn học**: Kỹ Năng Nghề Nghiệp
**Giảng viên**: ThS. Nguyễn Văn Toàn
**Trường**: Trường Đại học Công nghệ Thông tin (UIT)

## 🔧 Công Cụ Phát Triển

- **Quản lý công việc**: [Trello Board](https://trello.com/invite/b/693024cd112ba6767e45fd9a/ATTI060c7059b51bb4a69e34c070c2254ff40261BBCD/5ducks)
- **Quản lý mã nguồn**: [GitHub Repository](https://github.com/lqnhat/5ducks-tetris)
- **Giao tiếp**: [Slack Workspace](https://app.slack.com/client/T09M5KGA799/C0A0AR9KJ4X)
- **Soạn thảo tài liệu**: [Overleaf](https://www.overleaf.com/read/jnjfgkqtvpsh#9f751d)

## 🤝 Đóng Góp

Chúng tôi hoan nghênh các đóng góp! Đây là cách bạn có thể giúp đỡ:

1. Fork repository
2. Tạo feature branch của bạn (`git checkout -b feature/TinhNangTuyetVoi`)
3. Commit các thay đổi (`git commit -m 'Thêm tính năng tuyệt vời'`)
4. Push lên branch (`git push origin feature/TinhNangTuyetVoi`)
5. Mở Pull Request

### Hướng Dẫn Phát Triển

- Tuân theo style code hiện có
- Viết commit message rõ ràng
- Thêm comment cho logic phức tạp
- Test kỹ lưỡng trước khi submit
- Cập nhật tài liệu khi cần thiết

## 📝 Câu Hỏi Thường Gặp

**H: Sự khác biệt giữa phiên bản Struct và Class là gì?**
Đ: Cả hai đều có gameplay giống hệt nhau. Điểm khác biệt nằm ở cách tổ chức code - Struct dùng lập trình thủ tục còn Class dùng OOP. Chọn dựa trên mục tiêu học tập của bạn!

**H: Tôi nên dùng phiên bản nào?**
Đ: Nếu bạn mới học C++, bắt đầu với phiên bản Struct. Nếu muốn học OOP và thiết kế phần mềm, thử phiên bản Class. Bạn cũng có thể chơi cả hai để so sánh!

**H: Game có chạy trên Windows không?**
Đ: Chưa hỗ trợ trực tiếp, nhưng bạn có thể dùng WSL (Windows Subsystem for Linux) để chạy trên Windows.

**H: Tại sao terminal không hiển thị màu đúng?**
Đ: Đảm bảo terminal hỗ trợ ANSI escape codes. Hầu hết terminal hiện đại (Terminal.app, GNOME Terminal, iTerm2) đều hỗ trợ.

**H: Game bị giật hoặc phím không phản hồi?**
Đ: Thử đóng các ứng dụng terminal khác, tăng buffer size của terminal, hoặc khởi động lại terminal.

## 📞 Hỗ Trợ & Liên Hệ

- **GitHub Issues**: [Báo lỗi hoặc yêu cầu tính năng](https://github.com/lqnhat/5ducks-tetris/issues)
- **Slack Community**: [Tham gia workspace](https://app.slack.com/client/T09M5KGA799/C0A0AR9KJ4X)


## 🙏 Ghi Nhận

- **Alexey Pajitnov** - Người sáng tạo Tetris gốc (1985)
- **ThS. Nguyễn Văn Toàn** - Giảng viên hướng dẫn
- **UIT (Trường Đại học Công nghệ Thông tin)** - Hỗ trợ học thuật
- Tất cả những người đóng góp và tester

---

<div align="center">

**🎮 Được làm với ❤️ bởi Nhóm 5 Ducks**

*"Trong Tetris như trong cuộc sống, những thành tựu (achievements) biến mất, còn những sai lầm (mistakes) thì tích lũy lại."*

</div>
