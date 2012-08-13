#ifndef LUMINOUS_DISTANCE_FIELD_GENERATOR_HPP
#define LUMINOUS_DISTANCE_FIELD_GENERATOR_HPP

#include <Luminous/Luminous.hpp>

#include <Nimble/Vector2.hpp>

namespace Luminous
{
  namespace DistanceFieldGenerator
  {
    /// Generates (usually low-resolution) distance field from a (high-resolution) src image
    /// Both images should be grayscale 8 bit images
    /// @param radius search neighbourhood size in src texels
    LUMINOUS_API void generate(const Luminous::Image & src, Nimble::Vector2i srcSize,
                               Luminous::Image & target, int radius);
  }
}

#endif // LUMINOUS_DISTANCE_FIELD_GENERATOR_HPP
