/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#pragma once

namespace Luminous
{
  class RenderDriver;
  class RenderContext;
  class GfxDriver
  {
  public:
    virtual RenderContext & renderContext(unsigned int threadIndex) = 0;
    virtual unsigned int renderThreadCount() const = 0;
    virtual RenderDriver & renderDriver(unsigned int threadIndex) = 0;
  };
}
