/**
 * @file   FileUtil.h
 * @brief  文件、目录相关工具函数
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"

#include <cstdio>
#include <string>

namespace afl
{
namespace file
{
/**
 * @brief 文件/目录工具命名空间
 */
namespace FileUtil
{
/** @brief 当前进程可执行文件完整路径 */
std::string getBinaryPath();
/** @brief 当前进程可执行文件名 */
std::string getBinaryName();
/** @brief 当前进程可执行文件所在目录 */
std::string getBinaryDir();

/**
 * @brief 判断路径是否为目录
 * @param dir 路径
 * @return 是目录返回 true
 */
bool isDirectory(const char* dir);

/**
 * @brief 递归创建目录
 * @param dir 路径
 * @return 成功返回 true
 */
bool createRecursionDir(const char* dir);

/**
 * @brief 取目录部分（最后一个 '/' 之前）
 * @param dir 路径
 * @return 父目录路径
 */
std::string dirName(const char* dir);

/**
 * @brief 取文件名部分
 * @param dir 路径
 * @return 基名
 */
std::string baseName(const char* dir);

/**
 * @brief 文件是否存在
 * @param filepath 路径
 */
bool isFileExist(const char* filepath);

/**
 * @brief 文件大小（已打开 FILE*）
 * @param file FILE 指针
 */
long getFileSize(FILE* file);

/**
 * @brief 文件大小（按路径）
 * @param filepath 路径
 */
long getFileSize(const char* filepath);

/**
 * @brief 读入整个文件到字符串
 * @param filepath 路径
 * @param buf      输出缓冲
 * @return 读取字节数
 */
size_t readFile(const char* filepath, std::string& buf);

} // namespace FileUtil

} // namespace file
} // namespace afl
