# src/daemon/CMakeLists.txt
# Builds sspm-daemon: the standalone REST API server binary.
# Can be built independently: cmake --build build --target sspm-daemon

# sspm-daemon shares the core library but does NOT include the CLI layer.
# It exposes the REST API on :7373 for GUI frontends (SSPM Center, KDE Discover, etc.)

add_executable(sspm-daemon daemon_main.cpp)

target_include_directories(sspm-daemon PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Link against the sspm_core library (defined in parent CMakeLists)
target_link_libraries(sspm-daemon
    sspm_core
    sqlite3
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    ${CMAKE_DL_LIBS}
)

# Platform definitions
if(UNIX AND NOT APPLE)
    target_compile_definitions(sspm-daemon PRIVATE SSPM_LINUX)
elseif(APPLE)
    target_compile_definitions(sspm-daemon PRIVATE SSPM_MACOS)
elseif(WIN32)
    target_compile_definitions(sspm-daemon PRIVATE SSPM_WINDOWS)
endif()

install(TARGETS sspm-daemon
    RUNTIME DESTINATION bin
)

# Systemd service file (Linux only)
if(UNIX AND NOT APPLE)
    install(FILES ${PROJECT_SOURCE_DIR}/dist/sspm-daemon.service
        DESTINATION lib/systemd/system
        OPTIONAL
    )
endif()
