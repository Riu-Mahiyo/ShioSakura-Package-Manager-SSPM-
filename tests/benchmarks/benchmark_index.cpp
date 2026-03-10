#include "sspm/index.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>

using namespace sspm;
namespace fs = std::filesystem;

void create_mock_repo(const std::string& path, int count) {
    std::ofstream f(path);
    f << "[\n";
    for (int i = 0; i < count; i++) {
        f << "  {\n"
          << "    \"name\": \"pkg-" << i << "\",\n"
          << "    \"version\": \"1.0.0\",\n"
          << "    \"description\": \"Test package " << i << " with some long description to simulate real data.\"\n"
          << "  }";
        if (i < count - 1) f << ",";
        f << "\n";
    }
    f << "]\n";
}

int main() {
    const std::string mock_path = "/tmp/sspm-mock-repo.json";
    const int pkg_count = 50000;
    
    std::cout << "🌸 SSPM Performance Benchmark\n";
    std::cout << "══════════════════════════════════════════\n";
    
    std::cout << "[1/3] Generating " << pkg_count << " mock packages...\n";
    create_mock_repo(mock_path, pkg_count);
    
    PackageIndex index;
    auto start_load = std::chrono::high_resolution_clock::now();
    index.load(mock_path, "test-backend");
    auto end_load = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> load_time = end_load - start_load;
    std::cout << "✅ Load time: " << load_time.count() << " ms\n";
    
    // Benchmark 1: Fuzzy search with prefix (Optimized by Trie)
    std::cout << "[2/3] Benchmarking fuzzy search (prefix matching)...\n";
    auto start_search = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100; i++) {
        index.search_fuzzy("pkg-49"); // Should hit Trie
    }
    auto end_search = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> search_time = (end_search - start_search) / 100.0;
    std::cout << "✅ Average fuzzy search time: " << search_time.count() << " ms\n";
    
    // Benchmark 2: Dependency resolution
    std::cout << "[3/3] Benchmarking dependency resolution...\n";
    auto start_dep = std::chrono::high_resolution_clock::now();
    index.resolve_deps("pkg-0");
    auto end_dep = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> dep_time = end_dep - start_dep;
    std::cout << "✅ Dependency resolution time: " << dep_time.count() << " ms\n";
    
    std::cout << "══════════════════════════════════════════\n";
    std::cout << "Benchmark complete.\n";
    
    fs::remove(mock_path);
    return 0;
}
