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
#ifndef POETIC_CPU_MANAGED_FONT_HPP
#define POETIC_CPU_MANAGED_FONT_HPP

#include <Poetic/CPUBitmapFont.hpp>

#include <Luminous/Collectable.hpp>

#include <vector>

namespace Poetic
{
  
  /// A managed font on the CPU that uses multiple fonts internally to provide
  /// better matches at various different scales.
  class CPUManagedFont : public Luminous::Collectable
  {
    public:
      CPUManagedFont();

      bool load(const char * fontFilePath);

      uint32_t selectFont(float pointSize);

      int fontCount() const { return static_cast<int> (m_fonts.size()); }
      CPUFont * getFont(int i);

      CPUFont * getMetricFont() { return m_metricFont; }

    private:
      typedef std::vector<CPUFont *> container;
  
      std::string m_file;
      container m_fonts;

      CPUFont * m_metricFont;      
 };


}

#endif
