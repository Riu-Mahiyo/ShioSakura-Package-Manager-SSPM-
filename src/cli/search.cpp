#include "sspm/cli.h"
#include "sspm/index.h"
#include <iostream>
#include <iomanip>

namespace sspm::cli {

void cmd_search(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: sspm search <query> [--json] [--regex] [--backend <n>]\n";
        return;
    }

    std::string query;
    bool json_out  = false;
    bool use_regex = false;
    std::string filter_backend;

    for (size_t i = 0; i < args.size(); i++) {
        if      (args[i] == "--json")    json_out = true;
        else if (args[i] == "--regex")   use_regex = true;
        else if (args[i] == "--backend" && i+1 < args.size()) filter_backend = args[++i];
        else if (args[i][0] != '-')      query = args[i];
    }

    if (query.empty()) { std::cerr << "sspm search: no query given\n"; return; }

    // Load index from cache and search
    PackageIndex index;
    std::string index_path = CACHE_PATH + "/repos/official.json";
    index.load(index_path, "sspm");  // TODO: load all backend indexes

    std::vector<IndexEntry> results;
    if (use_regex)
        results = index.search_regex(query);
    else
        results = index.search_fuzzy(query, 20);

    if (!filter_backend.empty()) {
        std::vector<IndexEntry> filtered;
        for (auto& r : results)
            if (r.backend == filter_backend) filtered.push_back(r);
        results = filtered;
    }

    if (results.empty()) {
        std::cout << "No packages found for: " << query << "\n";
        return;
    }

    if (json_out) {
        std::cout << "[\n";
        for (size_t i = 0; i < results.size(); i++) {
            std::cout << "  {\"name\":\"" << results[i].name
                      << "\",\"version\":\"" << results[i].version
                      << "\",\"description\":\"" << results[i].description
                      << "\",\"backend\":\"" << results[i].backend << "\"}";
            if (i+1 < results.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]\n";
    } else {
        std::cout << std::left
                  << std::setw(28) << "Name"
                  << std::setw(12) << "Version"
                  << std::setw(12) << "Backend"
                  << "Description\n"
                  << std::string(80, '-') << "\n";
        for (auto& r : results) {
            std::cout << std::setw(28) << r.name
                      << std::setw(12) << r.version
                      << std::setw(12) << r.backend
                      << r.description << "\n";
        }
        std::cout << results.size() << " result(s)\n";
    }
}

} // namespace sspm::cli
