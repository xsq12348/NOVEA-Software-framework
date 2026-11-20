#ifndef NOVEA_TYPE_H
#define NOVEA_TYPE_H

#include<stdio.h>
#include<stdlib.h>

#define NOVEA_NULL (0) //空
#define NOVEA_TRUE (1) //真
#define NOVEA_FALSE (0) //假	
#define NOVEA_BOOL_CONVERT(novea_bool) (!!(novea_bool)) //布尔值转换
#define NOVEA_STR(str) ""#str //转换字符串
#define NOVEA_CODE_ON if(1) //代码块开启
#define NOVEA_CODE_OFF if(0) //代码块关闭

typedef unsigned char NOVEA_BOOL;

/*
* NOVEA_INFO
*  用于提示信息,或当作函数返回值
* 修改于
*  2026-11-12 00:05 创建枚举
*/
typedef enum
{
	NOVEA_INFO_SUCCESS = 0, //成功
	NOVEA_INFO_ERROR, //错误
	NOVEA_INFO_ERROR_AVERAGE, //一般错误
	NOVEA_INFO_ERROR_SERIOUS, //严重错误
	NOVEA_INFO_ERROR_FATAL, //致命错误
	NOVEA_INFO_WARNING, //警告
	//NOVEA_INFO, //信息
	NOVEA_INFO_PROMPT, //提示
	NOVEA_INFO_MEM_QUESTION, //内存问题
	NOVEA_INFO_MEM_INS, //内存不足
	NOVEA_INFO_PLACEHOLDER, //占位符
}NOVEA_INFO;

#endif
