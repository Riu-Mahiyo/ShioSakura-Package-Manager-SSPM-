#include "sspm/api.h"
#include "sspm/log.h"
#include <iostream>
#include <csignal>
#include <cstdlib>

// sspm-daemon: standalone REST API server
// Started by: sspm daemon start  (or directly: sspm-daemon)
// Listens on :7373 by default, configurable via SSPM_DAEMON_PORT env var

static sspm::api::Daemon* g_daemon = nullptr;

static void handle_signal(int sig) {
    std::cout << "\n[daemon] Received signal " << sig << ", shutting down...\n";
    if (g_daemon) g_daemon->stop();
    std::exit(0);
}

int main(int argc, char* argv[]) {
    // Port from env or argument
    int port = 7373;
    const char* env_port = std::getenv("SSPM_DAEMON_PORT");
    if (env_port) port = std::atoi(env_port);
    if (argc >= 3 && std::string(argv[1]) == "--port") port = std::atoi(argv[2]);

    // Init logger
    std::string log_path = std::string(getenv("HOME")) + "/.local/state/sspm/log/daemon.log";
    sspm::Logger::instance().init(log_path, sspm::LogLevel::Info);
    SSPM_INFO("sspm-daemon starting on port " + std::to_string(port));

    // Handle SIGTERM / SIGINT for clean shutdown
    std::signal(SIGTERM, handle_signal);
    std::signal(SIGINT,  handle_signal);

    sspm::api::Daemon daemon(port);
    g_daemon = &daemon;

    std::cout << "🌸 SSPM Daemon v4.0.0-Sakura\n";
    std::cout << "   Listening on http://localhost:" << port << "\n";
    std::cout << "   Log: " << log_path << "\n";
    std::cout << "   Press Ctrl+C to stop\n\n";

    daemon.start(); // blocking
    return 0;
}
