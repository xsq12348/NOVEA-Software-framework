#ifndef NOVEA_GPU_H
#define NOVEA_GPU_H
#pragma warning(disable: 4996)
#define _CRT_SECURE_NO_WARNINGS
#include"novea_type.h"

/*
* x.y.z
* x 大版本
* y 更新
* z 修复问题,重构
* 不稳定版为-后缀
* novea_gpu.h
* 0.0.1 2026.11.16 01:16 完成原型
* 0.0.1 2026.11.17 21:24 开始编写多kernel支持
* 0.0.1 2026.11.17 23:11 将NOVEA_GPU_OBJECT挪进了具体实现文件
* 0.0.1 2026.11.17 23:12 修改了所有带NOVEA_GPU_OBJECT参数的函数形参
* 0.0.1 2026.11.17 23:30 完成了所有函数修改NOVEA_GPU_OBJECT参数后配套的逻辑
* 0.0.1 2026.11.17 23:50 NOVEA_GPU_OBJECT_TYPE的枚举修改成负数,方便后续计数
* 0.0.1 2026.11.18 23:04 正式开始多kernel的编写工作
* 0.1.0 2026.11.19 00:49 完成了多kernel的编写工作
*                        这是第一个正式版 0.1.0
* 0.2.0 2026.11.20 00:20 重构了struct NOVEA_GPU_OBJECT,完成了缓冲池的设计
*/

typedef enum
{
	NOVEA_GPU_OBJECT_NULL = 0, //空
	NOVEA_GPU_OBJECT_MEM_R = -1, //输入缓冲区
	NOVEA_GPU_OBJECT_MEM_W = -2, //输出缓冲区
	NOVEA_GPU_OBJECT_MEM_RW = -3, //输入输出缓冲区
	NOVEA_GPU_OBJECT_INT = -4, //整型常量
	NOVEA_GPU_OBJECT_UINT = -5, //无符号整型常量
	NOVEA_GPU_OBJECT_FLOAT = -6, //单精度浮点数常量
}NOVEA_GPU_OBJECT_TYPE;


/*
* NOVEA基本GPU对象
* 一个对象只能对应一份内核代码,但是可以有多个内核函数
*/
typedef struct NOVEA_GPU_OBJECT* NOVEA_GPU_OBJECT;


/*
* Function: noveaGpuVsn
* 用处: 查询版本号
* 修改时间
*    2026-11-19 01:03 创建完成函数
*/

extern unsigned int noveaGpuVsn();


/*
* Function: noveaGpuInit
* 用处: 初始化OpenCL环境,包括平台,设备,上下文和命令队列的创建
* 修改时间
*    2026-11-12 00:00 创建完成函数
*    2026-11-13 00:59 修改部分,正在编写打分机制
*    2026-11-14 04:37 完成函数,并且修复了clCreateCommandQueueWithProperties在老旧电脑上无法使用的问题
*                     现在代码会自动根据版本来选择使用clCreateCommandQueueWithProperties还是clCreateCommandQueue
*/

extern NOVEA_INFO noveaGpuInit();


/*
* Function: noveaGpuOver
* 用处: 释放OpenCL环境,包括平台,设备,上下文和命令队列的释放
* 修改时间
*    2026-11-12 23:54 创建完成函数
*    2026-11-15 00:23 优化了收尾处理
*    2026-11-15 19:14 取消了函数开始时的init检查
*    2026-11-17 22:12 删除了device_score 的冗余代码
*/

extern NOVEA_INFO noveaGpuOver();


/*
* Function: noveaGpuSetSetting
* 用处: 修改默认GPU权重配置
*    不更改某个参数可以填0
* 修改时间
*    2026-11-14 02:51 创建完成函数
*/

extern NOVEA_INFO noveaGpuSetSetting(float comput, 
	float global_vram, 
	float local_mem, 
	float max_group);


/*
* Function: noveaGpuObjectCreate
* 用处: 初始化Gpu对象
* 修改时间
*    2026-11-17 23:30 创建完成函数
*    2026-11-20 00:20 根据新struct NOVEA_GPU_OBJECT重构了函数
*/

extern NOVEA_INFO noveaGpuObjectCreate(NOVEA_GPU_OBJECT* object);


/*
* Function: noveaGpuKernelCompile
* 用处: 编译内核函数
* 提示: kernel_code_count是kernel_code的配套参数
*      kernel_code可以包含多段源码
*      比如
*          const char* kernel_code[] = { kernel_code1, kernel_code2, kernel_code3};
*          unsigned int kernel_code_count = 3; //因为kernel_code包含了三段代码,所以这里为3
*      调用函数时 
*	       noveaGpuKernelCompile(obj, kernel_code, kernel_code_count, 1, "kernel_function");
* 修改时间
*    2026-11-15 00:27 创建完成函数
*    2026-11-16 01:12 修正了一些问题
*    2026-11-18 21:39 重写了错误处理
*    2026-11-18 23:57 根据多kernel的需求,重构了函数,新增了kernel_id参数,用于指定内核编号,以及kernel_name参数,用于指定内核名称
*/

extern NOVEA_INFO noveaGpuKernelCompile(NOVEA_GPU_OBJECT object,
	const char** kernel_code,
	unsigned int kernel_code_count,
	unsigned int kernel_id,
	const char* kernel_name);


/*
* Function: noveaGpuObjectCreateBuffer
* 用处: 创建Gpu对象
* 修改时间
*    2026-11-15 03:01 创建完成函数
*    2026-11-15 03:50 将noveaGpuCreateBuffer / noveaGpuCreateBufferSuccess合并成noveaGpuCreateBuffer
*    2026-11-15 17:10 新增type参数
*    2026-11-15 17:49 根据新NOVEA_GPU_OBJECT结构体,重构了函数,新增了对int,uint,float类型的支持
*    2026-11-15 18:14 noveaGpuCreateBuffer 改名为 noveaGpuObjectCreateBuffer
*    2026-11-15 23:44 重构了函数,将其改为了严格的顺序输入参数,不再是之前的空位填充
*    2026-11-18 22:21 重构了错误处理
*    2026-11-19 00:33 支持多kernel,新增了kernel_id参数,用于指定内核编号
*    2026-11-20 00:20 根据新struct NOVEA_GPU_OBJECT重构了函数
*/

extern NOVEA_INFO noveaGpuObjectCreateBuffer(NOVEA_GPU_OBJECT object,
	void* buffer,
	size_t size,
	NOVEA_GPU_OBJECT_TYPE type,
	unsigned int kernel_id);


/*
* Function: noveaGpuObjectRelease
* 用处: 释放Gpu对象
* 修改时间
*    2026-11-15 03:09 创建完成函数
*    2026-11-20 00:20 根据新struct NOVEA_GPU_OBJECT重构了函数
*/

extern NOVEA_INFO noveaGpuObjectRelease(NOVEA_GPU_OBJECT object);


/*
* Function: noveaGpuCalculate
* 用处: 执行Gpu计算
* 修改时间
*    2026-11-18 18:41 创建完成函数
*    2026-11-18 21:55 重构了错误处理
*    2026-11-18 22:42 添加了参数gpu_rww,意思是(GPU RW/W),在计算后将输入输出缓冲区的数据写回主机内存,如果为NOVEA_TRUE,则会将输入输出缓冲区的数据写回主机内存,如果为NOVEA_FALSE,则不会
*                     内部使用了NOVEA_BOOL_CONVERT,您不必担心输入的参数是否为NOVEA_TRUE/NOVEA_FALSE,只要是非0即为NOVEA_TRUE,0即为NOVEA_FALSE
*    2026-11-19 00:33 支持多kernel
*/

extern NOVEA_INFO noveaGpuCalculate(NOVEA_GPU_OBJECT object, 
	unsigned int dim123, 
	size_t actual_count[3],
	NOVEA_BOOL gpu_rww, 
	unsigned int kernel_id);

#endif
