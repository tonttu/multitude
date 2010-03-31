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
#include "CPUManagedFont.hpp"

#include <Radiant/Trace.hpp>

#define METRIC_FONT_POINT_SIZE 64

namespace Poetic
{

  // Maintain a user-defined list of the font-sizes i.e. [3,4,6,8,10,16,32//
  static int g_faceSizes[] = { 8, 16, 32, 64, 128};

  CPUManagedFont::CPUManagedFont()
  : m_metricFont(0)
  {}

  bool CPUManagedFont::load(const char * fontFilePath)
  {
    m_file = std::string(fontFilePath);

    bool ok = true;

    // Load the specified font sizes
    for(uint32_t i = 0; i < sizeof(g_faceSizes) / sizeof(g_faceSizes[0]); i++) {
      CPUBitmapFont * font = new CPUBitmapFont();

      ok = font->load(m_file.c_str());
      if(!ok) return false;
      ok = font->setFaceSize(g_faceSizes[i]);
      if(!ok) return false;
      m_fonts.push_back(font);
    }

    // Load the specified metric font
    CPUBitmapFont * bf = new CPUBitmapFont();
    ok = bf->load(m_file.c_str());
    assert(ok);
    ok = bf->setFaceSize(METRIC_FONT_POINT_SIZE);
    assert(ok);

    m_metricFont = bf;

    return ok;
  }

  uint32_t CPUManagedFont::selectFont(float request) 
  {
    assert(!m_fonts.empty());

    int r = static_cast<int> (request);
    int numFonts = static_cast<int> (m_fonts.size());

    for(int i = 0; i < numFonts; i++) {
      int s = m_fonts[i]->faceSize();

      if(s >= r) return i;
    }

    return numFonts - 1;
  }

  CPUFont * CPUManagedFont::getFont(int i) 
  {
    assert(i < (int)m_fonts.size());
    return m_fonts[i];
  }

}
