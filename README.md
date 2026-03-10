<div align="center">

# 🌸 SSPM — ShioSakura Package Manager

**Universal Package Orchestrator · 跨发行版包管理调度器**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#supported-platforms)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![GitHub](https://img.shields.io/badge/GitHub-Riu--Mahiyo-181717?logo=github)](https://github.com/Riu-Mahiyo)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)

> **SSPM** is a universal package manager orchestrator that sits on top of your system's native package managers.  
> One command. Any distro. Any backend.

</div>

---

## 📖 Table of Contents

- [What is SSPM?](#what-is-sspm)
- [Architecture](#architecture)
- [Supported Platforms](#supported-platforms)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [CLI Reference](#cli-reference)
- [Backend System](#backend-system)
- [SPK Package Format](#spk-package-format)
- [Repo System](#repo-system)
- [Profile System](#profile-system)
- [Mirror System](#mirror-system)
- [Security](#security)
- [Doctor & Diagnostics](#doctor--diagnostics)
- [API Mode & GUI](#api-mode--gui)
- [Plugin System](#plugin-system)
- [Configuration](#configuration)
- [Project Structure](#project-structure)
- [Contributing](#contributing)
- [License](#license)

---

## 🌸 What is SSPM?

SSPM (**ShioSakura Package Manager**) is a **universal package orchestrator** — it does not replace your system's package manager, it *coordinates* them.

```
You  →  sspm install nginx
              ↓
     Backend Abstraction Layer
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPM detects your environment, picks the best available backend, resolves dependencies, handles transactions, and gives you a unified experience across **Linux, macOS, BSD, and Windows**.

---

## 🏗️ Architecture

```
 sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  apt / pacman / brew style — your choice
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Dep        │  Resolve transitive dependencies
 │  Resolver   │  Version constraint solving (>=, <=, !=, …)
 │             │  Conflict / breaks detection
 │             │  Topological sort → install order
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Backend    │  Pick best backend per package
 │  Resolver   │  Strategy: user priority → lazy probe → availability
 │  + Registry │  20 backends auto-detected, never loaded if not present
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ Transaction │  begin → verify → install → commit
 │             │  On failure: rollback via undo log
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   Backend Abstraction Layer                  │
 │                                              │
 │  Linux:    apt · pacman · dnf · zypper       │
 │            portage · apk (Alpine‑only/non‑glibc) · xbps · nix · run  │
 │  BSD:      pkg · pkg_add · pkgin             │
 │  macOS:    brew · macports                   │
 │  Windows:  winget · scoop · chocolatey       │
 │  Universal: flatpak · snap · appimage · spk  │
 └──────┬───────────────────────────────────────┘
        │
        ▼
  System package manager  (or SPK: handled natively in SSPM)
```

**Support subsystems:**

| Subsystem | Role |
|-----------|------|
| Mirror | Geo-IP country detection, latency ranking, VPN-aware auto-select |
| Network | libcurl parallel download, resume, mirror fallback |
| Security | ed25519 signatures + sha256 per-file verification |
| Cache | `~/.cache/sspm` — downloads + metadata + repo indexes |
| SkyDB | SQLite: packages · history · transactions · repos · profiles |
| Repo | SPK repo sync, index fetch, signature check |
| Index | Fuzzy search, regex search, dependency graph |
| Profile | Named package groups: dev / desktop / server / gaming |
| Logger | `~/.local/state/sspm/log/` — leveled + tail mode |
| Doctor | Backend · permissions · network · repos · cache · db health |
| Plugin | `dlopen()` backend extensions from `~/.local/share/sspm/plugins/` |
| REST API | `sspm-daemon` on `:7373` — powers SSPM Center and GUI frontends |

| Component | Description |
|-----------|-------------|
| `CLI` | Argument parsing, command routing, output formatting |
| `Package Resolver` | Chooses the best backend per package |
| `Backend Layer` | Adapters for each native package manager |
| `Transaction System` | Atomic install/remove with rollback support |
| `SkyDB` | SQLite-based local state database |
| `Repo System` | Official, third-party, and local `.spk` repos |
| `SPK Format` | SSPM's own portable package format |
| `Cache System` | Download cache, metadata, repo indexes |
| `Profile System` | Environment-based package groups |
| `Mirror System` | Auto geo-detection + latency-ranked mirrors |
| `Security` | ed25519 signatures + sha256 verification |
| `Doctor` | System diagnostics and health checks |
| `REST API` | Daemon mode for GUI frontends |
| `Plugin System` | Extensible backends and hooks |

---

## 💻 Supported Platforms

### Linux

| Family | Package Managers |
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

| System | Package Manager |
|--------|----------------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| Tool | Notes |
|------|-------|
| `homebrew` | Also available as Linuxbrew on Linux |
| `macports` | Alternative backend |

### Windows

| Tool | Notes |
|------|-------|
| `winget` | Built-in Windows package manager |
| `scoop` | User-space installer |
| `chocolatey` | Community package manager |

### Universal (Cross-platform)

| Backend | Notes |
|---------|-------|
| `flatpak` | Sandboxed Linux apps |
| `snap` | Canonical's universal format |
| `AppImage` | Portable Linux binaries |
| `nix profile` | Reproducible cross-platform packages |
| `spk` | SSPM's native package format |

---

## 📦 Installation

### Linux / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

Or build from source:

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build && cmake --build build
sudo cmake --install build
```

### Windows

```powershell
winget install SSPM
# or
scoop install sspm
```

---

## 🚀 Quick Start

```bash
# Install a package
sspm install nginx

# Search for packages
sspm search nodejs

# Update package database
sspm update

# Upgrade all packages
sspm upgrade

# Remove a package
sspm remove nginx

# List installed packages
sspm list

# Get package info
sspm info nginx

# Run system diagnostics
sspm doctor
```

---

## 📟 CLI Reference

### Basic Commands

| Command | Description |
|---------|-------------|
| `sspm install <pkg>` | Install a package |
| `sspm remove <pkg>` | Remove a package |
| `sspm search <query>` | Search for packages |
| `sspm update` | Sync package database |
| `sspm upgrade` | Upgrade all installed packages |
| `sspm list` | List installed packages |
| `sspm info <pkg>` | Show package details |
| `sspm doctor` | Run system diagnostics |

### Advanced Commands

| Command | Description |
|---------|-------------|
| `sspm repo <sub>` | Manage repositories |
| `sspm cache <sub>` | Manage download cache |
| `sspm config <sub>` | Edit configuration |
| `sspm profile <sub>` | Manage environment profiles |
| `sspm history` | View install/remove history |
| `sspm rollback` | Roll back last transaction |
| `sspm verify <pkg>` | Verify package integrity |
| `sspm mirror <sub>` | Manage mirror sources |
| `sspm log` | View SSPM logs |
| `sspm log tail` | Follow live log output |

### Output Flags

```bash
sspm search nginx --json          # JSON output
sspm list --format table          # Table output
sspm install nginx --verbose      # Verbose mode
sspm install nginx --dry-run      # Preview without executing
sspm install nginx --backend apt  # Force a specific backend
```

### Custom Command Style

SSPM supports multiple command conventions. Switch to pacman-style if you prefer:

```bash
sspm config set cli.style pacman
# sspm -S nginx      → install
# sspm -Rs nginx     → remove
# sspm -Ss nginx     → search
# sspm -Syu          → upgrade all

sspm config set cli.style apt     # default
```

---

## 🔧 Backend System

### Lazy Auto-Detection

SSPM never loads a backend that isn't installed. The Backend Registry probes each of the 20 backends by checking for the presence of their binary — and **caches the result**. No binary = no load, no overhead.

```
On Arch Linux:
  /usr/bin/pacman   ✅  loaded   (priority 10)
  /usr/bin/apt-get  ⬜  skipped
  /usr/bin/dnf      ⬜  skipped
  /usr/bin/flatpak  ✅  loaded   (priority 30)
  spk (built-in)    ✅  loaded   (priority 50)
```

After installing a new tool (e.g. `flatpak`), run `sspm doctor` to re-probe.

### Backend Priority

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

Force a specific backend for one command:

```bash
sspm install firefox --backend flatpak
```

### Backend Labels (SSPM Center / Store Integration)

| Label | Backend |
|-------|---------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | Native SPK |

---

## 🔍 Dependency Resolver

SSPM's resolver handles three problems before installing anything:

**1. Dependency solving** — all transitive deps are expanded recursively  
**2. Version solving** — multiple constraints on the same package are ANDed  
**3. Conflict solving** — `conflicts`, `breaks`, and reverse-dep checks

```bash
sspm install certbot --dry-run

Resolving dependencies...
Install plan (6 packages):
  install  libc      2.38    dependency of python3
  install  libffi    3.4.4   dependency of python3
  install  openssl   3.1.4   dependency of certbot (>= 3.0)
  install  python3   3.12.0  dependency of certbot (>= 3.9 after acme adds constraint)
  install  acme      2.7.0   dependency of certbot
  install  certbot   2.7.4   requested
```

See [docs/RESOLVER.md](docs/RESOLVER.md) for the full algorithm.

---



```
sspm install nginx
      ↓
Package Resolver checks:
  1. User-configured backend_priority
  2. Which backends have this package available
  3. Select the highest-priority available backend
      ↓
Example result: pacman (Arch) → executes: pacman -S nginx
```

### Configuring Backend Priority

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### Forcing a Backend

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### Backend Interface

Every backend adapter implements the same abstract interface:

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

## 📦 SPK Package Format

`.spk` is SSPM's native portable package format.

### Package Structure

```
package.spk
├── metadata.toml       # Package name, version, deps, scripts
├── payload.tar.zst     # Compressed file payload (zstd)
├── install.sh          # Pre/post install hooks
├── remove.sh           # Pre/post remove hooks
└── signature           # ed25519 signature
```

### metadata.toml Example

```toml
[package]
name = "example"
version = "1.0.0"
description = "An example SPK package"
author = "Riu-Mahiyo"
license = "MIT"
arch = ["x86_64", "aarch64"]

[dependencies]
requires = ["libc >= 2.17", "zlib"]
optional = ["libssl"]

[scripts]
pre_install  = "install.sh pre"
post_install = "install.sh post"
pre_remove   = "remove.sh pre"
post_remove  = "remove.sh post"
```

### Building a SPK Package

```bash
sspm build ./mypackage/
# Output: mypackage-1.0.0.spk
```

---

## 🗄️ Repo System

### Repo Commands

```bash
sspm repo add https://repo.example.com/sspm     # Add a repo
sspm repo remove example                        # Remove a repo
sspm repo sync                                  # Sync all repos
sspm repo list                                  # List configured repos
```

### Repo Format

```
repo/
├── repo.json       # Repo metadata & package index
├── packages/       # .spk package files
└── signature       # Repo signing key (ed25519)
```

Repos support: **official**, **third-party**, and **local** sources.

---

## 🧑‍💼 Profile System

Profiles group packages by environment, making it easy to reproduce a full setup.

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### Built-in Profile Templates

| Profile | Typical Packages |
|---------|-----------------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | browser, media player, office suite |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 Mirror System

### Auto Geo-Detection

SSPM detects your country via IP geolocation and automatically switches **all backends** to the fastest regional mirrors — even through VPN.

For users with **rule-based proxies** (not full-tunnel VPN), SSPM detects the actual outbound IP and handles mirror selection correctly.

```bash
sspm mirror list              # List available mirrors
sspm mirror test              # Benchmark mirrors by latency
sspm mirror select            # Manually choose a mirror
```

### Mirror Config

```yaml
# ~/.config/sspm/config.yaml
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP
```

---

## 🔐 Security

| Feature | Implementation |
|---------|---------------|
| Package signatures | `ed25519` |
| Hash verification | `sha256` |
| Repo trust anchors | Per-repo public key pinning |
| Mandatory for `.spk` | Signature check before install |

```bash
sspm verify nginx              # Verify an installed package
sspm verify ./package.spk      # Verify a local SPK file
```

---

## 🏥 Doctor & Diagnostics

```bash
sspm doctor
```

Checks performed:

- ✅ Backend availability and version
- ✅ File permissions
- ✅ Network connectivity  
- ✅ Repo reachability
- ✅ Cache integrity
- ✅ SkyDB database integrity
- ✅ Mirror latency

---

## 🔌 API Mode & GUI

### Daemon Mode

```bash
sspm daemon start      # Start REST API daemon
sspm daemon stop
sspm daemon status
```

### REST API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/packages` | List installed packages |
| `GET` | `/packages/search?q=` | Search packages |
| `POST` | `/packages/install` | Install a package |
| `DELETE` | `/packages/:name` | Remove a package |
| `GET` | `/repos` | List repos |
| `POST` | `/repos` | Add a repo |
| `GET` | `/health` | Daemon health check |

### SSPM Center (GUI Frontend)

SSPM Center is the official graphical frontend, integrating with:

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

Packages are labeled by backend source:

| Label | Meaning |
|-------|---------|
| `SSPM-APT` | Installed via apt |
| `SSPM-PACMAN` | Installed via pacman |
| `SSPM-FLATPAK` | Installed via Flatpak |
| `SSPM-SNAP` | Installed via Snap |
| `SSPM-NIX` | Installed via Nix |
| `SSPM-SPK` | SSPM native package |

Category tags: `🛠 Tools` · `🎮 Games` · `🎵 Media` · `⚙️ System` · `📦 Development` · `🌐 Network`

---

## 🧩 Plugin System

Extend SSPM with custom backends and hooks:

```
~/.local/share/sspm/plugins/
├── aur/            # AUR (Arch User Repository) backend
├── brew-tap/       # Homebrew tap support
└── docker/         # Docker image backend
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ Configuration

**Config file:** `~/.config/sspm/config.yaml`

```yaml
# Backend priority order
backend_priority:
  - pacman
  - flatpak
  - nix

# CLI style: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# Mirror & geo settings
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# Cache settings
cache:
  path: ~/.cache/sspm
  max_size: 2GB
  prune_after: 30d

# Logging
log:
  level: info             # debug | info | warn | error
  path: ~/.local/state/sspm/log
```

---

## 📁 Project Structure

```
ShioSakura-Package-Manager-SSPM-/
├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # CLI argument parsing & command routing
│   ├── resolver/         # Package resolver & backend selector
│   ├── backends/         # Backend adapters
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
│   │   └── spk/          # SSPM native
│   ├── transaction/      # Atomic transaction system + rollback
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # Repo management
│   ├── spk/              # SPK package builder & parser
│   ├── cache/            # Cache system
│   ├── profile/          # Profile system
│   ├── mirror/           # Mirror ranking & geo-detection
│   ├── security/         # ed25519 + sha256 verification
│   ├── doctor/           # System diagnostics
│   ├── api/              # REST API daemon
│   ├── log/              # Logging system
│   ├── network/          # HTTP client (libcurl)
│   └── plugin/           # Plugin loader
│
├── include/              # Public headers
├── tests/                # Unit & integration tests
├── docs/                 # Full documentation
├── packages/             # Official SPK package recipes
└── assets/               # Logos, icons
```

---

## 🤝 Contributing

Contributions are very welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) before submitting a PR.

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 License

GPLv2 License — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

Made with 🌸 by [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>
