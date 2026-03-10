#pragma once
// ═══════════════════════════════════════════════════════════
//  plugin/plugin_api.h — 插件 ABI 定义（清单 2.1 + 3.1）
//
//  这是 ABI 冻结版本：v1
//  一旦发布，此接口不得破坏性修改
//
//  两类插件：
//    sky_layer_v1  — Layer 插件（包管理后端）
//    sky_plugin_v1 — 通用功能插件（多语言）
//
//  加载方式（Phase 3）：
//    dlopen("/usr/lib/sspm/layers/apt.so")
//    dlsym → sky_layer_entry
// ═══════════════════════════════════════════════════════════

#ifdef __cplusplus
extern "C" {
#endif

// ── Layer ABI（清单 2.1）─────────────────────────────────
#define SKY_LAYER_ABI_VERSION  1
#define SKY_PLUGIN_ABI_VERSION 1

// Layer 支持的平台标志
#define SKY_PLATFORM_LINUX   0x01
#define SKY_PLATFORM_MACOS   0x02
#define SKY_PLATFORM_FREEBSD 0x04
#define SKY_PLATFORM_ANY     0xFF

// Layer 支持的架构标志
#define SKY_ARCH_X86_64  0x01
#define SKY_ARCH_AARCH64 0x02
#define SKY_ARCH_ANY     0xFF

// Layer 特性标志
#define SKY_FEATURE_INSTALL  0x01
#define SKY_FEATURE_REMOVE   0x02
#define SKY_FEATURE_SEARCH   0x04
#define SKY_FEATURE_UPGRADE  0x08
#define SKY_FEATURE_VERSION  0x10
#define SKY_FEATURE_ALL      0xFF

// Layer 操作错误码（统一）
typedef enum {
    SKY_OK            = 0,
    SKY_ERR_NOT_FOUND = 1,   // 包不存在
    SKY_ERR_CONFLICT  = 2,   // 依赖冲突
    SKY_ERR_NETWORK   = 3,   // 网络错误
    SKY_ERR_AUTH      = 4,   // 权限不足
    SKY_ERR_DISK      = 5,   // 磁盘空间不足
    SKY_ERR_LOCKED    = 6,   // 包管理器被锁定
    SKY_ERR_TIMEOUT   = 7,   // 操作超时
    SKY_ERR_CORRUPT   = 8,   // 包文件损坏
    SKY_ERR_UNKNOWN   = 99,  // 未分类错误
} sky_error_t;

// Layer 操作结果
typedef struct {
    sky_error_t  error;
    int          exit_code;
    char*        output;      // heap-allocated, caller must free
    char*        error_msg;   // heap-allocated, caller must free
} sky_result_t;

// ── Layer 接口（清单 2.1）────────────────────────────────
typedef struct {
    // 元数据
    int         abi_version;       // 必须 == SKY_LAYER_ABI_VERSION
    const char* name;              // "apt", "pacman", ...
    const char* version;           // layer 版本号
    uint32_t    supported_platform;
    uint32_t    supported_arch;
    uint32_t    supported_features;

    // 生命周期
    int  (*init)   (void);
    void (*cleanup)(void);

    // 核心操作
    sky_result_t (*install)(const char** pkgs, int n);
    sky_result_t (*remove) (const char** pkgs, int n);
    sky_result_t (*upgrade)(void);
    sky_result_t (*search) (const char* query);
    const char*  (*version_of)(const char* pkg);  // 查询已安装版本

    // 可用性检测
    int (*available)(void);  // 1=可用 0=不可用

} sky_layer_v1_t;

// Layer .so 入口点（导出此符号）
// extern const sky_layer_v1_t sky_layer_entry;

// ── Plugin ABI（清单 3.1）────────────────────────────────
typedef struct {
    int         abi_version;    // 必须 == SKY_PLUGIN_ABI_VERSION
    const char* name;
    const char* version;
    const char* language;       // "c", "bash", "python", "node", "java", "go", "rust"

    int (*init)    (void);
    int (*execute) (const char* args_json);  // JSON 参数，JSON 返回
    int (*shutdown)(void);

} sky_plugin_v1_t;

// Plugin .so 入口点
// extern const sky_plugin_v1_t sky_plugin_entry;

#ifdef __cplusplus
}
#endif
