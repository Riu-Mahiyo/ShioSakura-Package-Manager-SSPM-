#include "sspm/security.h"
#include "sspm/log.h"
#include "sspm/network.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/pem.h>

namespace fs = std::filesystem;

namespace sspm::security {

// ── SecurityPolicy::load ──────────────────────────────────────────────────────
// Reads the [security] section from config.yaml.
// Uses simple line-by-line YAML parse (full YAML parser is a future dependency).

SecurityPolicy SecurityPolicy::load(const std::string& config_path) {
    SecurityPolicy policy;  // all defaults: strict on
    if (!fs::exists(config_path)) return policy;

    std::ifstream f(config_path);
    std::string line;
    bool in_security = false;

    while (std::getline(f, line)) {
        // Strip comment
        auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);

        if (line.find("security:") != std::string::npos) {
            in_security = true;
            continue;
        }
        if (in_security && !line.empty() && line[0] != ' ' && line[0] != '\t') {
            in_security = false;  // left the security block
        }
        if (!in_security) continue;

        auto trim_kv = [](const std::string& s) -> std::pair<std::string,std::string> {
            auto eq = s.find(':');
            if (eq == std::string::npos) return {"",""};
            std::string k = s.substr(0, eq);
            std::string v = s.substr(eq + 1);
            auto trim = [](std::string& x) {
                x.erase(0, x.find_first_not_of(" \t"));
                x.erase(x.find_last_not_of(" \t\r") + 1);
            };
            trim(k); trim(v);
            return {k, v};
        };

        auto [k, v] = trim_kv(line);
        auto is_true = [](const std::string& s) {
            return s == "true" || s == "yes" || s == "1";
        };

        if (k == "verify_signatures") policy.verify_signatures = is_true(v);
        else if (k == "verify_checksums") policy.verify_checksums = is_true(v);
        else if (k == "allow_unsigned")   policy.allow_unsigned   = is_true(v);
    }

    // Emit a warning if security has been weakened
    if (!policy.verify_signatures) {
        std::cerr << "\n⚠️  WARNING: Signature verification is DISABLED in config.\n"
                  << "   Packages will be installed without cryptographic verification.\n"
                  << "   This is insecure. Enable with: sspm config set security.verify_signatures true\n\n";
    }
    if (policy.allow_unsigned) {
        std::cerr << "⚠️  WARNING: allow_unsigned = true — unsigned packages will be installed.\n\n";
    }

    return policy;
}

// ── Trusted keys ──────────────────────────────────────────────────────────────

std::vector<TrustedKey> load_trusted_keys(const std::string& keys_dir) {
    std::vector<TrustedKey> keys;
    if (!fs::exists(keys_dir)) return keys;

    for (auto& entry : fs::directory_iterator(keys_dir)) {
        if (entry.path().extension() != ".pub") continue;
        TrustedKey k;
        k.path        = entry.path().string();
        k.name        = entry.path().stem().string();
        k.fingerprint = key_fingerprint(k.path);
        keys.push_back(k);
        SSPM_DEBUG("trusted key loaded: " + k.name + " (" + k.fingerprint.substr(0, 16) + "...)");
    }
    return keys;
}

bool add_trusted_key(const std::string& keys_dir,
                     const std::string& name,
                     const std::string& source) {
    fs::create_directories(keys_dir);
    std::string dest = keys_dir + "/" + name + ".pub";

    // If source is a URL, download it first
    if (source.substr(0, 4) == "http") {
        network::DownloadRequest req;
        req.url = source;
        req.dest_path = dest;
        auto res = network::download(req);
        if (!res.success) {
            std::cerr << "[security] Failed to download key from: " << source 
                      << " (" << res.error << ")\n";
            return false;
        }
    } else {
        // Local file — copy to keys dir
        std::error_code ec;
        fs::copy_file(source, dest, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            std::cerr << "[security] Failed to copy key: " << ec.message() << "\n";
            return false;
        }
    }

    std::string fp = key_fingerprint(dest);
    std::cout << "[security] Trusted key added: " << name
              << " (fingerprint: " << fp.substr(0, 32) << "...)\n";
    return true;
}

// ── Permission & Sandbox ──────────────────────────────────────────────────────

static uid_t original_uid = 0;
static gid_t original_gid = 0;

void drop_privileges() {
    if (getuid() != 0) return; // Not running as root

    const char* sudo_user = getenv("SUDO_USER");
    if (!sudo_user) return;

    struct passwd* pw = getpwnam(sudo_user);
    if (pw) {
        original_uid = getuid();
        original_gid = getgid();
        
        if (setgid(pw->pw_gid) != 0) SSPM_ERROR("[security] Failed to drop GID");
        if (setuid(pw->pw_uid) != 0) SSPM_ERROR("[security] Failed to drop UID");
        
        SSPM_DEBUG("Dropped privileges to user: " + std::string(sudo_user));
    }
}

void restore_privileges() {
    if (original_uid == 0) return;
    
    if (setreuid(original_uid, original_uid) != 0) SSPM_ERROR("[security] Failed to restore UID");
    if (setregid(original_gid, original_gid) != 0) SSPM_ERROR("[security] Failed to restore GID");
    
    SSPM_DEBUG("Restored root privileges");
}

// ── SHA-256 ───────────────────────────────────────────────────────────────────

std::string sha256(const std::string& file_path) {
    std::ifstream f(file_path, std::ios::binary);
    if (!f) return "";

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);

    char buf[65536];
    while (f.read(buf, sizeof(buf)) || f.gcount()) {
        EVP_DigestUpdate(ctx, buf, f.gcount());
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  len = 0;
    EVP_DigestFinal_ex(ctx, hash, &len);
    EVP_MD_CTX_free(ctx);

    std::ostringstream ss;
    for (unsigned int i = 0; i < len; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

std::string sha256_bytes(const std::vector<uint8_t>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    std::ostringstream ss;
    for (auto b : hash)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
    return ss.str();
}

bool verify_hash(const std::string& file_path, const std::string& expected) {
    return sha256(file_path) == expected;
}

// ── ed25519 via OpenSSL EVP ───────────────────────────────────────────────────

bool sign_file(const std::string& file_path,
               const std::string& sig_path,
               const std::string& privkey_path) {
    // Load private key
    FILE* kf = fopen(privkey_path.c_str(), "r");
    if (!kf) { std::cerr << "[security] Cannot open private key: " << privkey_path << "\n"; return false; }
    EVP_PKEY* pkey = PEM_read_PrivateKey(kf, nullptr, nullptr, nullptr);
    fclose(kf);
    if (!pkey) { std::cerr << "[security] Failed to load private key\n"; return false; }

    // Read file to sign
    std::ifstream f(file_path, std::ios::binary);
    if (!f) { EVP_PKEY_free(pkey); return false; }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), {});

    // Sign
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    size_t sig_len = 0;
    EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey);
    EVP_DigestSign(ctx, nullptr, &sig_len, data.data(), data.size());
    std::vector<uint8_t> sig(sig_len);
    EVP_DigestSign(ctx, sig.data(), &sig_len, data.data(), data.size());
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    // Write signature
    std::ofstream sf(sig_path, std::ios::binary);
    sf.write(reinterpret_cast<char*>(sig.data()), sig_len);
    return true;
}

bool verify_signature_key(const std::string& file_path,
                            const std::string& sig_path,
                            const std::string& pubkey_path) {
    // Load public key
    FILE* kf = fopen(pubkey_path.c_str(), "r");
    if (!kf) return false;
    EVP_PKEY* pkey = PEM_read_PUBKEY(kf, nullptr, nullptr, nullptr);
    fclose(kf);
    if (!pkey) return false;

    // Read file
    std::ifstream f(file_path, std::ios::binary);
    if (!f) { EVP_PKEY_free(pkey); return false; }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), {});

    // Read signature
    std::ifstream sf(sig_path, std::ios::binary);
    if (!sf) { EVP_PKEY_free(pkey); return false; }
    std::vector<uint8_t> sig((std::istreambuf_iterator<char>(sf)), {});

    // Verify
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_DigestVerifyInit(ctx, nullptr, nullptr, nullptr, pkey);
    int rc = EVP_DigestVerify(ctx, sig.data(), sig.size(), data.data(), data.size());
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);

    return rc == 1;
}

bool verify_signature(const std::string& file_path,
                      const std::string& sig_path,
                      const std::vector<TrustedKey>& trusted_keys) {
    for (auto& key : trusted_keys) {
        if (verify_signature_key(file_path, sig_path, key.path)) {
            SSPM_DEBUG("signature valid: " + file_path + " (key: " + key.name + ")");
            return true;
        }
    }
    return false;
}

std::string key_fingerprint(const std::string& pubkey_path) {
    return sha256(pubkey_path).substr(0, 64);
}

// ── .spkg verification pipeline ──────────────────────────────────────────────

VerifyResult verify_spkg(const std::string& spkg_path,
                          const SecurityPolicy& policy,
                          const std::vector<TrustedKey>& trusted_keys) {
    // If verification is fully disabled in config, skip all checks
    if (!policy.verify_signatures && !policy.verify_checksums) {
        return { VerifyStatus::SKIPPED, "Verification disabled in config", {} };
    }

    // Extract to temp dir for inspection
    std::string tmp = "/tmp/sspm-verify-" +
        std::to_string(std::hash<std::string>{}(spkg_path));
    fs::create_directories(tmp);
    if (std::system(("tar -xf " + spkg_path + " -C " + tmp + " 2>/dev/null").c_str()) != 0) {
        return { VerifyStatus::BAD_SIGNATURE, "Failed to extract package for verification", {} };
    }

    // ── Step 1: Signature check ───────────────────────────────────────────────
    if (policy.verify_signatures) {
        std::string sig_path = tmp + "/signature";

        if (!fs::exists(sig_path)) {
            if (policy.allow_unsigned) {
                std::cerr << "⚠️  Package has no signature: " << spkg_path
                          << "\n   Installing anyway (allow_unsigned = true in config)\n";
                // Proceed to checksum check
            } else {
                return {
                    VerifyStatus::NO_SIGNATURE,
                    "Package has no signature file.\n"
                    "   To install unsigned packages: sspm config set security.allow_unsigned true\n"
                    "   WARNING: This is insecure and you do it at your own risk. 💀",
                    {}
                };
            }
        } else {
            // Build a manifest to sign (control/ directory contents)
            std::string manifest_path = tmp + "/control/checksums.sha256";
            if (!fs::exists(manifest_path)) {
                // Fall back to signing the whole archive
                manifest_path = spkg_path;
            }

            if (!verify_signature(manifest_path, sig_path, trusted_keys)) {
                if (policy.allow_unsigned) {
                    std::cerr << "⚠️  Signature INVALID for: " << spkg_path
                              << "\n   Installing anyway (allow_unsigned = true)\n";
                } else {
                    return {
                        VerifyStatus::BAD_SIGNATURE,
                        "Signature verification FAILED for: " + spkg_path + "\n"
                        "   The package may be corrupted or tampered with.",
                        {}
                    };
                }
            } else {
                std::cout << "✅ Signature verified: " << fs::path(spkg_path).filename().string() << "\n";
            }
        }
    }

    // ── Step 2: Per-file checksum verification ────────────────────────────────
    if (policy.verify_checksums) {
        std::string checksums_path = tmp + "/control/checksums.sha256";
        if (!fs::exists(checksums_path)) {
            // No checksums — can only warn, not block (checksums are optional in v1 compat mode)
            SSPM_WARN("No checksums.sha256 in package: " + spkg_path);
        } else {
            // Extract payload for checksum verification
            std::string payload_dir = tmp + "/payload";
            std::system(("zstd -d " + tmp + "/payload.tar.zst --stdout 2>/dev/null | "
                         "tar -x -C " + payload_dir + " 2>/dev/null").c_str());

            std::ifstream cf(checksums_path);
            std::string line;
            std::vector<std::string> failed;

            while (std::getline(cf, line)) {
                auto sp = line.find("  ");
                if (sp == std::string::npos) continue;
                std::string expected_hash = line.substr(0, sp);
                std::string rel_path      = line.substr(sp + 2);
                std::string abs_path      = payload_dir + rel_path;

                if (!fs::exists(abs_path)) {
                    failed.push_back(rel_path + " (missing)");
                    continue;
                }
                if (sha256(abs_path) != expected_hash) {
                    failed.push_back(rel_path + " (hash mismatch)");
                }
            }

            if (!failed.empty()) {
                return {
                    VerifyStatus::BAD_CHECKSUM,
                    "Checksum verification failed for " + std::to_string(failed.size()) + " file(s)",
                    failed
                };
            }
            std::cout << "✅ Checksums verified: " << fs::path(spkg_path).filename().string() << "\n";
        }
    }

    // Cleanup temp dir
    fs::remove_all(tmp);
    return { VerifyStatus::OK, "Verification passed", {} };
}

// Legacy shim for code that calls verify_spk() before policy loading
bool verify_spk(const std::string& spk_path) {
    SecurityPolicy policy;  // defaults: strict
    // No trusted keys available in legacy call — only checksum verify
    auto result = verify_spkg(spk_path, policy, {});
    if (!result.ok()) {
        std::cerr << "[security] " << result.message << "\n";
        for (auto& f : result.failed_files)
            std::cerr << "  failed: " << f << "\n";
    }
    return result.ok();
}

} // namespace sspm::security
