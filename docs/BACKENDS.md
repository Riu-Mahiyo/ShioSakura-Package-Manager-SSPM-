# SSPM Backend Guide

## How Backends Work

Every backend implements the same abstract interface:

```cpp
class Backend {
public:
    virtual std::string name() const = 0;
    virtual bool is_available() const = 0;
    virtual Result install(const Package& pkg) = 0;
    virtual Result remove(const Package& pkg) = 0;
    virtual SearchResult search(const std::string& query) = 0;
    virtual Result update() = 0;
    virtual Result upgrade() = 0;
    virtual PackageList list_installed() = 0;
    virtual std::optional<Package> info(const std::string& name) = 0;
};
```

## Supported Backends

### Linux

| Backend | Family | Command |
|---------|--------|---------|
| `apt` | Debian/Ubuntu | `apt-get` |
| `pacman` | Arch Linux | `pacman` |
| `dnf` | Fedora/RHEL | `dnf` |
| `zypper` | OpenSUSE | `zypper` |
| `portage` | Gentoo | `emerge` |
| `apk` | Alpine (musl, non-glibc) *– only enabled on Alpine Linux proper* | `apk` |
| `xbps` | Void Linux | `xbps-install` |
| `nix` | NixOS | `nix-env` |

### BSD

| Backend | System | Command |
|---------|--------|---------|
| `pkg` | FreeBSD | `pkg` |
| `pkg_add` | OpenBSD | `pkg_add` |
| `pkgin` | NetBSD | `pkgin` |

### macOS

| Backend | Notes |
|---------|-------|
| `brew` | Also available as Linuxbrew on Linux |
| `macports` | MacPorts alternative |

### Windows

| Backend | Notes |
|---------|-------|
| `winget` | Built-in (Windows 10+) |
| `scoop` | User-space, no admin required |
| `chocolatey` | Community packages |

### Universal

| Backend | Notes |
|---------|-------|
| `flatpak` | Sandboxed apps, requires Flatpak runtime |
| `snap` | Canonical's universal format |
| `appimage` | Portable single-file binaries |
| `nix` | Nix profile (cross-platform reproducible) |
| `spk` | SSPM native format |

## Adding a New Backend

1. Create `src/backends/<n>/<n>_backend.cpp`
2. Subclass `sspm::Backend`
3. Implement all pure virtual methods
4. Register in `src/resolver/resolver.cpp`
5. Add to `CMakeLists.txt`
6. Add tests in `tests/`

### Minimal Backend Template

```cpp
#include "sspm/backend.h"
#include <cstdlib>

namespace sspm::backends {

class MyBackend : public Backend {
public:
    std::string name() const override { return "mybackend"; }

    bool is_available() const override {
        return std::system("which mybackend > /dev/null 2>&1") == 0;
    }

    Result install(const Package& pkg) override {
        int r = std::system(("mybackend install " + pkg.name).c_str());
        return { r == 0, "", r != 0 ? "install failed" : "" };
    }

    Result remove(const Package& pkg) override {
        int r = std::system(("mybackend remove " + pkg.name).c_str());
        return { r == 0, "", r != 0 ? "remove failed" : "" };
    }

    SearchResult search(const std::string& q) override {
        std::system(("mybackend search " + q).c_str());
        return { {}, true, "" };
    }

    Result update() override {
        return { std::system("mybackend update") == 0, "", "" };
    }

    Result upgrade() override {
        return { std::system("mybackend upgrade") == 0, "", "" };
    }

    PackageList list_installed() override { return {}; }
    std::optional<Package> info(const std::string&) override { return std::nullopt; }
};

} // namespace sspm::backends
```

## Backend Labels (SSPM Center / GUI)

When packages are displayed in SSPM Center or integrated desktop stores,
they carry backend labels:

| Label | Backend |
|-------|---------|
| `SSPM-APT` | apt |
| `SSPM-PACMAN` | pacman |
| `SSPM-DNF` | dnf |
| `SSPM-FLATPAK` | flatpak |
| `SSPM-SNAP` | snap |
| `SSPM-BREW` | brew |
| `SSPM-WINGET` | winget |
| `SSPM-NIX` | nix |
| `SSPM-SPK` | native SPK |
