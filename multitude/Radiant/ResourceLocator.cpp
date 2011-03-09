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
#include <Radiant/ResourceLocator.hpp>

#include <Radiant/Trace.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/PlatformUtils.hpp>
#include <Radiant/StringUtils.hpp>
#include <Radiant/Trace.hpp>

#include <sstream>

#include <QStringList>

namespace Radiant
{

  ResourceLocator ResourceLocator::s_instance;

  QString   ResourceLocator::separator = ";";

  ResourceLocator::ResourceLocator()
  {}

  ResourceLocator::~ResourceLocator()
  {}

  void ResourceLocator::addPath(const QString & path, bool front)
  {
    if(path.isEmpty()) {
		  error("ResourceLocator::addPath # attempt to add an empty path");
		  return;
	  }

  if(m_paths.isEmpty())
      m_paths = path;
    else {
      if(front)
        m_paths = path + separator + m_paths;
      else
        m_paths += separator + path;
    }
  }

  void ResourceLocator::addModuleDataPath(const QString & module,
					  bool front)
  {
    QString p1 = 
      PlatformUtils::getModuleUserDataPath(module.toUtf8().data(), false);
    QString p2 =
      PlatformUtils::getModuleGlobalDataPath(module.toUtf8().data(), false);

    addPath(p1 + separator + p2, front);
  }

  QString ResourceLocator::locate(const QString & file) const
  {
    if(file.isEmpty()) return file;

    QString r = FileUtils::findFile(file, m_paths);

    /*if(r.empty()) {
      Radiant::trace(WARNING, "ResourceLocator::locate # couldn't locate %s", file.c_str());
    }
   */
    return r;
  }

  QString ResourceLocator::locateWriteable(const QString & file) const
  {
    foreach(QString str, m_paths.split(";")) {
      const QString path = str + "/" + file;

      FILE * file = fopen(path.toUtf8().data(), "w");
      if(file) {
        fclose(file);
        return path;
      }      
    }

    return QString();
  }

  QString ResourceLocator::locateOverWriteable(const QString & file)
    const
  {
    return FileUtils::findOverWritable(file, m_paths);
  }

}
