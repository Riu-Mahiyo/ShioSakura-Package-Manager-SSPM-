#pragma once
#include <string>
#include <vector>
#include <unordered_map>

namespace sspm::security {

// ── Security policy ───────────────────────────────────────────────────────────
//
// Controlled via ~/.config/sspm/config.yaml:
//
//   security:
//     verify_signatures: true       # enforce ed25519 signature check
//     verify_checksums:  true       # enforce sha256 per-file checksum
//     allow_unsigned:    false      # allow packages with no signature at all
//     trusted_keys:                 # trusted public key fingerprints
//       - name: "sspm-official"
//         fingerprint: "abc123..."
//     untrusted_repos: []           # repos where verification is relaxed
//
// If verify_signatures: false  → skip signature check entirely
// If allow_unsigned: true      → warn but install packages with missing signatures
//
// Philosophy: security is ON by default. Users may disable it in config
// at their own risk. SSPM will always print a visible warning when
// signature verification is skipped.

struct SecurityPolicy {
    bool verify_signatures = true;
    bool verify_checksums  = true;
    bool allow_unsigned    = false;

    // Load policy from config.yaml
    static SecurityPolicy load(const std::string& config_path);

    // Returns true if this install should be blocked due to policy
    // (i.e. not signed and allow_unsigned is false)
    bool should_block_unsigned() const { return verify_signatures && !allow_unsigned; }
};

// ── Trusted key store ─────────────────────────────────────────────────────────
// Public keys stored in ~/.local/share/sspm/keys/
// Key filename convention: <name>.pub (raw ed25519 public key, 32 bytes)

struct TrustedKey {
    std::string name;
    std::string fingerprint;  // sha256 of raw key bytes (hex)
    std::string path;         // path to .pub file
};

// Load all trusted keys from the keys directory
std::vector<TrustedKey> load_trusted_keys(const std::string& keys_dir);

// Add a trusted key (e.g. from a repo's pubkey URL)
bool add_trusted_key(const std::string& keys_dir,
                     const std::string& name,
                     const std::string& pubkey_url_or_path);

// ── Permission & Sandbox ──────────────────────────────────────────────────────

// Sudo dropping: run command as unprivileged user
// Useful for builds and untrusted scripts
void drop_privileges();

// Restore original privileges (if possible)
void restore_privileges();

// ── ed25519 signature operations ─────────────────────────────────────────────
// Uses OpenSSL EVP (OpenSSL 1.1.1+)

// Sign a file and write the signature to sig_path
bool sign_file(const std::string& file_path,
               const std::string& sig_path,
               const std::string& privkey_path);

// Verify a file's ed25519 signature against ANY trusted key
// Returns true if at least one trusted key validates the signature.
bool verify_signature(const std::string& file_path,
                      const std::string& sig_path,
                      const std::vector<TrustedKey>& trusted_keys);

// Single key verification (exact key path)
bool verify_signature_key(const std::string& file_path,
                           const std::string& sig_path,
                           const std::string& pubkey_path);

// ── SHA-256 checksum ──────────────────────────────────────────────────────────

// Compute SHA-256 of a file, return hex string
std::string sha256(const std::string& file_path);

// Compute SHA-256 of in-memory data
std::string sha256_bytes(const std::vector<uint8_t>& data);

// Verify a file against an expected hex hash
bool verify_hash(const std::string& file_path, const std::string& expected_hash);

// ── .spkg verification pipeline ──────────────────────────────────────────────
// Runs the full verification sequence:
//   1. Check signature file exists
//   2. If policy.verify_signatures: verify signature against trusted keys
//   3. If policy.verify_checksums:  verify every payload file against checksums
//
// Returns VerifyResult so callers can decide based on policy.

enum class VerifyStatus {
    OK,             // all checks passed
    NO_SIGNATURE,   // signature file missing
    BAD_SIGNATURE,  // signature present but verification failed
    BAD_CHECKSUM,   // file(s) don't match checksums
    SKIPPED         // verification disabled in policy
};

struct VerifyResult {
    VerifyStatus status = VerifyStatus::OK;
    std::string  message;
    std::vector<std::string> failed_files;  // for BAD_CHECKSUM

    bool ok() const { return status == VerifyStatus::OK || status == VerifyStatus::SKIPPED; }
    bool is_unsigned() const { return status == VerifyStatus::NO_SIGNATURE; }
};

VerifyResult verify_spkg(const std::string& spkg_path,
                          const SecurityPolicy& policy,
                          const std::vector<TrustedKey>& trusted_keys);

// Legacy: single-call verify (uses default policy)
bool verify_spk(const std::string& spk_path);

// ── Key fingerprint ───────────────────────────────────────────────────────────

// Returns the SHA-256 fingerprint of a public key file (for display/comparison)
std::string key_fingerprint(const std::string& pubkey_path);

} // namespace sspm::security
