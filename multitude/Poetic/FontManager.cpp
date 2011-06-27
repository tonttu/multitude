/* COPYRIGHT
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

  CPUWrapperFont * FontManager::getFont(const std::string & name)
  {
    Radiant::Guard g(m_mutex);

    if(name.empty()) {
      Radiant::error("FontManager::getFont # empty fontname");
      return 0;
    }

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

  CPUWrapperFont * FontManager::getDefaultFont()
  {
    return getFont("DejaVuSans.ttf");
  }

  std::string FontManager::locate(const std::string & name)
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

DEFINE_SINGLETON(Poetic::FontManager);
