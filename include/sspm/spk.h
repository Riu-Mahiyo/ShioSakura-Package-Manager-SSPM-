#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace sspm::spk {

// ── SPK Package Format v2 (.spkg) ─────────────────────────────────────────────
//
// Archive structure (tar, no compression on outer — payload uses zstd internally):
//
//   package.spkg
//   ├── metadata.toml           Package identity, version, description
//   ├── control/
//   │   ├── depends.toml        Runtime/build/optional dependency declarations
//   │   ├── conflicts.toml      Conflict, replaces, breaks declarations
//   │   ├── provides.toml       Virtual package declarations
//   │   ├── triggers.toml       Post-install trigger hooks
//   │   └── checksums.sha256    sha256 of every installed file (for verify)
//   ├── payload.tar.zst         Installed files (mirrors filesystem layout)
//   ├── scripts/
//   │   ├── pre-install.sh
//   │   ├── post-install.sh
//   │   ├── pre-remove.sh
//   │   └── post-remove.sh
//   └── signature               ed25519 signature over (control/ + payload manifest)

struct SpkMetadata {
    // Identity
    std::string name;
    std::string version;           // semver: 1.2.3  or  epoch:version
    int         epoch = 0;         // epoch always beats version in comparisons
    std::string description;
    std::string long_description;
    std::string homepage;
    std::string license;

    // Maintainer
    std::string author;
    std::string maintainer;
    std::string email;

    // Build info
    std::vector<std::string> arch;  // ["x86_64","aarch64","riscv64","any"]
    std::string os;                 // "linux" | "macos" | "bsd" | "any"
    std::string build_date;
    std::string build_host;

    // Store classification (SSPM Center / KDE Discover / GNOME Software)
    std::string category;           // "tools" | "games" | "multimedia" | "devel" ...
    std::vector<std::string> tags;
};

struct SpkDepends {
    std::vector<std::string> required;       // ["libc >= 2.17", "zlib"]
    std::vector<std::string> build_requires; // build-time only
    std::vector<std::string> recommends;     // optional, installed if available
    std::vector<std::string> suggests;       // shown as suggestions, never auto-installed
};

struct SpkConflicts {
    std::vector<std::string> conflicts; // ["old-package", "legacy-lib < 2.0"]
    std::vector<std::string> replaces;  // files from these pkgs will be taken over
    std::vector<std::string> breaks;    // these pkgs will break if we are installed
};

struct SpkProvides {
    std::vector<std::string> provides;        // virtual packages: ["libjpeg", "mail-transport-agent"]
    std::vector<std::string> provides_files;  // specific files: ["/usr/bin/sh"]
};

struct SpkTrigger {
    std::string name;
    std::string watch_path;  // trigger fires when files under this path change
    std::string action;      // command to run (e.g. "ldconfig", "update-desktop-database")
};

struct SpkPackage {
    SpkMetadata  metadata;
    SpkDepends   depends;
    SpkConflicts conflicts;
    SpkProvides  provides;
    std::vector<SpkTrigger> triggers;

    std::string payload_path;   // extracted payload directory
    std::string checksums_path; // path to checksums.sha256
    std::string signature;      // raw ed25519 signature bytes

    // file path (relative to /) → sha256
    std::unordered_map<std::string, std::string> file_checksums;
};

// Parse a .spkg archive
SpkPackage parse(const std::string& spkg_path);

// Build .spkg from a structured source directory
bool build(const std::string& source_dir, const std::string& output_path);

// Extract payload.tar.zst to target_dir
bool extract(const std::string& spkg_path, const std::string& target_dir);

// Run a lifecycle hook (pre-install / post-install / pre-remove / post-remove)
bool run_hook(const std::string& scripts_dir, const std::string& hook_name);

// Generate control/checksums.sha256 from all files under payload/
bool generate_checksums(const std::string& payload_dir,
                        const std::string& output_path);

// Verify every installed file against stored checksums (for sspm verify)
bool verify_installed(const std::string& pkg_name,
                      const std::string& db_path);

} // namespace sspm::spk
