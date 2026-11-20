#include"novea_gpu.h"
#include<assert.h>
#include<CL/cl.h>

/*
* NOVEA_GPU_OBJECT
*  用于gpu对象
* 修改于
*  2026-11-12 00:00 创建结构体
*  2026-11-15 02:41 新添calculate
*  2026-11-15 02:51 calculate新添resource
*  2026-11-15 16:30 重构了calculate
*  2026-11-15 18:17 新添resource_c
*
*/

struct NOVEA_GPU_OBJECT
{
	cl_program program; //程序代码
	struct
	{
		struct
		{
			cl_kernel kernel; //内核代码
			unsigned int id;  //内核编号
			struct
			{
				struct
				{
					NOVEA_GPU_OBJECT_TYPE type;
					union
					{
						unsigned parameter_id; //缓冲区指针
						int parameter_i; //整形常量
						unsigned int parameter_ui; //无符号整型常量
						float parameter_f; //单精度浮点数常量
					}parameter;
					void* resource; //资源指针
					size_t size; //资源大小
				}*calculate;
				unsigned int size; //大小
			}calculate; //计算对象
		}*kernel;
		unsigned int size; //内核数量
	}kernel;
	struct
	{
		struct
		{
			cl_mem parameter; //缓冲区指针
			void* resource; //资源指针
		}*pool; //缓冲区
		unsigned int size; //缓冲区大小
	}pool; //缓冲区池
};

static cl_platform_id novea_gpu_platform;
static cl_device_id novea_gpu_device;
static cl_context novea_gpu_context;
static cl_command_queue novea_gpu_queue;
static int novea_gpu_init = NOVEA_FALSE;

static unsigned int novea_gpu_vsn[3] = { 0,2,0 };

static struct
{
	float comput; //计算能力
	float global_vram; //全局显存
	float local_mem; //本地内存
	float max_group; //最大工作组
}novea_gpu_setting = { .5f, .25f, .1f, .1f };

extern unsigned int noveaGpuVsn()
{
	printf("NOVEA GPU | %d,%d,%d\n", novea_gpu_vsn[0], novea_gpu_vsn[1], novea_gpu_vsn[2]);
	return (novea_gpu_vsn[0] << 16) | (novea_gpu_vsn[1] << 8) | novea_gpu_vsn[2];
}

extern NOVEA_INFO noveaGpuSetSetting(float comput, 
	float global_vram, 
	float local_mem, 
	float max_group)
{
	if (novea_gpu_init)
	{
		printf("PROMPT | [ NOVEA GPU SETTING ] The device is ready, settings can't be changed\n");
		return NOVEA_INFO_PROMPT;
	}
	novea_gpu_setting.comput = (comput > 0 ? comput : .5f);
	novea_gpu_setting.global_vram = (global_vram > 0 ? global_vram : .25f);
	novea_gpu_setting.local_mem = (local_mem > 0 ? local_mem : .1f);
	novea_gpu_setting.max_group = (max_group > 0 ? max_group : .1f);
	return NOVEA_INFO_SUCCESS;
}

extern NOVEA_INFO noveaGpuInit()
{
	if(novea_gpu_init)
	{
		printf("INFO | [ NOVEA GPU INIT ] OpenCL has already been initialized\n");
		goto error_error;
	}

	cl_int error = NOVEA_FALSE;
	
	cl_uint num_platform = 0;
	cl_platform_id* platform = 0;
	
	{
		if (clGetPlatformIDs(NOVEA_NULL, NOVEA_NULL, &num_platform) != CL_SUCCESS
			|| num_platform == 0)
			goto error_fatal;


		platform = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platform);
		if (platform == NOVEA_NULL)
		{
			printf("ERROR | [ NOVEA GPU INIT ] Failed to allocate memory for OpenCL platform IDs\n");
			goto error_fatal;
		}

		error = clGetPlatformIDs(num_platform, platform, NOVEA_NULL);
		if (error != CL_SUCCESS)
		{
			free(platform);
			printf("ERROR | [ NOVEA GPU INIT ] Failed to get OpenCL platform ID: %d\n", error);
			goto error_fatal;
		}

	}

	float max_scroe = 0;
	for (unsigned int index = 0; index < num_platform; ++index)
	{
		cl_uint num_device = 0;

		if (clGetDeviceIDs(platform[index], CL_DEVICE_TYPE_GPU, NOVEA_NULL, NOVEA_NULL, &num_device) != CL_SUCCESS
			|| num_device == 0)
			continue;

		cl_device_id* device = (cl_device_id*)malloc(sizeof(cl_device_id) * num_device);
		if (!device)
		{
			free(platform);
			printf("INFO | [ NOVEA GPU INIT ] Failed to allocate memory for OpenCL device IDs\n");
			goto info_mem_ins;
		}

		error = clGetDeviceIDs(platform[index], CL_DEVICE_TYPE_GPU, num_device, device, NOVEA_NULL);
		if (error != CL_SUCCESS)
		{
			free(platform);
			free(device);
			printf("ERROR | [ NOVEA GPU INIT ] Failed to get OpenCL device ID: %d\n", error);
			goto error_fatal;
		}

		/*
			CL_DEVICE_MAX_COMPUTE_UNITS	cl_uint	计算单元数
			CL_DEVICE_MAX_CLOCK_FREQUENCY	cl_uint	最高频率(MHz)
			CL_DEVICE_GLOBAL_MEM_SIZE	cl_ulong	全局内存(字节)
			CL_DEVICE_LOCAL_MEM_SIZE	cl_ulong	本地内存(字节)
			CL_DEVICE_MAX_WORK_GROUP_SIZE	size_t	最大工作组
		*/


		for (unsigned int index_2 = 0; index_2 < num_device; ++index_2)
		{
			float scroe = 0;

			{
				//计算能力
				cl_uint 
					count = 0,
					count_2 = 0;
				clGetDeviceInfo(device[index_2], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &count, NOVEA_NULL);
				clGetDeviceInfo(device[index_2], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(cl_uint), &count_2, NOVEA_NULL);
				scroe += ((float)count * ((float)count_2 / 1000.f) / 100.f) * novea_gpu_setting.comput;
			}

			{
				//全局内存
				cl_ulong count = 0;
				clGetDeviceInfo(device[index_2], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &count, NOVEA_NULL);
				scroe += (((float)count / (1024.f * 1024.f * 1024.f)) / 16.f) * 100.f * novea_gpu_setting.global_vram;
			}

			{
				//本地内存
				cl_ulong count = 0;
				clGetDeviceInfo(device[index_2], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &count, NOVEA_NULL);
				scroe += (((float)count / 1024.f) / 64.f) * 100.f * novea_gpu_setting.local_mem;
			}

			{
				//最大工作组
				size_t count = 0;
				clGetDeviceInfo(device[index_2], CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &count, NOVEA_NULL);
				scroe += ((float)count / 1024.0f) * 100.0f * novea_gpu_setting.max_group;
			}

			if (max_scroe < scroe)
			{
				max_scroe = scroe;
				novea_gpu_platform = platform[index];
				novea_gpu_device = device[index_2];
			}

		}
		
		free(device);
	}

	free(platform);


	novea_gpu_context = clCreateContext(NOVEA_NULL, 1, &novea_gpu_device, NOVEA_NULL, NOVEA_NULL, &error);
	if (error != CL_SUCCESS)
	{

		printf("ERROR | [ NOVEA GPU INIT ] OpenCL Context: %d\n", error);
		goto error_fatal;
	}


	{
		int opencl_vsn[2] = { 0 };

		char vsn[256] = { 0 };
		clGetPlatformInfo(novea_gpu_platform, CL_PLATFORM_VERSION, sizeof(vsn), vsn, NOVEA_NULL);
		int sscanf_out = sscanf(vsn, "OpenCL %d.%d", &opencl_vsn[0], &opencl_vsn[1]);

		if (opencl_vsn[0] < 2 || sscanf_out != 2)
			novea_gpu_queue = clCreateCommandQueue(novea_gpu_context, novea_gpu_device, NOVEA_NULL, &error);
		else
			novea_gpu_queue = clCreateCommandQueueWithProperties(novea_gpu_context, novea_gpu_device, NOVEA_NULL, &error);

		if (error != CL_SUCCESS)
		{
			printf("ERROR | [ NOVEA GPU INIT ] OpenCL Command Queue: %d\n", error);
			goto error_fatal;
		}
	}


	novea_gpu_init = NOVEA_TRUE;
	return NOVEA_INFO_SUCCESS;

	//错误处理

	{
	error_error:
		return NOVEA_INFO_ERROR;
	}

	{
	error_fatal:
		return NOVEA_INFO_ERROR_FATAL;
	}

	{
	info_mem_ins:
		return NOVEA_INFO_MEM_INS;
	}
}

extern NOVEA_INFO noveaGpuOver()
{
	if (novea_gpu_queue)
		clReleaseCommandQueue(novea_gpu_queue);
	if (novea_gpu_context)
		clReleaseContext(novea_gpu_context);
	novea_gpu_queue = NOVEA_NULL;
	novea_gpu_context = NOVEA_NULL;
	novea_gpu_init = NOVEA_FALSE;
	return NOVEA_INFO_SUCCESS;
}

extern NOVEA_INFO noveaGpuObjectCreate(NOVEA_GPU_OBJECT* object)
{
	if (*object)
		return NOVEA_INFO_WARNING;

	*object = (NOVEA_GPU_OBJECT)malloc(sizeof(struct NOVEA_GPU_OBJECT));
	
	if (!*object)
	{
		printf("ERROR | [ NOVEA GPU OBJECT CREATE ] malloc failed\n");
		return NOVEA_INFO_ERROR_AVERAGE;
	}
	(*object)->program = NOVEA_NULL;
	(*object)->kernel.kernel = NOVEA_NULL;
	(*object)->kernel.size = 0;
	(*object)->pool.pool = NOVEA_NULL;
	(*object)->pool.size = 0;
	return NOVEA_INFO_SUCCESS;
}

extern NOVEA_INFO noveaGpuKernelCompile(NOVEA_GPU_OBJECT object, 
	const char** kernel_code, 
	unsigned int kernel_code_count,
	unsigned int kernel_id,
	const char* kernel_name)
{
	if (!novea_gpu_init || !kernel_id)
		return NOVEA_INFO_ERROR_AVERAGE;

	if (!object->program)
	{
		cl_int error = 0;
		cl_uint count = kernel_code_count;

		object->program = clCreateProgramWithSource(novea_gpu_context, count, kernel_code, NOVEA_NULL, &error);
		if (error != CL_SUCCESS)
		{
			printf("ERROR | [ NOVEA GPU KERNEL COMPILE ] Failed to create OpenCL program: %d\n", error);
			goto error_serious;
		}

		error = clBuildProgram(object->program, 1, &novea_gpu_device, NOVEA_NULL, NOVEA_NULL, NOVEA_NULL);
		switch (error)
		{
		case CL_BUILD_PROGRAM_FAILURE:
			printf("ERROR | [ NOVEA GPU KERNEL COMPILE ] Compilation failed\n");
			goto error_serious_clrelease;
		break;

		case CL_OUT_OF_HOST_MEMORY:
			printf("ERROR | [ NOVEA GPU KERNEL COMPILE ] Not enough memory on the host\n");
			goto error_serious_clrelease;
		break;

		}
	
		if (error != CL_SUCCESS)
		{
			printf("ERROR | [ NOVEA GPU KERNEL COMPILE ] Unknown OpenCL issue: %d\n", error);
			goto error_fatal_clrelease;
		}

	}

	{
		{
			for(unsigned int index = 0; index < object->kernel.size; ++index)
				if (object->kernel.kernel[index].id == kernel_id)
				{
					printf("WARNING | [ NOVEA GPU KERNEL COMPILE ] Kernel ID %d already exists\n", kernel_id);
					goto error_warning;
				}

			void* kernel_buffer = (void*)realloc(object->kernel.kernel, sizeof(*(object->kernel.kernel)) * (object->kernel.size + 1));
			if (!kernel_buffer)
			{
				printf("ERROR | [ NOVEA GPU KERNEL COMPILE ] malloc failed\n");
				goto error_serious;
			}
			object->kernel.kernel = kernel_buffer;
			object->kernel.kernel[object->kernel.size].calculate.calculate = NOVEA_NULL;
			object->kernel.kernel[object->kernel.size].calculate.size = 0;
			object->kernel.kernel[object->kernel.size].kernel = NOVEA_NULL;
			object->kernel.kernel[object->kernel.size].id = kernel_id;
			object->kernel.size++;
		}
		cl_int error = 0;
		object->kernel.kernel[object->kernel.size - 1].kernel = clCreateKernel(object->program, kernel_name, &error);
		if (error != CL_SUCCESS)
		{
			object->kernel.size--;
			printf("ERROR | [ NOVEA GPU KERNEL COMPILE ] Failed to create OpenCL kernel: %d\n", error);
			goto error_serious;
		}
	}

	return NOVEA_INFO_SUCCESS;

	//错误处理

	{
	error_serious_clrelease:
		clReleaseProgram(object->program);
		object->program = NOVEA_NULL;
		goto error_serious;
	}

	{
	error_fatal_clrelease:
		clReleaseProgram(object->program);
		object->program = NOVEA_NULL;
		goto error_fatal;
	}

	{
	error_serious:
		return NOVEA_INFO_ERROR_SERIOUS;
	}

	{
	error_fatal:
		return NOVEA_INFO_ERROR_FATAL;
	}

	{
	error_warning:
		return NOVEA_INFO_WARNING;
	}
}

extern NOVEA_INFO noveaGpuObjectCreateBuffer(NOVEA_GPU_OBJECT object, 
	void* buffer, 
	size_t size, 
	NOVEA_GPU_OBJECT_TYPE type,
	unsigned int kernel_id)
{
	if (!novea_gpu_init)
		goto error_average;

	if (type == NOVEA_GPU_OBJECT_NULL)
		goto error_error;

	unsigned int kernel_index = 0;

	{
		for (kernel_index; kernel_index < object->kernel.size; ++kernel_index)
			if (object->kernel.kernel[kernel_index].id == kernel_id)
				break;

		if (kernel_index == object->kernel.size)
		{
			printf("ERROR | [ NOVEA GPU CALCULATE ] Kernel ID %d not found\n", kernel_id);
			goto error_average;
		}
	}

	{
		cl_int error = 0;
		void* mem_buffer = (void*)realloc(object->kernel.kernel[kernel_index].calculate.calculate, sizeof(*(object->kernel.kernel[kernel_index].calculate.calculate)) * (object->kernel.kernel[kernel_index].calculate.size + 1));

		if (!mem_buffer)
		{
			printf("ERROR | [ NOVEA GPU CREATE BUFFER] malloc failed\n");
			goto error_average;
		}

		object->kernel.kernel[kernel_index].calculate.calculate = mem_buffer;
		object->kernel.kernel[kernel_index].calculate.calculate[object->kernel.kernel[kernel_index].calculate.size].type = NOVEA_GPU_OBJECT_NULL;
		object->kernel.kernel[kernel_index].calculate.size++;

	}

	{
		unsigned int object_index = object->kernel.kernel[kernel_index].calculate.size - 1;
		cl_int error = 0;
		cl_mem_flags flags = 0;
		object->kernel.kernel[kernel_index].calculate.calculate[object_index].type = type;
		object->kernel.kernel[kernel_index].calculate.calculate[object_index].size = size;
		switch (type)
		{
		case NOVEA_GPU_OBJECT_MEM_R:
			flags = CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR;
			// fall through
		case NOVEA_GPU_OBJECT_MEM_RW:
			if (!flags)
				flags = CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR;
			// fall through
		case NOVEA_GPU_OBJECT_MEM_W:
		{
			unsigned int mem_index = 0;
			{
				NOVEA_BOOL found = NOVEA_FALSE;
				for (unsigned int index = 0; index < object->pool.size; ++index)
					if (object->pool.pool[index].resource == buffer)
					{
						mem_index = index;
						found = NOVEA_TRUE;
					}

				if (!found)
				{
					void* parameter_buffer = realloc(object->pool.pool, sizeof(*(object->pool.pool)) * (object->pool.size + 1));
					if (!parameter_buffer)
					{
						printf("ERROR | [ NOVEA GPU CREATE BUFFER] buffer realloc failed\n");
						goto error_average;
					}
					mem_index = object->pool.size;
					object->pool.pool = parameter_buffer;
					object->pool.pool[object->pool.size].parameter = NOVEA_NULL;
					object->pool.pool[object->pool.size].resource = buffer;
					object->pool.size++;

					if (!flags)
					{
						flags = CL_MEM_WRITE_ONLY;
						object->pool.pool[mem_index].parameter = clCreateBuffer(novea_gpu_context, flags, size, NOVEA_NULL, &error);
					}
					else
						object->pool.pool[mem_index].parameter = clCreateBuffer(novea_gpu_context, flags, size, buffer, &error);

				}
			}
			
			if (error != CL_SUCCESS)
			{
				object->pool.size--;
				object->kernel.kernel[kernel_index].calculate.size--;
				object->kernel.kernel[kernel_index].calculate.calculate[object_index].type = NOVEA_GPU_OBJECT_NULL;
				printf("ERROR | [ NOVEA GPU CREATE BUFFER ] Failed to create OpenCL buffer: %d\n", error);
				goto error_serious;
			}
			object->kernel.kernel[kernel_index].calculate.calculate[object_index].parameter.parameter_id = mem_index;
			object->kernel.kernel[kernel_index].calculate.calculate[object_index].resource = buffer;
		}
		break;

		case NOVEA_GPU_OBJECT_INT:
			object->kernel.kernel[kernel_index].calculate.calculate[object_index].parameter.parameter_i = *(int*)buffer;
			object->kernel.kernel[kernel_index].calculate.calculate[object_index].resource = NOVEA_NULL;
			break;

		case NOVEA_GPU_OBJECT_UINT:
			object->kernel.kernel[kernel_index].calculate.calculate[object_index].parameter.parameter_ui = *(unsigned int*)buffer;
			object->kernel.kernel[kernel_index].calculate.calculate[object_index].resource = NOVEA_NULL;
			break;

		case NOVEA_GPU_OBJECT_FLOAT:
			object->kernel.kernel[kernel_index].calculate.calculate[object_index].parameter.parameter_f = *(float*)buffer;
			object->kernel.kernel[kernel_index].calculate.calculate[object_index].resource = NOVEA_NULL;
			break;

		}

	}

	return NOVEA_INFO_SUCCESS;

	//错误处理

	{
	error_error:
		return NOVEA_INFO_ERROR;
	}

	{
	error_average:
		return NOVEA_INFO_ERROR_AVERAGE;
	}

	{
	error_serious:
		return NOVEA_INFO_ERROR_SERIOUS;
	}

}

extern NOVEA_INFO noveaGpuObjectRelease(NOVEA_GPU_OBJECT object)
{
	if (!object)
		return NOVEA_INFO_ERROR;

	for(unsigned int index = 0; index < object->kernel.size; ++index)
	{
	
		if (object->kernel.kernel[index].kernel)
			clReleaseKernel(object->kernel.kernel[index].kernel);
		free(object->kernel.kernel[index].calculate.calculate);
		object->kernel.kernel[index].calculate.calculate = NOVEA_NULL;
		object->kernel.kernel[index].calculate.size = 0;
	}

	for (unsigned int index = 0; index < object->pool.size; ++index)
		clReleaseMemObject(object->pool.pool[index].parameter);

	if(object->program)
		clReleaseProgram(object->program);

	free(object->kernel.kernel);
	free(object->pool.pool);

	object->program = NOVEA_FALSE;
	object->kernel.kernel = NOVEA_FALSE;
	object->kernel.size = 0;
	object->pool.pool = NOVEA_FALSE;
	object->pool.size = 0;
	free(object);
	return NOVEA_INFO_SUCCESS;
}

extern NOVEA_INFO noveaGpuCalculate(NOVEA_GPU_OBJECT object, 
	unsigned int dim123, 
	size_t actual_count[3], 
	NOVEA_BOOL gpu_rww, 
	unsigned int kernel_id)
{
	if (!novea_gpu_init)
		goto error_average;

	unsigned int kernel_index = 0;

	{
		for (kernel_index; kernel_index < object->kernel.size; ++kernel_index)
			if (object->kernel.kernel[kernel_index].id == kernel_id)
				break;

		if (kernel_index == object->kernel.size)
		{
			printf("ERROR | [ NOVEA GPU CALCULATE ] Kernel ID %d not found\n", kernel_id);
			goto error_average;
		}
	}


	for (unsigned int index = 0; index < object->kernel.kernel[kernel_index].calculate.size; ++index)
	{
		cl_int error = 0;
		switch (object->kernel.kernel[kernel_index].calculate.calculate[index].type)
		{
		case NOVEA_GPU_OBJECT_MEM_R:
			// fall through
		case NOVEA_GPU_OBJECT_MEM_RW:
			// fall through
		case NOVEA_GPU_OBJECT_MEM_W:
			error = clSetKernelArg(object->kernel.kernel[kernel_index].kernel, index, sizeof(cl_mem), &object->pool.pool[object->kernel.kernel[kernel_index].calculate.calculate[index].parameter.parameter_id].parameter);
			break;
		case NOVEA_GPU_OBJECT_INT:
			error = clSetKernelArg(object->kernel.kernel[kernel_index].kernel, index, sizeof(int), &object->kernel.kernel[kernel_index].calculate.calculate[index].parameter.parameter_i);
			break;
		case NOVEA_GPU_OBJECT_UINT:
			error = clSetKernelArg(object->kernel.kernel[kernel_index].kernel, index, sizeof(unsigned int), &object->kernel.kernel[kernel_index].calculate.calculate[index].parameter.parameter_ui);
			break;
		case NOVEA_GPU_OBJECT_FLOAT:
			error = clSetKernelArg(object->kernel.kernel[kernel_index].kernel, index, sizeof(float), &object->kernel.kernel[kernel_index].calculate.calculate[index].parameter.parameter_f);
			break;
		}

		if (error != CL_SUCCESS)
		{
			printf("ERROR | [ NOVEA GPU CALCULATE ] Failed to set kernel argument %d: %d\n", index, error);
			goto error_serious;
		}

	}

	{
		cl_int error = 0;
		error = clEnqueueNDRangeKernel(novea_gpu_queue, object->kernel.kernel[kernel_index].kernel, dim123 > 3 ? 3 : (dim123 ? dim123 : 1), NOVEA_NULL, actual_count, NOVEA_NULL, 0, NOVEA_NULL, NOVEA_NULL);
		if(error != CL_SUCCESS)
		{
			printf("ERROR | [ NOVEA GPU CALCULATE ] Failed to enqueue kernel: %d\n", error);
			goto error_serious;
		}
	}

	if (NOVEA_BOOL_CONVERT(gpu_rww))
		for (unsigned int index = 0; index < object->kernel.kernel[kernel_index].calculate.size; ++index)
			switch (object->kernel.kernel[kernel_index].calculate.calculate[index].type)
			{
			case NOVEA_GPU_OBJECT_MEM_RW:
				// fall through
			case NOVEA_GPU_OBJECT_MEM_W:
			{
				cl_int error = 0;
				error = clEnqueueReadBuffer(novea_gpu_queue, object->pool.pool[object->kernel.kernel[kernel_index].calculate.calculate[index].parameter.parameter_id].parameter, CL_TRUE, 0, object->kernel.kernel[kernel_index].calculate.calculate[index].size, object->kernel.kernel[kernel_index].calculate.calculate[index].resource, 0, NOVEA_NULL, NOVEA_NULL);
				if (error != CL_SUCCESS)
				{
					printf("ERROR | [ NOVEA GPU CALCULATE ] Failed to read buffer %d: %d\n", index, error);
					goto error_serious;
				}
			}
			break;
			}

	return NOVEA_INFO_SUCCESS;

	//错误处理

	{
	error_average:
		return NOVEA_INFO_ERROR_AVERAGE;
	}

	{
	error_serious:
		return NOVEA_INFO_ERROR_SERIOUS;
	}

}

