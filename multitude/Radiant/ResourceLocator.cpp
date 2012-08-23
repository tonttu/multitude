/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */
#include "ResourceLocator.hpp"
#include "Platform.hpp"

#include <QAbstractFileEngineHandler>
#include <QStringList>
#include <QVector>

#include <sys/stat.h>
#include <sys/types.h>

#if defined(RADIANT_WINDOWS)
# define stat     _stat
# define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
# define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
# define S_IWRITE _S_IWRITE
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

    return ((buf.st_mode & S_IWRITE) == S_IWRITE);
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

}

namespace Radiant
{

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
      QStringList matches = locate(fileName, ResourceLocator::AllEntries);

      // If there was no match, return 0 meaning we can't handle this file;
      // otherwise return the default handler for the new filename
      if(matches.isEmpty())
        return 0;
      else
        return QAbstractFileEngine::create(matches.front());
    }

    QStringList locate(const QString & path, ResourceLocator::Filter filter) const
    {
      // Always check if the path exists before searching anything
      // Can't use Qt I/O here either (QFileInfo)
      if(rawPathExists(path))
        return QStringList() << path;

      QStringList result;

      foreach(const QString & searchPath, m_searchPaths) {
        const QString candidate = searchPath + "/" + path;

        // The file/directory should exist
        if(!rawPathExists(candidate))
          continue;

        // Apply filters
        if(filter & Files && !rawPathIsFile(candidate))
          continue;

        if(filter & Dirs && !rawPathIsDirectory(candidate))
          continue;

        if(filter & Writeable && !rawPathIsWriteable(candidate))
          continue;

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

//  void ResourceLocator::addModuleDataPath(const QString & module,
//					  bool front)
//  {
//    QString p1 =
//      PlatformUtils::getModuleUserDataPath(module.toUtf8().data(), false);
//    QString p2 =
//      PlatformUtils::getModuleGlobalDataPath(module.toUtf8().data(), false);

//    addPath(p1 + separator + p2, front);
//  }

  DEFINE_SINGLETON(ResourceLocator);
}
