/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "CullMode.hpp"

namespace Luminous
{

CullMode::CullMode()
  : m_enabled(false)
  , m_face(BACK)
{
}

CullMode::CullMode(bool enabled, Face face)
  : m_enabled(enabled)
  , m_face(face)
{}

}
