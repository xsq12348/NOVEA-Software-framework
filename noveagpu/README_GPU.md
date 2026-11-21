# NOVEA

NOVEA 是一系列工具库/框架的集合，每个子模块可独立使用。

## 模块

| 模块 | 作用 | 版本 |
|---|---|---|
| `noveagpu` | OpenCL 异构计算：平台/设备查询、上下文与命令队列、内核编译、主机与设备数据传输 | 0.2.0 |

## 构建

需要：
- OpenCL 头文件（`noveagpu/CL/` 已包含 Khronos 头文件，无需额外安装）
- `OpenCL.lib`（未随仓库提供，需自行准备）

生成 `OpenCL.lib` 的方式（Windows）：

1. 导出系统 OpenCL.dll 的函数表：
