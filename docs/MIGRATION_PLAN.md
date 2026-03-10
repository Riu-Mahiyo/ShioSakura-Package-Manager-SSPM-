# 🌸 SSPM 模块化迁移与验证计划 (v4.0.0-Sakura)

本计划旨在将现有的单体式后端集成平滑迁移至高度模块化的插件架构。

## 1. 迁移路线图 (Migration Roadmap)

### 第一阶段：接口标准化 (已完成)
- [x] 定义 `IModule` 接口规范。
- [x] 重构 `Backend` 继承自 `IModule`。
- [x] 建立 `sspm-sdk.h` 开发工具包。

### 第二阶段：内置后端解耦 (已完成)
- [x] 增强 `PluginManager` 支持动态加载。
- [x] 实现基于语义化版本的模块依赖管理。
- [x] 提供统一的 `ConfigManager` 配置体系。
- [x] 建立基于 `/metrics` 的全局模块观测系统。

### 第三阶段：动态加载适配 (进行中)
- [ ] 逐步将 `apt`, `pacman`, `brew` 等核心后端逻辑拆分为插件模块。
- [ ] 实现插件热插拔机制（Hot-plugging）。
- [ ] 为每个插件建立自动化基准测试套件。

## 2. 模块观测与监控 (Observability)

每个模块现在都必须实现 `metrics()` 接口，系统将通过 API `/metrics` 端点聚合所有模块的运行数据：
```json
{
  "backend.apt": { "install_count": 12, "last_error": "none" },
  "core.db": { "query_time_avg": "0.1ms", "wal_enabled": true }
}
```

## 3. 模块化测试框架 (Testing Framework)

每个模块必须能够通过以下独立测试流程：

### 3.1 单元测试 (Unit Testing)
- **隔离环境**：在 mock 目录下运行，不直接操作真实系统包管理器。
- **覆盖率要求**：每个新模块的逻辑覆盖率必须达到 **90%** 以上。

### 3.2 兼容性测试 (Compatibility)
- 验证模块在 Linux (Debian/Arch), macOS (Homebrew), Windows (WSL) 上的加载情况。

## 4. 回滚方案 (Rollback Strategy)

如果模块化加载失败，系统将自动回滚至“安全模式”：
1. **自动禁用故障插件**：若 `initialize()` 返回 false 或 `dlopen` 失败，系统将跳过该模块。
2. **核心备份**：在迁移期间，`apt` 和 `brew` 等关键后端将暂时保留内置静态版本作为兜底，直至插件化引擎验证稳定。

---
*负责人：SSPM 核心架构组*
*日期：2026-03-08*
