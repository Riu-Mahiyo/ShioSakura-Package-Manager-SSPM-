#include "sspm/index.h"
#include "sspm/log.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <stack>

namespace sspm {

void PackageIndex::clear() { 
    entries_.clear(); 
    search_cache_.clear();
    if (root_) { delete root_; root_ = nullptr; }
}

void PackageIndex::insert_trie(const std::string& name) {
    if (!root_) root_ = new TrieNode();
    TrieNode* cur = root_;
    for (char c : name) {
        c = std::tolower(c);
        if (!cur->children.count(c)) cur->children[c] = new TrieNode();
        cur = cur->children[c];
        // Only store if not too many
        if (cur->entries.size() < 100) cur->entries.push_back(name);
    }
}

std::vector<std::string> PackageIndex::search_trie(const std::string& prefix) const {
    if (!root_) return {};
    TrieNode* cur = root_;
    for (char c : prefix) {
        c = std::tolower(c);
        if (!cur->children.count(c)) return {};
        cur = cur->children[c];
    }
    return cur->entries;
}

// Load from a simple JSON index produced by repo sync
void PackageIndex::load(const std::string& json_path, const std::string& backend_name) {
    std::ifstream f(json_path);
    if (!f) { std::cerr << "[index] Cannot open: " << json_path << "\n"; return; }
    
    // Slightly more robust but still simple JSON-like parser
    std::string line;
    IndexEntry cur;
    cur.backend = backend_name;
    
    auto trim = [](std::string s) {
        s.erase(0, s.find_first_not_of(" \t\r\n\""));
        auto end = s.find_last_not_of(" \t\r\n\",}]");
        if (end != std::string::npos) s.erase(end + 1);
        return s;
    };

    while (std::getline(f, line)) {
        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            if (line.find('}') != std::string::npos && !cur.name.empty()) {
                entries_[cur.name] = cur;
                insert_trie(cur.name); // Optimization: Build index at load time
                cur = IndexEntry{};
                cur.backend = backend_name;
            }
            continue;
        }

        std::string key = trim(line.substr(0, colon));
        std::string val = line.substr(colon + 1);

        if (key == "name") cur.name = trim(val);
        else if (key == "version") cur.version = trim(val);
        else if (key == "description") cur.description = trim(val);
        else if (key == "depends") {
            // Handle depends array if it's on the same line like: "depends": ["a", "b"]
            size_t start = val.find('[');
            size_t end = val.find(']');
            if (start != std::string::npos && end != std::string::npos) {
                std::string arr = val.substr(start + 1, end - start - 1);
                std::stringstream ss(arr);
                std::string dep;
                while (std::getline(ss, dep, ',')) {
                    cur.depends.push_back(trim(dep));
                }
            }
        }
    }
}

std::optional<IndexEntry> PackageIndex::find(const std::string& name) const {
    auto it = entries_.find(name);
    if (it != entries_.end()) return it->second;
    return std::nullopt;
}

// Fuzzy score: rewards prefix match, substring match, and character match
int PackageIndex::fuzzy_score(const std::string& query, const IndexEntry& entry) const {
    std::string name = entry.name;
    std::string q = query;
    // Case-insensitive
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    if (name == q) return 1000;                               // exact
    if (name.substr(0, q.size()) == q) return 800;           // prefix
    if (name.find(q) != std::string::npos) return 600;       // substring in name
    // Substring in description
    std::string desc = entry.description;
    std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);
    if (desc.find(q) != std::string::npos) return 200;
    // Character-by-character (typo tolerance)
    int score = 0;
    size_t qi = 0;
    for (char c : name) {
        if (qi < q.size() && c == q[qi]) { score += 10; qi++; }
    }
    return (qi == q.size()) ? score : 0;
}

std::vector<IndexEntry> PackageIndex::search_fuzzy(const std::string& query, int max) const {
    auto it = search_cache_.find(query);
    if (it != search_cache_.end()) return it->second;

    std::vector<std::pair<int, IndexEntry>> scored;
    
    // Performance Optimization: Use Trie for prefix candidates first
    if (query.length() >= 2) {
        auto candidates = search_trie(query);
        for (const auto& name : candidates) {
            auto entry_it = entries_.find(name);
            if (entry_it != entries_.end()) {
                int s = fuzzy_score(query, entry_it->second);
                if (s > 0) scored.emplace_back(s, entry_it->second);
            }
        }
    }

    // If not enough results from Trie, do a full scan (fallback)
    if (scored.size() < (size_t)max) {
        for (auto& [name, entry] : entries_) {
            // Skip if already in scored (simple check for exact match)
            bool skip = false;
            for (auto& pair : scored) if (pair.second.name == name) { skip = true; break; }
            if (skip) continue;

            int s = fuzzy_score(query, entry);
            if (s > 0) scored.emplace_back(s, entry);
        }
    }

    std::sort(scored.begin(), scored.end(),
        [](auto& a, auto& b) { return a.first > b.first; });
    
    std::vector<IndexEntry> result;
    for (int i = 0; i < std::min(max, (int)scored.size()); i++)
        result.push_back(scored[i].second);
    
    search_cache_[query] = result;
    return result;
}

std::vector<IndexEntry> PackageIndex::search_regex(const std::string& pattern) const {
    std::vector<IndexEntry> result;
    try {
        std::regex re(pattern, std::regex::icase);
        for (auto& [name, entry] : entries_) {
            if (std::regex_search(name, re) || std::regex_search(entry.description, re))
                result.push_back(entry);
        }
    } catch (const std::regex_error& e) {
        std::cerr << "[index] Invalid regex: " << e.what() << "\n";
    }
    return result;
}

// Topological sort for dependency resolution (DFS)
std::vector<std::string> PackageIndex::resolve_deps(const std::string& pkg_name) const {
    std::vector<std::string> order;
    std::unordered_map<std::string, int> state; // 0: unvisited, 1: visiting, 2: visited

    std::function<void(const std::string&)> dfs = [&](const std::string& name) {
        if (state[name] == 2) return;
        if (state[name] == 1) {
            SSPM_WARN("Circular dependency detected involving: " + name);
            return;
        }

        state[name] = 1; // visiting
        auto it = entries_.find(name);
        if (it != entries_.end()) {
            for (auto& dep : it->second.depends)
                dfs(dep);
        }
        state[name] = 2; // visited
        order.push_back(name);
    };

    dfs(pkg_name);
    return order;
}

} // namespace sspm
