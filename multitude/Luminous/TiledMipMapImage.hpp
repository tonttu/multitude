/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_TILED_MIPMAP_IMAGE_HPP
#define LUMINOUS_TILED_MIPMAP_IMAGE_HPP

#include <Luminous/BGThread.hpp>
#include <Luminous/Image.hpp>
#include <Luminous/Texture.hpp>

#include <Nimble/Rect.hpp>
#include <Radiant/RefPtr.hpp>

namespace Luminous
{
  /** A tiled mipmap image. This type of images can be useful for
      displaying map information etc.

      This class is experimental, and it may change yet.
      
      @author Tommi Ilmonen */
  class TiledMipMapImage
  {
    class Loader : public Loadable
    {
      public:

        Loader(const char * file) : Loadable(file) {}
        virtual ~Loader() {}
        virtual void load()
        {
          m_state = Loadable::LOADING;
          bool ok = m_image.read(m_filepath.c_str());
          if(!ok) {
            m_state = Loadable::FAILURE;
          }
          else {
            m_state = Loadable::LOADED;
          }
        }

        Image m_image;
    };

    class Tile
    {
      public:
        Tile(BGThread * thread = 0)
          : m_thread(thread), m_loader(0), m_failed(false), m_level(0)
        { m_area.clear(); }

        /** Render this tile.

          @return Returns true is the tile was rendered or if the tile
          does not need to be rendered.*/
        bool render(float level,
            Nimble::Rect area,
            TiledMipMapImage * host);
        void startLoading(TiledMipMapImage * host);
        void handleLoading(TiledMipMapImage * host);

        BGThread * m_thread;
        Loader * m_loader;
        bool     m_failed;
        Image    m_image;
        int      m_level;

        Radiant::RefPtr<Texture2D> m_texture;
        Radiant::RefPtr<Tile>      m_children[2][2];

        Nimble::Rect m_area;
    };

    TiledMipMapImage();
    ~TiledMipMapImage();

    void init(const char * imgpath, int tilesize, int tilen, int levels);

    void render(Nimble::Rect area, float level);
    const char * file() { return m_file.c_str(); }
    protected:
    int m_levels;
    std::string m_file;
    Tile m_root;
  };
}


#endif
