/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "GraphicsCoordinates.hpp"

namespace Valuable
{
  GraphicsCoordinates::GraphicsCoordinates(Valuable::Node * host, const QByteArray & name)
    : Node(host, name)
  {
    eventAddOut("graphics-bounds-changed");
  }

  GraphicsCoordinates::~GraphicsCoordinates()
  {
  }
}
