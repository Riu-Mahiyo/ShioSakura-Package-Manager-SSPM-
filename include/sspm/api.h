#pragma once
#include <string>
#include <functional>
#include <map>

namespace sspm::api {

struct Request {
    std::string method;   // GET POST DELETE
    std::string path;
    std::string body;
    std::map<std::string, std::string> params;
};

struct Response {
    int status = 200;
    std::string body;
    std::string content_type = "application/json";
};

using Handler = std::function<Response(const Request&)>;

// Lightweight HTTP daemon for SSPM REST API
// Enables GUI frontends and web dashboards via:
//   GET  /packages
//   GET  /packages/search?q=nginx
//   POST /packages/install   { "name": "nginx" }
//   DELETE /packages/:name
//   GET  /repos
//   POST /repos              { "name": "...", "url": "..." }
//   GET  /health
class Daemon {
public:
    explicit Daemon(int port = 7373);

    void start();   // sspm daemon start
    void stop();    // sspm daemon stop
    bool is_running() const;

    void register_route(const std::string& method,
                        const std::string& path,
                        Handler handler);

private:
    int port_;
    bool running_ = false;
    std::map<std::string, Handler> routes_;

    void setup_routes();
    void handle_request(int client_fd);
    Response route(const Request& req);
};

} // namespace sspm::api
