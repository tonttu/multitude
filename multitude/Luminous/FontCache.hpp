/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_FONT_CACHE_HPP
#define LUMINOUS_FONT_CACHE_HPP

/// @cond

#include <Luminous/TextureAtlas.hpp>

class QRawFont;

namespace Luminous
{
  struct FontCacheSettings
  {
    bool enabled = true;
    int padding = 60;
    int maxHiresSize = 3072;
  };

  class FontCache
  {
  public:
    class Glyph : public TextureAtlasItem
    {
    public:
      Glyph()
        : m_location(0, 0)
        , m_size(0, 0)
      {}

      Texture & texture() { assert(m_atlas); return m_atlas->texture(); }
      Nimble::Vector2f location() const { return m_location; }
      Nimble::Vector2f size() const { return m_size; }
      std::array<Nimble::Vector2f, 4> uv() const { return m_uv; }

      bool isEmpty() const { return m_size.x <= 0.0f; }

      void setLocation(const Nimble::Vector2f & location) { m_location = location; }
      void setSize(const Nimble::Vector2f & size) { m_size = size; }

    private:
      Nimble::Vector2f m_location;
      Nimble::Vector2f m_size;
    };

  public:
    LUMINOUS_API ~FontCache();

    LUMINOUS_API Glyph * glyph(const QRawFont & rawFont, quint32 glyph);

    LUMINOUS_API float pixelSize() const;

    LUMINOUS_API static FontCache & acquire(const QRawFont & rawFont);

    LUMINOUS_API static int generation();

    LUMINOUS_API static void deinitialize();
    LUMINOUS_API static void init();

    /// @cond

    LUMINOUS_API static TextureAtlasGroup<Glyph> & atlas();
    LUMINOUS_API static Radiant::Mutex & atlasMutex();


    /// Set maximum size for rendered glyph bitmaps which are used to generate
    /// distance fields for actual rendering.
    ///
    /// Bigger values mean better quality and slower glyph generation. The
    /// setting is only taken into account when generating new glyphs, so any
    /// glyphs in the cache will not be regenerated.
    ///
    /// @param size maximum size, sensible range is around 300 - 5000
    LUMINOUS_API static void setMaximumGlyphHighResSize(int size);

    /// @param enabled should generated glyphs be cached to the filesystem
    LUMINOUS_API static void setGlyphPersistenceEnabled(bool enabled);
    /// @endcond

  private:
    FontCache(const QRawFont & rawFont);

  private:
    class GlyphGenerator;
    class FileCacheIndexLoader;
    class FileCacheLoader;
    class D;
    D * m_d;
  };
} // namespace Luminous

/// @endcond

#endif
