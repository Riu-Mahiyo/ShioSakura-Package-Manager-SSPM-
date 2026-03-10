#include "sspm/spk.h"
#include "sspm/security.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>

namespace fs = std::filesystem;

namespace sspm::spk {

// Helper to escape single quotes in shell arguments
static std::string quote(const std::string& s) {
    std::string res = "'";
    for (char c : s) {
        if (c == '\'') res += "'\\''";
        else res += c;
    }
    res += "'";
    return res;
}

// ── Minimal TOML line parser ──────────────────────────────────────────────────
// (replace with tomlplusplus or similar in production)

static std::string toml_get(const std::string& toml, const std::string& key) {
    std::istringstream ss(toml);
    std::string line;
    while (std::getline(ss, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\""));
            s.erase(s.find_last_not_of(" \t\"") + 1);
        };
        trim(k); trim(v);
        if (k == key) return v;
    }
    return "";
}

static std::vector<std::string> toml_get_array(const std::string& toml,
                                                const std::string& key) {
    std::vector<std::string> result;
    auto pos = toml.find(key + " =");
    if (pos == std::string::npos) pos = toml.find(key + "=");
    if (pos == std::string::npos) return result;

    auto start = toml.find('[', pos);
    auto end   = toml.find(']', start);
    if (start == std::string::npos || end == std::string::npos) return result;

    std::string arr = toml.substr(start + 1, end - start - 1);
    std::istringstream ss(arr);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item.erase(0, item.find_first_not_of(" \t\""));
        item.erase(item.find_last_not_of(" \t\"") + 1);
        if (!item.empty()) result.push_back(item);
    }
    return result;
}

static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

// ── Parse a .spkg archive ─────────────────────────────────────────────────────

SpkPackage parse(const std::string& spkg_path) {
    SpkPackage pkg;
    std::string tmp = "/tmp/sspm-spkg-" +
        std::to_string(std::hash<std::string>{}(spkg_path));
    fs::create_directories(tmp);

    // Extract the outer tar
    std::string extract_cmd = "tar -xf " + quote(spkg_path) + " -C " + quote(tmp) + " 2>/dev/null";
    if (std::system(extract_cmd.c_str()) != 0) {
        std::cerr << "[spk] Failed to extract: " << spkg_path << "\n";
        return pkg;
    }

    // ── metadata.toml ─────────────────────────────────────────────────────────
    std::string meta_toml = read_file(tmp + "/metadata.toml");
    pkg.metadata.name             = toml_get(meta_toml, "name");
    pkg.metadata.version          = toml_get(meta_toml, "version");
    pkg.metadata.description      = toml_get(meta_toml, "description");
    pkg.metadata.long_description = toml_get(meta_toml, "long_description");
    pkg.metadata.author           = toml_get(meta_toml, "author");
    pkg.metadata.maintainer       = toml_get(meta_toml, "maintainer");
    pkg.metadata.license          = toml_get(meta_toml, "license");
    pkg.metadata.homepage         = toml_get(meta_toml, "homepage");
    pkg.metadata.os               = toml_get(meta_toml, "os");
    pkg.metadata.category         = toml_get(meta_toml, "category");
    pkg.metadata.arch             = toml_get_array(meta_toml, "arch");
    pkg.metadata.tags             = toml_get_array(meta_toml, "tags");

    std::string epoch_str = toml_get(meta_toml, "epoch");
    if (!epoch_str.empty()) pkg.metadata.epoch = std::stoi(epoch_str);

    // ── control/depends.toml ──────────────────────────────────────────────────
    std::string dep_toml = read_file(tmp + "/control/depends.toml");
    pkg.depends.required       = toml_get_array(dep_toml, "requires");
    pkg.depends.build_requires = toml_get_array(dep_toml, "build_requires");
    pkg.depends.recommends     = toml_get_array(dep_toml, "recommends");
    pkg.depends.suggests       = toml_get_array(dep_toml, "suggests");

    // ── control/conflicts.toml ────────────────────────────────────────────────
    std::string conf_toml = read_file(tmp + "/control/conflicts.toml");
    pkg.conflicts.conflicts = toml_get_array(conf_toml, "conflicts");
    pkg.conflicts.replaces  = toml_get_array(conf_toml, "replaces");
    pkg.conflicts.breaks    = toml_get_array(conf_toml, "breaks");

    // ── control/provides.toml ─────────────────────────────────────────────────
    std::string prov_toml = read_file(tmp + "/control/provides.toml");
    pkg.provides.provides       = toml_get_array(prov_toml, "provides");
    pkg.provides.provides_files = toml_get_array(prov_toml, "provides_files");

    // ── control/checksums.sha256 ──────────────────────────────────────────────
    pkg.checksums_path = tmp + "/control/checksums.sha256";
    std::ifstream chk_f(pkg.checksums_path);
    std::string chk_line;
    while (std::getline(chk_f, chk_line)) {
        auto sp = chk_line.find("  ");
        if (sp != std::string::npos) {
            pkg.file_checksums[chk_line.substr(sp + 2)] = chk_line.substr(0, sp);
        }
    }

    // ── signature ─────────────────────────────────────────────────────────────
    std::string sig_path = tmp + "/signature";
    if (fs::exists(sig_path)) pkg.signature = read_file(sig_path);

    pkg.payload_path = tmp + "/payload.tar.zst";
    return pkg;
}

// ── Build a .spkg from source directory ──────────────────────────────────────
//
// Expected source layout:
//   source_dir/
//   ├── metadata.toml
//   ├── control/
//   │   ├── depends.toml
//   │   ├── conflicts.toml
//   │   └── provides.toml
//   ├── payload/            (files to install, mirroring /)
//   └── scripts/
//       ├── pre-install.sh
//       └── post-install.sh

bool build(const std::string& source_dir, const std::string& output_path) {
    if (!fs::exists(source_dir + "/metadata.toml")) {
        std::cerr << "[spk] Missing metadata.toml in: " << source_dir << "\n";
        return false;
    }

    // 1. Generate checksums for payload
    std::string payload_dir = source_dir + "/payload";
    std::string checksums_out = source_dir + "/control/checksums.sha256";
    fs::create_directories(source_dir + "/control");
    if (fs::exists(payload_dir)) {
        generate_checksums(payload_dir, checksums_out);
    }

    // 2. Compress payload → payload.tar.zst
    if (fs::exists(payload_dir)) {
        std::string compress_cmd =
            "tar -c -C " + quote(source_dir + "/payload") + " . | "
            "zstd -T0 -19 -o " + quote(source_dir + "/payload.tar.zst");
        if (std::system(compress_cmd.c_str()) != 0) {
            std::cerr << "[spk] Failed to compress payload\n";
            return false;
        }
    }

    // 3. Bundle into .spkg
    std::string bundle_cmd =
        "tar -cf " + quote(output_path) +
        " -C " + quote(source_dir) +
        " metadata.toml payload.tar.zst"
        " --add-file control/"     // control/ directory
        " --add-file scripts/ "    // scripts/ directory
        " 2>/dev/null";

    if (std::system(bundle_cmd.c_str()) != 0) {
        // Fallback: simple bundle
        std::string fallback = "tar -cf " + quote(output_path) + " -C " + quote(source_dir) + " .";
        if (std::system(fallback.c_str()) != 0) {
            std::cerr << "[spk] Failed to bundle package\n";
            return false;
        }
    }

    std::cout << "[spk] Built: " << output_path << "\n";
    return true;
}

// ── Extract payload to target directory ──────────────────────────────────────

bool extract(const std::string& spkg_path, const std::string& target_dir) {
    SpkPackage pkg = parse(spkg_path);
    if (pkg.payload_path.empty() || !fs::exists(pkg.payload_path)) {
        std::cerr << "[spk] No payload found in: " << spkg_path << "\n";
        return false;
    }
    fs::create_directories(target_dir);
    std::string cmd =
        "zstd -d " + quote(pkg.payload_path) + " --stdout 2>/dev/null | "
        "tar -x -C " + quote(target_dir);
    return std::system(cmd.c_str()) == 0;
}

// ── Run lifecycle hook ────────────────────────────────────────────────────────

bool run_hook(const std::string& scripts_dir, const std::string& hook_name) {
    std::string script = scripts_dir + "/" + hook_name + ".sh";
    if (!fs::exists(script)) return true;  // hook is optional

    std::cout << "[spk] Running hook: " << hook_name << "\n";
    std::string cmd = "bash " + quote(script);
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "[spk] Hook failed: " << hook_name << " (exit " << rc << ")\n";
        return false;
    }
    return true;
}

// ── Generate checksums ────────────────────────────────────────────────────────

bool generate_checksums(const std::string& payload_dir,
                        const std::string& output_path) {
    if (!fs::exists(payload_dir)) return false;

    std::ofstream out(output_path);
    if (!out) { std::cerr << "[spk] Cannot write checksums: " << output_path << "\n"; return false; }

    for (auto& entry : fs::recursive_directory_iterator(payload_dir)) {
        if (!entry.is_regular_file()) continue;
        std::string abs = entry.path().string();
        std::string rel = abs.substr(payload_dir.size());
        std::string hash = security::sha256(abs);
        if (!hash.empty()) out << hash << "  " << rel << "\n";
    }
    return true;
}

// ── Verify installed files ────────────────────────────────────────────────────

bool verify_installed(const std::string& pkg_name,
                      const std::string& db_path) {
    // TODO: load checksums from SkyDB (stored at install time)
    //       compare against current on-disk sha256
    std::cout << "[spk] Verifying: " << pkg_name << "\n";
    return true;
}

} // namespace sspm::spk
