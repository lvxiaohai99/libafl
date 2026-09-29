/**
 * @file   SingletonProgram.cpp
 * @brief  单实例程序的实现
 * @author libafl
 * @date   2026-09
 */
#include "afl/process/SingletonProgram.h"
#include "afl/file/FileUtil.h"
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>    // for exit
#include <sys/file.h>  // for open, O_RDWR
#include <sys/stat.h>  // for umask
#include <sys/param.h> // for NOFILE
#include <sstream>

namespace afl
{
namespace process
{
SingletonProgram::SingletonProgram(std::string pidFile, WorkCB master, WorkCB others, bool daemon,
                                   int nochdir, int noclose)
    : m_pidFile(pidFile), m_master(master), m_others(others), m_inDaemon(daemon), m_pid(0),
      m_nochdir(nochdir), m_noclose(noclose)
{
    if (m_inDaemon)
    {
        ::daemon(m_nochdir, m_noclose);
    }
}

SingletonProgram::~SingletonProgram()
{
}

int SingletonProgram::run()
{
    int retval = 0;
    if (alreadyRunning())
    {
        if (m_others)
        {
            retval = m_others(m_pid);
        }
    }
    else
    {
        if (m_master)
        {
            retval = m_master(m_pid);
        }
    }
    return retval;
}

bool SingletonProgram::alreadyRunning()
{
    if (!afl::file::FileUtil::createRecursionDir(
            afl::file::FileUtil::dirName(m_pidFile.c_str()).c_str()))
    {
        std::stringstream ss;
        ss << "log directory " << afl::file::FileUtil::dirName(m_pidFile.c_str()) << " illegal! "
           << std::endl;
        throw std::logic_error(ss.str());
    }

    int fd = ::open(m_pidFile.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0)
    {
        fprintf(stderr, "Create pid_file %s failed.\n", m_pidFile.c_str());
        return false;
    }

    FILE* file = ::fdopen(fd, "r+");
    if (file == NULL)
    {
        fprintf(stderr, "Open  pid_file %s failed.\n", m_pidFile.c_str());
        return false;
    }

    int pid = 0;
    if (::flock(fd, LOCK_EX | LOCK_NB) < 0)
    {
        int n = fscanf(file, "%d", &pid);
        fclose(file);
        if (n != 1)
        {
            fprintf(stderr, "Lock and read pid_file failed.\n");
        }
        else
        {
            m_pid = pid;
            //            fprintf(stderr, "Lock pid_file failed, lock is held by pid %d.\n", pid);
        }

        return true;
    }

    pid = ::getpid();
    if (!fprintf(file, "%d\n", pid))
    {
        fprintf(stderr, "Write pid %d to %s failed.\n", pid, m_pidFile.c_str());
        ::close(fd);
        return false;
    }
    fflush(file);

    m_pid = pid;
    return false;
}

} // namespace process
} // namespace afl
