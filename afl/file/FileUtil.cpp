/**
 * @file   FileUtil.cpp
 * @brief  文件、目录工具函数的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/file/FileUtil.h"
#include <cstdio>
#include <fstream>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
namespace afl
{
namespace file
{
std::string FileUtil::getBinaryPath()
{
    const static size_t pathLen = 1024;
    char appFullPath[pathLen] = {0};

    const static char* procExe = "/proc/self/exe";
    if (::readlink(procExe, appFullPath, pathLen) != -1)
        return appFullPath;

    return "";
}

std::string FileUtil::getBinaryName()
{
    std::string path = getBinaryPath();
    if (path.empty())
        return path;

    size_t pos = path.find_last_of("/");
    if (pos != std::string::npos)
        path = path.substr(pos + 1);
    return path;
}

std::string FileUtil::getBinaryDir()
{
    std::string path = getBinaryPath();
    if (path.empty())
        return path;

    size_t pos = path.find_last_of("/");
    if (pos != std::string::npos)
        path = path.substr(0, pos);
    return path;
}

bool FileUtil::isDirectory(const char* path)
{
    DIR* pdir = opendir(path);
    if (pdir != NULL)
    {
        closedir(pdir);
        pdir = NULL;
        return true;
    }
    return false;
}

void modifyDirPath(std::string& path) // 修改目录路径为X/Y/Z/
{
    if (path.empty())
    {
        return;
    }
    for (std::string::iterator iter = path.begin(); iter != path.end(); ++iter)
    {
        if (*iter == '\\')
        {
            *iter = '/';
        }
    }
    if (path.at(path.length() - 1) != '/')
    {
        path += "/";
    }
}

bool FileUtil::createRecursionDir(const char* dir)
{
    std::string dirs(dir);
    if (dirs.empty())
        return true;

    modifyDirPath(dirs);

    std::string::size_type pos = dirs.find('/');
    while (pos != std::string::npos)
    {
        std::string cur = dirs.substr(0, pos - 0);
        if (cur.length() > 0 && !isDirectory(cur.c_str()))
        {
            bool ret = false;
            ret = (mkdir(cur.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0);
            if (!ret)
            {
                return false;
            }
        }
        pos = dirs.find('/', pos + 1);
    }

    return true;
}

bool FileUtil::isFileExist(const char* filepath)
{
    FILE* file = fopen(filepath, "rb");
    if (!file)
        return false;

    ::fclose(file);
    return true;
    //std::ifstream infile(filepath);
    //return infile.good();
}

long FileUtil::getFileSize(FILE* file)
{
    if (file == NULL)
        return -1;
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    return fileSize;
}

long FileUtil::getFileSize(const char* filepath)
{
    struct stat st;
    if (::stat(filepath, &st) != 0)
        return -1;
    return st.st_size;
}

size_t FileUtil::readFile(const char* filepath, std::string& buf)
{
    FILE* file = fopen(filepath, "rb");
    if (file == NULL)
        return 0;

    const static size_t PER_READ_SIZE = 1024;
    size_t total = 0;
    char data[PER_READ_SIZE];
    while (!feof(file))
    {
        ::memset(data, '\0', PER_READ_SIZE);
        size_t size = fread(data, 1, PER_READ_SIZE, file);
        buf.append(data, size);
        total += size;
        if (size < PER_READ_SIZE)
        {
            break;
        }
    }

    fclose(file);
    return total;
}

string FileUtil::dirName(const char* dir)
{
    string name = string(dir);

    std::string::size_type pos = name.find_last_of('/');

    if (pos == std::string::npos)
        return string();

    return name.substr(0, pos);
}

string FileUtil::baseName(const char* dir)
{
    string name = string(dir);
    std::string::size_type pos = name.find_last_of('/');

    pos = (pos + 1 > name.size()) ? pos : pos + 1;
    return name.substr(pos, name.size());
}

} // namespace file
} // namespace afl
