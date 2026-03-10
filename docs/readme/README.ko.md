<div align="center">

# 🌸 SSPM — ShioSakura 패키지 매니저

**유니버설 패키지 오케스트레이터 · 크로스 배포 패키지 관리 오케스트레이터**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#supported-platforms)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![GitHub](https://img.shields.io/badge/GitHub-Riu--Mahiyo-181717?logo=github)](https://github.com/Riu-Mahiyo)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/sspm)

> **SSPM**은 시스템의 기본 패키지 매니저 위에 구축된 유니버설 패키지 매니저 오케스트레이터입니다.  
> 하나의 명령어로. 어떤 배포판에서도. 어떤 백엔드에서도.

</div>

---

## 📖 목차

- [SSPM이란?](#what-is-sspm)
- [아키텍처](#architecture)
- [지원되는 플랫폼](#supported-platforms)
- [설치](#installation)
- [빠른 시작](#quick-start)
- [CLI 참조](#cli-reference)
- [백엔드 시스템](#backend-system)
- [SPK 패키지 형식](#spk-package-format)
- [저장소 시스템](#repo-system)
- [프로필 시스템](#profile-system)
- [미러 시스템](#mirror-system)
- [보안](#security)
- [도ctor & 진단](#doctor--diagnostics)
- [API 모드 & GUI](#api-mode--gui)
- [플러그인 시스템](#plugin-system)
- [구성](#configuration)
- [프로젝트 구조](#project-structure)
- [기여](#contributing)
- [라이선스](#license)

---

## 🌸 SSPM이란?

SSPM (**ShioSakura Package Manager**)은 **유니버설 패키지 오케스트레이터**입니다 — 시스템의 패키지 매니저를 대체하는 것이 아니라, 그것들을 *조정*합니다.

```
당신  →  sspm install nginx
              ↓
     백엔드 추상화 레이어
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPM은 환경을 감지하고, 가장 적합한 사용 가능한 백엔드를 선택하고, 의존성을 해결하고, 트랜잭션을 처리하며, **Linux, macOS, BSD, Windows** 전체에서 통합된 경험을 제공합니다.

---

## 🏗️ 아키텍처

```
sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  apt / pacman / brew 스타일 — 원하는 대로 선택
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  의존       │  전이적 의존성 해결
 │  해결기     │  버전 제약 해결 (>=, <=, !=, …)
 │             │  충돌 / 중단 감지
 │             │  위상 정렬 → 설치 순서
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  백엔드     │  패키지별로 가장 적합한 백엔드 선택
 │  해결기     │  전략: 사용자 우선순위 → 게으른 프로브 → 가용성
 │  + 레지스트리│  20개의 백엔드 자동 감지, 존재하지 않으면 로드되지 않음
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ 트랜잭션   │  begin → verify → install → commit
 │             │  실패 시: undo log를 통한 롤백
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   백엔드 추상화 레이어                        │
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
  시스템 패키지 매니저  (또는 SPK: SSPM 내에서 기본적으로 처리)
```

**지원되는 서브시스템:**

| 서브시스템 | 역할 |
|-----------|------|
| 미러 | Geo-IP 국가 감지, 지연 시간 순위 매기기, VPN 인식 자동 선택 |
| 네트워크 | libcurl 병렬 다운로드, 재개, 미러 대체 |
| 보안 | ed25519 서명 + sha256 파일별 확인 |
| 캐시 | `~/.cache/sspm` — 다운로드 + 메타데이터 + 저장소 인덱스 |
| SkyDB | SQLite: 패키지 · 기록 · 트랜잭션 · 저장소 · 프로필 |
| 저장소 | SPK 저장소 동기화, 인덱스 가져오기, 서명 확인 |
| 인덱스 | 퍼지 검색, 정규식 검색, 의존성 그래프 |
| 프로필 | 명명된 패키지 그룹: dev / desktop / server / gaming |
| 로거 | `~/.local/state/sspm/log/` — 수준별 + 꼬리 모드 |
| 도ctor | 백엔드 · 권한 · 네트워크 · 저장소 · 캐시 · DB 상태 |
| 플러그인 | `~/.local/share/sspm/plugins/` 에서 `dlopen()` 백엔드 확장 |
| REST API | `:7373` 에서 `sspm-daemon` — SSPM Center 및 GUI 프론트엔드 구동 |

| 구성 요소 | 설명 |
|-----------|-------------|
| `CLI` | 인수 구문 분석, 명령 라우팅, 출력 형식 지정 |
| `Package Resolver` | 패키지별로 가장 적합한 백엔드 선택 |
| `Backend Layer` | 각 기본 패키지 매니저에 대한 어댑터 |
| `Transaction System` | 롤백 지원을 포함한 원자적 설치/제거 |
| `SkyDB` | SQLite 기반 로컬 상태 데이터베이스 |
| `Repo System` | 공식, 타사 및 로컬 `.spk` 저장소 |
| `SPK Format` | SSPM 고유의 이식 가능한 패키지 형식 |
| `Cache System` | 다운로드 캐시, 메타데이터, 저장소 인덱스 |
| `Profile System` | 환경 기반 패키지 그룹 |
| `Mirror System` | 자동 지리 감지 + 지연 시간 순위 미러 |
| `Security` | ed25519 서명 + sha256 확인 |
| `Doctor` | 시스템 진단 및 상태 확인 |
| `REST API` | GUI 프론트엔드용 데몬 모드 |
| `Plugin System` | 확장 가능한 백엔드 및 훅 |

---

## 💻 지원되는 플랫폼

### Linux

| 패밀리 | 패키지 매니저 |
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

| 시스템 | 패키지 매니저 |
|--------|----------------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| 도구 | 참고 사항 |
|------|-------|
| `homebrew` | Linux에서도 Linuxbrew로 사용 가능 |
| `macports` | 대체 백엔드 |

### Windows

| 도구 | 참고 사항 |
|------|-------|
| `winget` | Windows 기본 패키지 매니저 |
| `scoop` | 사용자 공간 설치 프로그램 |
| `chocolatey` | 커뮤니티 패키지 매니저 |

### 유니버설 (크로스 플랫폼)

| 백엔드 | 참고 사항 |
|---------|-------|
| `flatpak` | 샌드박스화된 Linux 앱 |
| `snap` | Canonical의 유니버설 형식 |
| `AppImage` | 이식 가능한 Linux 바이너리 |
| `nix profile` | 재현 가능한 크로스 플랫폼 패키지 |
| `spk` | SSPM의 기본 패키지 형식 |

---

## 📦 설치

### Linux / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

또는 소스에서 빌드:

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build && cmake --build build
sudo cmake --install build
```

### Windows

```powershell
winget install SSPM
# 또는
scoop install sspm
```

---

## 🚀 빠른 시작

```bash
# 패키지 설치
sspm install nginx

# 패키지 검색
sspm search nodejs

# 패키지 데이터베이스 업데이트
sspm update

# 모든 패키지 업그레이드
sspm upgrade

# 패키지 제거
sspm remove nginx

# 설치된 패키지 나열
sspm list

# 패키지 정보 가져오기
sspm info nginx

# 시스템 진단 실행
sspm doctor
```

---

## 📟 CLI 참조

### 기본 명령

| 명령 | 설명 |
|---------|-------------|
| `sspm install <pkg>` | 패키지 설치 |
| `sspm remove <pkg>` | 패키지 제거 |
| `sspm search <query>` | 패키지 검색 |
| `sspm update` | 패키지 데이터베이스 동기화 |
| `sspm upgrade` | 모든 설치된 패키지 업그레이드 |
| `sspm list` | 설치된 패키지 나열 |
| `sspm info <pkg>` | 패키지 세부 정보 표시 |
| `sspm doctor` | 시스템 진단 실행 |

### 고급 명령

| 명령 | 설명 |
|---------|-------------|
| `sspm repo <sub>` | 저장소 관리 |
| `sspm cache <sub>` | 다운로드 캐시 관리 |
| `sspm config <sub>` | 구성 편집 |
| `sspm profile <sub>` | 환경 프로필 관리 |
| `sspm history` | 설치/제거 기록 보기 |
| `sspm rollback` | 마지막 트랜잭션 롤백 |
| `sspm verify <pkg>` | 패키지 무결성 확인 |
| `sspm mirror <sub>` | 미러 소스 관리 |
| `sspm log` | SSPM 로그 보기 |
| `sspm log tail` | 라이브 로그 출력 추적 |

### 출력 플래그

```bash
sspm search nginx --json          # JSON 출력
sspm list --format table          # 테이블 출력
sspm install nginx --verbose      # 자세한 모드
sspm install nginx --dry-run      # 실행하지 않고 미리 보기
sspm install nginx --backend apt  # 특정 백엔드 강제
```

### 사용자 정의 명령 스타일

SSPM은 여러 명령 규칙을 지원합니다. pacman 스타일로 전환하려면:

```bash
sspm config set cli.style pacman
# sspm -S nginx      → 설치
# sspm -Rs nginx     → 제거
# sspm -Ss nginx     → 검색
# sspm -Syu          → 모두 업그레이드

sspm config set cli.style apt     # 기본값
```

---

## 🔧 백엔드 시스템

### 게으른 자동 감지

SSPM은 설치되지 않은 백엔드를 로드하지 않습니다. Backend Registry는 20개의 백엔드 각각을 바이너리 존재 여부를 확인하여 프로브하고 **결과를 캐시**합니다. 바이너리 없음 = 로드되지 않음, 오버헤드 없음.

```
Arch Linux에서:
  /usr/bin/pacman   ✅  로드됨   (우선순위 10)
  /usr/bin/apt-get  ⬜  건너뜀
  /usr/bin/dnf      ⬜  건너뜀
  /usr/bin/flatpak  ✅  로드됨   (우선순위 30)
  spk (내장)        ✅  로드됨   (우선순위 50)
```

새 도구 (예: `flatpak`)를 설치한 후 `sspm doctor`를 실행하여 다시 프로브하십시오.

### 백엔드 우선순위

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

한 명령에 특정 백엔드 강제:

```bash
sspm install firefox --backend flatpak
```

### 백엔드 레이블 (SSPM Center / Store 통합)

| 레이블 | 백엔드 |
|-------|---------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | 기본 SPK |

---

## 🔍 의존성 해결기

SSPM의 해결기는 무언가를 설치하기 전에 세 가지 문제를 처리합니다:

**1. 의존성 해결** — 모든 전이적 의존성이 재귀적으로 확장됩니다  
**2. 버전 해결** — 동일 패키지에 대한 여러 제약이 AND됩니다  
**3. 충돌 해결** — `conflicts`、`breaks` 및 역 의존성 검사

```bash
sspm install certbot --dry-run

의존성 해결 중...
설치 계획 (6개 패키지):
  설치  libc      2.38    python3의 의존성
  설치  libffi    3.4.4   python3의 의존성
  설치  openssl   3.1.4   certbot의 의존성 (>= 3.0)
  설치  python3   3.12.0  certbot의 의존성 (acme이 제약을 추가한 후 >= 3.9)
  설치  acme      2.7.0   certbot의 의존성
  설치  certbot   2.7.4   요청됨
```

전체 알고리즘은 [docs/RESOLVER.md](docs/RESOLVER.md)를 참조하십시오.

---




```
sspm install nginx
      ↓
패키지 해결기 검사:
  1. 사용자 구성된 backend_priority
  2. 어떤 백엔드가 이 패키지를 사용할 수 있는지
  3. 가장 높은 우선순위의 사용 가능한 백엔드 선택
      ↓
예제 결과: pacman (Arch) → 실행: pacman -S nginx
```

### 백엔드 우선순위 구성

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### 백엔드 강제

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### 백엔드 인터페이스

모든 백엔드 어댑터는 동일한 추상 인터페이스를 구현합니다:

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

## 📦 SPK 패키지 형식

`.spk`는 SSPM의 기본 이식 가능한 패키지 형식입니다.

### 패키지 구조

```
package.spk
├── metadata.toml       # 패키지 이름, 버전, 의존성, 스크립트
├── payload.tar.zst     # 압축 파일 페이로드 (zstd)
├── install.sh          # 설치 전/후 훅
├── remove.sh           # 제거 전/후 훅
└── signature           # ed25519 서명
```

### metadata.toml 예제

```toml
[package]
name = "example"
version = "1.0.0"
description = "An example SPK package"
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

### SPK 패키지 빌드

```bash
sspm build ./mypackage/
# 출력: mypackage-1.0.0.spk
```

---

## 🗄️ 저장소 시스템

### 저장소 명령

```bash
sspm repo add https://repo.example.com/sspm     # 저장소 추가
sspm repo remove example                        # 저장소 제거
sspm repo sync                                  # 모든 저장소 동기화
sspm repo list                                  # 구성된 저장소 나열
```

### 저장소 형식

```
repo/
├── repo.json       # 저장소 메타데이터 & 패키지 인덱스
├── packages/       # .spk 패키지 파일
└── signature       # 저장소 서명 키 (ed25519)
```

저장소는 **공식**、**타사**、**로컬** 소스를 지원합니다.

---

## 🧑‍💼 프로필 시스템

프로필은 환경별로 패키지를 그룹화하여 전체 설정을 쉽게 재현할 수 있도록 합니다.

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### 기본 프로필 템플릿

| 프로필 | 일반 패키지 |
|---------|-----------------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | 브라우저, 미디어 플레이어, 오피스 스위트 |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 미러 시스템

### 자동 지리 감지

SSPM은 IP 지오로케이션을 통해 국가를 감지하고 **모든 백엔드**를 가장 빠른 지역 미러로 자동으로 전환합니다 — VPN을 통해서도 가능합니다.

**규칙 기반 프록시** (전체 터널 VPN이 아닌) 사용자의 경우 SSPM은 실제出站 IP를 감지하고 미러 선택을 올바르게 처리합니다.

```bash
sspm mirror list              # 사용 가능한 미러 나열
sspm mirror test              # 지연 시간으로 미러 벤치마크
sspm mirror select            # 수동으로 미러 선택
```

### 미러 구성

```yaml
# ~/.config/sspm/config.yaml
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP
```

---

## 🔐 보안

| 기능 | 구현 |
|---------|---------------|
| 패키지 서명 | `ed25519` |
| 해시 확인 | `sha256` |
| 저장소 신뢰 앵커 | 저장소별 공개 키 고정 |
| `.spk`에 필수 | 설치 전 서명 확인 |

```bash
sspm verify nginx              # 설치된 패키지 확인
sspm verify ./package.spk      # 로컬 SPK 파일 확인
```

---

## 🏥 도ctor & 진단

```bash
sspm doctor
```

실행된 검사:

- ✅ 백엔드 가용성 및 버전
- ✅ 파일 권한
- ✅ 네트워크 연결  
- ✅ 저장소 접근성
- ✅ 캐시 무결성
- ✅ SkyDB 데이터베이스 무결성
- ✅ 미러 지연 시간

---

## 🔌 API 모드 & GUI

### 데몬 모드

```bash
sspm daemon start      # REST API 데몬 시작
sspm daemon stop
sspm daemon status
```

### REST API 엔드포인트

| 메서드 | 엔드포인트 | 설명 |
|--------|----------|-------------|
| `GET` | `/packages` | 설치된 패키지 나열 |
| `GET` | `/packages/search?q=` | 패키지 검색 |
| `POST` | `/packages/install` | 패키지 설치 |
| `DELETE` | `/packages/:name` | 패키지 제거 |
| `GET` | `/repos` | 저장소 나열 |
| `POST` | `/repos` | 저장소 추가 |
| `GET` | `/health` | 데몬 상태 확인 |

### SSPM Center (GUI 프론트엔드)

SSPM Center는 공식 그래픽 프론트엔드로 다음과 통합됩니다:

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

패키지는 백엔드 소스별로 레이블이 지정됩니다:

| 레이블 | 의미 |
|-------|---------|
| `SSPM-APT` | apt를 통해 설치 |
| `SSPM-PACMAN` | pacman을 통해 설치 |
| `SSPM-FLATPAK` | Flatpak을 통해 설치 |
| `SSPM-SNAP` | Snap을 통해 설치 |
| `SSPM-NIX` | Nix를 통해 설치 |
| `SSPM-SPK` | SSPM 기본 패키지 |

카테고리 태그: `🛠 Tools` · `🎮 Games` · `🎵 Media` · `⚙️ System` · `📦 Development` · `🌐 Network`

---

## 🧩 플러그인 시스템

사용자 정의 백엔드와 훅으로 SSPM 확장:

```
~/.local/share/sspm/plugins/
├── aur/            # AUR (Arch User Repository) 백엔드
├── brew-tap/       # Homebrew tap 지원
└── docker/         # Docker 이미지 백엔드
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ 구성

**구성 파일:** `~/.config/sspm/config.yaml`

```yaml
# 백엔드 우선 순위
backend_priority:
  - pacman
  - flatpak
  - nix

# CLI 스타일: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# 미러 & 지리 설정
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# 캐시 설정
cache:
  path: ~/.cache/sspm
  max_size: 2GB
  prune_after: 30d

# 로깅
log:
  level: info             # debug | info | warn | error
  path: ~/.local/state/sspm/log
```

---

## 📁 프로젝트 구조

```
ShioSakura-Package-Manager-SSPM-/n├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # CLI 인수 구문 분석 & 명령 라우팅
│   ├── resolver/         # 패키지 해결기 & 백엔드 선택기
│   ├── backends/         # 백엔드 어댑터
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
│   │   └── spk/          # SSPM 기본
│   ├── transaction/      # 원자적 트랜잭션 시스템 + 롤백
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # 저장소 관리
│   ├── spk/              # SPK 패키지 빌더 & 파서
│   ├── cache/            # 캐시 시스템
│   ├── profile/          # 프로필 시스템
│   ├── mirror/           # 미러 순위 매기기 & 지리 감지
│   ├── security/         # ed25519 + sha256 확인
│   ├── doctor/           # 시스템 진단
│   ├── api/              # REST API 데몬
│   ├── log/              # 로깅 시스템
│   ├── network/          # HTTP 클라이언트 (libcurl)
│   └── plugin/           # 플러그인 로더
│
├── include/              # 공개 헤더
├── tests/                # 단위 & 통합 테스트
├── docs/                 # 전체 문서
├── packages/             # 공식 SPK 패키지 레시피
└── assets/               # 로고, 아이콘
```

---

## 🤝 기여

기여는 매우 환영합니다! PR을 제출하기 전에 [CONTRIBUTING.md](CONTRIBUTING.md)를 읽어주세요.

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 라이선스

GPLv2 License — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

Made with 🌸 by [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>