#pragma once
#include "backend.h"
#include <string>
#include <vector>
#include <regex>
#include <unordered_map>

namespace sspm {

struct IndexEntry {
    std::string name;
    std::string version;
    std::string description;
    std::string backend;
    std::vector<std::string> depends;
    std::vector<std::string> provides; // for dependency graph
};

/**
 * @brief In-memory package index with fuzzy and regex search capabilities.
 * 
 * This class maintains a searchable index of all packages available across all enabled repositories.
 * It uses a Trie-based optimization for prefix-based fuzzy searching and a topological sort
 * for dependency resolution.
 */
class PackageIndex {
public:
    /**
     * @brief Loads package metadata from a JSON repository index.
     * @param json_path Path to the repo.json file.
     * @param backend_name The backend this repository belongs to.
     */
    void load(const std::string& json_path, const std::string& backend_name);
    
    /** @brief Clears all cached entries and indices. */
    void clear();

    /**
     * @brief Performs an exact name lookup.
     * @param name The package name to find.
     * @return The package entry if found, otherwise nullopt.
     */
    std::optional<IndexEntry> find(const std::string& name) const;

    /**
     * @brief Performs a fuzzy search across the index.
     * @param query The search query.
     * @param max The maximum number of results to return (default 20).
     * @return A sorted list of matching package entries.
     */
    std::vector<IndexEntry> search_fuzzy(const std::string& query, int max = 20) const;

    /**
     * @brief Performs a regular expression search.
     * @param pattern The regex pattern to match.
     * @return A list of matching package entries.
     */
    std::vector<IndexEntry> search_regex(const std::string& pattern) const;

    /**
     * @brief Resolves dependencies for a given package.
     * @param pkg_name The package to resolve for.
     * @return A list of package names in topological order (install order).
     * @note Detects and warns about circular dependencies.
     */
    std::vector<std::string> resolve_deps(const std::string& pkg_name) const;

    /** @return Total number of packages in the index. */
    size_t size() const { return entries_.size(); }

private:
    struct TrieNode {
        std::unordered_map<char, TrieNode*> children;
        std::vector<std::string> entries;
        ~TrieNode() { for (auto& p : children) delete p.second; }
    };

    std::unordered_map<std::string, IndexEntry> entries_;
    mutable std::unordered_map<std::string, std::vector<IndexEntry> > search_cache_;
    TrieNode* root_ = nullptr;

    void insert_trie(const std::string& name);
    std::vector<std::string> search_trie(const std::string& prefix) const;

    int fuzzy_score(const std::string& query, const IndexEntry& entry) const;
};

} // namespace sspm
