/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_UNIFORM_DESCRIPTION_HPP
#define LUMINOUS_UNIFORM_DESCRIPTION_HPP

#include "Luminous/Luminous.hpp"

#include <QByteArray>
#include <vector>

namespace Luminous
{
  /// @cond
  class UniformDescription
  {
  public:
    LUMINOUS_API UniformDescription();

    struct UniformVar
    {
      QByteArray name;
      int offsetBytes;
      int sizeBytes;
    };

    LUMINOUS_API void addAttribute(const QByteArray & name, int offsetBytes, int sizeBytes);

    const std::vector<UniformVar> & attributes() const { return m_attributes; }

  private:
    std::vector<UniformVar> m_attributes;
  };
  /// @endcond
}

#endif // LUMINOUS_UNIFORM_DESCRIPTION_HPP
