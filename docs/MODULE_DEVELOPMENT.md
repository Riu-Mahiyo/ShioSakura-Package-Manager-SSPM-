# 🌸 SSPM 模块与插件开发手册 (v4.0.0-Sakura)

SSPM (ShioSakura Package Manager) 采用高度模块化的架构。通过本手册，您可以学习如何为系统开发自定义后端、挂钩（Hooks）或功能扩展模块。

## 1. 核心架构概述

SSPM 的模块化基于 `IModule` 接口。每个模块通过动态链接库（`.so` / `.dylib` / `.dll`）的形式存在，并由 `PluginManager` 在运行时动态加载。

### 模块生命周期
1. **加载 (Load)**: 加载动态库，调用 `create_module()` 工厂函数。
2. **初始化 (Initialize)**: 传入系统上下文配置。
3. **运行 (Running)**: 模块执行其特定任务（如包管理操作）。
4. **停机 (Shutdown)**: 执行清理工作。
5. **卸载 (Unload)**: 释放资源，调用 `destroy_module()`。

## 2. 快速入门：Hello World 模块

创建一个名为 `hello_module.cpp` 的文件：

```cpp
#include "sspm/sspm-sdk.h"
#include <iostream>

class HelloModule : public sspm::IModule {
public:
    std::string id() const override { return "ext.hello"; }
    std::string name() const override { return "Hello World Extension"; }
    sspm::Version version() const override { return {1, 0, 0, "beta"}; }

    bool initialize(const std::map<std::string, std::string>& context) override {
        SSPM_INFO("Hello World 模块已成功初始化！");
        return true;
    }

    void shutdown() override {
        SSPM_INFO("Hello World 模块正在关闭...");
    }
};

// 使用宏导出工厂函数
SSPM_REGISTER_MODULE(HelloModule)
```

## 3. 编译与部署

### 编译
使用 C++20 标准编译为共享库：

```bash
g++ -shared -fPIC -std=c++20 -I./include hello_module.cpp -o libhello.so
```

### 部署目录结构
插件必须放置在 `~/.local/share/sspm/plugins/` 目录下：

```text
~/.local/share/sspm/plugins/
└── hello/
    ├── plugin.toml       <-- 模块元数据
    └── libhello.so       <-- 编译后的二进制文件
```

### plugin.toml 配置
```toml
[plugin]
name = "hello"
version = "1.0.0"
type = "module"
description = "一个简单的示例模块"
entry = "libhello.so"
author = "Your Name"
```

## 4. 后端扩展 (Backend Development)

如果您希望添加一个新的包管理器支持（如 Docker 或 Homebrew Tap），请继承 `sspm::Backend` 类并实现相关方法。

```cpp
class MyBackend : public sspm::Backend {
    // 实现 install, remove, search 等方法...
};
SSPM_REGISTER_BACKEND(MyBackend)
```

## 5. 最佳实践

- **解耦设计**：模块间不应直接相互依赖，应通过 `IModule` 接口或 REST API 进行通信。
- **资源管理**：确保在 `shutdown()` 中释放所有已分配的资源。
- **版本兼容**：遵循语义化版本（SemVer）规范。
- **日志记录**：使用 `SSPM_INFO`, `SSPM_WARN`, `SSPM_ERROR` 宏进行标准化日志输出。

---
*更多信息请参考 [API_DOCUMENTATION.md](./API_DOCUMENTATION.md)*
