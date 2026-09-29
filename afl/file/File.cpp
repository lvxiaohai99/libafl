/**
 * @file   File.cpp
 * @brief  文件操作的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/file/File.h"
#include "afl/file/FileUtil.h"
#include <memory.h>
#include <stdarg.h>

namespace afl
{
namespace file
{
File::File() : m_fil(NULL), m_closeOwned(true), m_rptr(0), m_wptr(0) {}

File::File(FILE* fil) : m_fil(fil), m_closeOwned(false), m_rptr(0), m_wptr(0) {}

File::File(const std::string& path, const std::string& mode)
    : m_fil(NULL), m_closeOwned(true), m_rptr(0), m_wptr(0)
{
    fopen(path, mode);
}

File::~File()
{
    if (m_closeOwned)
    {
        fclose();
    }
}

bool File::fopen(const std::string& path, const std::string& mode)
{
    m_path = path;
    m_mode = mode;

    m_fil = ::fopen(path.c_str(), mode.c_str());
    return m_fil ? true : false;
}

void File::fclose() const
{
    if (m_fil)
    {
        ::fclose(m_fil);
        m_fil = NULL;
    }
}

size_t File::fread(char* ptr, size_t size, size_t nmemb) const
{
    size_t r = 0;
    if (m_fil)
    {
        fseek(m_fil, m_rptr, SEEK_SET);
        r = ::fread(ptr, size, nmemb, m_fil);
        m_rptr = ftell(m_fil);
    }
    return r;
}

size_t File::fwrite(const char* ptr, size_t size, size_t nmemb)
{
    size_t r = 0;
    if (m_fil)
    {
        fseek(m_fil, m_wptr, SEEK_SET);
        r = ::fwrite(ptr, size, nmemb, m_fil);
        m_wptr = ftell(m_fil);
    }
    return r;
}

char* File::fgets(char* s, int size) const
{
    char* r = NULL;
    if (m_fil)
    {
        fseek(m_fil, m_rptr, SEEK_SET);
        r = ::fgets(s, size, m_fil);
        m_rptr = ftell(m_fil);
    }
    return r;
}

void File::fprintf(const char* format, ...)
{
    if (!m_fil)
        return;
    va_list ap;
    va_start(ap, format);
    fseek(m_fil, m_wptr, SEEK_SET);
    vfprintf(m_fil, format, ap);
    m_wptr = ftell(m_fil);
    va_end(ap);
}

off_t File::size() const
{
    return FileUtil::getFileSize(m_path.c_str());
}

bool File::eof() const
{
    if (m_fil)
    {
        if (feof(m_fil))
            return true;
    }
    return false;
}

void File::resetRead() const
{
    m_rptr = 0;
}

void File::resetWrite()
{
    m_wptr = 0;
}

const std::string& File::path() const
{
    return m_path;
}

//--------------------------- MemFile ------------------------
MemFile::MemFile()
    : m_src(m_src), m_srcValid(false), m_base(new block_t), m_currentRead(m_base),
      m_currentWrite(m_base), m_currentWriteNr(0), m_readPtr(0), m_writePtr(0),
      m_readCausedEof(false), m_refCount(0), m_refDecreased(false)
{
}

MemFile::MemFile(MemFile& s)
    : m_src(s), m_srcValid(true), m_base(s.m_base), m_currentRead(m_base),
      m_currentWrite(s.m_currentWrite), m_currentWriteNr(s.m_currentWriteNr), m_readPtr(0),
      m_writePtr(s.m_writePtr), m_readCausedEof(false), m_refCount(0), m_refDecreased(false),
      m_path(s.m_path)
{
    m_src.increase();
}

MemFile::MemFile(File& f)
    : m_src(m_src), m_srcValid(false), m_base(new block_t), m_currentRead(NULL),
      m_currentWrite(NULL), m_currentWriteNr(0), m_readPtr(0), m_writePtr(0),
      m_readCausedEof(false), m_refCount(0), m_refDecreased(false), m_path(f.path())
{
    m_currentRead = m_base;
    m_currentWrite = m_base;
    char slask[32768];
    size_t n;
    while ((n = f.fread(slask, 1, 32768)) > 0)
    {
        fwrite(slask, 1, n);
    }
}

MemFile::~MemFile()
{
    if (m_refCount)
    {
        std::cerr << "MemFile destructor with a ref count of " << m_refCount << std::endl;
    }
    while (m_base && !m_srcValid)
    {
        block_t* p = m_base;
        m_base = p->next;
        delete p;
    }
    if (m_srcValid && !m_refDecreased)
    {
        m_src.decrease();
        m_refDecreased = true;
    }
}

bool MemFile::fopen(const std::string& path, const std::string& mode)
{
    return true;
}

void MemFile::fclose() const
{
    if (m_srcValid && !m_refDecreased)
    {
        m_src.decrease();
        m_refDecreased = true;
    }
}

size_t MemFile::fread(char* ptr, size_t size, size_t nmemb) const
{
    size_t p = m_readPtr % BLOCKSIZE;
    size_t sz = size * nmemb;
    size_t available = m_writePtr - m_readPtr;
    if (sz > available) // read beyond eof
    {
        sz = available;
        m_readCausedEof = true;
    }
    else if (p + sz < BLOCKSIZE)
    {
        memcpy(ptr, m_currentRead->data + p, sz);
        m_readPtr += sz;
    }
    else
    {
        size_t sz1 = BLOCKSIZE - p;
        size_t sz2 = sz - sz1;
        memcpy(ptr, m_currentRead->data + p, sz1);
        m_readPtr += sz1;
        while (sz2 > BLOCKSIZE)
        {
            if (m_currentRead->next)
            {
                m_currentRead = m_currentRead->next;
                memcpy(ptr + sz1, m_currentRead->data, BLOCKSIZE);
                m_readPtr += BLOCKSIZE;
                sz1 += BLOCKSIZE;
                sz2 -= BLOCKSIZE;
            }
            else
            {
                return sz1;
            }
        }
        if (m_currentRead->next)
        {
            m_currentRead = m_currentRead->next;
            memcpy(ptr + sz1, m_currentRead->data, sz2);
            m_readPtr += sz2;
        }
        else
        {
            return sz1;
        }
    }
    return sz;
}

size_t MemFile::fwrite(const char* ptr, size_t size, size_t nmemb)
{
    size_t p = m_writePtr % BLOCKSIZE;
    int nr = (int)m_writePtr / BLOCKSIZE;
    size_t sz = size * nmemb;
    if (m_currentWriteNr < nr)
    {
        block_t* next = new block_t;
        m_currentWrite->next = next;
        m_currentWrite = next;
        m_currentWriteNr++;
    }
    if (p + sz <= BLOCKSIZE)
    {
        memcpy(m_currentWrite->data + p, ptr, sz);
        m_writePtr += sz;
    }
    else
    {
        size_t sz1 = BLOCKSIZE - p; // size left
        size_t sz2 = sz - sz1;
        memcpy(m_currentWrite->data + p, ptr, sz1);
        m_writePtr += sz1;
        while (sz2 > BLOCKSIZE)
        {
            if (m_currentWrite->next)
            {
                m_currentWrite = m_currentWrite->next;
                m_currentWriteNr++;
            }
            else
            {
                block_t* next = new block_t;
                m_currentWrite->next = next;
                m_currentWrite = next;
                m_currentWriteNr++;
            }
            memcpy(m_currentWrite->data, ptr + sz1, BLOCKSIZE);
            m_writePtr += BLOCKSIZE;
            sz1 += BLOCKSIZE;
            sz2 -= BLOCKSIZE;
        }
        if (m_currentWrite->next)
        {
            m_currentWrite = m_currentWrite->next;
            m_currentWriteNr++;
        }
        else
        {
            block_t* next = new block_t;
            m_currentWrite->next = next;
            m_currentWrite = next;
            m_currentWriteNr++;
        }
        memcpy(m_currentWrite->data, ptr + sz1, sz2);
        m_writePtr += sz2;
    }
    return sz;
}

char* MemFile::fgets(char* s, int size) const
{
    int n = 0;
    while (n < size - 1 && !eof())
    {
        char c;
        size_t sz = fread(&c, 1, 1);
        if (sz)
        {
            if (c == 10)
            {
                s[n] = 0;
                return s;
            }
            s[n++] = c;
        }
    }
    s[n] = 0;
    return s;
}

void MemFile::fprintf(const char* format, ...)
{
    va_list ap;
    char tmp[BLOCKSIZE];
    va_start(ap, format);
    vsnprintf(tmp, sizeof(tmp), format, ap);
    va_end(ap);
    fwrite(tmp, 1, strlen(tmp));
}

off_t MemFile::size() const
{
    return (off_t)m_writePtr;
}

bool MemFile::eof() const
{
    return m_readCausedEof; //(m_readPtr < m_writePtr) ? false : true;
}

void MemFile::resetRead() const
{
    m_readPtr = 0;
    m_currentRead = m_base;
}

void MemFile::resetWrite()
{
    m_writePtr = 0;
    m_currentWrite = m_base;
    m_currentWriteNr = 0;
}

const std::string& MemFile::path() const
{
    return m_path;
}

int MemFile::refCount() const
{
    return m_refCount;
}

void MemFile::increase()
{
    ++m_refCount;
}

void MemFile::decrease()
{
    --m_refCount;
}

} // namespace file
} // namespace afl
