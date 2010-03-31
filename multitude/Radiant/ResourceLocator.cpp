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

#include <Radiant/Endian.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/PlatformUtils.hpp>
#include <Radiant/StringUtils.hpp>
#include <Radiant/Trace.hpp>

#include <sstream>

namespace Radiant
{

  ResourceLocator ResourceLocator::s_instance;

  std::string   ResourceLocator::separator = ";";

  ResourceLocator::ResourceLocator()
  {}

  ResourceLocator::~ResourceLocator()
  {}

  void ResourceLocator::addPath(const std::string & path, bool front)
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

  void ResourceLocator::addModuleDataPath(const std::string & module,
					  bool front)
  {
    std::string p1 = 
      PlatformUtils::getModuleUserDataPath(module.c_str(), false);
    std::string p2 =
      PlatformUtils::getModuleGlobalDataPath(module.c_str(), false);

    addPath(p1 + separator + p2, front);
  }

  std::string ResourceLocator::locate(const std::string & file) const
  {
    std::string r = FileUtils::findFile(file, m_paths);

    if(r.empty()) 
      Radiant::trace(WARNING, "ResourceLocator::locate # couldn't locate %s", file.c_str());

    return r;
  }

  std::string ResourceLocator::locateWriteable(const std::string & file) const
  {
    StringUtils::StringList list;
    StringUtils::split(m_paths, ";", list, true);

    for(StringUtils::StringList::iterator it = list.begin(); it != list.end(); it++) {
      const std::string path = (*it) + "/" + file;

      FILE * file = fopen(path.c_str(), "w");
      if(file) {
        fclose(file);
        return path;
      }      
    }

    return std::string();
  }

  std::string ResourceLocator::locateOverWriteable(const std::string & file)
    const
  {
    return FileUtils::findOverWritable(file, m_paths);
  }

}
