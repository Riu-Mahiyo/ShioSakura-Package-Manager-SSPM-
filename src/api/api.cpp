#include "sspm/api.h"
#include "sspm/database.h"
#include "sspm/plugin.h"
#include <iostream>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>
#include <thread>

#ifndef PLUGIN_DIR
#define PLUGIN_DIR "/usr/lib/sspm/plugins"
#endif

namespace sspm::api {

static std::string json_package(const Package& p) {
    return "{\"name\":\"" + p.name + "\",\"version\":\"" + p.version +
           "\",\"backend\":\"" + p.backend + "\",\"description\":\"" + p.description + "\"}";
}

static std::string json_array(const std::vector<std::string>& items) {
    std::string s = "[";
    for (size_t i = 0; i < items.size(); i++) {
        if (i) s += ",";
        s += "\"" + items[i] + "\"";
    }
    return s + "]";
}

Daemon::Daemon(int port) : port_(port) {
    setup_routes();
}

void Daemon::setup_routes() {
    // GET /health
    register_route("GET", "/health", [this](const Request&) -> Response {
        return { 200, "{\"status\":\"ok\",\"version\":\"4.0.0-Sakura\"}" };
    });

    // GET /metrics (New: Aggregate Module Metrics)
    register_route("GET", "/metrics", [this](const Request&) -> Response {
        plugin::PluginManager pm(PLUGIN_DIR);
        pm.load_all();
        return { 200, pm.aggregate_metrics() };
    });

    // GET /packages
    register_route("GET", "/packages", [](const Request&) -> Response {
        SkyDB db("~/.local/share/sspm/sky.db");
        if (!db.open()) return { 500, "{\"error\":\"db open failed\"}" };
        auto pkgs = db.get_installed();
        std::string body = "[";
        for (size_t i = 0; i < pkgs.size(); i++) {
            if (i) body += ",";
            body += json_package(pkgs[i]);
        }
        body += "]";
        return { 200, body };
    });

    // GET /packages/search?q=nginx
    register_route("GET", "/packages/search", [](const Request& req) -> Response {
        auto it = req.params.find("q");
        if (it == req.params.end()) return { 400, "{\"error\":\"missing q\"}" };
        // TODO: call Resolver::search_all(it->second)
        return { 200, "[]" };
    });

    // POST /packages/install  { "name": "nginx" }
    register_route("POST", "/packages/install", [](const Request& req) -> Response {
        // TODO: parse body JSON, call install pipeline
        std::cout << "[api] Install request: " << req.body << "\n";
        return { 200, "{\"status\":\"queued\"}" };
    });

    // DELETE /packages/:name
    register_route("DELETE", "/packages/", [](const Request& req) -> Response {
        std::string name = req.path.substr(std::string("/packages/").size());
        // TODO: call remove pipeline
        std::cout << "[api] Remove request: " << name << "\n";
        return { 200, "{\"status\":\"queued\"}" };
    });

    // GET /repos
    register_route("GET", "/repos", [](const Request&) -> Response {
        SkyDB db("~/.local/share/sspm/sky.db");
        if (!db.open()) return { 500, "{\"error\":\"db open failed\"}" };
        auto repos = db.get_repos();
        std::string body = "[";
        for (size_t i = 0; i < repos.size(); i++) {
            if (i) body += ",";
            body += "{\"name\":\"" + repos[i].first + "\",\"url\":\"" + repos[i].second + "\"}";
        }
        body += "]";
        return { 200, body };
    });
}

void Daemon::register_route(const std::string& method,
                             const std::string& path,
                             Handler handler) {
    routes_[method + " " + path] = handler;
}

Response Daemon::route(const Request& req) {
    // Exact match first
    auto key = req.method + " " + req.path;
    if (routes_.count(key)) return routes_[key](req);
    // Prefix match (for /packages/:name etc.)
    for (auto& [k, h] : routes_) {
        if (req.method + " " == k.substr(0, req.method.size() + 1)) {
            std::string route_path = k.substr(req.method.size() + 1);
            if (req.path.substr(0, route_path.size()) == route_path)
                return h(req);
        }
    }
    return { 404, "{\"error\":\"not found\"}" };
}

void Daemon::handle_request(int client_fd) {
    char buf[4096] = {};
    read(client_fd, buf, sizeof(buf) - 1);
    std::string raw(buf);

    // Parse HTTP request line
    Request req;
    std::istringstream ss(raw);
    std::string path_with_query;
    ss >> req.method >> path_with_query;

    // Split path and query string
    auto qpos = path_with_query.find('?');
    if (qpos != std::string::npos) {
        req.path = path_with_query.substr(0, qpos);
        std::string qs = path_with_query.substr(qpos + 1);
        // Parse key=value pairs
        std::istringstream qss(qs);
        std::string kv;
        while (std::getline(qss, kv, '&')) {
            auto eq = kv.find('=');
            if (eq != std::string::npos)
                req.params[kv.substr(0, eq)] = kv.substr(eq + 1);
        }
    } else {
        req.path = path_with_query;
    }

    // Find body (after double CRLF)
    auto body_pos = raw.find("\r\n\r\n");
    if (body_pos != std::string::npos) req.body = raw.substr(body_pos + 4);

    Response resp = route(req);

    std::string http =
        "HTTP/1.1 " + std::to_string(resp.status) + " OK\r\n"
        "Content-Type: " + resp.content_type + "\r\n"
        "Content-Length: " + std::to_string(resp.body.size()) + "\r\n"
        "Connection: close\r\n\r\n" +
        resp.body;

    write(client_fd, http.c_str(), http.size());
    close(client_fd);
}

void Daemon::start() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    running_ = true;
    std::cout << "[daemon] SSPM API listening on port " << port_ << "\n";

    while (running_) {
        int client = accept(server_fd, nullptr, nullptr);
        if (client < 0) continue;
        std::thread([this, client]() { handle_request(client); }).detach();
    }
    close(server_fd);
}

void Daemon::stop() { running_ = false; }
bool Daemon::is_running() const { return running_; }

} // namespace sspm::api
