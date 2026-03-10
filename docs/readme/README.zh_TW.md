<div align="center">

# 🌸 SSPM — ShioSakura 套件管理器

**通用套件管理調度器 · 跨發行版套件管理調度器**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](../../LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#支援的平台)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![GitHub](https://img.shields.io/badge/GitHub-Riu--Mahiyo-181717?logo=github)](https://github.com/Riu-Mahiyo)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/sspm)

> **SSPM** 是一個通用套件管理調度器，它位於系統原生套件管理器之上。
> 一個命令。任何發行版。任何後端。

</div>

---

## 📖 目錄

- [什麼是 SSPM？](#什麼是-sspm)
- [架構](#架構)
- [支援的平台](#支援的平台)
- [安裝](#安裝)
- [快速開始](#快速開始)
- [CLI 參考](#cli-參考)
- [後端系統](#後端系統)
- [SPK 套件格式](#spk-套件格式)
- [倉庫系統](#倉庫系統)
- [設定檔系統](#設定檔系統)
- [鏡像系統](#鏡像系統)
- [安全性](#安全性)
- [診斷工具](#診斷工具)
- [API 模式和 GUI](#api-模式和-gui)
- [外掛系統](#外掛系統)
- [設定](#設定)
- [專案結構](#專案結構)
- [貢獻](#貢獻)
- [授權](#授權)

---

## 🌸 什麼是 SSPM？

SSPM（**ShioSakura 套件管理器**）是一個**通用套件管理調度器**——它不會替換系統的套件管理器，而是*協調*它們。

```
你  →  sspm install nginx
              ↓
     後端抽象層
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPM 檢測您的環境，選擇最佳可用後端，解決依賴關係，處理事務，並在**Linux、macOS、BSD 和 Windows** 上為您提供統一的體驗。

---

## 🏗️ 架構

```
sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  apt / pacman / brew 風格 — 您的選擇
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  依賴       │  解析傳遞依賴
 │  解析器     │  版本約束解決 (>=, <=, !=, …)
 │             │  衝突/中斷檢測
 │             │  拓撲排序 → 安裝順序
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  後端       │  為每個套件選擇最佳後端
 │  解析器     │  策略：使用者優先級 → 延遲探測 → 可用性
 │  + 登錄表   │  20 個後端自動檢測，不存在則不載入
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ 事務處理    │  開始 → 驗證 → 安裝 → 提交
 │             │  失敗時：透過撤銷日誌回滾
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   後端抽象層                                  │
 │                                              │
 │  Linux:    apt · pacman · dnf · zypper       │
 │            portage · apk · xbps · nix · run  │
 │  BSD:      pkg · pkg_add · pkgin             │
 │  macOS:    brew · macports                   │
 │  Windows:  winget · scoop · chocolatey       │
 │  通用:      flatpak · snap · appimage · spk  │
 └──────┬───────────────────────────────────────┘
        │
        ▼
  系統套件管理器  (或 SPK: 在 SSPM 中原生處理)
```

**支援的子系統：**

| 子系統 | 角色 |
|-------|------|
| 鏡像 | Geo-IP 國家檢測，延遲排名，VPN 感知自動選擇 |
| 網路 | libcurl 平行下載，恢復，鏡像回退 |
| 安全性 | ed25519 簽名 + 每個檔案的 sha256 驗證 |
| 快取 | `~/.cache/sspm` — 下載 + 中繼資料 + 倉庫索引 |
| SkyDB | SQLite: 套件 · 歷史 · 事務 · 倉庫 · 設定檔 |
| 倉庫 | SPK 倉庫同步，索引獲取，簽名檢查 |
| 索引 | 模糊搜尋，正則搜尋，依賴圖 |
| 設定檔 | 命名套件組：開發 / 桌面 / 伺服器 / 遊戲 |
| 日誌 | `~/.local/state/sspm/log/` — 分級 + 尾部模式 |
| 診斷 | 後端 · 許可權 · 網路 · 倉庫 · 快取 · 資料庫健康 |
| 外掛 | 從 `~/.local/share/sspm/plugins/` 動態載入後端擴充 |
| REST API | `sspm-daemon` 在 `:7373` 上 — 為 SSPM Center 和 GUI 前端提供支援 |

| 元件 | 描述 |
|-------|------|
| `CLI` | 參數解析，命令路由，輸出格式化 |
| `套件解析器` | 為每個套件選擇最佳後端 |
| `後端層` | 每個原生套件管理器的配接器 |
| `事務系統` | 原子安裝/刪除，支援回滾 |
| `SkyDB` | 基於 SQLite 的本地狀態資料庫 |
| `倉庫系統` | 官方、第三方和本地 `.spk` 倉庫 |
| `SPK 格式` | SSPM 自己的便攜式套件格式 |
| `快取系統` | 下載快取，中繼資料，倉庫索引 |
| `設定檔系統` | 基於環境的套件組 |
| `鏡像系統` | 自動地理檢測 + 延遲排名鏡像 |
| `安全性` | ed25519 簽名 + sha256 驗證 |
| `診斷` | 系統診斷和健康檢查 |
| `REST API` | 用於 GUI 前端的守護程序模式 |
| `外掛系統` | 可擴充的後端和鉤子 |

---

## 💻 支援的平台

### Linux

| 系列 | 套件管理器 |
|------|---------|
| Debian / Ubuntu | `apt`, `apt-get`, `dpkg` |
| Red Hat / Fedora | `dnf`, `yum`, `rpm` |
| SUSE | `zypper` |
| Arch | `pacman` |
| Gentoo | `portage` (emerge) |
| Alpine | `apk` |
| Void | `xbps` |
| NixOS | `nix-env` |

### BSD

| 系統 | 套件管理器 |
|------|---------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| 工具 | 說明 |
|------|------|
| `homebrew` | 在 Linux 上也可用作 Linuxbrew |
| `macports` | 替代後端 |

### Windows

| 工具 | 說明 |
|------|------|
| `winget` | 內建 Windows 套件管理器 |
| `scoop` | 使用者空間安裝程式 |
| `chocolatey` | 社群套件管理器 |

### 通用（跨平台）

| 後端 | 說明 |
|------|------|
| `flatpak` | 沙盒化 Linux 應用 |
| `snap` | Canonical 的通用格式 |
| `AppImage` | 便攜式 Linux 二進位檔案 |
| `nix profile` | 可重現的跨平台套件 |
| `spk` | SSPM 的原生套件格式 |

---

## 📦 安裝

### Linux / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

或從原始碼建構：

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build && cmake --build build
sudo cmake --install build
```

### Windows

```powershell
winget install SSPM
# 或
scoop install sspm
```

---

## 🚀 快速開始

```bash
# 安裝套件
sspm install nginx

# 搜尋套件
sspm search nodejs

# 更新套件資料庫
sspm update

# 升級所有套件
sspm upgrade

# 刪除套件
sspm remove nginx

# 列出已安裝的套件
sspm list

# 獲取套件資訊
sspm info nginx

# 執行系統診斷
sspm doctor
```

---

## 📟 CLI 參考

### 基本命令

| 命令 | 描述 |
|------|------|
| `sspm install <pkg>` | 安裝套件 |
| `sspm remove <pkg>` | 刪除套件 |
| `sspm search <query>` | 搜尋套件 |
| `sspm update` | 同步套件資料庫 |
| `sspm upgrade` | 升級所有已安裝的套件 |
| `sspm list` | 列出已安裝的套件 |
| `sspm info <pkg>` | 顯示套件詳情 |
| `sspm doctor` | 執行系統診斷 |

### 進階命令

| 命令 | 描述 |
|------|------|
| `sspm repo <sub>` | 管理倉庫 |
| `sspm cache <sub>` | 管理下載快取 |
| `sspm config <sub>` | 編輯設定 |
| `sspm profile <sub>` | 管理環境設定檔 |
| `sspm history` | 查看安裝/刪除歷史 |
| `sspm rollback` | 回滾最後一個事務 |
| `sspm verify <pkg>` | 驗證套件完整性 |
| `sspm mirror <sub>` | 管理鏡像源 |
| `sspm log` | 查看 SSPM 日誌 |
| `sspm log tail` | 跟蹤即時日誌輸出 |

### 輸出標誌

```bash
sspm search nginx --json          # JSON 輸出
sspm list --format table          # 表格輸出
sspm install nginx --verbose      # 詳細模式
sspm install nginx --dry-run      # 預覽而不執行
sspm install nginx --backend apt  # 強制使用特定後端
```

### 自訂命令風格

SSPM 支援多種命令約定。如果您喜歡，可以切換到 pacman 風格：

```bash
sspm config set cli.style pacman
# sspm -S nginx      → 安裝
# sspm -Rs nginx     → 刪除
# sspm -Ss nginx     → 搜尋
# sspm -Syu          → 升級所有

sspm config set cli.style apt     # 預設
```

---

## 🔧 後端系統

### 延遲自動檢測

SSPM 永遠不會載入未安裝的後端。後端登錄表透過檢查每個後端的二進位檔案是否存在來探測 20 個後端 — 並**快取結果**。沒有二進位檔案 = 不載入，無開銷。

```
在 Arch Linux 上：
  /usr/bin/pacman   ✅  已載入   (優先級 10)
  /usr/bin/apt-get  ⬜  已跳過
  /usr/bin/dnf      ⬜  已跳過
  /usr/bin/flatpak  ✅  已載入   (優先級 30)
  spk (內建)         ✅  已載入   (優先級 50)
```

安裝新工具（例如 `flatpak`）後，執行 `sspm doctor` 重新探測。

### 後端優先級

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

為一個命令強制使用特定後端：

```bash
sspm install firefox --backend flatpak
```

### 後端標籤（SSPM Center / 商店整合）

| 標籤 | 後端 |
|------|------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | 原生 SPK |

---

## 🔍 依賴解析器

SSPM 的解析器在安裝任何東西之前處理三個問題：

**1. 依賴解決** — 所有傳遞依賴都被遞迴展開  
**2. 版本解決** — 對同一個套件的多個約束被 AND 運算  
**3. 衝突解決** — `conflicts`、`breaks` 和反向依賴檢查

```bash
sspm install certbot --dry-run

解析依賴...
安裝計畫 (6 個套件)：
  安裝  libc      2.38    python3 的依賴
  安裝  libffi    3.4.4   python3 的依賴
  安裝  openssl   3.1.4   certbot 的依賴 (>= 3.0)
  安裝  python3   3.12.0  certbot 的依賴 (acme 新增約束後 >= 3.9)
  安裝  acme      2.7.0   certbot 的依賴
  安裝  certbot   2.7.4   請求
```

有關完整演算法，請參閱 [docs/RESOLVER.md](../../docs/RESOLVER.md)。

---





```
sspm install nginx
      ↓
套件解析器檢查：
  1. 使用者設定的 backend_priority
  2. 哪些後端有這個套件可用
  3. 選擇最高優先級的可用後端
      ↓
範例結果：pacman (Arch) → 執行：pacman -S nginx
```

### 設定後端優先級

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### 強制使用後端

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### 後端介面

每個後端配接器實作相同的抽象介面：

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

## 📦 SPK 套件格式

`.spk` 是 SSPM 的原生便攜式套件格式。

### 套件結構

```
package.spk
├── metadata.toml       # 套件名稱，版本，依賴，腳本
├── payload.tar.zst     # 壓縮檔案有效載荷 (zstd)
├── install.sh          # 安裝前/後鉤子
├── remove.sh           # 刪除前/後鉤子
└── signature           # ed25519 簽名
```

### metadata.toml 範例

```toml
[package]
name = "example"
version = "1.0.0"
description = "一個範例 SPK 套件"
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

### 建構 SPK 套件

```bash
sspm build ./mypackage/
# 輸出: mypackage-1.0.0.spk
```

---

## 🗄️ 倉庫系統

### 倉庫命令

```bash
sspm repo add https://repo.example.com/sspm     # 新增倉庫
sspm repo remove example                        # 刪除倉庫
sspm repo sync                                  # 同步所有倉庫
sspm repo list                                  # 列出設定的倉庫
```

### 倉庫格式

```
repo/
├── repo.json       # 倉庫中繼資料和套件索引
├── packages/       # .spk 套件檔案
└── signature       # 倉庫簽名金鑰 (ed25519)
```

倉庫支援：**官方**、**第三方**和**本地**源。

---

## 🧑‍💼 設定檔系統

設定檔按環境分組套件，使重現完整設定變得容易。

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### 內建設定檔範本

| 設定檔 | 典型套件 |
|---------|--------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | 瀏覽器，媒體播放器，辦公套件 |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 鏡像系統

### 自動地理檢測

SSPM 透過 IP 地理定位檢測您的國家，並自動將**所有後端**切換到最快的區域鏡像 — 即使透過 VPN。

對於使用**基於規則的代理**（不是全隧道 VPN）的使用者，SSPM 檢測實際的出站 IP 並正確處理鏡像選擇。

```bash
sspm mirror list              # 列出可用鏡像
sspm mirror test              # 透過延遲基準測試鏡像
sspm mirror select            # 手動選擇鏡像
```

### 鏡像設定

```yaml
# ~/.config/sspm/config.yaml
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP
```

---

## 🔐 安全性

| 功能 | 實現 |
|------|------|
| 套件簽名 | `ed25519` |
| 雜湊驗證 | `sha256` |
| 倉庫信任錨 | 每個倉庫的公鑰固定 |
| 對 `.spk` 強制要求 | 安裝前簽名檢查 |

```bash
sspm verify nginx              # 驗證已安裝的套件
sspm verify ./package.spk      # 驗證本地 SPK 檔案
```

---

## 🏥 診斷工具

```bash
sspm doctor
```

執行的檢查：

- ✅ 後端可用性和版本
- ✅ 檔案許可權
- ✅ 網路連線  
- ✅ 倉庫可達性
- ✅ 快取完整性
- ✅ SkyDB 資料庫完整性
- ✅ 鏡像延遲

---

## 🔌 API 模式和 GUI

### 守護程序模式

```bash
sspm daemon start      # 啟動 REST API 守護程序
sspm daemon stop
sspm daemon status
```

### REST API 端點

| 方法 | 端點 | 描述 |
|------|------|------|
| `GET` | `/packages` | 列出已安裝的套件 |
| `GET` | `/packages/search?q=` | 搜尋套件 |
| `POST` | `/packages/install` | 安裝套件 |
| `DELETE` | `/packages/:name` | 刪除套件 |
| `GET` | `/repos` | 列出倉庫 |
| `POST` | `/repos` | 新增倉庫 |
| `GET` | `/health` | 守護程序健康檢查 |

### SSPM Center（GUI 前端）

SSPM Center 是官方圖形前端，與以下整合：

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

套件按後端源標記：

| 標籤 | 含義 |
|------|------|
| `SSPM-APT` | 透過 apt 安裝 |
| `SSPM-PACMAN` | 透過 pacman 安裝 |
| `SSPM-FLATPAK` | 透過 Flatpak 安裝 |
| `SSPM-SNAP` | 透過 Snap 安裝 |
| `SSPM-NIX` | 透過 Nix 安裝 |
| `SSPM-SPK` | SSPM 原生套件 |

類別標籤：`🛠 工具` · `🎮 遊戲` · `🎵 媒體` · `⚙️ 系統` · `📦 開發` · `🌐 網路`

---

## 🧩 外掛系統

使用自訂後端和鉤子擴充 SSPM：

```
~/.local/share/sspm/plugins/
├── aur/            # AUR (Arch 使用者倉庫) 後端
├── brew-tap/       # Homebrew tap 支援
└── docker/         # Docker 鏡像後端
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ 設定

**設定檔：** `~/.config/sspm/config.yaml`

```yaml
# 後端優先級順序
backend_priority:
  - pacman
  - flatpak
  - nix

# CLI 風格: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# 鏡像和地理設定
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# 快取設定
cache:
  path: ~/.cache/sspm
  max_size: 2GB
  prune_after: 30d

# 日誌
log:
  level: info             # debug | info | warn | error
  path: ~/.local/state/sspm/log
```

---

## 📁 專案結構

```
ShioSakura-Package-Manager-SSPM-/n├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # CLI 參數解析和命令路由
│   ├── resolver/         # 套件解析器和後端選擇器
│   ├── backends/         # 後端配接器
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
│   │   └── spk/          # SSPM 原生
│   ├── transaction/      # 原子事務系統 + 回滾
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # 倉庫管理
│   ├── spk/              # SPK 套件建構器和解析器
│   ├── cache/            # 快取系統
│   ├── profile/          # 設定檔系統
│   ├── mirror/           # 鏡像排名和地理檢測
│   ├── security/         # ed25519 + sha256 驗證
│   ├── doctor/           # 系統診斷
│   ├── api/              # REST API 守護程序
│   ├── log/              # 日誌系統
│   ├── network/          # HTTP 用戶端 (libcurl)
│   └── plugin/           # 外掛載入器
│
├── include/              # 公共標頭檔
├── tests/                # 單元和整合測試
├── docs/                 # 完整文件
├── packages/             # 官方 SPK 套件配方
└── assets/               # 標誌，圖示
```

---

## 🤝 貢獻

非常歡迎貢獻！提交 PR 前請閱讀 [CONTRIBUTING.md](../../CONTRIBUTING.md)。

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 授權

GPLv2 授權 — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

由 [Riu-Mahiyo](https://github.com/Riu-Mahiyo) 用 🌸 製作

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>