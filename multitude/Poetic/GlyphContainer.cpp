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
#include "GlyphContainer.hpp"

namespace Poetic
{

  GlyphContainer::GlyphContainer(Face * face)
  : m_face(face),
    m_error(0)
  {
    m_glyphs.push_back(0);
    m_charmap = new Charmap(m_face);
  }

  GlyphContainer::~GlyphContainer()
  {
    while(!m_glyphs.empty()) {
      delete m_glyphs.front();
      m_glyphs.erase(m_glyphs.begin());
    }

    delete m_charmap;
  }

  void GlyphContainer::add(Glyph * glyph, unsigned int charCode)
  {
    m_charmap->insertIndex(charCode, m_glyphs.size());
    m_glyphs.push_back(glyph);
  }

  float GlyphContainer::advance(unsigned int charCode, unsigned int nextCharCode)
  {
    unsigned int left = m_charmap->fontIndex(charCode);
    unsigned int right = m_charmap->fontIndex(nextCharCode);

    float width = m_face->kernAdvance(left, right).x;
    width += m_glyphs[m_charmap->glyphListIndex(charCode)]->advance().x;

    return width;
  }

  BBox GlyphContainer::bbox(unsigned int charCode) const
  {
    return m_glyphs[m_charmap->glyphListIndex(charCode)]->bbox();
  }

  bool GlyphContainer::charMap(int encoding)
  {
    bool result = m_charmap->charmap(encoding);
    m_error = m_charmap->error();
    return result;
  }

  unsigned int GlyphContainer::fontIndex(unsigned int charCode) const 
  {
    return m_charmap->fontIndex(charCode);  
  }

  const Glyph * GlyphContainer::glyph(unsigned int charCode) const
  {
    int index = m_charmap->glyphListIndex(charCode);
    return m_glyphs[index];
  }  

  Nimble::Vector2 GlyphContainer::render(unsigned int charCode, unsigned int nextCharCode, Nimble::Vector2 penPos, const Nimble::Matrix3 & m)
  {
    Nimble::Vector2 advance(0.f, 0.f);

    unsigned int left = m_charmap->fontIndex(charCode);
    unsigned int right = m_charmap->fontIndex(nextCharCode);

    Nimble::Vector2 kernAdvance = m_face->kernAdvance(left, right);

    if(!m_face->error()) {
      unsigned int index = m_charmap->glyphListIndex(charCode);
      Glyph * glyph = m_glyphs[index];
      assert(glyph);
      advance = glyph->render(penPos, m);
    }

    kernAdvance += advance; 
  
    return kernAdvance;
  }

}

