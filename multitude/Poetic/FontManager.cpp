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
#include <QString>

namespace Poetic
{

  FontManager::FontManager()
  {
    Radiant::Guard g(m_mutex);

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
    "/Library/Frameworks/MultiTouch.framework/data/Fonts"
#endif
  );
    m_locator.addPath(".");
  }

  FontManager::~FontManager()
  {
    Radiant::Guard g(m_mutex);

    for(TextureVBOMap::iterator it = m_vbos.begin(); it != m_vbos.end(); it++)
      delete it->second;
  }

  CPUWrapperFont * FontManager::getFont(const QString & name)
  {
    Radiant::Guard g(m_mutex);

    if(name.isEmpty()) {
      Radiant::error("FontManager::getFont # empty fontname");
      return 0;
    }

    container::iterator it = m_managedFonts.find(name);

    CPUManagedFont * mfont = 0;

    if(it == m_managedFonts.end()) {

      const QString path = m_locator.locate(name);
      if(path.isEmpty()) {
        Radiant::error("FontManager::getFont # failed to locate font \"%s\"",
           name.toUtf8().data());
        return 0;
      }
  
      // Need to create a new managed font
      mfont = new CPUManagedFont();
      m_managedFonts[name] = mfont;

      if(!mfont->load(path.toUtf8().data())) {
        Radiant::error(
		       "FontManager::getFont # failed to load '%s'",
           path.toUtf8().data());
        return 0;
      }
    }
    else
      mfont = it->second;

    return new CPUWrapperFont(mfont);
  }

  QString FontManager::locate(const QString & name)
  {
    Radiant::Guard g(m_mutex);

    return m_locator.locate(name);
  }

  Radiant::ResourceLocator & FontManager::locator()
  {
    Radiant::Guard g(m_mutex);

    return m_locator;
  }

  Luminous::VertexBuffer * FontManager::fontVBO(GLuint textureId)
  {
    Radiant::Guard g(m_mutex);

    TextureVBOMap::iterator it = m_vbos.find(textureId);

    // No vbo found, create new
    if(it == m_vbos.end()) {
      std::pair<TextureVBOMap::iterator, bool> tmp = m_vbos.insert(std::make_pair(textureId, new Luminous::VertexBuffer()));
      it = tmp.first;

      it->second->allocate(4 * sizeof(GLfloat) * 1024, Luminous::VertexBuffer::DYNAMIC_DRAW);

      Radiant::info("CREATE NEW FONT VBO");
    }

    return it->second;
  }



}

