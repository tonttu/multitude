/* COPYRIGHT
 *
 * This file is part of Poetic.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Poetic.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "FontManager.hpp"
#include "CPUManagedFont.hpp"

#include <Radiant/Trace.hpp>

#include <map>
#include <string>

namespace Poetic
{

  FontManager::FontManager()
  {    
    m_locator.addPath("../../share/MultiTouch/Fonts");

    // Add platform specific paths 
    m_locator.addPath(
#ifdef WIN32
    /// @todo Windows might not be installed on drive C...
    "C:/Windows/Fonts"
#elif __linux__
    "/usr/share/fonts/truetype/ttf-dejavu"
#else
    // Mac OSX
    "/Library/Frameworks/MultiTouch.framework/Fonts"
#endif
  );
    m_locator.addPath(".");
  }

  FontManager::~FontManager()
  {}

  CPUWrapperFont * FontManager::getFont(const std::string & name)
  {
    assert(!name.empty());
    
    /*
    if(name.empty()) {
      Radiant::error("FontManager::getFont # empty fontname");
      return 0;
    }
    */

    container::iterator it = m_managedFonts.find(name);

    CPUManagedFont * mfont = 0;

    std::string dirredname("Configs/");
    dirredname += name;

    if(it == m_managedFonts.end()) {

      const std::string path = m_locator.locate(name);
      if(path.empty()) {
        Radiant::error("FontManager::getFont # failed to locate font \"%s\"",
		       name.c_str());
        return 0;
      }
  
      // Need to create a new managed font
      mfont = new CPUManagedFont();
      m_managedFonts[name] = mfont;

      if(!mfont->load(path.c_str())) {
        Radiant::error(
		       "FontManager::getFont # failed to load '%s'",
		       path.c_str());
        return 0;
      }
    }
    else
      mfont = it->second;

    return new CPUWrapperFont(mfont);
  }

  std::string FontManager::locate(const std::string & name)
  {
    return m_locator.locate(name);
  }

  Radiant::ResourceLocator & FontManager::locator()
  {
    return m_locator;
  }

}

