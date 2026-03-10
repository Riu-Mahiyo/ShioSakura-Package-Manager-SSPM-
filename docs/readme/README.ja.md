<div align="center">

# 🌸 SSPM — ShioSakura パッケージマネージャー

**ユニバーサルパッケージオーケストレーター · クロスディストリビューションパッケージ管理オーケストレーター**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#supported-platforms)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/sspm)

> **SSPM** は、システムのネイティブパッケージマネージャーの上に構築されたユニバーサルパッケージマネージャーオーケストレーターです。  
> 一つのコマンド。どのディストリビューションでも。どのバックエンドでも。

</div>

---

## 📖 目次

- [SSPMとは？](#what-is-sspm)
- [アーキテクチャ](#architecture)
- [サポートされるプラットフォーム](#supported-platforms)
- [インストール](#installation)
- [クイックスタート](#quick-start)
- [CLIリファレンス](#cli-reference)
- [バックエンドシステム](#backend-system)
- [SPKパッケージ形式](#spk-package-format)
- [リポジトリシステム](#repo-system)
- [プロファイルシステム](#profile-system)
- [ミラーシステム](#mirror-system)
- [セキュリティ](#security)
- [ドクター＆診断](#doctor--diagnostics)
- [APIモード＆GUI](#api-mode--gui)
- [プラグインシステム](#plugin-system)
- [設定](#configuration)
- [プロジェクト構造](#project-structure)
- [コントリビューション](#contributing)
- [ライセンス](#license)

---

## 🌸 SSPMとは？

SSPM (**ShioSakura Package Manager**) は **ユニバーサルパッケージオーケストレーター** です — システムのパッケージマネージャーを置き換えるのではなく、それらを*調整*します。

```
あなた  →  sspm install nginx
              ↓
     バックエンド抽象化レイヤー
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPMは環境を検出し、最適な利用可能なバックエンドを選択し、依存関係を解決し、トランザクションを処理し、**Linux、macOS、BSD、Windows** 全体で統一されたエクスペリエンスを提供します。

---

## 🏗️ アーキテクチャ

```
sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  apt / pacman / brew スタイル — お好みで選択
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  依存       │  推移的依存関係の解決
 │  解決器     │  バージョン制約の解決 (>=, <=, !=, …)
 │             │  競合 / ブレークの検出
 │             │  トポロジカルソート → インストール順序
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  バックエンド│  パッケージごとに最適なバックエンドを選択
 │  解決器     │  戦略: ユーザー優先度 → レイジープローブ → 利用可能性
 │  + レジストリ│  20のバックエンドが自動検出、存在しない場合はロードされない
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ トランザクション│  begin → verify → install → commit
 │             │  失敗時: アンドゥログによるロールバック
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   バックエンド抽象化レイヤー                  │
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
  システムパッケージマネージャー  (または SPK: SSPM内でネイティブに処理)
```

**サポートされるサブシステム:**

| サブシステム | 役割 |
|-----------|------|
| ミラー | Geo-IP国検出、レイテンシーランキング、VPN対応オートセレクト |
| ネットワーク | libcurl並列ダウンロード、再開、ミラーフォールバック |
| セキュリティ | ed25519署名 + sha256ファイル毎検証 |
| キャッシュ | `~/.cache/sspm` — ダウンロード + メタデータ + リポジトリインデックス |
| SkyDB | SQLite: パッケージ · 履歴 · トランザクション · リポジトリ · プロファイル |
| リポジトリ | SPKリポジトリ同期、インデックス取得、署名チェック |
| インデックス | ファジー検索、正規表現検索、依存関係グラフ |
| プロファイル | 名前付きパッケージグループ: dev / desktop / server / gaming |
| ロガー | `~/.local/state/sspm/log/` — レベル別 + テールモード |
| ドクター | バックエンド · パーミッション · ネットワーク · リポジトリ · キャッシュ · DBヘルス |
| プラグイン | `~/.local/share/sspm/plugins/` からの `dlopen()` バックエンド拡張 |
| REST API | `:7373` 上の `sspm-daemon` — SSPM Center と GUIフロントエンドを駆動 |

| コンポーネント | 説明 |
|-----------|-------------|
| `CLI` | 引数解析、コマンドルーティング、出力フォーマット |
| `Package Resolver` | パッケージごとに最適なバックエンドを選択 |
| `Backend Layer` | 各ネイティブパッケージマネージャーのアダプター |
| `Transaction System` | ロールバックサポートを備えたアトミックなインストール/削除 |
| `SkyDB` | SQLiteベースのローカル状態データベース |
| `Repo System` | 公式、サードパーティ、ローカルの `.spk` リポジトリ |
| `SPK Format` | SSPM独自のポータブルパッケージ形式 |
| `Cache System` | ダウンロードキャッシュ、メタデータ、リポジトリインデックス |
| `Profile System` | 環境ベースのパッケージグループ |
| `Mirror System` | 自動地理検出 + レイテンシーランクされたミラー |
| `Security` | ed25519署名 + sha256検証 |
| `Doctor` | システム診断とヘルスチェック |
| `REST API` | GUIフロントエンドのためのデーモンモード |
| `Plugin System` | 拡張可能なバックエンドとフック |

---

## 💻 サポートされるプラットフォーム

### Linux

| ファミリー | パッケージマネージャー |
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

| システム | パッケージマネージャー |
|--------|----------------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| ツール | ノート |
|------|-------|
| `homebrew` | LinuxではLinuxbrewとしても利用可能 |
| `macports` | 代替バックエンド |

### Windows

| ツール | ノート |
|------|-------|
| `winget` | Windows組み込みパッケージマネージャー |
| `scoop` | ユーザー空間インストーラー |
| `chocolatey` | コミュニティパッケージマネージャー |

### ユニバーサル (クロスプラットフォーム)

| バックエンド | ノート |
|---------|-------|
| `flatpak` | サンドボックス化されたLinuxアプリ |
| `snap` | Canonicalのユニバーサル形式 |
| `AppImage` | ポータブルLinuxバイナリ |
| `nix profile` | 再現可能なクロスプラットフォームパッケージ |
| `spk` | SSPMのネイティブパッケージ形式 |

---

## 📦 インストール

### Linux / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

またはソースからビルド:

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build && cmake --build build
sudo cmake --install build
```

### Windows

```powershell
winget install SSPM
# または
scoop install sspm
```

---

## 🚀 クイックスタート

```bash
# パッケージをインストール
sspm install nginx

# パッケージを検索
sspm search nodejs

# パッケージデータベースを更新
sspm update

# すべてのパッケージをアップグレード
sspm upgrade

# パッケージを削除
sspm remove nginx

# インストール済みパッケージを一覧表示
sspm list

# パッケージ情報を取得
sspm info nginx

# システム診断を実行
sspm doctor
```

---

## 📟 CLIリファレンス

### 基本コマンド

| コマンド | 説明 |
|---------|-------------|
| `sspm install <pkg>` | パッケージをインストール |
| `sspm remove <pkg>` | パッケージを削除 |
| `sspm search <query>` | パッケージを検索 |
| `sspm update` | パッケージデータベースを同期 |
| `sspm upgrade` | すべてのインストール済みパッケージをアップグレード |
| `sspm list` | インストール済みパッケージを一覧表示 |
| `sspm info <pkg>` | パッケージの詳細を表示 |
| `sspm doctor` | システム診断を実行 |

### 高度なコマンド

| コマンド | 説明 |
|---------|-------------|
| `sspm repo <sub>` | リポジトリを管理 |
| `sspm cache <sub>` | ダウンロードキャッシュを管理 |
| `sspm config <sub>` | 設定を編集 |
| `sspm profile <sub>` | 環境プロファイルを管理 |
| `sspm history` | インストール/削除履歴を表示 |
| `sspm rollback` | 前回のトランザクションをロールバック |
| `sspm verify <pkg>` | パッケージの整合性を検証 |
| `sspm mirror <sub>` | ミラーソースを管理 |
| `sspm log` | SSPMログを表示 |
| `sspm log tail` | ライブログ出力を追跡 |

### 出力フラグ

```bash
sspm search nginx --json          # JSON出力
sspm list --format table          # テーブル出力
sspm install nginx --verbose      # 詳細モード
sspm install nginx --dry-run      # 実行せずにプレビュー
sspm install nginx --backend apt  # 特定のバックエンドを強制
```

### カスタムコマンドスタイル

SSPMは複数のコマンド規約をサポートしています。pacmanスタイルに切り替えたい場合は:

```bash
sspm config set cli.style pacman
# sspm -S nginx      → インストール
# sspm -Rs nginx     → 削除
# sspm -Ss nginx     → 検索
# sspm -Syu          → すべてアップグレード

sspm config set cli.style apt     # デフォルト
```

---

## 🔧 バックエンドシステム

### レイジー自動検出

SSPMはインストールされていないバックエンドをロードすることはありません。Backend Registryは20のバックエンドのそれぞれを、そのバイナリの存在をチェックすることでプローブし、**結果をキャッシュ**します。バイナリがない = ロードされず、オーバーヘッドなし。

```
Arch Linux上:
  /usr/bin/pacman   ✅  ロード済み   (優先度 10)
  /usr/bin/apt-get  ⬜  スキップ
  /usr/bin/dnf      ⬜  スキップ
  /usr/bin/flatpak  ✅  ロード済み   (優先度 30)
  spk (組み込み)    ✅  ロード済み   (優先度 50)
```

新しいツール (例: `flatpak`) をインストールした後は、`sspm doctor` を実行して再プローブしてください。

### バックエンド優先度

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

1つのコマンドに特定のバックエンドを強制:

```bash
sspm install firefox --backend flatpak
```

### バックエンドラベル (SSPM Center / Store統合)

| ラベル | バックエンド |
|-------|---------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | ネイティブSPK |

---

## 🔍 依存関係解決器

SSPMの解決器は何かをインストールする前に3つの問題を処理します:

**1. 依存関係解決** — すべての推移的依存関係が再帰的に展開されます  
**2. バージョン解決** — 同じパッケージに対する複数の制約がANDされます  
**3. 競合解決** — `conflicts`、`breaks`、および逆依存関係チェック

```bash
sspm install certbot --dry-run

依存関係を解決中...
インストール計画 (6パッケージ):
  インストール  libc      2.38    python3の依存関係
  インストール  libffi    3.4.4   python3の依存関係
  インストール  openssl   3.1.4   certbotの依存関係 (>= 3.0)
  インストール  python3   3.12.0  certbotの依存関係 (acmeが制約を追加した後 >= 3.9)
  インストール  acme      2.7.0   certbotの依存関係
  インストール  certbot   2.7.4   要求された
```

完全なアルゴリズムについては、[docs/RESOLVER.md](docs/RESOLVER.md) を参照してください。

---




```
sspm install nginx
      ↓
パッケージ解決器のチェック:
  1. ユーザー設定のbackend_priority
  2. どのバックエンドがこのパッケージを利用可能にしているか
  3. 最も高い優先度の利用可能なバックエンドを選択
      ↓
例の結果: pacman (Arch) → 実行: pacman -S nginx
```

### バックエンド優先度の設定

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### バックエンドの強制

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### バックエンドインターフェース

すべてのバックエンドアダプターは同じ抽象インターフェースを実装しています:

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

## 📦 SPKパッケージ形式

`.spk` はSSPMのネイティブポータブルパッケージ形式です。

### パッケージ構造

```
package.spk
├── metadata.toml       # パッケージ名、バージョン、依存関係、スクリプト
├── payload.tar.zst     # 圧縮ファイルペイロード (zstd)
├── install.sh          # インストール前/後のフック
├── remove.sh           # 削除前/後のフック
└── signature           # ed25519署名
```

### metadata.tomlの例

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

### SPKパッケージのビルド

```bash
sspm build ./mypackage/
# 出力: mypackage-1.0.0.spk
```

---

## 🗄️ リポジトリシステム

### リポジトリコマンド

```bash
sspm repo add https://repo.example.com/sspm     # リポジトリを追加
sspm repo remove example                        # リポジトリを削除
sspm repo sync                                  # すべてのリポジトリを同期
sspm repo list                                  # 設定されたリポジトリを一覧表示
```

### リポジトリ形式

```
repo/
├── repo.json       # リポジトリメタデータ & パッケージインデックス
├── packages/       # .spkパッケージファイル
└── signature       # リポジトリ署名キー (ed25519)
```

リポジトリは **公式**、**サードパーティ**、**ローカル** ソースをサポートしています。

---

## 🧑‍💼 プロファイルシステム

プロファイルは環境ごとにパッケージをグループ化し、完全なセットアップを再現しやすくします。

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### 組み込みプロファイルテンプレート

| プロファイル | 典型的なパッケージ |
|---------|-----------------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | ブラウザ、メディアプレイヤー、オフィススイート |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 ミラーシステム

### 自動地理検出

SSPMはIPジオロケーションを介してあなたの国を検出し、**すべてのバックエンド**を最速の地域ミラーに自動的に切り替えます — VPNを介しても可能です。

**ルールベースプロキシ** (フルトンネルVPNではない) を使用するユーザーの場合、SSPMは実際の送信IPを検出し、ミラー選択を正しく処理します。

```bash
sspm mirror list              # 利用可能なミラーを一覧表示
sspm mirror test              # レイテンシーによるミラーのベンチマーク
sspm mirror select            # 手動でミラーを選択
```

### ミラー設定

```yaml
# ~/.config/sspm/config.yaml
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP
```

---

## 🔐 セキュリティ

| 機能 | 実装 |
|---------|---------------|
| パッケージ署名 | `ed25519` |
| ハッシュ検証 | `sha256` |
| リポジトリトラストアンカー | リポジトリごとの公開鍵ピンニング |
| `.spk`に必須 | インストール前の署名チェック |

```bash
sspm verify nginx              # インストール済みパッケージを検証
sspm verify ./package.spk      # ローカルSPKファイルを検証
```

---

## 🏥 ドクター＆診断

```bash
sspm doctor
```

実行されるチェック:

- ✅ バックエンドの利用可能性とバージョン
- ✅ ファイルパーミッション
- ✅ ネットワーク接続  
- ✅ リポジトリの到達可能性
- ✅ キャッシュの整合性
- ✅ SkyDBデータベースの整合性
- ✅ ミラーレイテンシー

---

## 🔌 APIモード＆GUI

### デーモンモード

```bash
sspm daemon start      # REST APIデーモンを起動
sspm daemon stop
sspm daemon status
```

### REST APIエンドポイント

| メソッド | エンドポイント | 説明 |
|--------|----------|-------------|
| `GET` | `/packages` | インストール済みパッケージを一覧表示 |
| `GET` | `/packages/search?q=` | パッケージを検索 |
| `POST` | `/packages/install` | パッケージをインストール |
| `DELETE` | `/packages/:name` | パッケージを削除 |
| `GET` | `/repos` | リポジトリを一覧表示 |
| `POST` | `/repos` | リポジトリを追加 |
| `GET` | `/health` | デーモンヘルスチェック |

### SSPM Center (GUIフロントエンド)

SSPM Centerは公式のグラフィカルフロントエンドで、以下と統合しています:

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

パッケージはバックエンドソースによってラベル付けされます:

| ラベル | 意味 |
|-------|---------|
| `SSPM-APT` | aptを介してインストール |
| `SSPM-PACMAN` | pacmanを介してインストール |
| `SSPM-FLATPAK` | Flatpakを介してインストール |
| `SSPM-SNAP` | Snapを介してインストール |
| `SSPM-NIX` | Nixを介してインストール |
| `SSPM-SPK` | SSPMネイティブパッケージ |

カテゴリタグ: `🛠 Tools` · `🎮 Games` · `🎵 Media` · `⚙️ System` · `📦 Development` · `🌐 Network`

---

## 🧩 プラグインシステム

カスタムバックエンドとフックでSSPMを拡張:

```
~/.local/share/sspm/plugins/
├── aur/            # AUR (Arch User Repository)バックエンド
├── brew-tap/       # Homebrew tapサポート
└── docker/         # Dockerイメージバックエンド
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ 設定

**設定ファイル:** `~/.config/sspm/config.yaml`

```yaml
# バックエンド優先順位
backend_priority:
  - pacman
  - flatpak
  - nix

# CLIスタイル: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# ミラー＆地理設定
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# キャッシュ設定
cache:
  path: ~/.cache/sspm
  max_size: 2GB
  prune_after: 30d

# ロギング
log:
  level: info             # debug | info | warn | error
  path: ~/.local/state/sspm/log
```

---

## 📁 プロジェクト構造

```
ShioSakura-Package-Manager-SSPM-/n├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # CLI引数解析 & コマンドルーティング
│   ├── resolver/         # パッケージ解決器 & バックエンドセレクター
│   ├── backends/         # バックエンドアダプター
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
│   │   └── spk/          # SSPMネイティブ
│   ├── transaction/      # アトミックトランザクションシステム + ロールバック
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # リポジトリ管理
│   ├── spk/              # SPKパッケージビルダー & パーサー
│   ├── cache/            # キャッシュシステム
│   ├── profile/          # プロファイルシステム
│   ├── mirror/           # ミラーランキング & 地理検出
│   ├── security/         # ed25519 + sha256検証
│   ├── doctor/           # システム診断
│   ├── api/              # REST APIデーモン
│   ├── log/              # ロギングシステム
│   ├── network/          # HTTPクライアント (libcurl)
│   └── plugin/           # プラグインローダー
│
├── include/              # パブリックヘッダー
├── tests/                # ユニット & インテグレーションテスト
├── docs/                 # 完全なドキュメント
├── packages/             # 公式SPKパッケージレシピ
└── assets/               # ロゴ、アイコン
```

---

## 🤝 コントリビューション

コントリビューションは大歓迎です！PRを送信する前に [CONTRIBUTING.md](CONTRIBUTING.md) を読んでください。

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 ライセンス

GPLv2 License — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

Made with 🌸 by [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>