# SSPM — ShioSakura Package Manager

**版本：v3.0.0** | GPLv2 | C++17

> SSPM 是一个**包管理调度器**，不是传统包管理器。
> 它统一了 25 个不同的包管理后端，提供一致的 CLI、事务安全、跨平台兼容。

---

## 核心定位

```
用户 → sspm → [事务层] → [Backend Driver 抽象层] → apt / dnf / pacman / brew / cargo / ...
                  ↓
            [统一状态数据库]
```

SSPM 不替换你的包管理器，而是**调度它们,起了冲突我们介入调停**

---

## 特性亮点

| 特性 | 说明 |
|------|------|
| 🔀 **25 个 Backend** | apt / dnf / pacman / brew / macports / cargo / gem / flatpak / snap / nix / npm / pip / … |
| 🧠 **智能镜像选择** | 自动通过 IP 检测区域，配置最近镜像源（CN/JP/US/EU/…） |
| 🔒 **原子事务** | 任何安装失败自动回滚，防止系统半残状态 |
| 📋 **Profile 系统** | 声明式安装配置，一键复现工作环境 |
| 🩺 **12 项健康检查** | `sspm doctor` 全面检查系统状态 |
| 🛡️ **Shell 注入防护** | 所有命令通过 ExecEngine，禁止 system()/popen() |
| 🌏 **国际化** | 中文/英文，自动检测 LANG 环境变量 |
| 🖥️ **桌面集成** | 自动生成 .desktop 文件，注册 MIME 类型 |

---

## 快速开始

```bash
# 编译
make

# 安装（需要 root）
sudo make install

# 基本用法
sspm install curl                     # 使用默认 backend
sspm install htop --backend=apt       # 指定 backend
sspm install gimp --backend=flatpak   # 用 Flatpak 安装
sspm install ripgrep --backend=cargo  # 用 cargo 安装

# 搜索
sspm search firefox

# 升级全部
sspm upgrade

# 系统诊断
sspm doctor
```

---

## 支持的 Backend（25 个）

### Linux 原生
| Backend | 发行版 | 说明 |
|---------|--------|------|
| `apt` | Debian / Ubuntu / Mint | APT 包管理 |
| `dpkg` | Debian 系 | .deb 直接安装 |
| `pacman` | Arch Linux | Pacman |
| `aur` | Arch Linux | AUR (yay/paru/makepkg) |
| `dnf` | Fedora / RHEL / CentOS | DNF |
| `zypper` | openSUSE | Zypper |
| `portage` | Gentoo | Emerge（3600s 编译超时）|

### macOS
| Backend | 说明 |
|---------|------|
| `brew` | Homebrew（需 macOS ≥ 13）|
| `macports` | MacPorts（支持老版本 macOS，提供 .pkg 安装器）|

> ⚠️ 若检测到 macOS < 13，SSPM 会提示改用 MacPorts 并引导安装。

### FreeBSD（Linux/macOS 自动禁用）
| Backend | 说明 |
|---------|------|
| `pkg` | FreeBSD 二进制包 |
| `ports` | FreeBSD Ports Collection（源码编译）|

### 跨平台通用
| Backend | 说明 |
|---------|------|
| `flatpak` | Flatpak 沙箱包 |
| `snap` | Snap 通用包 |
| `nix` | Nix 函数式包管理 |
| `npm` | Node.js 全局包 |
| `pip` | Python 包 |
| `cargo` | Rust crate（全局安装）|
| `gem` | Ruby gem |
| `portable` | AppImage 管理 |
| `wine` | Wine/winetricks |

### SSPM 自有格式
| Backend | 说明 |
|---------|------|
| `spk` | .spk 包格式（SSPM 原生）|
| `amber` | Amber PM 包管理器（Spark Store） |

---

## 智能镜像源

SSPM 可以自动检测你的 IP 区域并配置最近的镜像源：

```bash
# 手动设置区域
SSPM_REGION=CN sspm install python3

# 或通过命令行
sspm install python3 --mirror=CN
```

支持的区域：** CN | JP | KR | DE | US | UK | SG | AU **
> ✅ 只修改 SSPM 管理的源配置，用户自定义源绝对不会被修改。

---

## Profile 系统（声明式安装）

```bash
# 导出当前安装状态
sspm profile export > my-setup.profile

# 应用 profile（幂等）
sspm profile apply my-setup.profile

# 预览将要安装的包（dry-run）
sspm profile apply my-setup.profile --dry-run

# 对比两个 profile
sspm profile diff current.profile target.profile
```

Profile 文件格式：
```toml
[meta]
name = "dev-workstation"
version = "1.0"

[packages]
apt    = ["curl", "git", "vim", "htop", "build-essential"]
pip    = ["black", "pytest", "requests", "mypy"]
npm    = ["typescript", "eslint", "prettier"]
cargo  = ["ripgrep", "bat", "fd-find", "tokei"]

[settings]
mirror_region = "CN"
dry_run_first = true
```

---

## Backend 安装向导

如果某个 backend 未安装，SSPM 会询问是否自动安装配置：

```
$ sspm install gimp --backend=flatpak
[WARN] Backend 'flatpak' 未安装
安装方法: apt install flatpak
[?] 是否现在安装 flatpak？ [y/N] y
→ 正在安装 flatpak...
→ 配置 Flathub 仓库...
✓ flatpak 安装并配置完成
```

---

## 目录结构

```
sspm/
├── main.cpp                  # 程序入口（i18n + 系统检测 + 命令分发）
├── core/
│   ├── detect.{h,cpp}        # OS/架构检测
│   ├── exec_engine.{h,cpp}   # 安全执行层（禁止 system/popen）
│   ├── mirror.{h,cpp}        # 智能镜像选择
│   ├── resolver.{h,cpp}      # 依赖图构建 + 冲突检测
│   └── backend_setup.{h,cpp} # Backend 安装向导
├── layer/
│   ├── backend.h             # 抽象接口（install/remove/upgrade/search/...）
│   ├── layer_manager.{h,cpp} # 25 个 backend 注册中心
│   └── backends/             # 具体实现
├── transaction/              # 原子事务 + 回滚 + 锁
├── db/                       # 统一状态数据库（SkyDB）
├── cli/                      # CLI 路由（20+ 命令）
├── doctor/                   # 12 项系统健康检查
├── plugin/                   # 插件系统
├── profile/                  # Profile / 声明式安装
├── i18n/                     # 多语言（中/英）
├── desktop/                  # Linux 桌面集成
└── docs/                     # 架构文档
```

---

## CLI 命令参考

```
核心命令
  install <pkg...>           安装包
  remove  <pkg...>           卸载包
  upgrade                    升级全部
  search  <query>            搜索包
  list                       列出 SkyDB 已记录包

系统管理
  doctor                     12 项系统健康检查
  backends                   列出所有 backend 及可用性
  db [list|query|orphan]     数据库管理
  lock                       显示安装锁状态
  fix-path [nix|brew]        修复 PATH 配置

Profile 系统
  profile apply <file>       应用安装配置
  profile export             导出当前安装状态
  profile diff <a> <b>       对比两个 profile
  profile list               列出已保存的 profile

Amber PM
  amber-token <token>        设置 Amber PM 认证 token

选项
  --backend=<n>              指定 backend
  --dry-run                  仅预览，不实际执行
  --mirror=<region>          指定镜像区域 (CN/US/EU/JP/...)
  --lang=<code>              指定语言 (zh/en)
  --yes / -y                 跳过确认提示
  --verbose / -v             详细输出
```

---

## 发行版兼容说明

> ⚠️ **不建议在 Manjaro Linux 上使用**，因为 Manjaro 与 Arch 的软件包版本可能出现莫名其妙的兼容 bug。

---

## 安全设计

- **禁止 `system()` / `popen()`**：所有外部命令通过 `ExecEngine::run()` 执行
- **包名白名单**：`InputValidator` 严格验证 `[a-zA-Z0-9._+@-]`，拒绝 Shell 特殊字符
- **参数向量**：命令参数以 `vector<string>` 传递，不拼接字符串
- **原子事务**：安装失败自动回滚，防止系统进入不一致状态
- **安装锁**：防止多个 SSPM 实例并发修改系统

---

## 构建要求

- **C++17** 编译器（GCC 7+ / Clang 5+）
- **POSIX** 系统（Linux / macOS / FreeBSD）
- 标准库：`<filesystem>` `<thread>` `<optional>`

```bash
make          # 编译
make install  # 安装到 /usr/local/bin/sspm
make test     # 运行集成测试
make clean    # 清理
```

---

## 版本历史

| 版本 | 说明 |
|------|------|
| v3.0.0 | 正式更名 SSPM；25 backends；Profile 系统；智能镜像；依赖图；cargo/gem/ports |
| v2.3.0 | (原 Sky) 21 backends；Amber PM；doctor 12项；i18n |
| v2.0.0 | (原 Sky) 12 backends；事务回滚；SkyDB |
| v1.9.1 | (原 Sky v1) 基础版本 |

---

## 许可证

SSPM 采用 **GNU General Public License v2 (GPLv2)** 协议开源。

详见 [LICENSE](LICENSE) 文件。

---

*SSPM — Because one package manager is never enough.*
