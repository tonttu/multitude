/* COPYRIGHT
 */

#include "LockFile.hpp"
#include <sys/file.h>
#include <unistd.h>

namespace Radiant
{
  class LockFile_Impl
  {
  public:
    LockFile_Impl(const char * filename)
    {
      m_fd = open(filename, O_CREAT, 0644);
      m_locked = (flock(m_fd, LOCK_EX | LOCK_NB) == 0);
    }

    ~LockFile_Impl()
    {
      flock(m_fd, LOCK_UN);
      close(m_fd);
    }

    bool isLocked() const
    {
      return m_locked;
    }
  private:
    int m_fd;
    bool m_locked;
  };

  LockFile::LockFile(const char * filename)
  {
    m_impl = new LockFile_Impl(filename);
  }

  LockFile::~LockFile()
  {
    delete m_impl;
  }

  bool LockFile::isLocked() const
  {
    return m_impl->isLocked();
  }
}
