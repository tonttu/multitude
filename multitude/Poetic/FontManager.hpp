/* COPYRIGHT
 */
#ifndef POETIC_FONT_MANAGER_HPP
#define POETIC_FONT_MANAGER_HPP

#include "Export.hpp"
#include "CPUFont.hpp"
#include "GPUFont.hpp"
#include "CPUBitmapFont.hpp"
#include "CPUWrapperFont.hpp"

#include <Radiant/Singleton.hpp>

#include <Luminous/VertexBuffer.hpp>

namespace Poetic
{

/** FontManager provides high level access to fonts that need to be scaled
  during runtime. It provides access to managed fonts that internally use
  glyphs rendered at different point sizes to improve rendered text quality.
*/
  class POETIC_API FontManager
  {
    DECLARE_SINGLETON(FontManager);
    public:
    ~FontManager();

    /// Returns a font that matches the given name
    CPUWrapperFont * getFont(const QString & name);
    /// Returns a default font.
    /// @returns Currently this method tries to return the basic DejaVuSans font
    CPUWrapperFont * getDefaultFont();

      /// Locates a file by searching through the FontManager's resource paths
      QString locate(const QString & name);
      /// Returns the ResourceLocator of the FontManager
      Radiant::ResourceLocator & locator();

      /// @cond
      Luminous::VertexBuffer * fontVBO(GLuint textureId);
      /// @endcond

    private:
      FontManager();

      // filename -> cpu font
      typedef std::map<QString, CPUManagedFont *> container;
      container m_managedFonts;

      /// @todo remove this and use the global locator
      Radiant::ResourceLocator m_locator;

      typedef std::map<GLuint, Luminous::VertexBuffer *> TextureVBOMap;
      TextureVBOMap m_vbos;

      Radiant::Mutex m_mutex;
  };

}

#endif
