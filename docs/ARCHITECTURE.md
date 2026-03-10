# SSPM Architecture

## System Overview

```
User input
    │
    ▼
┌──────────────────────────────────────────────────────────────┐
│                        CLI Layer                             │
│  src/cli/main.cpp                                            │
│  src/cli/install.cpp · remove.cpp · search.cpp · update.cpp  │
│  src/cli/commands.cpp  (doctor, list, info, profile, …)      │
│                                                              │
│  Supports: apt-style (default) / pacman-style / brew-style   │
└───────────────────────────┬──────────────────────────────────┘
                            │
                            ▼
┌──────────────────────────────────────────────────────────────┐
│                   Dependency Resolver                        │
│  src/resolver/dep_resolver.cpp                               │
│                                                              │
│  1. Expand transitive dependency graph (recursive DFS)       │
│  2. Accumulate & validate version constraints                │
│  3. Detect package conflicts and breaks                      │
│  4. Topological sort → ordered install plan                  │
└───────────────────────────┬──────────────────────────────────┘
                            │
                            ▼
┌──────────────────────────────────────────────────────────────┐
│                   Backend Resolver                           │
│  src/resolver/resolver.cpp                                   │
│                                                              │
│  For each package in the plan:                               │
│    1. Check backend_priority from config                     │
│    2. Probe only available backends (lazy, cached)           │
│    3. Select highest-priority backend that has the package   │
└───────────────────────────┬──────────────────────────────────┘
                            │
                            ▼
┌──────────────────────────────────────────────────────────────┐
│                   Transaction System                         │
│  src/transaction/transaction.cpp                             │
│                                                              │
│  begin → download → verify → extract → install → commit      │
│                                    ↓ on failure              │
│                                 rollback (undo log)          │
└──────────┬────────────────────────────────────────────────── ┘
           │
    ┌──────┴──────┐
    ▼             ▼
┌────────┐  ┌──────────────────────────────────────────────────┐
│ SkyDB  │  │              Backend Abstraction Layer            │
│SQLite  │  │  src/backends/                                    │
│        │  │                                                   │
│packages│  │  Linux:    apt · pacman · dnf · zypper · portage  │
│history │  │            apk · xbps · nix                      │
│tx log  │  │  BSD:      pkg · pkg_add · pkgin                  │
│repos   │  │  macOS:    brew · macports                        │
│profiles│  │  Windows:  winget · scoop · chocolatey            │
└────────┘  │  Universal: flatpak · snap · appimage · spk       │
            └─────────────────────┬────────────────────────────┘
                                  │
                                  ▼
                        System Package Manager
                     (apt-get, pacman, dnf, brew…)
                     or SPK: handled entirely in SSPM
```

---

## Backend Registry (Lazy Detection)

```
BackendRegistry::create()
  Registers 19 BackendDescriptors (O(1), no I/O)
        │
        │ First use of a backend
        ▼
  probe(entry):
    fs::exists(probe_binary)?  →  yes: instantiate, mark available
                               →  no:  mark unavailable, skip
        │
        │ Result is cached — never probed twice in one session
        ▼
  available_backends()
    Returns only present backends, sorted by default_priority

Example on Arch Linux:
  /usr/bin/pacman   exists  → pacman  ✅  priority 10
  /usr/bin/apt-get  missing → apt     ⬜  (not loaded)
  /usr/bin/dnf      missing → dnf     ⬜  (not loaded)
  /usr/bin/flatpak  exists  → flatpak ✅  priority 30
  spk (built-in)            → spk     ✅  priority 50
```

---

## Dependency Resolution Flow

```
sspm install certbot
       │
       ▼
DepResolver::resolve_install("certbot")
       │
       ├─ expand_deps(certbot)
       │     requires: python3 >= 3.8, openssl >= 3.0, acme >= 2.0
       │
       ├─ expand_deps(python3, >= 3.8)
       │     requires: libc >= 2.17, libffi
       │     ← later: acme adds >= 3.9 constraint
       │     → accumulate: python3 must satisfy both → pick 3.12
       │
       ├─ expand_deps(openssl, >= 3.0)
       │     requires: libc >= 2.17   (already selected, constraint ANDed)
       │
       ├─ expand_deps(acme, >= 2.0)
       │     requires: python3 >= 3.9  ← NEW constraint on python3
       │
       ├─ Conflict check: none
       │
       └─ Topological sort (Kahn's):
            libc → libffi → openssl → python3 → acme → certbot
```

---

## SPK Package Format v2

```
package.spkg  (tar archive)
├── metadata.toml          Identity, version, arch, category, tags
├── control/
│   ├── depends.toml       requires / build_requires / recommends / suggests
│   ├── conflicts.toml     conflicts / replaces / breaks
│   ├── provides.toml      virtual packages + provided files
│   ├── triggers.toml      post-install trigger hooks (ldconfig, desktop-db…)
│   └── checksums.sha256   sha256 of every installed file (for sspm verify)
├── payload.tar.zst        Installed files (mirrors / layout)
├── scripts/
│   ├── pre-install.sh
│   ├── post-install.sh
│   ├── pre-remove.sh
│   └── post-remove.sh
└── signature              ed25519 over (control/ manifest + payload hash)
```

---

## SkyDB Schema

```sql
packages     (name PK, version, epoch, description, backend, repo, install_time,
              category, arch, checksums_path)
history      (id, action, pkg_name, pkg_version, backend, timestamp)
transactions (tx_id PK, status, data, created_at)
repos        (name PK, url, enabled, last_sync, pubkey)
profiles     (name PK, packages, created_at)
dep_cache    (pkg_name, dep_name, constraint, backend)   ← dependency graph cache
```

---

## REST API (Daemon Mode)

```
sspm-daemon  (standalone binary, built when SSPM_BUILD_DAEMON=ON)
     │
     └── listens on :7373

GET    /health                    → { "status": "ok", "version": "..." }
GET    /packages                  → installed package list
GET    /packages/search?q=nginx   → search results (all backends)
POST   /packages/install          → { "name": "nginx" }
DELETE /packages/:name            → remove package
GET    /repos                     → configured repos
POST   /repos                     → add repo
DELETE /repos/:name               → remove repo
GET    /backends                  → detected backends + availability
GET    /history                   → install/remove history
```

GUI frontends connect to this API:
- **SSPM Center** (native Qt/GTK app)
- **KDE Discover** integration
- **GNOME Software** integration
- **Cinnamon Software Manager** integration

---

## Plugin System

```
~/.local/share/sspm/plugins/
└── aur/
    ├── plugin.toml          name, version, type="backend"
    └── libaur.so            implements sspm::Backend via create_backend()

PluginManager::load_all()
  for each plugin dir:
    dlopen(lib<name>.so)
    dlsym("create_backend") → shared_ptr<Backend>
    register with BackendRegistry
```

---

## Build Targets

| Target | Binary | Description |
|--------|--------|-------------|
| `sspm` | `sspm` | Main CLI (links sspm_core) |
| `sspm-daemon` | `sspm-daemon` | Standalone REST API daemon |
| `sspm_tests` | — | Unit tests (requires GTest) |
| `sspm_core` | `libsspm_core.a` | Static library shared by all targets |
