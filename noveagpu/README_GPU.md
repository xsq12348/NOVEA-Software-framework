# NOVEA

NOVEA 是一系列工具库/框架的集合，每个子模块可独立使用。

## 模块

| 模块 | 作用 | 版本 |
|---|---|---|
| noveagpu | OpenCL 异构计算：平台/设备查询、上下文与命令队列、内核编译、主机与设备数据传输 | 0.2.0 |

## 构建

需要：

- OpenCL 头文件（noveagpu/CL/ 已包含 Khronos 头文件，无需额外安装）
- OpenCL.lib（未随仓库提供，需自行准备）

生成 OpenCL.lib 的方式（Windows）：

1. 导出系统 OpenCL.dll 的函数表

       dumpbin /exports %WINDIR%\System32\OpenCL.dll > exports.txt

2. 整理成 opencl.def（首行 LIBRARY OpenCL，第二行 EXPORTS，下面逐行函数名）

3. 生成导入库

       lib /def:opencl.def /out:OpenCL.lib /machine:x64

装了显卡驱动的话，%WINDIR%\System32\OpenCL.dll 一般已经存在；如果装了 CUDA Toolkit 或厂商 SDK，里面也会带 OpenCL.lib，可以直接用。

## 用法

    #define CL_TARGET_OPENCL_VERSION 120
    #include "novea_gpu.h"

    noveaGpuInit();

    NOVEA_GPU_OBJECT obj = NOVEA_NULL;
    noveaGpuObjectCreate(&obj);

    const char* src[] = {
    "__kernel void add_k(__global const float* a, __global const float* b, __global float* o)\n"
    "{ int g = get_global_id(0); o[g] = a[g] + b[g]; }\n"
    };

    noveaGpuKernelCompile(obj, src, 1, 1, "add_k");
    noveaGpuObjectCreateBuffer(obj, a, sizeof(float) * N, NOVEA_GPU_OBJECT_MEM_R, 1);
    noveaGpuObjectCreateBuffer(obj, b, sizeof(float) * N, NOVEA_GPU_OBJECT_MEM_R, 1);
    noveaGpuObjectCreateBuffer(obj, o, sizeof(float) * N, NOVEA_GPU_OBJECT_MEM_W, 1);

    size_t global[3] = { N, 0, 0 };
    noveaGpuCalculate(obj, 1, global, NOVEA_TRUE, 1);

    noveaGpuObjectRelease(obj);
    noveaGpuOver();

## 约定

- kernel_id 从 1 开始，0 无效
- 缓冲区按参数顺序依次创建，顺序必须与内核参数一致
- 一个对象对应一份内核源码（program），可以有多个内核函数（kernel_id）

## 许可

MIT，见 LICENSE。noveagpu/CL/ 下的头文件来自 Khronos Group，遵循 Apache-2.0。
