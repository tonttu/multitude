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
	  if(path.empty()) {
		  error("ResourceLocator::addPath # attempt to add an empty path");
		  return;
	  }

	if(m_paths.empty())
      m_paths = path;
    else {
      std::ostringstream os;

      if(front)
		os << path << separator << m_paths;
      else
		os << m_paths << separator << path;
      
      m_paths = os.str();
    }
  }

  void ResourceLocator::addModuleDataPath(const QString & module,
					  bool front)
  {
    QString p1 = 
      PlatformUtils::getModuleUserDataPath(module.c_str(), false);
    QString p2 =
      PlatformUtils::getModuleGlobalDataPath(module.c_str(), false);

    addPath(p1 + separator + p2, front);
  }

  QString ResourceLocator::locate(const QString & file) const
  {
    if(file.empty()) return file;

    QString r = FileUtils::findFile(file, m_paths);

    /*if(r.empty()) {
      Radiant::trace(WARNING, "ResourceLocator::locate # couldn't locate %s", file.c_str());
    }
   */
    return r;
  }

  QString ResourceLocator::locateWriteable(const QString & file) const
  {
    StringUtils::StringList list;
    StringUtils::split(m_paths, ";", list, true);

    for(StringUtils::StringList::iterator it = list.begin(); it != list.end(); it++) {
      const QString path = (*it) + "/" + file;

      FILE * file = fopen(path.c_str(), "w");
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
