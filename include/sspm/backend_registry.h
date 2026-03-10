#pragma once
#include "sspm/backend.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

namespace sspm {

namespace plugin { class PluginManager; }

// Describes how to detect a backend on the current system
struct BackendDescriptor {
    std::string name;
    std::string probe_binary;   // e.g. "/usr/bin/pacman"
    std::string probe_fallback; // secondary path, e.g. "/bin/pacman"
    std::string platform;       // "linux" | "macos" | "bsd" | "windows" | "universal"
    std::string family;         // "arch" | "debian" | "redhat" | etc.
    int default_priority;       // lower = higher priority when multiple match

    // Factory: creates the concrete Backend instance
    std::function<std::shared_ptr<Backend>()> factory;
};

// BackendRegistry: auto-detects which backends are present on the current system.
//
// Design:
//   - All 20 backends are registered at startup (O(1), no I/O)
//   - is_available() probes are lazy: only run when the backend is first used
//   - probe results are cached — no repeated filesystem hits
//   - On Arch (pacman present): pacman scores high, apt/dnf etc. are probed but
//     return false immediately and are never instantiated
//   - If a user later installs pacman on Debian, 'sspm doctor' will detect it
//
// Usage:
//   auto registry = BackendRegistry::create();
//   auto available = registry.available_backends();   // only probed+present ones
//   auto all       = registry.all_backends();         // all registered, lazy
class BackendRegistry {
public:
    BackendRegistry();

    // Register a backend descriptor (called once at startup per backend)
    void register_backend(BackendDescriptor desc);

    // Register a backend factory function
    using FactoryFn = std::function<std::shared_ptr<Backend>()>;
    static void register_factory(const std::string& name, FactoryFn factory);

    // Register an external backend instance (e.g. from a plugin)
    void register_instance(std::shared_ptr<Backend> instance);

    // Returns only backends whose probe binary exists on this system.
    // Results are cached after first call.
    std::vector<std::shared_ptr<Backend>> available_backends();

    // Returns ALL registered backends (probe not yet run).
    // Use this if you want to show the full list in 'sspm doctor'.
    std::vector<std::shared_ptr<Backend>> all_backends();

    // Look up a specific backend by name
    std::shared_ptr<Backend> get(const std::string& name);

    // Re-probe all backends (useful after the user installs a new tool)
    void refresh();

    // Pretty-print detection results for 'sspm doctor'
    void print_detection_report() const;

    // Performance: Async probing
    void probe_all_async();

    // OS/Env detection helpers
    static bool binary_exists(const std::string& path);
    static bool is_musl();
    static bool is_alpine();

    // test helpers - override detection results.  Only used by unit tests.
    // Setting either flag to true forces the corresponding predicate.
    static void set_test_force_alpine(bool);
    static void set_test_force_musl(bool);

    // Static factory: creates and populates the registry with all 20 backends
    static BackendRegistry create();

    // Load additional backends from plugins directory
    void load_plugins(const std::string& plugin_dir);

    // Integrate with PluginManager
    void register_from_plugins(const plugin::PluginManager& manager);

private:
    struct Entry {
        BackendDescriptor desc;
        std::shared_ptr<Backend> instance;
        bool probed    = false;
        bool available = false;
    };

    std::vector<Entry> entries_;
    bool cache_valid_ = false;

    bool probe(Entry& e);
};

} // namespace sspm
