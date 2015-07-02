/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#include "ResourceLocator.hpp"
#include "Platform.hpp"

#include <QFSFileEngine>
#include <QStringList>
#include <QVector>

#include <sys/stat.h>
#include <sys/types.h>

#include <Radiant/Trace.hpp>

#if defined(RADIANT_WINDOWS)
# define stat     _stat
# define S_ISDIR(mode)   (((mode) & S_IFMT) == S_IFDIR)
# define S_ISREG(mode)   (((mode) & S_IFMT) == S_IFREG)
# define S_ISWRITE(mode) (((mode) & _S_IWRITE) == _S_IWRITE)
#else
# define S_ISWRITE(mode) (((mode) & S_IWRITE) == S_IWRITE)
#endif

namespace {

  // Helper function that does not use Qt I/O
  bool rawPathExists(const QString & path)
  {
    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    return (stat(path.toUtf8().data(), &buf) == 0);
  }

  bool rawPathIsWriteable(const QString & path)
  {
    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    if(stat(path.toUtf8().data(), &buf) != 0)
      return false;

    return S_ISWRITE(buf.st_mode);
  }

  bool rawPathIsDirectory(const QString & path)
  {
    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    if(stat(path.toUtf8().data(), &buf) != 0)
      return false;

    return S_ISDIR(buf.st_mode);
  }

  bool rawPathIsFile(const QString & path)
  {
    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    if(stat(path.toUtf8().data(), &buf) != 0)
      return false;

    return S_ISREG(buf.st_mode);
  }

  bool rawPathExistsAndMatches(const QString & path,
                               Radiant::ResourceLocator::Filter filter)
  {
    struct stat buf;
    memset(&buf, 0, sizeof(struct stat));

    if(stat(path.toUtf8().data(), &buf) != 0)
      return false;

    if((filter & Radiant::ResourceLocator::FILES) && !S_ISREG(buf.st_mode))
      return false;

    if((filter & Radiant::ResourceLocator::DIRS) && !S_ISDIR(buf.st_mode))
      return false;

    if((filter & Radiant::ResourceLocator::WRITEABLE) && !S_ISWRITE(buf.st_mode))
      return false;

    return true;
  }

}

namespace Radiant
{
  class ResourceLocatorFileEngine : public QFSFileEngine
  {
  public:
    ResourceLocatorFileEngine(const QString & file) : QFSFileEngine(file) {}
    // We are wrapping file that has absolute path, but from a user point
    // of view this is always relative, since the path user gave was relative.
    // Without this for example QFileInfo("style.css").isAbsolute() would
    // return true if there was style.css somewhere in the search path.
    virtual bool isRelativePath() const OVERRIDE { return true; }
  };

  class ResourceLocator::D : public QAbstractFileEngineHandler
  {
  public:
    QStringList m_searchPaths;

    QAbstractFileEngine * create(const QString &fileName) const
    {
      // Do not search ff the filename exists or defines an absolute path Note
      // that we can not use Qt I/O functions (QFileInfo) here because they
      // would cause infinite loop by calling this function themselves.
      if(rawPathExists(fileName))
        return 0;

      // Search for a match
      QStringList matches = locate(fileName, ResourceLocator::ALL_ENTRIES);

      // If there was no match, return 0 meaning we can't handle this file;
      // otherwise return our wrapped handler for the new filename
      if(matches.isEmpty())
        return 0;
      else
        return new ResourceLocatorFileEngine(matches.front());
    }

    QStringList locate(const QString & path, ResourceLocator::Filter filter) const
    {
      // Always check if the path exists before searching anything
      // Can't use Qt I/O here either (QFileInfo)
      if(rawPathExistsAndMatches(path, filter)) {
#ifdef RADIANT_UNIX
        // For executables, we may need to set the "./" explicitly
        if(path.startsWith("/"))
          return QStringList() << path;
        return QStringList() << QString("./") + path;
#else
        return QStringList() << path;
#endif
      }

      QStringList result;

      foreach(const QString & searchPath, m_searchPaths) {
        const QString candidate = searchPath + "/" + path;

        if(rawPathExistsAndMatches(candidate, filter))
          result.append(candidate);
      }

      return result;
    }


  };

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  ResourceLocator::ResourceLocator()
    : m_d(new D())
  {}

  ResourceLocator::~ResourceLocator()
  {
    delete m_d;
  }

  void ResourceLocator::addSearchPath(const QString &path, bool inFront)
  {
    addSearchPaths(QStringList() << path, inFront);
  }

  void ResourceLocator::addSearchPaths(const QStringList &paths, bool inFront)
  {
    if(inFront)
      m_d->m_searchPaths = paths + m_d->m_searchPaths;
    else
      m_d->m_searchPaths.append(paths);
  }

  const QStringList & ResourceLocator::searchPaths() const
  {
    return m_d->m_searchPaths;
  }

  QStringList ResourceLocator::locate(const QString &path, Filter filter) const
  {
    return m_d->locate(path, filter);
  }

  DEFINE_SINGLETON(ResourceLocator);
}
