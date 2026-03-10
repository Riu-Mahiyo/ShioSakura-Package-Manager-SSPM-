cmake_minimum_required(VERSION 3.20)
project(sspm VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SSPM_BUILD_TESTS  "Build unit tests"        ON)
option(SSPM_BUILD_DAEMON "Build standalone daemon" ON)

# ── Dependencies ──────────────────────────────────────────────────────────────
find_package(SQLite3 REQUIRED)
find_package(CURL    REQUIRED)
find_package(OpenSSL REQUIRED)

# ── Backend sources ───────────────────────────────────────────────────────────
set(SSPM_BACKEND_SOURCES
    # Linux
    src/backends/apt/apt_backend.cpp
    src/backends/pacman/pacman_backend.cpp
    src/backends/dnf/dnf_backend.cpp
    src/backends/zypper/zypper_backend.cpp
    src/backends/portage/portage_backend.cpp
    src/backends/apk/apk_backend.cpp
    src/backends/xbps/xbps_backend.cpp
    src/backends/nix/nix_backend.cpp
    src/backends/dpkg/dpkg_backend.cpp
    src/backends/aur/aur_backend.cpp
    src/backends/run/run_backend.cpp
    # BSD
    src/backends/pkg/pkg_backend.cpp
    src/backends/pkgin/pkgin_backend.cpp
    src/backends/ports/ports_backend.cpp
    # macOS
    src/backends/brew/brew_backend.cpp
    src/backends/macports/macports_backend.cpp
    # Windows
    src/backends/winget/winget_backend.cpp
    src/backends/scoop/scoop_backend.cpp
    src/backends/chocolatey/chocolatey_backend.cpp
    # Universal
    src/backends/flatpak/flatpak_backend.cpp
    src/backends/snap/snap_backend.cpp
    src/backends/appimage/appimage_backend.cpp
    src/backends/spk/spk_backend.cpp
    src/backends/npm/npm_backend.cpp
    src/backends/pip/pip_backend.cpp
    src/backends/cargo/cargo_backend.cpp
    src/backends/gem/gem_backend.cpp
    src/backends/wine/wine_backend.cpp
    src/backends/amber/amber_backend.cpp
)

# ── sspm_core: shared static library used by sspm + sspm-daemon ──────────────
# This avoids duplicating compilation between the two binaries.
add_library(sspm_core STATIC
    src/core/exec.cpp
    src/core/validator.cpp
    src/core/config_manager.cpp
    src/resolver/resolver.cpp
    src/resolver/dep_resolver.cpp
    src/resolver/dep_graph.cpp
    src/resolver/version_solver.cpp
    src/resolver/conflict_solver.cpp
    src/transaction/transaction.cpp
    src/database/database.cpp
    src/repo/repo.cpp
    src/cache/cache.cpp
    src/profile/profile.cpp
    src/mirror/mirror.cpp
    src/security/security.cpp
    src/spk/spk.cpp
    src/network/network.cpp
    src/log/log.cpp
    src/plugin/plugin.cpp
    src/doctor/doctor.cpp
    src/index/index.cpp
    src/api/api.cpp
    src/backend_registry/backend_registry.cpp
    src/i18n/i18n.cpp
    src/desktop/desktop_integration.cpp
    ${SSPM_BACKEND_SOURCES}
)

target_include_directories(sspm_core PUBLIC include)
target_link_libraries(sspm_core
    sqlite3
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CMAKE_DL_LIBS}
)

# ── Platform definitions ──────────────────────────────────────────────────────
if(UNIX AND NOT APPLE)
    target_compile_definitions(sspm_core PUBLIC SSPM_LINUX)
elseif(APPLE)
    target_compile_definitions(sspm_core PUBLIC SSPM_MACOS)
elseif(WIN32)
    target_compile_definitions(sspm_core PUBLIC SSPM_WINDOWS)
endif()

# ── Benchmarks ──────────────────────────────────────────────────────────────
if(SSPM_BUILD_TESTS)
    add_executable(benchmark_index tests/benchmarks/benchmark_index.cpp)
    target_link_libraries(benchmark_index PRIVATE sspm_core)
endif()

# ── Binaries ──────────────────────────────────────────────────────────────────
add_executable(sspm
    src/cli/main.cpp
    src/cli/commands.cpp
)
target_link_libraries(sspm sspm_core)

# ── sspm-daemon: standalone REST API daemon ───────────────────────────────────
if(SSPM_BUILD_DAEMON)
    add_subdirectory(src/daemon)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS sspm RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

# ── Tests ─────────────────────────────────────────────────────────────────────
if(SSPM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
