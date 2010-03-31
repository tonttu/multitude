/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */
#ifndef RADIANT_SIZE2D_HPP
#define RADIANT_SIZE2D_HPP

#include <Nimble/Vector2.hpp>

#include <Radiant/Export.hpp>

/// @todo These two functions are kinda orphaned here. Maybe they should be
/// moved to Nimble.

namespace Radiant
{

  /// Given a size and target size, return a new size optionally preserving the
  /// original aspect ratio
  RADIANT_API Nimble::Vector2i resize(Nimble::Vector2i size, Nimble::Vector2i newSize, bool keepAspect);

  /// Given aspect ratio and size to fit, return the maximum size with the
  /// aspect ratio that still fits within the constraint
  /// @param aspect aspect ratio that must be maintained
  /// @param constraint size to fit to
  RADIANT_API Nimble::Vector2i fitToSize(float aspect, Nimble::Vector2i constraint);

}

#endif
