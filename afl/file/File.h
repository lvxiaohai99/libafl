/**
 * @file   File.h
 * @brief  文件 I/O 抽象与磁盘/内存实现（源自 C++ sockets IFile，已适配 libafl）
 * @author libafl
 * @date   2026-09
 */
#pragma once

#include "afl/base/Common.h"
#include "afl/base/NonCopy.h"

#include <stdio.h>
#include <string>

namespace afl
{
namespace file
{
/**
 * @brief 文件 I/O 纯虚接口
 */
class IFile
{
public:
    virtual ~IFile() {}

    /** @brief 打开文件 @param path 路径 @param mode fopen 模式 @return 是否成功 */
    virtual bool fopen(const std::string& path, const std::string& mode) = 0;
    /** @brief 关闭文件 */
    virtual void fclose() const = 0;

    /** @brief 读数据 @return 实际读取块数 */
    virtual size_t fread(char*, size_t, size_t) const = 0;
    /** @brief 写数据 @return 实际写入块数 */
    virtual size_t fwrite(const char*, size_t, size_t) = 0;

    /** @brief 读一行 */
    virtual char* fgets(char*, int) const = 0;
    /** @brief 格式化写入 */
    virtual void fprintf(const char* format, ...) = 0;

    /** @brief 文件大小（字节） */
    virtual off_t size() const = 0;
    /** @brief 是否到达末尾 */
    virtual bool eof() const = 0;

    /** @brief 重置读指针到开头 */
    virtual void resetRead() const = 0;
    /** @brief 重置写指针到开头 */
    virtual void resetWrite() = 0;

    /** @brief 当前路径 */
    virtual const std::string& path() const = 0;
};


/**
 * @brief 磁盘文件实现
 */
class File : base::NonCopy, public IFile
{
public:
    File();
    File(FILE*);
    File(const std::string& path, const std::string& mode);
    ~File();

    virtual bool fopen(const std::string& path, const std::string& mode);
    virtual void fclose() const;

    virtual size_t fread(char*, size_t, size_t) const;
    virtual size_t fwrite(const char*, size_t, size_t);

    virtual char* fgets(char*, int) const;
    virtual void fprintf(const char* format, ...);

    virtual off_t size() const;
    virtual bool eof() const;

    virtual void resetRead() const;
    virtual void resetWrite();

    virtual const std::string& path() const;

private:
    std::string m_path;
    std::string m_mode;
    mutable FILE* m_fil;
    bool m_closeOwned;
    mutable long m_rptr;
    long m_wptr;
};


/**
 * @brief 内存块链式文件，支持引用计数共享缓冲
 */
class MemFile : public IFile
{
    enum
    {
        BLOCKSIZE = 32768
    };

public:
    /** File block structure. */
    struct block_t
    {
        block_t() : next(NULL) {}
        struct block_t* next;
        char data[BLOCKSIZE];
    };

public:
    /** @brief 创建临时内存缓冲，析构时释放 */
    MemFile();
    /** @brief 从源拷贝缓冲并重置读指针 */
    MemFile(MemFile&);
    /** @brief 读入文件内容，按 f.path() 建立非临时内存缓冲 */
    MemFile(File& f);
    ~MemFile();

    virtual bool fopen(const std::string& path, const std::string& mode);
    virtual void fclose() const;

    virtual size_t fread(char* ptr, size_t size, size_t nmemb) const;
    virtual size_t fwrite(const char* ptr, size_t size, size_t nmemb);

    virtual char* fgets(char*, int) const;
    virtual void fprintf(const char* format, ...);

    virtual off_t size() const;
    virtual bool eof() const;

    virtual void resetRead() const;
    virtual void resetWrite();

    virtual const std::string& path() const;

    /** @brief 拷贝构造时的引用计数 */
    int refCount() const;
    void increase();
    void decrease();

private:
    MemFile& operator=(const MemFile&)
    {
        return *this; // assignment operator
    }

    MemFile& m_src;
    bool m_srcValid;
    block_t* m_base;
    mutable block_t* m_currentRead;
    block_t* m_currentWrite;
    int m_currentWriteNr;
    mutable size_t m_readPtr;
    size_t m_writePtr;
    mutable bool m_readCausedEof;
    int m_refCount;
    mutable bool m_refDecreased;
    std::string m_path;
};

} // namespace file
} // namespace afl
