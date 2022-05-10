/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_DISTANCE_FIELD_GENERATOR_HPP
#define LUMINOUS_DISTANCE_FIELD_GENERATOR_HPP

#include <Luminous/Luminous.hpp>

#include <Nimble/Vector2.hpp>

/// @cond
namespace Luminous
{
  class DistanceFieldGenerator
  {
  public:
    /// Generates (usually low-resolution) distance field from a (high-resolution) src image
    /// Both images should be grayscale 8 bit images
    /// @param radius search neighbourhood size in src texels
    static  void LUMINOUS_API generate(const Luminous::Image & src, Nimble::Vector2i srcSize,
                               Luminous::Image & target, int radius);
  };
}
/// @endcond

#endif // LUMINOUS_DISTANCE_FIELD_GENERATOR_HPP
