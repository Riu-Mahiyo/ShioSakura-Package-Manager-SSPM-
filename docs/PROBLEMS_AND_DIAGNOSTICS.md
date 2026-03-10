# 🔍 SSPM 问题与诊断

本文档详细介绍 SSPM 的诊断工具和常见问题解决方案。

## 🩺 SSPM 诊断工具

SSPM 包含一个内置的诊断工具，用于检查系统状态、检测问题并提供解决方案。

### 运行诊断

```bash
sspm doctor
```

执行此命令将运行一系列检查，评估 SSPM 的健康状态。

### 诊断检查项目

SSPM 诊断工具执行以下检查：

| 检查项 | 描述 | 成功标志 |
|--------|------|----------|
| **Backends** | 检测系统上可用的包管理后端 | 找到至少一个后端 |
| **Permissions** | 检查缓存和数据目录的写入权限 | 目录可写 |
| **Network** | 测试网络连接 | 能够连接到示例网站 |
| **SkyDB** | 检查数据库完整性 | 数据库打开成功且完整性检查通过 |
| **Cache** | 检查缓存系统状态 | 缓存大小计算成功 |
| **Repos** | 检查仓库配置 | 能够读取仓库配置 |

### 诊断输出示例

```
🌸 SSPM Doctor
══════════════════════════════════════════
✅  Backends         Found: apt flatpak snap 
✅  Permissions      Cache and data dirs writable
✅  Network          Connected
✅  SkyDB            Database integrity OK
✅  Cache            Size: 128 MB
✅  Repos            2 repos configured
══════════════════════════════════════════
✅ All 6/6 checks passed
```

## 📋 常见问题与解决方案

### 1. 后端相关问题

#### 问题：没有找到后端

**症状：**
```
❌  Backends         No backends found
```

**解决方案：**
- 安装至少一个受支持的包管理器（如 apt、pacman、brew 等）
- 确保包管理器在系统 PATH 中
- 检查包管理器是否正常工作

#### 问题：后端检测失败

**症状：**
某些后端未被检测到，即使它们已安装

特别是 `apk` 后端只能在真正的 Alpine 系统上启用。如果你在
其他 musl 发行版上看到类似以下消息，则说明这是预期行为：

```
apk backend skipped: not an Alpine system
```

这意味着当前系统虽然使用 musl libc，但并不是 Alpine，因此
`apk` 命令不会为 SSPM 提供支持。

**解决方案：**
- 确保后端的可执行文件在系统 PATH 中
- 运行 `sspm doctor` 重新检测后端
- 检查后端是否正常运行

### 2. 权限相关问题

#### 问题：缓存或数据目录权限不足

**症状：**
```
❌  Permissions      Permission denied
```

**解决方案：**
- 确保用户对 `~/.cache/sspm` 和 `~/.local/share/sspm` 目录有写入权限
- 运行 `chmod` 命令修复权限：
  ```bash
  chmod -R 755 ~/.cache/sspm ~/.local/share/sspm
  ```

### 3. 网络相关问题

#### 问题：网络连接失败

**症状：**
```
❌  Network          Error: Couldn't connect to server
```

**解决方案：**
- 检查网络连接是否正常
- 确保防火墙没有阻止 SSPM 的网络访问
- 检查代理设置（如果使用代理）

### 4. 数据库相关问题

#### 问题：数据库创建失败

**症状：**
```
❌  SkyDB            Failed to create database
```

**解决方案：**
- 确保用户对 `~/.local/share/sspm` 目录有写入权限
- 检查磁盘空间是否充足
- 尝试删除损坏的数据库文件并重新运行 `sspm doctor`：
  ```bash
  rm ~/.local/share/sspm/sky.db
  sspm doctor
  ```

#### 问题：数据库完整性失败

**症状：**
```
❌  SkyDB            Database integrity FAILED
```

**解决方案：**
- 删除损坏的数据库文件并重新创建：
  ```bash
  rm ~/.local/share/sspm/sky.db
  sspm doctor
  ```

### 5. 缓存相关问题

#### 问题：缓存目录不可写

**症状：**
```
❌  Cache            Permission denied
```

**解决方案：**
- 确保用户对 `~/.cache/sspm` 目录有写入权限
- 运行 `chmod` 命令修复权限：
  ```bash
  chmod -R 755 ~/.cache/sspm
  ```

#### 问题：缓存过大

**症状：**
缓存大小超过预期

**解决方案：**
- 运行缓存清理命令：
  ```bash
  sspm cache clean
  ```
- 配置缓存大小限制（在 `~/.config/sspm/config.yaml` 中）：
  ```yaml
  cache:
    max_size: 2GB
    prune_after: 30d
  ```

### 6. 仓库相关问题

#### 问题：仓库配置错误

**症状：**
```
❌  Repos            Cannot open database
```

**解决方案：**
- 检查数据库是否损坏（参见数据库相关问题的解决方案）
- 重新添加仓库：
  ```bash
  sspm repo remove <repo-name>
  sspm repo add <repo-url>
  ```

## 🔧 高级故障排除

### 查看详细日志

SSPM 会记录详细的日志，可用于诊断复杂问题：

```bash
# 查看最近的日志
sspm log

# 实时查看日志
sspm log tail
```

日志文件位于：`~/.local/state/sspm/log/`

### 手动检查后端

如果诊断工具无法检测到后端，可以手动检查：

```bash
# 检查后端是否安装
which <backend-name>

# 检查后端版本
<backend-name> --version
```

### 检查系统环境

```bash
# 检查系统信息
uname -a

# 检查 PATH 环境变量
echo $PATH

# 检查用户权限
id
```

## 📞 获取帮助

如果遇到无法解决的问题，可以通过以下方式获取帮助：

1. **GitHub Issues**：[https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/issues)

2. **GitHub Discussions**：[https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions](https://github.com/Riu-Mahiyo/ShioSakura-Package-Manager-SSPM-/discussions)

3. **运行详细诊断**：
   ```bash
   sspm doctor --verbose
   ```

## 📝 问题报告模板

当报告问题时，请提供以下信息：

1. **SSPM 版本**：`sspm --version`
2. **系统信息**：`uname -a`
3. **诊断输出**：`sspm doctor`
4. **错误日志**：相关日志片段
5. **复现步骤**：如何重现问题
6. **预期行为**：期望的结果
7. **实际行为**：实际发生的情况

提供这些信息将帮助开发者更快地诊断和解决问题。