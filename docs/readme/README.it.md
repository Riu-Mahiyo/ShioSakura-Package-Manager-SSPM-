<div align="center">

# 🌸 SSPM — ShioSakura Package Manager

**Orchestatore di pacchetti universale · Orchestatore di gestione pacchetti cross-distribuzione**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#supported-platforms)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![GitHub](https://img.shields.io/badge/GitHub-Riu--Mahiyo-181717?logo=github)](https://github.com/Riu-Mahiyo)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/sspm)

> **SSPM** è un orchestratore di gestori di pacchetti universale che si posiziona sopra i gestori di pacchetti nativi del tuo sistema.  
> Un comando. Qualsiasi distribuzione. Qualsiasi backend.

</div>

---

## 📖 Indice

- [Cos'è SSPM?](#what-is-sspm)
- [Architettura](#architecture)
- [Piattaforme supportate](#supported-platforms)
- [Installazione](#installation)
- [Avvio rapido](#quick-start)
- [Riferimento CLI](#cli-reference)
- [Sistema di backend](#backend-system)
- [Formato pacchetto SPK](#spk-package-format)
- [Sistema di repository](#repo-system)
- [Sistema di profili](#profile-system)
- [Sistema di mirror](#mirror-system)
- [Sicurezza](#security)
- [Doctor & Diagnostica](#doctor--diagnostics)
- [Modalità API & GUI](#api-mode--gui)
- [Sistema di plugin](#plugin-system)
- [Configurazione](#configuration)
- [Struttura del progetto](#project-structure)
- [Contribuire](#contributing)
- [Licenza](#license)

---

## 🌸 Cos'è SSPM?

SSPM (**ShioSakura Package Manager**) è un **orchestatore di pacchetti universale** — non sostituisce il gestore di pacchetti del tuo sistema, ma lo *coordina*.

```
Tu  →  sspm install nginx
              ↓
     Livello di astrazione backend
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPM rileva il tuo ambiente, sceglie il miglior backend disponibile, risolve le dipendenze, gestisce le transazioni e ti offre un'esperienza unificata su **Linux, macOS, BSD e Windows**.

---

## 🏗️ Architettura

```
sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  Stile apt / pacman / brew — a tuo piacimento
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Risolutore │  Risoluzione dipendenze transitive
 │  dipendenze │  Risoluzione vincoli versione (>=, <=, !=, …)
 │             │  Rilevamento conflitti / interruzioni
 │             │  Ordinamento topologico → ordine installazione
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Risolutore │  Scegli il miglior backend per ogni pacchetto
 │  backend    │  Strategia: priorità utente → probe pigro → disponibilità
 │  + Registro │  20 backend rilevati automaticamente, mai caricati se non presenti
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ Transazione │  begin → verify → install → commit
 │             │  In caso di fallimento: rollback tramite log di annullamento
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   Livello di astrazione backend              │
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
  Gestore di pacchetti di sistema  (o SPK: gestito nativamente in SSPM)
```

**Sottosistemi supportati:**

| Sottosistema | Ruolo |
|-----------|------|
| Mirror | Rilevamento paese Geo-IP, classificazione latenza, auto-selezione VPN-aware |
| Network | Download parallelo libcurl, ripristino, fallback mirror |
| Security | firme ed25519 + verifica sha256 per file |
| Cache | `~/.cache/sspm` — download + metadati + indici repository |
| SkyDB | SQLite: pacchetti · storia · transazioni · repository · profili |
| Repo | Sincronizzazione repository SPK, recupero indici, verifica firme |
| Index | Ricerca fuzzy, ricerca regex, grafico dipendenze |
| Profile | Gruppi di pacchetti denominati: dev / desktop / server / gaming |
| Logger | `~/.local/state/sspm/log/` — per livello + modalità tail |
| Doctor | Backend · permessi · rete · repository · cache · salute DB |
| Plugin | Estensioni backend `dlopen()` da `~/.local/share/sspm/plugins/` |
| REST API | `sspm-daemon` su `:7373` — alimenta SSPM Center e interfacce GUI |

| Componente | Descrizione |
|-----------|-------------|
| `CLI` | Analisi argomenti, routing comandi, formattazione output |
| `Package Resolver` | Sceglie il miglior backend per ogni pacchetto |
| `Backend Layer` | Adattatori per ogni gestore di pacchetti nativo |
| `Transaction System` | Installazione/rimozione atomica con supporto rollback |
| `SkyDB` | Database stato locale basato su SQLite |
| `Repo System` | Repository ufficiali, terzi e locali `.spk` |
| `SPK Format` | Formato pacchetto portatile nativo di SSPM |
| `Cache System` | Cache download, metadati, indici repository |
| `Profile System` | Gruppi di pacchetti basati su ambiente |
| `Mirror System` | Rilevamento geografico automatico + mirror classificati per latenza |
| `Security` | firme ed25519 + verifica sha256 |
| `Doctor` | Diagnostica sistema e controlli salute |
| `REST API` | Modalità demone per interfacce GUI |
| `Plugin System` | Backend e hook estensibili |

---

## 💻 Piattaforme supportate

### Linux

| Famiglia | Gestori di pacchetti |
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

| Sistema | Gestore di pacchetti |
|--------|----------------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| Strumento | Note |
|------|-------|
| `homebrew` | Disponibile anche come Linuxbrew su Linux |
| `macports` | Backend alternativo |

### Windows

| Strumento | Note |
|------|-------|
| `winget` | Gestore di pacchetti integrato in Windows |
| `scoop` | Installatore spazio utente |
| `chocolatey` | Gestore di pacchetti comunitario |

### Universale (Cross-platform)

| Backend | Note |
|---------|-------|
| `flatpak` | App Linux sandboxate |
| `snap` | Formato universale di Canonical |
| `AppImage` | Binari Linux portatili |
| `nix profile` | Pacchetti cross-platform riproducibili |
| `spk` | Formato pacchetto nativo di SSPM |

---

## 📦 Installazione

### Linux / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

Oppure compilare dalle fonti:

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build && cmake --build build
sudo cmake --install build
```

### Windows

```powershell
winget install SSPM
# o
scoop install sspm
```

---

## 🚀 Avvio rapido

```bash
# Installare un pacchetto
sspm install nginx

# Cercare pacchetti
sspm search nodejs

# Aggiornare database pacchetti
sspm update

# Aggiornare tutti i pacchetti
sspm upgrade

# Rimuovere un pacchetto
sspm remove nginx

# Elencare pacchetti installati
sspm list

# Ottenere informazioni su un pacchetto
sspm info nginx

# Eseguire diagnostica sistema
sspm doctor
```

---

## 📟 Riferimento CLI

### Comandi di base

| Comando | Descrizione |
|---------|-------------|
| `sspm install <pkg>` | Installare un pacchetto |
| `sspm remove <pkg>` | Rimuovere un pacchetto |
| `sspm search <query>` | Cercare pacchetti |
| `sspm update` | Sincronizzare database pacchetti |
| `sspm upgrade` | Aggiornare tutti i pacchetti installati |
| `sspm list` | Elencare pacchetti installati |
| `sspm info <pkg>` | Mostrare dettagli pacchetto |
| `sspm doctor` | Eseguire diagnostica sistema |

### Comandi avanzati

| Comando | Descrizione |
|---------|-------------|
| `sspm repo <sub>` | Gestire repository |
| `sspm cache <sub>` | Gestire cache download |
| `sspm config <sub>` | Modificare configurazione |
| `sspm profile <sub>` | Gestire profili ambiente |
| `sspm history` | Visualizzare storia installazione/rimozione |
| `sspm rollback` | Annullare ultima transazione |
| `sspm verify <pkg>` | Verificare integrità pacchetto |
| `sspm mirror <sub>` | Gestire fonti mirror |
| `sspm log` | Visualizzare log SSPM |
| `sspm log tail` | Seguire output log in tempo reale |

### Flag output

```bash
sspm search nginx --json          # Output JSON
sspm list --format table          # Output tabella

sspm install nginx --verbose      # Modalità verbosa

sspm install nginx --dry-run      # Anteprima senza esecuzione

sspm install nginx --backend apt  # Forzare un backend specifico
```

### Stile comando personalizzato

SSPM supporta più convenzioni di comando. Passa allo stile pacman se preferisci:

```bash
sspm config set cli.style pacman
# sspm -S nginx      → installare
# sspm -Rs nginx     → rimuovere
# sspm -Ss nginx     → cercare
# sspm -Syu          → aggiornare tutto

sspm config set cli.style apt     # predefinito
```

---

## 🔧 Sistema di backend

### Rilevamento automatico pigro

SSPM non carica mai un backend non installato. Il Backend Registry sonda ciascuno dei 20 backend controllando la presenza del loro binario — e **memorizza in cache il risultato**. Nessun binario = nessun caricamento, nessun sovraccarico.

```
Su Arch Linux:
  /usr/bin/pacman   ✅  caricato   (priorità 10)
  /usr/bin/apt-get  ⬜  saltato
  /usr/bin/dnf      ⬜  saltato
  /usr/bin/flatpak  ✅  caricato   (priorità 30)
  spk (integrato)   ✅  caricato   (priorità 50)
```

Dopo aver installato un nuovo strumento (ad esempio, `flatpak`), esegui `sspm doctor` per sondare di nuovo.

### Priorità backend

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

Forzare un backend specifico per un comando:

```bash
sspm install firefox --backend flatpak
```

### Etichette backend (Integrazione SSPM Center / Store)

| Etichetta | Backend |
|-------|---------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | SPK nativo |

---

## 🔍 Risolutore di dipendenze

Il risolutore di SSPM gestisce tre problemi prima di installare qualsiasi cosa:

**1. Risoluzione dipendenze** — tutte le dipendenze transitive vengono espandute ricorsivamente  
**2. Risoluzione versioni** — più vincoli sullo stesso pacchetto vengono ANDati  
**3. Risoluzione conflitti** — controlli `conflicts`, `breaks` e dipendenze inverse

```bash
sspm install certbot --dry-run

Risoluzione dipendenze...
Piano installazione (6 pacchetti):
  installare  libc      2.38    dipendenza di python3
  installare  libffi    3.4.4   dipendenza di python3
  installare  openssl   3.1.4   dipendenza di certbot (>= 3.0)
  installare  python3   3.12.0  dipendenza di certbot (>= 3.9 dopo aggiunta vincolo acme)
  installare  acme      2.7.0   dipendenza di certbot
  installare  certbot   2.7.4   richiesto
```

Vedi [docs/RESOLVER.md](docs/RESOLVER.md) per l'algoritmo completo.

---




```
sspm install nginx
      ↓
Controlli risolutore pacchetti:
  1. backend_priority configurato dall'utente
  2. Quali backend hanno questo pacchetto disponibile
  3. Selezionare il backend disponibile con priorità più alta
      ↓
Esempio risultato: pacman (Arch) → esegue: pacman -S nginx
```

### Configurazione priorità backend

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### Forzare un backend

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### Interfaccia backend

Ogni adattatore backend implementa la stessa interfaccia astratta:

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

## 📦 Formato pacchetto SPK

`.spk` è il formato pacchetto portatile nativo di SSPM.

### Struttura pacchetto

```
package.spk
├── metadata.toml       # Nome pacchetto, versione, dipendenze, script
├── payload.tar.zst     # Payload file compresso (zstd)
├── install.sh          # Hook pre/post installazione
├── remove.sh           # Hook pre/post rimozione
└── signature           # Firma ed25519
```

### Esempio metadata.toml

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

### Costruire un pacchetto SPK

```bash
sspm build ./mypackage/
# Output: mypackage-1.0.0.spk
```

---

## 🗄️ Sistema di repository

### Comandi repository

```bash
sspm repo add https://repo.example.com/sspm     # Aggiungere un repository
sspm repo remove example                        # Rimuovere un repository
sspm repo sync                                  # Sincronizzare tutti i repository
sspm repo list                                  # Elencare repository configurati
```

### Formato repository

```
repo/
├── repo.json       # Metadati repository & indice pacchetti
├── packages/       # File pacchetti .spk
└── signature       # Chiave firma repository (ed25519)
```

I repository supportano: fonti **ufficiali**, **terze parti** e **locali**.

---

## 🧑‍💼 Sistema di profili

I profili raggruppano i pacchetti per ambiente, rendendo facile riprodurre un'impostazione completa.

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### Modelli profilo integrati

| Profilo | Pacchetti tipici |
|---------|-----------------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | browser, lettore multimediale, suite office |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 Sistema di mirror

### Rilevamento geografico automatico

SSPM rileva il tuo paese tramite geolocalizzazione IP e passa automaticamente **tutti i backend** ai mirror regionali più veloci — anche attraverso VPN.

Per gli utenti con **proxy basati su regole** (non VPN tunnel completo), SSPM rileva l'IP di uscita effettivo e gestisce correttamente la selezione del mirror.

```bash
sspm mirror list              # Elencare mirror disponibili
sspm mirror test              # Benchmark mirror per latenza
sspm mirror select            # Scegliere manualmente un mirror
```

### Configurazione mirror

```yaml
# ~/.config/sspm/config.yaml
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP
```

---

## 🔐 Sicurezza

| Funzionalità | Implementazione |
|---------|---------------|
| Firma pacchetti | `ed25519` |
| Verifica hash | `sha256` |
| Ancora fiducia repository | Pinning chiave pubblica per repository |
| Obbligatorio per `.spk` | Verifica firma prima installazione |

```bash
sspm verify nginx              # Verificare un pacchetto installato
sspm verify ./package.spk      # Verificare un file SPK locale
```

---

## 🏥 Doctor & Diagnostica

```bash
sspm doctor
```

Controlli eseguiti:

- ✅ Disponibilità e versione backend
- ✅ Permessi file
- ✅ Connettività rete  
- ✅ Accessibilità repository
- ✅ Integrità cache
- ✅ Integrità database SkyDB
- ✅ Latenza mirror

---

## 🔌 Modalità API & GUI

### Modalità demone

```bash
sspm daemon start      # Avviare demone REST API
sspm daemon stop
sspm daemon status
```

### Endpoint REST API

| Metodo | Endpoint | Descrizione |
|--------|----------|-------------|
| `GET` | `/packages` | Elencare pacchetti installati |
| `GET` | `/packages/search?q=` | Cercare pacchetti |
| `POST` | `/packages/install` | Installare un pacchetto |
| `DELETE` | `/packages/:name` | Rimuovere un pacchetto |
| `GET` | `/repos` | Elencare repository |
| `POST` | `/repos` | Aggiungere un repository |
| `GET` | `/health` | Controllo salute demone |

### SSPM Center (Interfaccia GUI)

SSPM Center è l'interfaccia grafica ufficiale, integrata con:

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

I pacchetti sono etichettati per sorgente backend:

| Etichetta | Significato |
|-------|---------|
| `SSPM-APT` | Installato via apt |
| `SSPM-PACMAN` | Installato via pacman |
| `SSPM-FLATPAK` | Installato via Flatpak |
| `SSPM-SNAP` | Installato via Snap |
| `SSPM-NIX` | Installato via Nix |
| `SSPM-SPK` | Pacchetto nativo SSPM |

Etichette categoria: `🛠 Tools` · `🎮 Games` · `🎵 Media` · `⚙️ System` · `📦 Development` · `🌐 Network`

---

## 🧩 Sistema di plugin

Estendi SSPM con backend e hook personalizzati:

```
~/.local/share/sspm/plugins/
├── aur/            # Backend AUR (Arch User Repository)
├── brew-tap/       # Supporto Homebrew tap
└── docker/         # Backend immagini Docker
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ Configurazione

**File di configurazione:** `~/.config/sspm/config.yaml`

```yaml
# Ordine priorità backend
backend_priority:
  - pacman
  - flatpak
  - nix

# Stile CLI: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# Impostazioni mirror e geolocalizzazione
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# Impostazioni cache
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

## 📁 Struttura del progetto

```
ShioSakura-Package-Manager-SSPM-/n├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # Analisi argomenti CLI e routing comandi
│   ├── resolver/         # Risolutore pacchetti e selettore backend
│   ├── backends/         # Adattatori backend
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
│   │   └── spk/          # Nativo SSPM
│   ├── transaction/      # Sistema transazione atomica + rollback
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # Gestione repository
│   ├── spk/              # Costruttore e parser pacchetti SPK
│   ├── cache/            # Sistema cache
│   ├── profile/          # Sistema profili
│   ├── mirror/           # Classificazione mirror e rilevamento geografico
│   ├── security/         # Verifica ed25519 + sha256
│   ├── doctor/           # Diagnostica sistema
│   ├── api/              # Demone REST API
│   ├── log/              # Sistema logging
│   ├── network/          # Client HTTP (libcurl)
│   └── plugin/           # Caricatore plugin
│
├── include/              # Intestazioni pubbliche
├── tests/                # Test unitari e di integrazione
├── docs/                 # Documentazione completa
├── packages/             # Ricette pacchetti SPK ufficiali
└── assets/               # Loghi, icone
```

---

## 🤝 Contribuire

I contributi sono benvenuti! Per favore leggi [CONTRIBUTING.md](CONTRIBUTING.md) prima di inviare una PR.

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 Licenza

GPLv2 License — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

Made with 🌸 by [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>