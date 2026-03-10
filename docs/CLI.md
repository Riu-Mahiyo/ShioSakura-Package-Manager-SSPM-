# SSPM CLI Reference

## Basic Commands

### `sspm install <package>`
Install a package using the best available backend.

> **Note:** the `apk` backend only participates on Alpine Linux.  On other
> distributions APK-related commands are disabled and produce a clear error.

```bash
sspm install nginx
sspm install nginx --backend flatpak    # Force specific backend
sspm install nginx --dry-run            # Preview, no changes
sspm install nginx --verbose
```

### `sspm remove <package>`
Remove an installed package.

```bash
sspm remove nginx
sspm remove nginx --purge               # Also remove config files
```

### `sspm search <query>`
Search for packages across all backends.

```bash
sspm search nginx
sspm search nginx --json                # JSON output
sspm search "ngin.*" --regex            # Regex search
sspm search nodejs --backend npm        # Search specific backend
```

### `sspm update`
Sync package databases from all configured repos and backends.

```bash
sspm update
```

### `sspm upgrade`
Upgrade all installed packages.

```bash
sspm upgrade
sspm upgrade nginx                      # Upgrade specific package
```

### `sspm list`
List installed packages.

```bash
sspm list
sspm list --format table
sspm list --format json
sspm list --backend flatpak             # Filter by backend
```

### `sspm info <package>`
Show detailed package info.

```bash
sspm info nginx
sspm info nginx --json
```

### `sspm doctor`
Run system diagnostics.

```bash
sspm doctor
```

---

## Repo Commands

```bash
sspm repo add official https://repo.sspm.dev
sspm repo add third-party https://example.com/sspm
sspm repo remove third-party
sspm repo sync
sspm repo list
```

---

## Cache Commands

```bash
sspm cache size           # Show cache disk usage
sspm cache clean          # Remove all cached files
sspm cache prune          # Remove files older than 30 days
```

---

## Config Commands

```bash
sspm config set backend_priority "pacman,flatpak,nix"
sspm config set cli.style pacman
sspm config set mirror.preferred_region JP
sspm config get backend_priority
```

---

## Profile Commands

```bash
sspm profile create dev
sspm profile create gaming
sspm profile apply dev
sspm profile list
sspm profile delete gaming
```

---

## Mirror Commands

```bash
sspm mirror list             # List all mirrors with latency
sspm mirror test             # Benchmark all mirrors
sspm mirror select archlinux-jp
sspm mirror auto             # Re-run geo auto-detection
```

---

## History & Rollback

```bash
sspm history                 # Show install/remove history
sspm rollback                # Roll back last transaction
sspm verify nginx            # Verify installed package integrity
sspm verify package.spk      # Verify a local .spk file
```

---

## Log Commands

```bash
sspm log                     # Show last 50 log lines
sspm log tail                # Follow live log output (Ctrl+C to stop)
```

---

## Daemon Commands

```bash
sspm daemon start            # Start REST API daemon on :7373
sspm daemon stop
sspm daemon status
```

---

## Plugin Commands

```bash
sspm plugin list
sspm plugin install aur
sspm plugin remove aur
```

---

## Build Commands

```bash
sspm build ./mypackage/      # Build a .spk from source directory
```

---

## Command Style Switching

SSPM supports multiple command style conventions:

### pacman style
```bash
sspm config set cli.style pacman

sspm -S nginx         # install
sspm -Rs nginx        # remove
sspm -Ss nginx        # search
sspm -Si nginx        # info
sspm -Syu             # update + upgrade
sspm -Q               # list installed
```

### apt style (default)
```bash
sspm config set cli.style apt
```

### brew style
```bash
sspm config set cli.style brew
```

---

## Global Flags

| Flag | Description |
|------|-------------|
| `--json` | Output as JSON |
| `--verbose` | Verbose output |
| `--dry-run` | Preview without executing |
| `--backend <n>` | Force a specific backend |
| `--no-color` | Disable colored output |
| `--quiet` | Suppress non-error output |
