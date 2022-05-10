/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_SPRITERENDERER_HPP
#define LUMINOUS_SPRITERENDERER_HPP

#include <Luminous/Image.hpp>
#include <Luminous/RenderContext.hpp>

namespace Luminous {

  /// This class implements a simple particle system.
  ///
  /// This class can be used to draw a great number of sprites on the screen.
  /// It relies on geometry, vertex, and pixels shaders to to increase its
  /// performance. Consequently it may not run on outdated or very low-end
  /// hardware.
  ///
  /// The maximum number of particles depends on the hardware, and the particle
  /// update logic. Typically the limiting factor is the CPU-based calculation
  /// of the particle parameters.
  class LUMINOUS_API SpriteRenderer : public Patterns::NotCopyable
  {
  public:

    /// Individual sprite
    struct Sprite
    {
      Sprite()
        : location(0,0)
        , velocity(0,0)
        , color(1.f, 1.f, 1.f, 1.f)
        , rotation(0.f)
        , size(10.f)
      {}

      /// Location of the sprite
      Nimble::Vector2f location;
      /// The velocity of the sprite
      /** The velocity information is used to implement motion blur/stretching. */
      Nimble::Vector2f velocity;
      /// The color of the sprite
      Radiant::ColorPMA color;
      /// The rotation of the sprite
      float rotation;
      /// The size (diameter) of the particle.
      float size;
    };

    /// Constructor
    SpriteRenderer();
    /// Destructor
    ~SpriteRenderer();

    /// Resize the sprite buffer
    void resize(size_t n);

    /// Returns the number of allocated sprites
    size_t spriteCount() const;

    /// The container type where the sprites are stored
    typedef std::vector<Sprite> SpriteVector;

    /// Return the vector containing the sprites
    SpriteVector & sprites();

    /// Renders the sprites
    void render(Luminous::RenderContext & r) const;

    /// Sets the texture that is used in the rendering process
    void setImage(const Luminous::Image & image);

    /// Create a blurry texture
    /// Creates a basic square texture with radial gradient pattern
    /// @param dim texture dimensions
    /// @param centerDotSize size of the opaque center dot
    /// @param haloweight weighting factor for the radial gradient
    /// @param halodescent factor for how fast the gradient drops to zero
    void createFuzzyTexture(int dim, float centerDotSize = 0.25f,
                            float haloweight = 0.75f, float halodescent = 1.0f);

    /// Set the blend mode used for rendering the particles.
    /// @param mode blend mode
    void setBlendMode(const Luminous::BlendMode & mode);

    /// Blend mode used during rendering.
    /// @return blend mode used
    const BlendMode & blendMode() const;

    /// Set the velocity scaling factor.
    /// @param velscale velocity scaling
    /// @sa velocityScale
    void setVelocityScale(float velscale);

    /// Velocity scaling factor is used to stretch the particles along the
    /// velocity vector during rendering. Set to zero to disable stretching.
    /// Default value is zero.
    /// @return velocity scaling factor
    float velocityScale() const;

    ///
    void uploadData();
  private:
    class D;
    D * m_d;
  };

}

#endif // SPRITERENDERER_HPP
