/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_HARDWARECOLORCORRECTION_HPP
#define LUMINOUS_HARDWARECOLORCORRECTION_HPP

#include "Export.hpp"

#include <Patterns/NotCopyable.hpp>
#include <Radiant/Singleton.hpp>

/// @cond

namespace Luminous
{

  class ColorCorrection;

  // This class is internal to MultiTouch Ltd. Do not use this class.
  // It will be removed in future revisions.
  class LUMINOUS_API HardwareColorCorrection : public Patterns::NotCopyable
  {
    DECLARE_SINGLETON(HardwareColorCorrection);
  public:
    HardwareColorCorrection();
    ~HardwareColorCorrection();
    void syncWith(ColorCorrection * cc);
    bool ok() const;

  private:
    class Private;
    Private * m_p;
  };

}

/// @endcond

#endif // LUMINOUS_HARDWARECOLORCORRECTION_HPP
