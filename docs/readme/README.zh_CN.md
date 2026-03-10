<div align="center">

# 🌸 SSPM — ShioSakura 包管理器

**通用包管理调度器 · 跨发行版包管理调度器**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](../../LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#支持的平台)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![GitHub](https://img.shields.io/badge/GitHub-Riu--Mahiyo-181717?logo=github)](https://github.com/Riu-Mahiyo)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)

> **SSPM** 是一个通用包管理调度器，它位于系统原生包管理器之上。
> 一个命令。任何发行版。任何后端。

</div>

---

## 📖 目录

- [什么是 SSPM？](#什么是-sspm)
- [架构](#架构)
- [支持的平台](#支持的平台)
- [安装](#安装)
- [快速开始](#快速开始)
- [CLI 参考](#cli-参考)
- [后端系统](#后端系统)
- [SPK 包格式](#spk-包格式)
- [仓库系统](#仓库系统)
- [配置文件系统](#配置文件系统)
- [镜像系统](#镜像系统)
- [安全性](#安全性)
- [诊断工具](#诊断工具)
- [API 模式和 GUI](#api-模式和-gui)
- [插件系统](#插件系统)
- [配置](#配置)
- [项目结构](#项目结构)
- [贡献](#贡献)
- [许可证](#许可证)

---

## 🌸 什么是 SSPM？

SSPM（**ShioSakura 包管理器**）是一个**通用包管理调度器**——它不会替换系统的包管理器，而是*协调*它们。

```
你  →  sspm install nginx
              ↓
     后端抽象层
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPM 检测您的环境，选择最佳可用后端，解决依赖关系，处理事务，并在**Linux、macOS、BSD 和 Windows** 上为您提供统一的体验。

---

## 🏗️ 架构

```
sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  apt / pacman / brew 风格 — 您的选择
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  依赖       │  解析传递依赖
 │  解析器     │  版本约束解决 (>=, <=, !=, …)
 │             │  冲突/中断检测
 │             │  拓扑排序 → 安装顺序
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  后端       │  为每个包选择最佳后端
 │  解析器     │  策略：用户优先级 → 延迟探测 → 可用性
 │  + 注册表   │  20 个后端自动检测，不存在则不加载
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ 事务处理    │  开始 → 验证 → 安装 → 提交
 │             │  失败时：通过撤销日志回滚
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   后端抽象层                                  │
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
  系统包管理器  (或 SPK: 在 SSPM 中原生处理)
```

**支持的子系统：**

| 子系统 | 角色 |
|-------|------|
| 镜像 | Geo-IP 国家检测，延迟排名，VPN 感知自动选择 |
| 网络 | libcurl 并行下载，恢复，镜像回退 |
| 安全性 | ed25519 签名 + 每个文件的 sha256 验证 |
| 缓存 | `~/.cache/sspm` — 下载 + 元数据 + 仓库索引 |
| SkyDB | SQLite: 包 · 历史 · 事务 · 仓库 · 配置文件 |
| 仓库 | SPK 仓库同步，索引获取，签名检查 |
| 索引 | 模糊搜索，正则搜索，依赖图 |
| 配置文件 | 命名包组：开发 / 桌面 / 服务器 / 游戏 |
| 日志 | `~/.local/state/sspm/log/` — 分级 + 尾部模式 |
| 诊断 | 后端 · 权限 · 网络 · 仓库 · 缓存 · 数据库健康 |
| 插件 | 从 `~/.local/share/sspm/plugins/` 动态加载后端扩展 |
| REST API | `sspm-daemon` 在 `:7373` 上 — 为 SSPM Center 和 GUI 前端提供支持 |

| 组件 | 描述 |
|-------|------|
| `CLI` | 参数解析，命令路由，输出格式化 |
| `包解析器` | 为每个包选择最佳后端 |
| `后端层` | 每个原生包管理器的适配器 |
| `事务系统` | 原子安装/删除，支持回滚 |
| `SkyDB` | 基于 SQLite 的本地状态数据库 |
| `仓库系统` | 官方、第三方和本地 `.spk` 仓库 |
| `SPK 格式` | SSPM 自己的便携式包格式 |
| `缓存系统` | 下载缓存，元数据，仓库索引 |
| `配置文件系统` | 基于环境的包组 |
| `镜像系统` | 自动地理检测 + 延迟排名镜像 |
| `安全性` | ed25519 签名 + sha256 验证 |
| `诊断` | 系统诊断和健康检查 |
| `REST API` | 用于 GUI 前端的守护进程模式 |
| `插件系统` | 可扩展的后端和钩子 |

---

## 💻 支持的平台

### Linux

| 系列 | 包管理器 |
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

| 系统 | 包管理器 |
|------|---------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| 工具 | 说明 |
|------|------|
| `homebrew` | 在 Linux 上也可用作 Linuxbrew |
| `macports` | 替代后端 |

### Windows

| 工具 | 说明 |
|------|------|
| `winget` | 内置 Windows 包管理器 |
| `scoop` | 用户空间安装程序 |
| `chocolatey` | 社区包管理器 |

### 通用（跨平台）

| 后端 | 说明 |
|------|------|
| `flatpak` | 沙盒化 Linux 应用 |
| `snap` | Canonical 的通用格式 |
| `AppImage` | 便携式 Linux 二进制文件 |
| `nix profile` | 可重现的跨平台包 |
| `spk` | SSPM 的原生包格式 |

---

## 📦 安装

### Linux / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

或从源代码构建：

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

## 🚀 快速开始

```bash
# 安装包
sspm install nginx

# 搜索包
sspm search nodejs

# 更新包数据库
sspm update

# 升级所有包
sspm upgrade

# 删除包
sspm remove nginx

# 列出已安装的包
sspm list

# 获取包信息
sspm info nginx

# 运行系统诊断
sspm doctor
```

---

## 📟 CLI 参考

### 基本命令

| 命令 | 描述 |
|------|------|
| `sspm install <pkg>` | 安装包 |
| `sspm remove <pkg>` | 删除包 |
| `sspm search <query>` | 搜索包 |
| `sspm update` | 同步包数据库 |
| `sspm upgrade` | 升级所有已安装的包 |
| `sspm list` | 列出已安装的包 |
| `sspm info <pkg>` | 显示包详情 |
| `sspm doctor` | 运行系统诊断 |

### 高级命令

| 命令 | 描述 |
|------|------|
| `sspm repo <sub>` | 管理仓库 |
| `sspm cache <sub>` | 管理下载缓存 |
| `sspm config <sub>` | 编辑配置 |
| `sspm profile <sub>` | 管理环境配置文件 |
| `sspm history` | 查看安装/删除历史 |
| `sspm rollback` | 回滚最后一个事务 |
| `sspm verify <pkg>` | 验证包完整性 |
| `sspm mirror <sub>` | 管理镜像源 |
| `sspm log` | 查看 SSPM 日志 |
| `sspm log tail` | 跟踪实时日志输出 |

### 输出标志

```bash
sspm search nginx --json          # JSON 输出
sspm list --format table          # 表格输出
sspm install nginx --verbose      # 详细模式
sspm install nginx --dry-run      # 预览而不执行
sspm install nginx --backend apt  # 强制使用特定后端
```

### 自定义命令风格

SSPM 支持多种命令约定。如果您喜欢，可以切换到 pacman 风格：

```bash
sspm config set cli.style pacman
# sspm -S nginx      → 安装
# sspm -Rs nginx     → 删除
# sspm -Ss nginx     → 搜索
# sspm -Syu          → 升级所有

sspm config set cli.style apt     # 默认
```

---

## 🔧 后端系统

### 延迟自动检测

SSPM 永远不会加载未安装的后端。后端注册表通过检查每个后端的二进制文件是否存在来探测 20 个后端 — 并**缓存结果**。没有二进制文件 = 不加载，无开销。

```
在 Arch Linux 上：
  /usr/bin/pacman   ✅  已加载   (优先级 10)
  /usr/bin/apt-get  ⬜  已跳过
  /usr/bin/dnf      ⬜  已跳过
  /usr/bin/flatpak  ✅  已加载   (优先级 30)
  spk (内置)         ✅  已加载   (优先级 50)
```

安装新工具（例如 `flatpak`）后，运行 `sspm doctor` 重新探测。

### 后端优先级

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

为一个命令强制使用特定后端：

```bash
sspm install firefox --backend flatpak
```

### 后端标签（SSPM Center / 商店集成）

| 标签 | 后端 |
|------|------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | 原生 SPK |

---

## 🔍 依赖解析器

SSPM 的解析器在安装任何东西之前处理三个问题：

**1. 依赖解决** — 所有传递依赖都被递归展开  
**2. 版本解决** — 对同一个包的多个约束被 AND 运算  
**3. 冲突解决** — `conflicts`、`breaks` 和反向依赖检查

```bash
sspm install certbot --dry-run

解析依赖...
安装计划 (6 个包)：
  安装  libc      2.38    python3 的依赖
  安装  libffi    3.4.4   python3 的依赖
  安装  openssl   3.1.4   certbot 的依赖 (>= 3.0)
  安装  python3   3.12.0  certbot 的依赖 (acme 添加约束后 >= 3.9)
  安装  acme      2.7.0   certbot 的依赖
  安装  certbot   2.7.4   请求
```

有关完整算法，请参阅 [docs/RESOLVER.md](../../docs/RESOLVER.md)。

---





```
sspm install nginx
      ↓
包解析器检查：
  1. 用户配置的 backend_priority
  2. 哪些后端有这个包可用
  3. 选择最高优先级的可用后端
      ↓
示例结果：pacman (Arch) → 执行：pacman -S nginx
```

### 配置后端优先级

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### 强制使用后端

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### 后端接口

每个后端适配器实现相同的抽象接口：

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

## 📦 SPK 包格式

`.spk` 是 SSPM 的原生便携式包格式。

### 包结构

```
package.spk
├── metadata.toml       # 包名称，版本，依赖，脚本
├── payload.tar.zst     # 压缩文件有效载荷 (zstd)
├── install.sh          # 安装前/后钩子
├── remove.sh           # 删除前/后钩子
└── signature           # ed25519 签名
```

### metadata.toml 示例

```toml
[package]
name = "example"
version = "1.0.0"
description = "一个示例 SPK 包"
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

### 构建 SPK 包

```bash
sspm build ./mypackage/
# 输出: mypackage-1.0.0.spk
```

---

## 🗄️ 仓库系统

### 仓库命令

```bash
sspm repo add https://repo.example.com/sspm     # 添加仓库
sspm repo remove example                        # 删除仓库
sspm repo sync                                  # 同步所有仓库
sspm repo list                                  # 列出配置的仓库
```

### 仓库格式

```
repo/
├── repo.json       # 仓库元数据和包索引
├── packages/       # .spk 包文件
└── signature       # 仓库签名密钥 (ed25519)
```

仓库支持：**官方**、**第三方**和**本地**源。

---

## 🧑‍💼 配置文件系统

配置文件按环境分组包，使重现完整设置变得容易。

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### 内置配置文件模板

| 配置文件 | 典型包 |
|---------|--------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | 浏览器，媒体播放器，办公套件 |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 镜像系统

### 自动地理检测

SSPM 通过 IP 地理定位检测您的国家，并自动将**所有后端**切换到最快的区域镜像 — 即使通过 VPN。

对于使用**基于规则的代理**（不是全隧道 VPN）的用户，SSPM 检测实际的出站 IP 并正确处理镜像选择。

```bash
sspm mirror list              # 列出可用镜像
sspm mirror test              # 通过延迟基准测试镜像
sspm mirror select            # 手动选择镜像
```

### 镜像配置

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

| 功能 | 实现 |
|------|------|
| 包签名 | `ed25519` |
| 哈希验证 | `sha256` |
| 仓库信任锚 | 每个仓库的公钥固定 |
| 对 `.spk` 强制要求 | 安装前签名检查 |

```bash
sspm verify nginx              # 验证已安装的包
sspm verify ./package.spk      # 验证本地 SPK 文件
```

---

## 🏥 诊断工具

```bash
sspm doctor
```

执行的检查：

- ✅ 后端可用性和版本
- ✅ 文件权限
- ✅ 网络连接  
- ✅ 仓库可达性
- ✅ 缓存完整性
- ✅ SkyDB 数据库完整性
- ✅ 镜像延迟

---

## 🔌 API 模式和 GUI

### 守护进程模式

```bash
sspm daemon start      # 启动 REST API 守护进程
sspm daemon stop
sspm daemon status
```

### REST API 端点

| 方法 | 端点 | 描述 |
|------|------|------|
| `GET` | `/packages` | 列出已安装的包 |
| `GET` | `/packages/search?q=` | 搜索包 |
| `POST` | `/packages/install` | 安装包 |
| `DELETE` | `/packages/:name` | 删除包 |
| `GET` | `/repos` | 列出仓库 |
| `POST` | `/repos` | 添加仓库 |
| `GET` | `/health` | 守护进程健康检查 |

### SSPM Center（GUI 前端）

SSPM Center 是官方图形前端，与以下集成：

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

包按后端源标记：

| 标签 | 含义 |
|------|------|
| `SSPM-APT` | 通过 apt 安装 |
| `SSPM-PACMAN` | 通过 pacman 安装 |
| `SSPM-FLATPAK` | 通过 Flatpak 安装 |
| `SSPM-SNAP` | 通过 Snap 安装 |
| `SSPM-NIX` | 通过 Nix 安装 |
| `SSPM-SPK` | SSPM 原生包 |

类别标签：`🛠 工具` · `🎮 游戏` · `🎵 媒体` · `⚙️ 系统` · `📦 开发` · `🌐 网络`

---

## 🧩 插件系统

使用自定义后端和钩子扩展 SSPM：

```
~/.local/share/sspm/plugins/
├── aur/            # AUR (Arch 用户仓库) 后端
├── brew-tap/       # Homebrew tap 支持
└── docker/         # Docker 镜像后端
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ 配置

**配置文件：** `~/.config/sspm/config.yaml`

```yaml
# 后端优先级顺序
backend_priority:
  - pacman
  - flatpak
  - nix

# CLI 风格: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# 镜像和地理设置
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# 缓存设置
cache:
  path: ~/.cache/sspm
  max_size: 2GB
  prune_after: 30d

# 日志
log:
  level: info             # debug | info | warn | error
  path: ~/.local/state/sspm/log
```

---

## 📁 项目结构

```
ShioSakura-Package-Manager-SSPM-/n├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # CLI 参数解析和命令路由
│   ├── resolver/         # 包解析器和后端选择器
│   ├── backends/         # 后端适配器
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
│   ├── transaction/      # 原子事务系统 + 回滚
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # 仓库管理
│   ├── spk/              # SPK 包构建器和解析器
│   ├── cache/            # 缓存系统
│   ├── profile/          # 配置文件系统
│   ├── mirror/           # 镜像排名和地理检测
│   ├── security/         # ed25519 + sha256 验证
│   ├── doctor/           # 系统诊断
│   ├── api/              # REST API 守护进程
│   ├── log/              # 日志系统
│   ├── network/          # HTTP 客户端 (libcurl)
│   └── plugin/           # 插件加载器
│
├── include/              # 公共头文件
├── tests/                # 单元和集成测试
├── docs/                 # 完整文档
├── packages/             # 官方 SPK 包配方
└── assets/               # 徽标，图标
```

---

## 🤝 贡献

非常欢迎贡献！提交 PR 前请阅读 [CONTRIBUTING.md](../../CONTRIBUTING.md)。

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 许可证

GPLv2 许可证 — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

由 [Riu-Mahiyo](https://github.com/Riu-Mahiyo) 用 🌸 制作

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>