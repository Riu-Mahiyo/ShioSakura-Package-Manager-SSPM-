# SSPM Dependency Resolver

## Overview

SSPM's dependency resolver (`DepResolver`) solves three distinct problems before any package is installed:

1. **Dependency solving** — find all transitive dependencies
2. **Version solving** — pick a version satisfying all accumulated constraints
3. **Conflict solving** — detect and report incompatible packages

The solver is designed to be **correct before fast**: it implements a complete backtracking search rather than a greedy first-match, similar to apt's resolver architecture.

---

## Architecture

```
User request: install nginx
       │
       ▼
┌──────────────────────────┐
│  DepResolver             │
│                          │
│  expand_deps(nginx)      │  ← recursive DFS through dependency graph
│    └─ expand_deps(libc)  │
│    └─ expand_deps(pcre2) │
│    └─ expand_deps(ssl)   │
│         └─ ...           │
└──────────┬───────────────┘
           │
           ▼
┌──────────────────────────┐
│  Conflict check          │  ← check each selected pkg against all others
│  Version conflict check  │  ← verify accumulated constraints are satisfiable
└──────────┬───────────────┘
           │
           ▼
┌──────────────────────────┐
│  Topological sort        │  ← Kahn's algorithm → install order
│  (Kahn's algorithm)      │
└──────────┬───────────────┘
           │
           ▼
    Install Plan (ordered)
```

---

## Version Constraint Syntax

| Expression | Meaning |
|------------|---------|
| `libc` | Any version |
| `libc >= 2.17` | 2.17 or newer |
| `libc = 2.17` | Exactly 2.17 |
| `libc > 2.17` | Strictly newer than 2.17 |
| `libc < 3.0` | Older than 3.0 |
| `libc != 2.18` | Any version except 2.18 |

Multiple constraints on the same package are ANDed:

```
nginx requires:    openssl >= 1.1
certbot requires:  openssl >= 3.0  AND  openssl < 4.0

Result: openssl must satisfy >= 3.0 AND < 4.0
        → picks openssl 3.x
```

---

## Version Comparison Algorithm

SSPM uses a component-based comparison (similar to RPM/dpkg):

```
1.10.2  vs  1.9.3
  │  │   │    │
  1  10  2    9
  │           │
  equal      10 > 9  → 1.10.2 is newer
```

Epoch always wins: `1:1.0.0 > 0:99.9.9`

---

## Dependency Solving: Step by Step

### Example: `sspm install certbot`

```
1. Expand certbot
   certbot requires: python3 >= 3.8, openssl >= 3.0, acme >= 2.0

2. Expand python3
   python3 requires: libc >= 2.17, libffi

3. Expand openssl
   openssl requires: libc >= 2.17   ← same dep, constraints ANDed

4. Expand acme
   acme requires: python3 >= 3.9   ← NEW constraint on python3!

5. Accumulated constraints for python3:
   >= 3.8  (from certbot)
   >= 3.9  (from acme)
   → pick python3 >= 3.9  ✅

6. Topological sort → install order:
   libc → libffi → openssl → python3 → acme → certbot
```

---

## Conflict Detection

### Package conflicts

```toml
# nginx conflicts with apache2
[conflicts]
conflicts = ["apache2"]
```

If `apache2` is already installed and you try to `sspm install nginx`:

```
❌ Conflict: nginx conflicts with apache2 (installed: 2.4.57)
   Resolution options:
     sspm remove apache2 && sspm install nginx
     sspm install nginx --backend flatpak   (sandboxed, no conflict)
```

### Version conflicts

```
packageA requires: libfoo >= 2.0
packageB requires: libfoo < 2.0

❌ Version conflict: libfoo
   packageA requires: >= 2.0
   packageB requires: < 2.0
   No version can satisfy both constraints simultaneously.
```

### Breaks

```toml
# new-lib breaks old-app < 3.0
[conflicts]
breaks = ["old-app < 3.0"]
```

SSPM checks installed packages against `breaks` declarations before installing.

---

## Install Plan Output

```
sspm install nginx --dry-run

Resolving dependencies...

Install plan (6 packages):
  install  libc          2.38      [already satisfied]
  install  libpcre2      10.42     dependency of nginx
  install  libssl        3.1.4     dependency of nginx
  install  libzlib       1.3       dependency of nginx
  install  libgeoip      1.6.12    recommended by nginx
  install  nginx         1.25.3    requested

Total download: 2.4 MB
Disk space:     8.1 MB
Proceed? [Y/n]
```

---

## Reverse Dependency Check (remove)

Before removing a package, SSPM checks what depends on it:

```
sspm remove openssl

⚠️  The following packages depend on openssl:
  nginx 1.25.3
  certbot 2.7.4
  curl 8.4.0

Remove openssl and all dependents? [y/N]
Or: sspm remove openssl --cascade   (auto-remove dependents)
```

---

## Future: Full SAT Solver

The current solver is a correct CDCL-lite implementation. For complex scenarios with hundreds of mutual constraints (NixOS-style), a full SAT/CUDF-based solver is planned:

- **CUDF format** compatibility for interop with apt/opam solvers
- **Parallel constraint propagation** for large package sets
- **Optimal version selection** (not just first-satisfying)

Tracked in: `docs/ROADMAP.md`
