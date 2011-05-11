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
#ifndef POETIC_FONT_MANAGER_HPP
#define POETIC_FONT_MANAGER_HPP

#include <Poetic/Export.hpp>
#include <Poetic/CPUFont.hpp>
#include <Poetic/GPUFont.hpp>
#include <Poetic/CPUBitmapFont.hpp>
#include <Poetic/CPUWrapperFont.hpp>

#include <Patterns/Singleton.hpp>

#include <Luminous/VertexBuffer.hpp>

namespace Poetic
{

/** FontManager provides high level access to fonts that need to be scaled
  during runtime. It provides access to managed fonts that internally use
  glyphs rendered at different point sizes to improve rendered text quality.
*/
  class POETIC_API FontManager : public Patterns::Singleton<FontManager>
  {
    public:
      /// Returns a font that matches the given name
      CPUWrapperFont * getFont(const std::string & name);

      /// Locates a file by searching through the FontManager's resource paths
      std::string locate(const std::string & name);
      /// Returns the ResourceLocator of the FontManager
      Radiant::ResourceLocator & locator();

      /// @cond
      Luminous::VertexBuffer * fontVBO(GLuint textureId);
      /// @endcond

    private:
      FontManager();
      ~FontManager();

      // filename -> cpu font
      typedef std::map<std::string, CPUManagedFont *> container;
      container m_managedFonts;

      /// @todo remove this and use the global locator
      Radiant::ResourceLocator m_locator;

      typedef std::map<GLuint, Luminous::VertexBuffer *> TextureVBOMap;
      TextureVBOMap m_vbos;

      friend class Patterns::Singleton<FontManager>;

      Radiant::Mutex m_mutex;
  };

}

#endif
