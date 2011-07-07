#include "LockFile.hpp"

#define NOMINMAX
#define WIN32_MEAN_AND_LEAN
#include <windows.h>

namespace Radiant
{
  class LockFile_Impl
  {
  public:
    LockFile_Impl(const char * filename)
    {
      m_fd = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    ~LockFile_Impl()
    {
      CloseHandle(m_fd);
    }

    bool isLocked() const
    {
      return (m_fd != INVALID_HANDLE_VALUE);
    }
  private:
    HANDLE m_fd;
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
