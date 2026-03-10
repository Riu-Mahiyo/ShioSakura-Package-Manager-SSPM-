// version_solver.cpp
// Responsible for: version string parsing, comparison, constraint satisfaction.
// This module is intentionally standalone — zero dependency on index or backend.

#include "sspm/dep_resolver.h"
#include <algorithm>
#include <sstream>

namespace sspm {

// ── Version component parser ──────────────────────────────────────────────────
// Handles: 1.2.3 | 1.2.3-4 | 1.2.3~beta | 2:1.0.0 (epoch:version)

struct VersionPart {
    int num = 0;
    std::string tag;

    bool is_tag() const { return !tag.empty(); }

    int compare(const VersionPart& other) const {
        if (num != other.num) return num < other.num ? -1 : 1;
        if (tag == other.tag) return 0;
        
        // Non-tag (release) is GREATER than any tag (pre-release)
        if (tag.empty()) return 1;
        if (other.tag.empty()) return -1;
        
        return tag < other.tag ? -1 : 1;
    }
};

static std::vector<VersionPart> parse_ver_parts(const std::string& v) {
    std::vector<VersionPart> parts;
    std::string cur;
    auto push_part = [&](bool is_num) {
        if (cur.empty()) return;
        VersionPart p;
        if (is_num) {
            try {
                p.num = std::stoi(cur);
            } catch (...) { p.num = 0; }
        } else {
            p.tag = cur;
        }
        parts.push_back(p);
        cur.clear();
    };

    bool parsing_num = true;
    for (char c : v) {
        if (c == '.' || c == '-' || c == '~' || c == ':') {
            push_part(parsing_num);
            parsing_num = true; // reset to numeric after separator
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            if (!parsing_num) { push_part(false); parsing_num = true; }
            cur += c;
        } else {
            if (parsing_num) { push_part(true); parsing_num = false; }
            cur += c;
        }
    }
    push_part(parsing_num);
    return parts;
}

// Returns: -1 if a < b,  0 if a == b,  +1 if a > b
// Epoch prefix (e.g. "1:2.0") always wins over plain version.
int DepResolver::compare_versions(const std::string& a, const std::string& b) {
    if (a == b) return 0;

    // Split epoch from version string
    auto split_epoch = [](const std::string& v) -> std::pair<int, std::string> {
        auto colon = v.find(':');
        if (colon != std::string::npos) {
            try {
                int epoch = std::stoi(v.substr(0, colon));
                return { epoch, v.substr(colon + 1) };
            } catch (...) {}
        }
        return { 0, v };
    };

    auto [ea, va] = split_epoch(a);
    auto [eb, vb] = split_epoch(b);

    if (ea != eb) return ea < eb ? -1 : 1;

    auto pa = parse_ver_parts(va);
    auto pb = parse_ver_parts(vb);
    size_t n = std::max(pa.size(), pb.size());
    for (size_t i = 0; i < n; i++) {
        VersionPart ia = (i < pa.size()) ? pa[i] : VersionPart{0, ""};
        VersionPart ib = (i < pb.size()) ? pb[i] : VersionPart{0, ""};
        int cmp = ia.compare(ib);
        if (cmp != 0) return cmp;
    }
    return 0;
}

// ── VersionConstraint ─────────────────────────────────────────────────────────

VersionConstraint VersionConstraint::parse(const std::string& expr) {
    VersionConstraint c;
    if (expr.empty() || expr == "*") return c;  // Any

    size_t i = 0;
    if      (expr.size() >= 2 && expr.substr(0,2) == ">=") { c.op = VersionOp::Gte; i = 2; }
    else if (expr.size() >= 2 && expr.substr(0,2) == "<=") { c.op = VersionOp::Lte; i = 2; }
    else if (expr.size() >= 2 && expr.substr(0,2) == "!=") { c.op = VersionOp::Ne;  i = 2; }
    else if (expr[0] == '>')  { c.op = VersionOp::Gt; i = 1; }
    else if (expr[0] == '<')  { c.op = VersionOp::Lt; i = 1; }
    else if (expr[0] == '=')  { c.op = VersionOp::Eq; i = 1; }
    else                      { c.op = VersionOp::Eq; }  // bare version → exact match

    while (i < expr.size() && expr[i] == ' ') i++;
    c.version = expr.substr(i);
    return c;
}

bool VersionConstraint::satisfied_by(const std::string& candidate) const {
    if (op == VersionOp::Any || version.empty()) return true;
    int cmp = DepResolver::compare_versions(candidate, version);
    switch (op) {
        case VersionOp::Eq:  return cmp == 0;
        case VersionOp::Gte: return cmp >= 0;
        case VersionOp::Gt:  return cmp > 0;
        case VersionOp::Lte: return cmp <= 0;
        case VersionOp::Lt:  return cmp < 0;
        case VersionOp::Ne:  return cmp != 0;
        default:             return true;
    }
}

std::string VersionConstraint::to_string() const {
    if (op == VersionOp::Any) return "*";
    static const char* ops[] = { "*", "=", ">=", ">", "<=", "<", "!=" };
    return std::string(ops[static_cast<int>(op)]) + version;
}

// ── Dependency expression parser ──────────────────────────────────────────────
// Formats: "libssl >= 3.0"  |  "zlib"  |  "?optional-lib >= 1.0"

Dependency Dependency::parse(const std::string& expr) {
    Dependency d;
    std::string e = expr;

    // Strip leading/trailing whitespace
    e.erase(0, e.find_first_not_of(" \t"));
    e.erase(e.find_last_not_of(" \t") + 1);

    if (!e.empty() && e[0] == '?') {
        d.optional = true;
        e = e.substr(1);
    }

    // Scan for the first version operator
    for (const char* op : { ">=", "<=", "!=", ">", "<", "=" }) {
        auto pos = e.find(op);
        if (pos != std::string::npos) {
            d.name = e.substr(0, pos);
            d.name.erase(d.name.find_last_not_of(" \t") + 1);
            std::string rest = e.substr(pos + strlen(op));
            rest.erase(0, rest.find_first_not_of(" \t"));
            d.constraint = VersionConstraint::parse(std::string(op) + rest);
            return d;
        }
    }
    d.name = e;
    return d;
}

} // namespace sspm
