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
