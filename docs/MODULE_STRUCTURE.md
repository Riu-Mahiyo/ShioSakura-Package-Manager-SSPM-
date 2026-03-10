# 🌸 SSPM 模块化架构文档 (v4.0.0-Sakura)

SSPM 的架构已演进为高度模块化的插件式系统，旨在实现核心引擎与具体功能实现（如包管理器后端、UI、监控）的完全解耦。

## 1. 核心抽象

### IModule (接口)
所有系统组件的基石。定义了统一的生命周期、版本管理、健康状态监测及性能指标上报接口。
- **ID**: 模块唯一标识（如 `core.db`, `backend.apt`）。
- **Lifecycle**: `initialize(context)` 和 `shutdown()`。
- **Observability**: `status()` 提供健康检查，`metrics()` 提供 JSON 格式的性能数据。

### PluginManager (引擎)
负责模块的动态发现与管理。
- **Topological Loading**: 基于语义化版本的依赖关系，确保模块按正确顺序加载。
- **Dynamic Linking**: 支持运行时加载 `.so` / `.dylib` 插件。
- **Monitoring**: 提供 `aggregate_metrics()` 接口，统一上报所有加载模块的运行状态。

## 2. 模块间通信

### ConfigManager
统一的配置总线。模块在初始化时通过 `context` 获得其专属配置项，实现配置与逻辑的分离。

### REST API
作为模块间及外部系统（如 GUI）通信的标准协议。

## 3. 开发与测试

### Module SDK
开发者只需包含 `sspm-sdk.h` 并使用 `SSPM_REGISTER_MODULE` 宏即可快速开发符合规范的插件。

### Module Testing Framework
通过 `module_test.h` 提供的基准测试类，每个模块都可以独立于系统核心进行单元测试，确保高内聚。

## 4. 目录结构

- `include/sspm/module.h`: 核心接口定义。
- `src/plugin/`: 插件加载引擎实现。
- `src/core/config_manager.cpp`: 配置管理中心。
- `docs/MODULE_DEVELOPMENT.md`: 开发者上手指南。

---
*SSPM 致力于打造最开放、最易扩展的通用包管理调度平台。*
