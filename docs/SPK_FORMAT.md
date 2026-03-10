# SPK Package Format v2 (.spkg)

## Overview

`.spkg` is SSPM's native portable package format. It is a **tar archive** containing structured metadata, a compressed payload, lifecycle scripts, and an ed25519 signature.

Unlike `.deb` or `.rpm`, a `.spkg` is:
- **Self-describing**: all dependency, conflict, and provides data lives inside the package
- **Verifiable**: every installed file is checksummed at build time
- **Cross-distro**: no distro-specific post-install tooling required
- **Signed**: ed25519 signature is mandatory for packages from repos

---

## Archive Structure

```
package.spkg                     (outer tar, uncompressed)
├── metadata.toml                Package identity, version, classification
├── control/
│   ├── depends.toml             Runtime / build / optional dependencies
│   ├── conflicts.toml           Conflicts, replaces, breaks
│   ├── provides.toml            Virtual package declarations
│   ├── triggers.toml            Post-install trigger hooks
│   └── checksums.sha256         sha256 of every file in payload
├── payload.tar.zst              Installed files (mirrors / filesystem layout)
├── scripts/
│   ├── pre-install.sh
│   ├── post-install.sh
│   ├── pre-remove.sh
│   └── post-remove.sh
└── signature                    ed25519 signature
```

---

## metadata.toml

```toml
[package]
name             = "nginx"
version          = "1.25.3"
epoch            = 0              # increment when version resets (e.g. 2.0 → 1.0)
description      = "HTTP and reverse proxy server"
long_description = "nginx is a web server that can also be used as..."
author           = "Igor Sysoev"
maintainer       = "packages@sspm.dev"
license          = "BSD-2-Clause"
homepage         = "https://nginx.org"
os               = "linux"        # linux | macos | bsd | any
arch             = ["x86_64", "aarch64", "riscv64"]
category         = "network"      # tools | games | multimedia | devel | network ...
tags             = ["web", "http", "proxy", "server"]
build_date       = "2024-01-15"
build_host       = "builder.sspm.dev"
```

---

## control/depends.toml

```toml
[dependencies]
# Required at runtime — must be installed before this package
requires       = ["libc >= 2.17", "libssl >= 3.0", "libpcre2"]

# Only needed to build from source (not enforced at install time)
build_requires = ["gcc >= 12", "make", "libssl-dev"]

# Optional — installed if available, package works without them
recommends     = ["logrotate"]

# Suggested — shown to the user, never auto-installed
suggests       = ["certbot", "nginx-extras"]
```

**Dependency expression syntax:**

| Expression | Meaning |
|------------|---------|
| `zlib` | Any version |
| `zlib >= 1.2.11` | Version 1.2.11 or newer |
| `zlib = 1.2.11` | Exact version |
| `zlib < 2.0` | Any version before 2.0 |
| `zlib != 1.2.9` | Any version except 1.2.9 |
| `?optional-lib` | Optional dependency |

---

## control/conflicts.toml

```toml
[conflicts]
# Packages that cannot coexist with this one
conflicts = ["nginx-legacy", "apache2"]

# Packages whose files this package takes over
replaces  = ["nginx-compat < 1.0"]

# Packages that will break if this is installed
breaks    = ["old-web-framework < 3.0"]
```

---

## control/provides.toml

```toml
[provides]
# Virtual packages this satisfies
# Other packages can depend on "httpd" and nginx will satisfy it
provides       = ["httpd", "web-server"]

# Specific files this package provides (for file-based deps)
provides_files = ["/usr/sbin/nginx", "/usr/bin/nginx"]
```

---

## control/triggers.toml

```toml
[[trigger]]
name       = "update-desktop-db"
watch_path = "/usr/share/applications/"
action     = "update-desktop-database"

[[trigger]]
name       = "ldconfig"
watch_path = "/usr/lib/"
action     = "ldconfig"
```

---

## scripts/ Lifecycle Hooks

```bash
# scripts/pre-install.sh   — runs before payload extraction
# scripts/post-install.sh  — runs after payload extraction
# scripts/pre-remove.sh    — runs before files are deleted
# scripts/post-remove.sh   — runs after files are deleted

#!/bin/bash
set -e
case "$SSPM_HOOK" in
  pre-install)  echo "Setting up..." ;;
  post-install) systemctl enable nginx ;;
  pre-remove)   systemctl stop nginx ;;
  post-remove)  echo "Cleaned up" ;;
esac
```

The environment variable `$SSPM_HOOK` is set by SSPM when running the script.

---

## Security

Every `.spkg` from a repo must be signed:

```bash
# Sign a package (maintainer workflow)
sspm sign mypackage-1.0.spkg --key maintainer.pem

# Verify before install (done automatically by SSPM)
sspm verify mypackage-1.0.spkg
```

SSPM verifies:
1. `signature` (ed25519) against the control manifest + payload hash
2. `control/checksums.sha256` matches the extracted payload files
3. After install: `sspm verify <name>` re-checks all installed files

---

## Building a Package

```
mypackage/
├── metadata.toml
├── control/
│   ├── depends.toml
│   ├── conflicts.toml
│   └── provides.toml
├── payload/
│   └── usr/
│       └── bin/
│           └── myprogram
└── scripts/
    ├── pre-install.sh
    └── post-install.sh
```

```bash
sspm build ./mypackage/
# Output: mypackage-1.0.0.spkg
# SSPM automatically: compresses payload with zstd, generates checksums, bundles archive
```

---

## Epoch

When a package's version numbering resets (e.g. `2.0 → 1.0`), increment the epoch:

```toml
epoch   = 1
version = "1.0.0"
# Full version string: 1:1.0.0
# This is always newer than 0:2.0.0
```
