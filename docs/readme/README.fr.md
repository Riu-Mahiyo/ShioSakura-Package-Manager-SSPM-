<div align="center">

# 🌸 SSPM — ShioSakura Package Manager

**Orchestrateur de paquets universel · Orchestrateur de gestion de paquets cross-distribution**

[![License: GPLv2](https://img.shields.io/badge/License-GPLv2-pink.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20BSD%20%7C%20Windows-blue)](#supported-platforms)
[![Status](https://img.shields.io/badge/status-active%20development-brightgreen)](#)
[![GitHub](https://img.shields.io/badge/GitHub-Riu--Mahiyo-181717?logo=github)](https://github.com/Riu-Mahiyo)
[![Version](https://img.shields.io/badge/version-v4.0.0--Sakura-pink)](https://github.com/Riu-Mahiyo/sspm)

> **SSPM** est un orchestrateur de gestionnaire de paquets universel qui s'appuie sur les gestionnaires de paquets natifs de votre système.  
> Une commande. N'importe quelle distribution. N'importe quel backend.

</div>

---

## 📖 Table des matières

- [Qu'est-ce que SSPM ?](#what-is-sspm)
- [Architecture](#architecture)
- [Plateformes prises en charge](#supported-platforms)
- [Installation](#installation)
- [Démarrage rapide](#quick-start)
- [Référence CLI](#cli-reference)
- [Système de backends](#backend-system)
- [Format de paquet SPK](#spk-package-format)
- [Système de dépôts](#repo-system)
- [Système de profils](#profile-system)
- [Système de miroirs](#mirror-system)
- [Sécurité](#security)
- [Doctor & Diagnostics](#doctor--diagnostics)
- [Mode API & GUI](#api-mode--gui)
- [Système de plugins](#plugin-system)
- [Configuration](#configuration)
- [Structure du projet](#project-structure)
- [Contribuer](#contributing)
- [Licence](#license)

---

## 🌸 Qu'est-ce que SSPM ?

SSPM (**ShioSakura Package Manager**) est un **orchestrateur de paquets universel** — il ne remplace pas le gestionnaire de paquets de votre système, il le *coordonne*.

```
Vous  →  sspm install nginx
              ↓
     Couche d'abstraction backend
     ↙    ↓    ↓    ↓    ↘
  apt   pacman  brew  winget  flatpak
```

SSPM détecte votre environnement, choisit le meilleur backend disponible, résout les dépendances, gère les transactions et vous offre une expérience unifiée sur **Linux, macOS, BSD et Windows**.

---

## 🏗️ Architecture

```
sspm install nginx
        │
        ▼
 ┌─────────────┐
 │   CLI       │  Style apt / pacman / brew — à votre guise
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Résolveur  │  Résolution des dépendances transitives
 │  de dépendances│  Résolution des contraintes de version (>=, <=, !=, …)
 │             │  Détection de conflits / ruptures
 │             │  Tri topologique → ordre d'installation
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │  Résolveur  │  Choix du meilleur backend par paquet
 │  de backends│  Stratégie: priorité utilisateur → sonde paresseuse → disponibilité
 │  + Registre │  20 backends détectés automatiquement, jamais chargés s'ils ne sont pas présents
 └──────┬──────┘
        │
        ▼
 ┌─────────────┐
 │ Transaction │  begin → verify → install → commit
 │             │  En cas d'échec: rollback via journal d'annulation
 └──────┬──────┘
        │
        ▼
 ┌──────────────────────────────────────────────┐
 │   Couche d'abstraction backend              │
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
  Gestionnaire de paquets système  (ou SPK: géré nativement dans SSPM)
```

**Sous-systèmes pris en charge:**

| Sous-système | Rôle |
|-----------|------|
| Mirror | Détection de pays Geo-IP, classement par latence, auto-sélection VPN-aware |
| Network | Téléchargement parallèle libcurl, reprise, secours sur miroir |
| Security | Signatures ed25519 + vérification sha256 par fichier |
| Cache | `~/.cache/sspm` — téléchargements + métadonnées + index de dépôts |
| SkyDB | SQLite: paquets · historique · transactions · dépôts · profils |
| Repo | Synchronisation de dépôts SPK, récupération d'index, vérification de signature |
| Index | Recherche floue, recherche regex, graphe de dépendances |
| Profile | Groupes de paquets nommés: dev / desktop / server / gaming |
| Logger | `~/.local/state/sspm/log/` — par niveau + mode tail |
| Doctor | Backend · permissions · réseau · dépôts · cache · santé de la DB |
| Plugin | Extensions backend `dlopen()` depuis `~/.local/share/sspm/plugins/` |
| REST API | `sspm-daemon` sur `:7373` — alimente SSPM Center et interfaces GUI |

| Composant | Description |
|-----------|-------------|
| `CLI` | Analyse des arguments, routage des commandes, formatage de sortie |
| `Package Resolver` | Choix du meilleur backend par paquet |
| `Backend Layer` | Adaptateurs pour chaque gestionnaire de paquets natif |
| `Transaction System` | Installation/suppression atomique avec support de rollback |
| `SkyDB` | Base de données d'état locale basée sur SQLite |
| `Repo System` | Dépôts officiels, tiers et locaux `.spk` |
| `SPK Format` | Format de paquet portable propre à SSPM |
| `Cache System` | Cache de téléchargement, métadonnées, index de dépôts |
| `Profile System` | Groupes de paquets basés sur l'environnement |
| `Mirror System` | Détection géographique automatique + miroirs classés par latence |
| `Security` | Signatures ed25519 + vérification sha256 |
| `Doctor` | Diagnostics système et vérifications de santé |
| `REST API` | Mode démon pour interfaces GUI |
| `Plugin System` | Backends et hooks extensibles |

---

## 💻 Plateformes prises en charge

### Linux

| Famille | Gestionnaires de paquets |
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

| Système | Gestionnaire de paquets |
|--------|----------------|
| FreeBSD | `pkg` |
| OpenBSD | `pkg_add` |
| NetBSD | `pkgin` |

### macOS

| Outil | Notes |
|------|-------|
| `homebrew` | Aussi disponible en tant que Linuxbrew sur Linux |
| `macports` | Backend alternatif |

### Windows

| Outil | Notes |
|------|-------|
| `winget` | Gestionnaire de paquets intégré à Windows |
| `scoop` | Installateur espace utilisateur |
| `chocolatey` | Gestionnaire de paquets communautaire |

### Universel (Cross-platform)

| Backend | Notes |
|---------|-------|
| `flatpak` | Applications Linux sandboxées |
| `snap` | Format universel de Canonical |
| `AppImage` | Binaires Linux portables |
| `nix profile` | Paquets cross-platform reproductibles |
| `spk` | Format de paquet natif de SSPM |

---

## 📦 Installation

### Linux / macOS / BSD

```bash
curl -fsSL https://sspm.dev/install.sh | sh
```

Ou construire à partir des sources:

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build && cmake --build build
sudo cmake --install build
```

### Windows

```powershell
winget install SSPM
# ou
scoop install sspm
```

---

## 🚀 Démarrage rapide

```bash
# Installer un paquet
sspm install nginx

# Rechercher des paquets
sspm search nodejs

# Mettre à jour la base de données de paquets
sspm update

# Mettre à jour tous les paquets
sspm upgrade

# Supprimer un paquet
sspm remove nginx

# Lister les paquets installés
sspm list

# Obtenir des informations sur un paquet
sspm info nginx

# Exécuter un diagnostic système
sspm doctor
```

---

## 📟 Référence CLI

### Commandes de base

| Commande | Description |
|---------|-------------|
| `sspm install <pkg>` | Installer un paquet |
| `sspm remove <pkg>` | Supprimer un paquet |
| `sspm search <query>` | Rechercher des paquets |
| `sspm update` | Synchroniser la base de données de paquets |
| `sspm upgrade` | Mettre à jour tous les paquets installés |
| `sspm list` | Lister les paquets installés |
| `sspm info <pkg>` | Afficher les détails d'un paquet |
| `sspm doctor` | Exécuter un diagnostic système |

### Commandes avancées

| Commande | Description |
|---------|-------------|
| `sspm repo <sub>` | Gérer les dépôts |
| `sspm cache <sub>` | Gérer le cache de téléchargement |
| `sspm config <sub>` | Modifier la configuration |
| `sspm profile <sub>` | Gérer les profils d'environnement |
| `sspm history` | Voir l'historique d'installation/suppression |
| `sspm rollback` | Annuler la dernière transaction |
| `sspm verify <pkg>` | Vérifier l'intégrité d'un paquet |
| `sspm mirror <sub>` | Gérer les sources de miroir |
| `sspm log` | Voir les journaux SSPM |
| `sspm log tail` | Suivre la sortie des journaux en direct |

### Drapeaux de sortie

```bash
sspm search nginx --json          # Sortie JSON
sspm list --format table          # Sortie table

sspm install nginx --verbose      # Mode verbeux
sspm install nginx --dry-run      # Aperçu sans exécution
sspm install nginx --backend apt  # Forcer un backend spécifique
```

### Style de commande personnalisé

SSPM prend en charge plusieurs conventions de commande. Passez au style pacman si vous préférez:

```bash
sspm config set cli.style pacman
# sspm -S nginx      → installer
# sspm -Rs nginx     → supprimer
# sspm -Ss nginx     → rechercher
# sspm -Syu          → mettre à jour tout

sspm config set cli.style apt     # par défaut
```

---

## 🔧 Système de backends

### Détection automatique paresseuse

SSPM ne charge jamais un backend qui n'est pas installé. Le Backend Registry sonde chacun des 20 backends en vérifiant la présence de leur binaire — et **met en cache le résultat**. Pas de binaire = pas de chargement, pas de surcharge.

```
Sur Arch Linux:
  /usr/bin/pacman   ✅  chargé   (priorité 10)
  /usr/bin/apt-get  ⬜  ignoré
  /usr/bin/dnf      ⬜  ignoré
  /usr/bin/flatpak  ✅  chargé   (priorité 30)
  spk (intégré)     ✅  chargé   (priorité 50)
```

Après avoir installé un nouvel outil (par exemple, `flatpak`), exécutez `sspm doctor` pour resonder.

### Priorité des backends

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

Forcer un backend spécifique pour une commande:

```bash
sspm install firefox --backend flatpak
```

### Étiquettes de backend (Intégration SSPM Center / Store)

| Étiquette | Backend |
|-------|---------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | Flatpak |
| `SSPM-SNAP` | Snap |
| `SSPM-NIX` | Nix |
| `SSPM-SPK` | SPK natif |

---

## 🔍 Résolveur de dépendances

Le résolveur de SSPM traite trois problèmes avant d'installer quoi que ce soit:

**1. Résolution des dépendances** — toutes les dépendances transitives sont étendues de manière récursive  
**2. Résolution des versions** — plusieurs contraintes sur le même paquet sont ANDées  
**3. Résolution des conflits** — vérifications de `conflicts`, `breaks` et de dépendances inverses

```bash
sspm install certbot --dry-run

Résolution des dépendances...
Plan d'installation (6 paquets):
  installer  libc      2.38    dépendance de python3
  installer  libffi    3.4.4   dépendance de python3
  installer  openssl   3.1.4   dépendance de certbot (>= 3.0)
  installer  python3   3.12.0  dépendance de certbot (>= 3.9 après ajout de contrainte par acme)
  installer  acme      2.7.0   dépendance de certbot
  installer  certbot   2.7.4   demandé
```

Voir [docs/RESOLVER.md](docs/RESOLVER.md) pour l'algorithme complet.

---




```
sspm install nginx
      ↓
Vérifications du résolveur de paquets:
  1. backend_priority configuré par l'utilisateur
  2. Quels backends ont ce paquet disponible
  3. Sélectionner le backend disponible avec la priorité la plus élevée
      ↓
Exemple de résultat: pacman (Arch) → exécute: pacman -S nginx
```

### Configuration de la priorité des backends

```yaml
# ~/.config/sspm/config.yaml
backend_priority:
  - pacman
  - flatpak
  - nix
```

### Forcer un backend

```bash
sspm install nginx --backend apt
sspm install firefox --backend flatpak
sspm install neovim --backend nix
```

### Interface backend

Chaque adaptateur backend implémente la même interface abstraite:

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

## 📦 Format de paquet SPK

`.spk` est le format de paquet portable natif de SSPM.

### Structure du paquet

```
package.spk
├── metadata.toml       # Nom du paquet, version, dépendances, scripts
├── payload.tar.zst     # Payload de fichiers compressé (zstd)
├── install.sh          # Hooks pre/post installation
├── remove.sh           # Hooks pre/post suppression
└── signature           # Signature ed25519
```

### Exemple metadata.toml

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

### Construire un paquet SPK

```bash
sspm build ./mypackage/
# Sortie: mypackage-1.0.0.spk
```

---

## 🗄️ Système de dépôts

### Commandes de dépôts

```bash
sspm repo add https://repo.example.com/sspm     # Ajouter un dépôt
sspm repo remove example                        # Supprimer un dépôt
sspm repo sync                                  # Synchroniser tous les dépôts
sspm repo list                                  # Lister les dépôts configurés
```

### Format de dépôt

```
repo/
├── repo.json       # Métadonnées du dépôt & index de paquets
├── packages/       # Fichiers de paquets .spk
└── signature       # Clé de signature du dépôt (ed25519)
```

Les dépôts prennent en charge: sources **officielles**, **tiers** et **locales**.

---

## 🧑‍💼 Système de profils

Les profils regroupent les paquets par environnement, facilitant la reproduction d'une configuration complète.

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
```

### Modèles de profil intégrés

| Profil | Paquets typiques |
|---------|-----------------|
| `dev` | git, neovim, gcc, python, node, docker |
| `desktop` | navigateur, lecteur multimédia, suite bureautique |
| `server` | nginx, openssh, ufw, htop |
| `gaming` | steam, lutris, wine, gamemode |

---

## 🌐 Système de miroirs

### Détection géographique automatique

SSPM détecte votre pays via la géolocalisation IP et bascule automatiquement **tous les backends** vers les miroirs régionaux les plus rapides — même à travers un VPN.

Pour les utilisateurs avec des **proxys basés sur des règles** (non VPN tunnel complet), SSPM détecte l'IP de sortie réelle et gère correctement la sélection des miroirs.

```bash
sspm mirror list              # Lister les miroirs disponibles
sspm mirror test              # Benchmark des miroirs par latence
sspm mirror select            # Choisir manuellement un miroir
```

### Configuration des miroirs

```yaml
# ~/.config/sspm/config.yaml
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP
```

---

## 🔐 Sécurité

| Fonctionnalité | Implémentation |
|---------|---------------|
| Signatures de paquets | `ed25519` |
| Vérification de hachage | `sha256` |
| Ancres de confiance de dépôt | Pinning de clé publique par dépôt |
| Obligatoire pour `.spk` | Vérification de signature avant installation |

```bash
sspm verify nginx              # Vérifier un paquet installé
sspm verify ./package.spk      # Vérifier un fichier SPK local
```

---

## 🏥 Doctor & Diagnostics

```bash
sspm doctor
```

Vérifications effectuées:

- ✅ Disponibilité et version du backend
- ✅ Permissions de fichiers
- ✅ Connectivité réseau  
- ✅ Accessibilité des dépôts
- ✅ Intégrité du cache
- ✅ Intégrité de la base de données SkyDB
- ✅ Latence des miroirs

---

## 🔌 Mode API & GUI

### Mode démon

```bash
sspm daemon start      # Démarrer le démon REST API
sspm daemon stop
sspm daemon status
```

### Points d'entrée REST API

| Méthode | Point d'entrée | Description |
|--------|----------|-------------|
| `GET` | `/packages` | Lister les paquets installés |
| `GET` | `/packages/search?q=` | Rechercher des paquets |
| `POST` | `/packages/install` | Installer un paquet |
| `DELETE` | `/packages/:name` | Supprimer un paquet |
| `GET` | `/repos` | Lister les dépôts |
| `POST` | `/repos` | Ajouter un dépôt |
| `GET` | `/health` | Vérification de santé du démon |

### SSPM Center (Interface GUI)

SSPM Center est l'interface graphique officielle, intégrée avec:

- **KDE Discover**
- **GNOME Software**
- **Cinnamon Software Manager**

Les paquets sont étiquetés par source de backend:

| Étiquette | Signification |
|-------|---------|
| `SSPM-APT` | Installé via apt |
| `SSPM-PACMAN` | Installé via pacman |
| `SSPM-FLATPAK` | Installé via Flatpak |
| `SSPM-SNAP` | Installé via Snap |
| `SSPM-NIX` | Installé via Nix |
| `SSPM-SPK` | Paquet natif SSPM |

Étiquettes de catégorie: `🛠 Tools` · `🎮 Games` · `🎵 Media` · `⚙️ System` · `📦 Development` · `🌐 Network`

---

## 🧩 Système de plugins

Étendez SSPM avec des backends et des hooks personnalisés:

```
~/.local/share/sspm/plugins/
├── aur/            # Backend AUR (Arch User Repository)
├── brew-tap/       # Support Homebrew tap
└── docker/         # Backend d'images Docker
```

```bash
sspm plugin install aur
sspm plugin list
sspm plugin remove aur
```

---

## ⚙️ Configuration

**Fichier de configuration:** `~/.config/sspm/config.yaml`

```yaml
# Ordre de priorité des backends
backend_priority:
  - pacman
  - flatpak
  - nix

# Style CLI: apt | pacman | brew
cli:
  style: apt
  output: text        # text | json | table

# Paramètres de miroir et de géolocalisation
mirror:
  auto_select: true
  geo_detect: true
  vpn_mode: rule-based    # full | rule-based | off
  preferred_region: JP

# Paramètres de cache
cache:
  path: ~/.cache/sspm
  max_size: 2GB
  prune_after: 30d

# Journalisation
log:
  level: info             # debug | info | warn | error
  path: ~/.local/state/sspm/log
```

---

## 📁 Structure du projet

```
ShioSakura-Package-Manager-SSPM-/n├── CMakeLists.txt
├── README.md
├── LICENSE
│
├── src/
│   ├── cli/              # Analyse des arguments CLI et routage des commandes
│   ├── resolver/         # Résolveur de paquets et sélecteur de backend
│   ├── backends/         # Adaptateurs de backend
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
│   │   └── spk/          # Natif SSPM
│   ├── transaction/      # Système de transaction atomique + rollback
│   ├── database/         # SkyDB (SQLite)
│   ├── repo/             # Gestion des dépôts
│   ├── spk/              # Constructeur et parseur de paquets SPK
│   ├── cache/            # Système de cache
│   ├── profile/          # Système de profils
│   ├── mirror/           # Classement de miroirs et détection géographique
│   ├── security/         # Vérification ed25519 + sha256
│   ├── doctor/           # Diagnostics système
│   ├── api/              # Démon REST API
│   ├── log/              # Système de journalisation
│   ├── network/          # Client HTTP (libcurl)
│   └── plugin/           # Chargeur de plugins
│
├── include/              # En-têtes publics
├── tests/                # Tests unitaires et d'intégration
├── docs/                 # Documentation complète
├── packages/             # Recettes de paquets SPK officielles
└── assets/               # Logos, icônes
```

---

## 🤝 Contribuer

Les contributions sont les bienvenues ! Veuillez lire [CONTRIBUTING.md](CONTRIBUTING.md) avant de soumettre une PR.

```bash
git clone https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-
cd ShioSakura-Package-Manager-SSPM-
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

---

## 📄 Licence

GPLv2 License — © 2025 [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

---

<div align="center">

Made with 🌸 by [Riu-Mahiyo](https://github.com/Riu-Mahiyo)

**[GitHub](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-)** · **[Issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)** · **[Discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)**

</div>