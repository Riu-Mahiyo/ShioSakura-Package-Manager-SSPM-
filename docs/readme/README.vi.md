<div align="center">

# 🌸 SSPM — Trình Quản Lý Gói ShioSakura

**Bộ Điều Hành Gói Phổ Biến · Bộ Điều Hành Quản Lý Gói Cross-distribution**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](../../LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#các-nền-tảng-hỗ-trợ)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![GitHub](https://img.shields.io/badge/GitHub-Riu--Mahiyo-181717?logo=github)](https://github.com/Riu-Mahiyo)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/sspm)

> **SSPM** là một bộ điều hành trình quản lý gói phổ biến nằm trên các trình quản lý gói gốc của hệ thống.
> Một lệnh. Bất kỳ phân phối nào. Bất kỳ backend nào.

</div>

---

## 📖 Mục Lục

- [SSPM là gì?](#sspm-là-gì)
- [Kiến Trúc](#kiến-trúc)
- [Các Nền Tảng Hỗ Trợ](#các-nền-tảng-hỗ-trợ)
- [Cài Đặt](#cài-đặt)
- [Khởi Động Nhanh](#khởi-động-nhanh)
- [Tham Khảo CLI](#tham-khảo-cli)
- [Hệ Thống Backend](#hệ-thống-backend)
- [Định Dạng Gói SPK](#định-dạng-gói-spk)
- [Hệ Thống Kho](#hệ-thống-kho)
- [Hệ Thống Hồ Sơ](#hệ-thống-hồ-sơ)
- [Hệ Thống Gương](#hệ-thống-gương)
- [Bảo Mật](#bảo-mật)
- [Bác Sĩ & Chẩn Đoán](#bác-sĩ--chẩn-đoán)
- [Chế Độ API & GUI](#chế-độ-api--gui)
- [Hệ Thống Plugin](#hệ-thống-plugin)
- [Cấu Hình](#cấu-hình)
- [Cấu Trúc Dự Án](#cấu-trúc-dự-án)
- [Đóng Góp](#đóng-góp)
- [Giấy Phép](#giấy-phép)

---

## 🌸 SSPM là gì?

SSPM (**Trình Quản Lý Gói ShioSakura**) là một **bộ điều hành trình quản lý gói phổ biến** — nó không thay thế trình quản lý gói của hệ thống, mà *điều phối* chúng.

```
Bạn  →  sspm install nginx
              ↓
     Lớp Trừu Tượng Backend
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPM phát hiện môi trường của bạn, chọn backend tốt nhất có sẵn, giải quyết các phụ thuộc, xử lý các giao dịch, và mang lại trải nghiệm thống nhất cho bạn trên **Linux, macOS, BSD và Windows**.

---

## 🏗️ Kiến Trúc

```
sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  phong cách apt / pacman / brew — tùy chọn của bạn
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Giải Quyết │  Giải quyết các phụ thuộc chuyển tiếp
 │  Phụ Thuộc  │  Giải quyết ràng buộc phiên bản (>=, <=, !=, …)
 │             │  Phát hiện xung đột / phá vỡ
 │             │  Sắp xếp topo → thứ tự cài đặt
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Backend    │  Chọn backend tốt nhất cho mỗi gói
 │  Giải Quyết │  Chiến lược: ưu tiên người dùng → thăm dò lười → sẵn có
 │  + Registry │  20 backend được phát hiện tự động, không bao giờ được tải nếu không có
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ Giao Dịch  │  bắt đầu → xác minh → cài đặt → cam kết
 │             │  Khi thất bại: quay lại qua log undo
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   Lớp Trừu Tượng Backend                    │
 │                                              │
 │  Linux:    apt · pacman · dnf · zypper       │
 │            portage · apk · xbps · nix · run  │
 │  BSD:      pkg · pkg_add · pkgin             │
 │  macOS:    brew · macports                   │
 │  Windows:  winget · scoop · chocolatey       │
 │  Universal: flatpak · snap · appimage · spk  │
 └──────┬───────────────────────────────────────┘
        │
        ▼
  Trình quản lý gói hệ thống  (hoặc SPK: được xử lý gốc trong SSPM)
```

**Hệ thống phụ hỗ trợ:**

| Hệ thống phụ | Vai trò |
|-----------|------|
| Gương | Phát hiện quốc gia Geo-IP, xếp hạng độ trễ, chọn tự động nhận biết VPN |
| Mạng | Tải xuống song song libcurl, tiếp tục, lùi về gương |
| Bảo mật | Chữ ký ed25519 + xác minh sha256 mỗi tệp |
| Bộ nhớ cache | `~/.cache/sspm` — tải xuống + siêu dữ liệu + chỉ mục kho |
| SkyDB | SQLite: gói · lịch sử · giao dịch · kho · hồ sơ |
| Kho | Đồng bộ kho SPK, lấy chỉ mục, kiểm tra chữ ký |
| Chỉ mục | Tìm kiếm mờ, tìm kiếm regex, đồ thị phụ thuộc |
| Hồ sơ | Nhóm gói được đặt tên: dev / desktop / server / gaming |
| Logger | `~/.local/state/sspm/log/` — cấp độ + chế độ tail |
| Bác sĩ | Backend · quyền · mạng · kho · cache · sức khỏe db |
| Plugin | Tải động mở rộng backend từ `~/.local/share/sspm/plugins/` |
| REST API | `sspm-daemon` trên `:7373` — cung cấp sức mạnh cho SSPM Center và giao diện GUI |

| Thành phần | Mô tả |
|-----------|-------------|
| `CLI` | Phân tích đối số, định tuyến lệnh, định dạng đầu ra |
| `Giải Quyết Gói` | Chọn backend tốt nhất cho mỗi gói |
| `Lớp Backend` | Bộ chuyển đổi cho mỗi trình quản lý gói gốc |
| `Hệ Thống Giao Dịch` | Cài đặt/xóa nguyên tử với hỗ trợ quay lại |
| `SkyDB` | Cơ sở dữ liệu trạng thái cục bộ dựa trên SQLite |
| `Hệ Thống Kho` | Kho `.spk` chính thức, bên thứ ba và cục bộ |
| `Định Dạng SPK` | Định dạng gói di động của SSPM |
| `Hệ Thống Cache` | Cache tải xuống, siêu dữ liệu, chỉ mục kho |
| `Hệ Thống Hồ Sơ` | Nhóm gói dựa trên môi trường |
| `Hệ Thống Gương` | Phát hiện địa lý tự động + gương được xếp hạng độ trễ |
| `Bảo Mật` | Chữ ký ed25519 + xác minh sha256 |
| `Bác Sĩ` | Chẩn đoán hệ thống và kiểm tra sức khỏe |
| `REST API` | Chế độ daemon cho giao diện GUI |
| `Hệ Thống Plugin` | Backend và hook có thể mở rộng |

---

## 💻 Các Nền Tảng Hỗ Trợ

### Linux

| Họ | Trình Quản Lý Gói |
|--------|-----------------|
| Debian / Ubuntu | `apt`, `apt-get`, `dpkg` |
| Red Hat / Fedora | `dnf`, `yum`, `rpm` |
| SUSE | `zypper` |
| Arch | `pacman` |
| Gentoo | `portage` (emerge) |
| Alpine | `apk` |
| Void | `xbps` |
| NixOS | `nix-env` |

### BSD

| Hệ Thống | Trình Quản Lý Gói |
|--------|----------------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| Công Cụ | Ghi Chú |
|------|-------|
| `homebrew` | Cũng có sẵn như Linuxbrew trên Linux |
| `macports` | Backend thay thế |

### Windows

| Công Cụ | Ghi Chú |
|------|-------|
| `winget` | Trình quản lý gói Windows tích hợp |
| `scoop` | Trình cài đặt không gian người dùng |
| `chocolatey` | Trình quản lý gói cộng đồng |

### Universal (Cross-platform)

| Backend | Ghi Chú |
|---------|-------|
| `flatpak` | Ứng dụng Linux được sanbox |
| `snap` | Định dạng phổ biến của Canonical |
| `AppImage` | Tệp nhị phân Linux di động |
| `nix profile` | Gói cross-platform có thể tái tạo |
| `spk` | Định dạng gói gốc của SSPM |

---

## 📦 Cài Đặt

### Linux / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

Hoặc xây dựng từ nguồn:

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build && cmake --build build
sudo cmake --install build
```

### Windows

```powershell
winget install SSPM
# hoặc
scoop install sspm
```

---

## 🚀 Khởi Động Nhanh

```bash
# Cài đặt gói
sspm install nginx

# Tìm kiếm gói
sspm search nodejs

# Cập nhật cơ sở dữ liệu gói
sspm update

# Nâng cấp tất cả gói
sspm upgrade

# Xóa gói
sspm remove nginx

# Liệt kê gói đã cài đặt
sspm list

# Lấy thông tin gói
sspm info nginx

# Chạy chẩn đoán hệ thống
sspm doctor
```

---

## 📟 Tham Khảo CLI

### Lệnh Cơ Bản

| Lệnh | Mô Tả |
|---------|-------------|
| `sspm install <pkg>` | Cài đặt gói |
| `sspm remove <pkg>` | Xóa gói |
| `sspm search <query>` | Tìm kiếm gói |
| `sspm update` | Đồng bộ cơ sở dữ liệu gói |
| `sspm upgrade` | Nâng cấp tất cả gói đã cài đặt |
| `sspm list` | Liệt kê gói đã cài đặt |
| `sspm info <pkg>` | Hiển thị chi tiết gói |
| `sspm doctor` | Chạy chẩn đoán hệ thống |

### Lệnh Nâng Cao

| Lệnh | Mô Tả |
|---------|-------------|
| `sspm repo <sub>` | Quản lý kho |
| `sspm cache <sub>` | Quản lý cache tải xuống |
| `sspm config <sub>` | Chỉnh sửa cấu hình |
| `sspm profile <sub>` | Quản lý hồ sơ môi trường |
| `sspm history` | Xem lịch sử cài đặt/xóa |
| `sspm rollback` | Quay lại giao dịch cuối cùng |
| `sspm verify <pkg>` | Xác minh tính toàn vẹn gói |
| `sspm mirror <sub>` | Quản lý nguồn gương |
| `sspm log` | Xem nhật ký SSPM |
| `sspm log tail` | Theo dõi đầu ra nhật ký trực tiếp |

### Cờ Đầu Ra

```bash
sspm search nginx --json          # Xuất JSON
sspm list --format table          # Xuất bảng
sspm install nginx --verbose      # Chế độ chi tiết
sspm install nginx --dry-run      # Xem trước mà không thực thi
sspm install nginx --backend apt  # Buộc sử dụng backend cụ thể
```

### Phong Cách Lệnh Tùy Chỉnh

SSPM hỗ trợ nhiều quy ước lệnh. Chuyển sang phong cách pacman nếu bạn thích:

```bash
sspm config set cli.style pacman
# sspm -S nginx      → cài đặt
# sspm -Rs nginx     → xóa
# sspm -Ss nginx     → tìm kiếm
# sspm -Syu          → nâng cấp tất cả

sspm config set cli.style apt     # mặc định
```

---

## 🔧 Hệ Thống Backend

### Phát Hiện Tự Động Lười

SSPM nunca tải backend không được cài đặt. Registry Backend thăm dò từng backend trong số 20 backend bằng cách kiểm tra sự hiện diện của tệp nhị phân của chúng — và **lưu vào cache kết quả**.

```
Trên Arch Linux:
  /usr/bin/pacman   ✅  đã tải   (ưu tiên 10)
  /usr/bin/apt-get  ⬜  bỏ qua
  /usr/bin/dnf      ⬜  bỏ qua
  /usr/bin/flatpak  ✅  đã tải   (ưu tiên 30)
  spk (tích hợp)    ✅  đã tải   (ưu tiên 50)
```

Sau khi cài đặt công cụ mới (ví dụ: `flatpak`), chạy `sspm doctor` để thăm dò lại.

### Ưu Tiên Backend

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

Buộc sử dụng backend cụ thể cho một lệnh:

```bash
sspm install firefox --backend flatpak
```

### Nhãn Backend (Tích Hợp SSPM Center / Cửa Hàng)

| Nhãn | Backend |
|-------|---------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | SPK gốc |

---

## 🔍 Giải Quyết Phụ Thuộc

Resolver của SSPM xử lý ba vấn đề trước khi cài đặt bất cứ thứ gì:

**1. Giải quyết phụ thuộc** — tất cả các phụ thuộc chuyển tiếp được mở rộng đệ quy  
**2. Giải quyết phiên bản** — nhiều ràng buộc trên cùng một gói được AND  
**3. Giải quyết xung đột** — kiểm tra `conflicts`, `breaks`, và phụ thuộc ngược

```bash
sspm install certbot --dry-run

Giải quyết phụ thuộc...
Kế hoạch cài đặt (6 gói):
  cài đặt  libc      2.38    phụ thuộc của python3
  cài đặt  libffi    3.4.4   phụ thuộc của python3
  cài đặt  openssl   3.1.4   phụ thuộc của certbot (>= 3.0)
  cài đặt  python3   3.12.0  phụ thuộc của certbot (>= 3.9 sau khi acme thêm ràng buộc)
  cài đặt  acme      2.7.0   phụ thuộc của certbot
  cài đặt  certbot   2.7.4   yêu cầu
```

Xem [docs/RESOLVER.md](../../docs/RESOLVER.md) để biết thuật toán đầy đủ.

---





```
sspm install nginx
      ↓
Resolver gói kiểm tra:
  1. backend_priority được cấu hình người dùng
  2. Backend nào có gói này sẵn có
  3. Chọn backend sẵn có có độ ưu tiên cao nhất
      ↓
Kết quả ví dụ: pacman (Arch) → thực thi: pacman -S nginx
```

### Cấu Hình Ưu Tiên Backend

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### Buộc Backend

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### Giao Diện Backend

Mỗi bộ chuyển đổi backend triển khai cùng một giao diện trừu tượng:

```cpp
class Backend {
public:
    virtual Result install(const Package& pkg) = 0;
    virtual Result remove(const Package& pkg) = 0;
    virtual SearchResult search(const std::string& query) = 0;
    virtual Result update() = 0;
    virtual Result upgrade() = 0;
    virtual PackageList list_installed() = 0;
};
```

---

## 📦 Định Dạng Gói SPK

`.spk` là định dạng gói di động gốc của SSPM.

### Cấu Trúc Gói

```
package.spk
├── metadata.toml       # Tên gói, phiên bản, phụ thuộc, script
├── payload.tar.zst     # Dữ liệu nén (zstd)
├── install.sh          # Hook cài đặt trước/sau
├── remove.sh           # Hook xóa trước/sau
└── signature           # Chữ ký ed25519
```

### Ví Dụ metadata.toml

```toml
[package]
name = "example"
version = "1.0.0"
description = "Một gói SPK ví dụ"
author = "Riu-Mahiyo"
license = "MIT"
arch = ["x86_64", "aarch64"]

[dependencies]
required = ["libc >= 2.17", "zlib"]
optional = ["libssl"]

[scripts]
pre_install  = "install.sh pre"
post_install = "install.sh post"
pre_remove   = "remove.sh pre"
post_remove  = "remove.sh post"
```

### Xây Dựng Gói SPK

```bash
sspm build ./mypackage/
# Xuất: mypackage-1.0.0.spk
```

---

## 🗄️ Hệ Thống Kho

### Lệnh Kho

```bash
sspm repo add https://repo.example.com/sspm     # Thêm kho
sspm repo remove example                        # Xóa kho
sspm repo sync                                  # Đồng bộ tất cả kho
sspm repo list                                  # Liệt kê kho đã cấu hình
```

### Định Dạng Kho

```
repo/
├── repo.json       # Siêu dữ liệu kho & chỉ mục gói
├── packages/       # Tệp gói .spk
└── signature       # Khóa ký kho (ed25519)
```

Kho hỗ trợ: **chính thức**, **bên thứ ba**, và **nguồn cục bộ**.

---

## 🧑‍💼 Hệ Thống Hồ Sơ

Hồ sơ nhóm gói theo môi trường, giúp dễ dàng tái tạo cấu hình đầy đủ.

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### Mẫu Hồ Sơ Tích Hợp

| Hồ Sơ | Gói Thường Dùng |
|---------|-----------------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | trình duyệt, trình phát đa phương tiện, bộ office |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 Hệ Thống Gương

### Phát Hiện Địa Lý Tự Động

SSPM phát hiện quốc gia của bạn thông qua định vị địa lý IP và tự động chuyển đổi **tất cả backend** sang gương khu vực nhanh nhất — ngay cả qua VPN.

Đối với người dùng **proxy dựa trên quy tắc** (không phải VPN toàn bộ đường hầm), SSPM phát hiện IP ra ngoài thực tế và xử lý chọn gương chính xác.

```bash
sspm mirror list              # Liệt kê gương sẵn có
sspm mirror test              # Đo đạc gương theo độ trễ
sspm mirror select            # Chọn gương thủ công
```

### Cấu Hình Gương

```yaml
# ~/.config/sspm/config.yaml
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP
```

---

## 🔐 Bảo Mật

| Tính Năng | Thực Hiện |
|---------|---------------|
| Chữ ký gói | `ed25519` |
| Xác minh hash | `sha256` |
| Cọc tin cậy kho | Kéo pin khóa công khai mỗi kho |
| Bắt buộc cho `.spk` | Kiểm tra chữ ký trước khi cài đặt |

```bash
sspm verify nginx              # Xác minh gói đã cài đặt
sspm verify ./package.spk      # Xác minh tệp SPK cục bộ
```

---

## 🏥 Bác Sĩ & Chẩn Đoán

```bash
sspm doctor
```

Các kiểm tra được thực hiện:

- ✅ Sẵn có và phiên bản backend
- ✅ Quyền tệp
- ✅ Kết nối mạng  
- ✅ Tìm kiếm được kho
- ✅ Tính toàn vẹn cache
- ✅ Tính toàn vẹn cơ sở dữ liệu SkyDB
- ✅ Độ trễ gương

---

## 🔌 Chế Độ API & GUI

### Chế Độ Daemon

```bash
sspm daemon start      # Khởi động daemon API REST
sspm daemon stop
sspm daemon status
```

### Điểm Kết API REST

| Phương Pháp | Điểm Kết | Mô Tả |
|--------|----------|-------------|
| `GET` | `/packages` | Liệt kê gói đã cài đặt |
| `GET` | `/packages/search?q=` | Tìm kiếm gói |
| `POST` | `/packages/install` | Cài đặt gói |
| `DELETE` | `/packages/:name` | Xóa gói |
| `GET` | `/repos` | Liệt kê kho |
| `POST` | `/repos` | Thêm kho |
| `GET` | `/health` | Kiểm tra sức khỏe daemon |

### SSPM Center (Giao Diện GUI)

SSPM Center là giao diện đồ họa chính thức, tích hợp với:

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

Gói được đánh dấu theo nguồn backend:

| Nhãn | Ý Nghĩa |
|-------|---------|
| `SSPM-APT` | Được cài đặt thông qua apt |
| `SSPM-PACMAN` | Được cài đặt thông qua pacman |
| `SSPM-FLATPAK` | Được cài đặt thông qua Flatpak |
| `SSPM-SNAP` | Được cài đặt thông qua Snap |
| `SSPM-NIX` | Được cài đặt thông qua Nix |
| `SSPM-SPK` | Gói gốc SSPM |

Thẻ danh mục: `🛠 Công cụ` · `🎮 Trò chơi` · `🎵 Phương tiện` · `⚙️ Hệ thống` · `📦 Phát triển` · `🌐 Mạng`

---

## 🧩 Hệ Thống Plugin

Mở rộng SSPM với backend và hook tùy chỉnh:

```
~/.local/share/sspm/plugins/
├── aur/            # Backend AUR (Kho Người Dùng Arch)
├── brew-tap/       # Hỗ trợ Homebrew tap
└── docker/         # Backend镜像 Docker
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ Cấu Hình

**Tệp cấu hình:** `~/.config/sspm/config.yaml`

```yaml
# Thứ tự ưu tiên backend
backend_priority:
  - pacman
  - flatpak
  - nix

# Phong cách CLI: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# Cài đặt gương & địa lý
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# Cài đặt cache
cache:
  path: ~/.cache/sspm
  max_size: 2GB
  prune_after: 30d

# Ghi log
log:
  level: info             # debug | info | warn | error
  path: ~/.local/state/sspm/log
```

---

## 📁 Cấu Trúc Dự Án

```
ShioSakura-Package-Manager-SSPM-/n├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # Phân tích đối số CLI và định tuyến lệnh
│   ├── resolver/         # Resolver gói & bộ chọn backend
│   ├── backends/         # Bộ chuyển đổi backend
│   │   ├── apt/
│   │   ├── pacman/
│   │   ├── dnf/
│   │   ├── zypper/
│   │   ├── portage/
│   │   ├── apk/
│   │   ├── xbps/
│   │   ├── nix/
│   │   ├── brew/
│   │   ├── macports/
│   │   ├── winget/
│   │   ├── scoop/
│   │   ├── chocolatey/
│   │   ├── flatpak/
│   │   ├── snap/
│   │   ├── appimage/
│   │   ├── pkg/          # FreeBSD
│   │   ├── pkg_add/      # OpenBSD
│   │   ├── pkgin/        # NetBSD
│   │   └── spk/          # SSPM gốc
│   ├── transaction/      # Hệ thống giao dịch nguyên tử + rollback
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # Quản lý kho
│   ├── spk/              # Nhà xây dựng và trình phân tích gói SPK
│   ├── cache/            # Hệ thống cache
│   ├── profile/          # Hệ thống hồ sơ
│   ├── mirror/           # Xếp hạng gương & phát hiện địa lý
│   ├── security/         # Xác minh ed25519 + sha256
│   ├── doctor/           # Chẩn đoán hệ thống
│   ├── api/              # Daemon API REST
│   ├── log/              # Hệ thống ghi log
│   ├── network/          # Client HTTP (libcurl)
│   └── plugin/           # Loader plugin
│
├── include/              # Tiêu đề công khai
├── tests/                # Kiểm tra đơn vị & tích hợp
├── docs/                 # Tài liệu đầy đủ
├── packages/             # Công thức gói SPK chính thức
└── assets/               # Logo, biểu tượng
```

---

## 🤝 Đóng Góp

Cám ơn sự đóng góp! Vui lòng đọc [CONTRIBUTING.md](../../CONTRIBUTING.md) trước khi gửi PR.

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 Giấy Phép

Giấy Phép GPLv2 — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

Được tạo với 🌸 bởi [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>