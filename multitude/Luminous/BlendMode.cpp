/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Luminous/BlendMode.hpp"

namespace Luminous
{
  BlendMode::BlendMode()
    : m_equation(ADD)
    , m_srcFunction(ONE)
    , m_dstFunction(ONE_MINUS_SOURCE_ALPHA)
  {
  }

  BlendMode::BlendMode(BlendMode::Equation equation, BlendMode::Function srcFunc, BlendMode::Function dstFunc)
  {
    m_equation = equation;
    m_srcFunction = srcFunc;
    m_dstFunction = dstFunc;
  }

  BlendMode BlendMode::Additive()
  {
    BlendMode mode;
    mode.setSourceFunction(SOURCE_ALPHA);
    mode.setDestFunction(ONE);
    mode.setEquation(ADD);
    return mode;
  }

  BlendMode BlendMode::Subtractive()
  {
    BlendMode mode;
    mode.setSourceFunction(SOURCE_ALPHA);
    mode.setDestFunction(ONE_MINUS_SOURCE_ALPHA);
    mode.setEquation(REVERSE_SUBTRACT);
    return mode;
  }
}
