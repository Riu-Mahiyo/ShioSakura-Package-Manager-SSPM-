<div align="center">

# 🌸 SSPM — ShioSakura Paketmanager

**Universeller Paketorchestrator · Cross-Distribution-Paketverwaltungsorchestrator**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#supported-platforms)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![GitHub](https://img.shields.io/badge/GitHub-Riu--Mahiyo-181717?logo=github)](https://github.com/Riu-Mahiyo)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/sspm)

> **SSPM** ist ein universeller Paketmanager-Orchestrator, der auf dem nativen Paketmanager Ihres Systems aufbaut.  
> Ein Befehl. Jede Distribution. Jedes Backend.

</div>

---

## 📖 Inhaltsverzeichnis

- [Was ist SSPM?](#what-is-sspm)
- [Architektur](#architecture)
- [Unterstützte Plattformen](#supported-platforms)
- [Installation](#installation)
- [Schnellstart](#quick-start)
- [CLI-Referenz](#cli-reference)
- [Backend-System](#backend-system)
- [SPK-Paketformat](#spk-package-format)
- [Repo-System](#repo-system)
- [Profil-System](#profile-system)
- [Mirror-System](#mirror-system)
- [Sicherheit](#security)
- [Doctor & Diagnose](#doctor--diagnostics)
- [API-Modus & GUI](#api-mode--gui)
- [Plugin-System](#plugin-system)
- [Konfiguration](#configuration)
- [Projektstruktur](#project-structure)
- [Beitragen](#contributing)
- [Lizenz](#license)

---

## 🌸 Was ist SSPM?

SSPM (**ShioSakura Package Manager**) ist ein **universeller Paketorchestrator** — er ersetzt nicht den Paketmanager Ihres Systems, sondern *koordiniert* sie.

```
Sie  →  sspm install nginx
              ↓
     Backend-Abstraktionsschicht
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPM erkennt Ihre Umgebung, wählt das beste verfügbare Backend aus, löst Abhängigkeiten auf, verwaltet Transaktionen und bietet Ihnen ein einheitliches Erlebnis auf **Linux, macOS, BSD und Windows**.

---

## 🏗️ Architektur

```
sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  apt / pacman / brew-Stil — nach Belieben
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Dep        │  Transitive Abhängigkeiten auflösen
 │  Resolver   │  Versionsbeschränkungsauflösung (>=, <=, !=, …)
 │             │  Konflikt / Unterbrechungsdetektion
 │             │  Topologische Sortierung → Installationsreihenfolge
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Backend    │  Bestes Backend pro Paket auswählen
 │  Resolver   │  Strategie: Benutzerpriorität → Lazy-Probe → Verfügbarkeit
 │  + Registry │  20 Backends automatisch erkannt, nie geladen wenn nicht vorhanden
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ Transaction │  begin → verify → install → commit
 │             │  Bei Fehler: Rollback über Undo-Log
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   Backend-Abstraktionsschicht                │
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
  System-Paketmanager  (oder SPK: nativ in SSPM behandelt)
```

**Unterstützte Subsysteme:**

| Subsystem | Rolle |
|-----------|------|
| Mirror | Geo-IP-Länderdetektion, Latenz-Ranking, VPN-bewusste Autoselektion |
| Network | libcurl-Parallel-Download, Fortsetzung, Mirror-Fallback |
| Security | ed25519-Signaturen + sha256-Dateiper-Überprüfung |
| Cache | `~/.cache/sspm` — Downloads + Metadaten + Repo-Indexe |
| SkyDB | SQLite: Pakete · Verlauf · Transaktionen · Repos · Profile |
| Repo | SPK-Repo-Synchronisation, Indexabruf, Signaturprüfung |
| Index | Fuzzy-Suche, Regex-Suche, Abhängigkeitsgraph |
| Profile | Benannte Paketgruppen: dev / desktop / server / gaming |
| Logger | `~/.local/state/sspm/log/` — Stufenweise + Tail-Modus |
| Doctor | Backend · Berechtigungen · Netzwerk · Repos · Cache · DB-Health |
| Plugin | `dlopen()`-Backend-Erweiterungen aus `~/.local/share/sspm/plugins/` |
| REST API | `sspm-daemon` auf `:7373` — treibt SSPM Center und GUI-Frontends an |

| Komponente | Beschreibung |
|-----------|-------------|
| `CLI` | Argumentenparsing, Befehlsrouting, Ausgabeformatierung |
| `Package Resolver` | Wählt das beste Backend pro Paket |
| `Backend Layer` | Adapter für jeden nativen Paketmanager |
| `Transaction System` | Atomare Installation/Entfernung mit Rollback-Unterstützung |
| `SkyDB` | SQLite-basierte lokale Zustandsdatenbank |
| `Repo System` | Offizielle, Drittanbieter- und lokale `.spk`-Repos |
| `SPK Format` | SSPMs eigenes portables Paketformat |
| `Cache System` | Download-Cache, Metadaten, Repo-Indexe |
| `Profile System` | Umgebungsbasierte Paketgruppen |
| `Mirror System` | Automatische Geodetektioun + Latenz-rankte Mirror |
| `Security` | ed25519-Signaturen + sha256-Überprüfung |
| `Doctor` | Systemdiagnose und Gesundheitschecks |
| `REST API` | Daemon-Modus für GUI-Frontends |
| `Plugin System` | Erweiterbare Backends und Hooks |

---

## 💻 Unterstützte Plattformen

### Linux

| Familie | Paketmanager |
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

| System | Paketmanager |
|--------|----------------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| Tool | Hinweise |
|------|-------|
| `homebrew` | Auch als Linuxbrew unter Linux verfügbar |
| `macports` | Alternatives Backend |

### Windows

| Tool | Hinweise |
|------|-------|
| `winget` | Integrierter Windows-Paketmanager |
| `scoop` | Benutzerraum-Installer |
| `chocolatey` | Community-Paketmanager |

### Universal (Cross-Platform)

| Backend | Hinweise |
|---------|-------|
| `flatpak` | Gesandboxte Linux-Apps |
| `snap` | Canonicals universelles Format |
| `AppImage` | Portable Linux-Binärdateien |
| `nix profile` | Reproduzierbare Cross-Platform-Pakete |
| `spk` | SSPMs natives Paketformat |

---

## 📦 Installation

### Linux / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

Oder aus der Quelle bauen:

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build && cmake --build build
sudo cmake --install build
```

### Windows

```powershell
winget install SSPM
# oder
scoop install sspm
```

---

## 🚀 Schnellstart

```bash
# Paket installieren
sspm install nginx

# Pakete suchen
sspm search nodejs

# Paketdatenbank aktualisieren
sspm update

# Alle Pakete upgraden
sspm upgrade

# Paket entfernen
sspm remove nginx

# Installierte Pakete auflisten
sspm list

# Paketinfo abrufen
sspm info nginx

# Systemdiagnose ausführen
sspm doctor
```

---

## 📟 CLI-Referenz

### Grundbefehle

| Befehl | Beschreibung |
|---------|-------------|
| `sspm install <pkg>` | Paket installieren |
| `sspm remove <pkg>` | Paket entfernen |
| `sspm search <query>` | Pakete suchen |
| `sspm update` | Paketdatenbank synchronisieren |
| `sspm upgrade` | Alle installierten Pakete upgraden |
| `sspm list` | Installierte Pakete auflisten |
| `sspm info <pkg>` | Paketdetails anzeigen |
| `sspm doctor` | Systemdiagnose ausführen |

### Erweiterte Befehle

| Befehl | Beschreibung |
|---------|-------------|
| `sspm repo <sub>` | Repositories verwalten |
| `sspm cache <sub>` | Download-Cache verwalten |
| `sspm config <sub>` | Konfiguration bearbeiten |
| `sspm profile <sub>` | Umgebungsprofile verwalten |
| `sspm history` | Installations/Entfernungsverlauf anzeigen |
| `sspm rollback` | Letzte Transaktion rückgängig machen |
| `sspm verify <pkg>` | Paketintegrität überprüfen |
| `sspm mirror <sub>` | Mirror-Quellen verwalten |
| `sspm log` | SSPM-Logs anzeigen |
| `sspm log tail` | Live-Logausgabe folgen |

### Ausgabe-Flags

```bash
sspm search nginx --json          # JSON-Ausgabe
sspm list --format table          # Tabellenausgabe
sspm install nginx --verbose      # Ausführlicher Modus
sspm install nginx --dry-run      # Vorschau ohne Ausführung
sspm install nginx --backend apt  # Bestimmtes Backend erzwingen
```

### Benutzerdefinierter Befehlsstil

SSPM unterstützt mehrere Befehlskonventionen. Wechseln Sie zu pacman-Stil, wenn Sie es vorziehen:

```bash
sspm config set cli.style pacman
# sspm -S nginx      → installieren
# sspm -Rs nginx     → entfernen
# sspm -Ss nginx     → suchen
# sspm -Syu          → alles upgraden

sspm config set cli.style apt     # Standard
```

---

## 🔧 Backend-System

### Lazy-Autodetektion

SSPM lädt niemals ein Backend, das nicht installiert ist. Das Backend-Registry sondiert jedes der 20 Backends, indem es auf das Vorhandensein ihrer Binärdatei prüft — und **cacht das Ergebnis**. Keine Binärdatei = kein Laden, keine Overhead.

```
Auf Arch Linux:
  /usr/bin/pacman   ✅  geladen   (Priorität 10)
  /usr/bin/apt-get  ⬜  übersprungen
  /usr/bin/dnf      ⬜  übersprungen
  /usr/bin/flatpak  ✅  geladen   (Priorität 30)
  spk (eingebaut)   ✅  geladen   (Priorität 50)
```

Nach der Installation eines neuen Tools (z. B. `flatpak`) führen Sie `sspm doctor` aus, um erneut zu sondieren.

### Backend-Priorität

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

Ein bestimmtes Backend für einen Befehl erzwingen:

```bash
sspm install firefox --backend flatpak
```

### Backend-Labels (SSPM Center / Store-Integration)

| Label | Backend |
|-------|---------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | Nativer SPK |

---

## 🔍 Abhängigkeitsresolver

SSPMs Resolver behandelt drei Probleme, bevor etwas installiert wird:

**1. Abhängigkeitsauflösung** — alle transitiven Abhängigkeiten werden rekursiv erweitert  
**2. Versionsauflösung** — mehrere Beschränkungen für dasselbe Paket werden mit AND verknüpft  
**3. Konfliktauflösung** — `conflicts`, `breaks` und Reverse-Dep-Checks

```bash
sspm install certbot --dry-run

Auflösen von Abhängigkeiten...
Installationsplan (6 Pakete):
  install  libc      2.38    Abhängigkeit von python3
  install  libffi    3.4.4   Abhängigkeit von python3
  install  openssl   3.1.4   Abhängigkeit von certbot (>= 3.0)
  install  python3   3.12.0  Abhängigkeit von certbot (>= 3.9 nach Hinzufügen der acme-Beschränkung)
  install  acme      2.7.0   Abhängigkeit von certbot
  install  certbot   2.7.4   angefordert
```

Siehe [docs/RESOLVER.md](docs/RESOLVER.md) für den vollständigen Algorithmus.

---




```
sspm install nginx
      ↓
Paketresolver prüft:
  1. Benutzerkonfigurierte backend_priority
  2. Welche Backends dieses Paket verfügbar haben
  3. Wählt das verfügbarste Backend mit höchster Priorität
      ↓
Beispielergebnis: pacman (Arch) → führt aus: pacman -S nginx
```

### Backend-Priorität konfigurieren

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### Backend erzwingen

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### Backend-Interface

Jeder Backend-Adapter implementiert das gleiche abstrakte Interface:

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

## 📦 SPK-Paketformat

`.spk` ist SSPMs natives portables Paketformat.

### Paketstruktur

```
package.spk
├── metadata.toml       # Paketname, Version, Abhängigkeiten, Skripte
├── payload.tar.zst     # Komprimierte Dateipayload (zstd)
├── install.sh          # Pre/Post-Install-Hooks
├── remove.sh           # Pre/Post-Remove-Hooks
└── signature           # ed25519-Signatur
```

### metadata.toml-Beispiel

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

### SPK-Paket bauen

```bash
sspm build ./mypackage/
# Ausgabe: mypackage-1.0.0.spk
```

---

## 🗄️ Repo-System

### Repo-Befehle

```bash
sspm repo add https://repo.example.com/sspm     # Repo hinzufügen
sspm repo remove example                        # Repo entfernen
sspm repo sync                                  # Alle Repos synchronisieren
sspm repo list                                  # Konfigurierte Repos auflisten
```

### Repo-Format

```
repo/
├── repo.json       # Repo-Metadaten & Paketindex
├── packages/       # .spk-Paketdateien
└── signature       # Repo-Signaturschlüssel (ed25519)
```

Repos unterstützen: **offizielle**, **Drittanbieter** und **lokale** Quellen.

---

## 🧑‍💼 Profil-System

Profile gruppieren Pakete nach Umgebung, was es leicht macht, eine vollständige Einrichtung zu reproduzieren.

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### Eingebaute Profilvorlagen

| Profil | Typische Pakete |
|---------|-----------------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | Browser, Mediaplayer, Office-Suite |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 Mirror-System

### Automatische Geodetektioun

SSPM erkennt Ihr Land über IP-Geolokalisierung und schaltet **alle Backends** automatisch auf die schnellsten regionalen Mirror um — sogar durch VPN.

Für Benutzer mit **regelbasierten Proxies** (kein Full-Tunnel-VPN) erkennt SSPM die tatsächliche ausgehende IP und behandelt die Mirror-Auswahl korrekt.

```bash
sspm mirror list              # Verfügbare Mirror auflisten
sspm mirror test              # Mirror nach Latenz benchmarken
sspm mirror select            # Mirror manuell auswählen
```

### Mirror-Konfiguration

```yaml
# ~/.config/sspm/config.yaml
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP
```

---

## 🔐 Sicherheit

| Feature | Implementierung |
|---------|---------------|
| Paketsignaturen | `ed25519` |
| Hash-Überprüfung | `sha256` |
| Repo-Trust-Anker | Pro-Repo-Public-Key-Pinning |
| Pflicht für `.spk` | Signaturprüfung vor Installation |

```bash
sspm verify nginx              # Installiertes Paket überprüfen
sspm verify ./package.spk      # Lokale SPK-Datei überprüfen
```

---

## 🏥 Doctor & Diagnose

```bash
sspm doctor
```

Durchgeführte Prüfungen:

- ✅ Backend-Verfügbarkeit und Version
- ✅ Dateiberechtigungen
- ✅ Netzwerkverbindung  
- ✅ Repo-Erreichbarkeit
- ✅ Cache-Integrität
- ✅ SkyDB-Datenbankintegrität
- ✅ Mirror-Latenz

---

## 🔌 API-Modus & GUI

### Daemon-Modus

```bash
sspm daemon start      # REST API-Daemon starten
sspm daemon stop
sspm daemon status
```

### REST API-Endpunkte

| Methode | Endpunkt | Beschreibung |
|--------|----------|-------------|
| `GET` | `/packages` | Installierte Pakete auflisten |
| `GET` | `/packages/search?q=` | Pakete suchen |
| `POST` | `/packages/install` | Paket installieren |
| `DELETE` | `/packages/:name` | Paket entfernen |
| `GET` | `/repos` | Repos auflisten |
| `POST` | `/repos` | Repo hinzufügen |
| `GET` | `/health` | Daemon-Health-Check |

### SSPM Center (GUI-Frontend)

SSPM Center ist das offizielle grafische Frontend, das integriert ist mit:

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

Pakete sind nach Backend-Quelle gekennzeichnet:

| Label | Bedeutung |
|-------|---------|
| `SSPM-APT` | Über apt installiert |
| `SSPM-PACMAN` | Über pacman installiert |
| `SSPM-FLATPAK` | Über Flatpak installiert |
| `SSPM-SNAP` | Über Snap installiert |
| `SSPM-NIX` | Über Nix installiert |
| `SSPM-SPK` | SSPM-Nativpaket |

Kategorietags: `🛠 Tools` · `🎮 Games` · `🎵 Media` · `⚙️ System` · `📦 Development` · `🌐 Network`

---

## 🧩 Plugin-System

Erweitern Sie SSPM mit benutzerdefinierten Backends und Hooks:

```
~/.local/share/sspm/plugins/
├── aur/            # AUR (Arch User Repository)-Backend
├── brew-tap/       # Homebrew tap-Unterstützung
└── docker/         # Docker-Image-Backend
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ Konfiguration

**Konfigurationsdatei:** `~/.config/sspm/config.yaml`

```yaml
# Backend-Prioritätsreihenfolge
backend_priority:
  - pacman
  - flatpak
  - nix

# CLI-Stil: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# Mirror & Geo-Einstellungen
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# Cache-Einstellungen
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

## 📁 Projektstruktur

```
ShioSakura-Package-Manager-SSPM-/n├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # CLI-Argumentenparsing & Befehlsrouting
│   ├── resolver/         # Paketresolver & Backend-Selektor
│   ├── backends/         # Backend-Adapter
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
│   │   └── spk/          # SSPM-nativ
│   ├── transaction/      # Atomares Transaktionssystem + Rollback
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # Repo-Verwaltung
│   ├── spk/              # SPK-Paket-Builder & Parser
│   ├── cache/            # Cache-System
│   ├── profile/          # Profil-System
│   ├── mirror/           # Mirror-Ranking & Geodetektioun
│   ├── security/         # ed25519 + sha256-Überprüfung
│   ├── doctor/           # Systemdiagnose
│   ├── api/              # REST API-Daemon
│   ├── log/              # Logging-System
│   ├── network/          # HTTP-Client (libcurl)
│   └── plugin/           # Plugin-Loader
│
├── include/              # Öffentliche Header
├── tests/                # Unit- & Integrations-Tests
├── docs/                 # Vollständige Dokumentation
├── packages/             # Offizielle SPK-Paketrezepte
└── assets/               # Logos, Icons
```

---

## 🤝 Beitragen

Beiträge sind sehr willkommen! Bitte lesen Sie [CONTRIBUTING.md](CONTRIBUTING.md) bevor Sie einen PR senden.

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 Lizenz

GPLv2 License — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

Made with 🌸 by [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>