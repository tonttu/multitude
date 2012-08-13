/* COPYRIGHT
 *
 * This file is part of Effects.
 *
 * Copyright: MultiTouch Oy, Finland, http://multitouch.fi
 *
 * All rights reserved, 2007-2010
 *
 * You may use this file only for purposes for which you have a
 * specific, written permission from MultiTouch Oy.
 *
 * See file "Effects.hpp" for authors and more details.
 *
 */

#ifndef LUMINOUS_SPRITERENDERER_HPP
#define LUMINOUS_SPRITERENDERER_HPP

#include <Luminous/Image.hpp>
#include <Luminous/RenderContext.hpp>

namespace Luminous {

  /** Optimized 2D sprite renderer.

      This class can be used to draw a great number of sprites on the screen. SpriteRenderer relies on
      geometry-, vertex-, and pixels shaders to to increase its performance. Consequently it may not run on outdated or
      very low-end hardware.

      The maximum number of sprites depends on the hardware, and the sprite update logic. Typically
      the limiting factor is the CPU-based calculation of the sprite parameters. Usually
      the maximum number of particles would be between 100 000 and 1000 0000.

      The typical use pattern of SpriteRenderer is as follows:

      @code

      class MyClass
      {

      void update()
      {
        m_sprites.resize(100);
        Luminous::SpriteRenderer::Sprite * sprite = m_sprites.sprites();

        for(int i = 0; i < 100; i++)
          updateSprite(sprite); // Fill the sprite with proper values
          sprite++;
        }
      }

      void render(Luminous::RenderContext & r)
      {
        m_sprites.uploadSpritesToGPU(r);
        m_sprites.renderSprites(r);
      }

      private:
        Luminous::SpriteRenderer m_sprites;
      };

      @endcode
  */
  class LUMINOUS_API SpriteRenderer : public Patterns::NotCopyable
  {
  public:

    /// Individual sprite
    struct Sprite
    {
      Sprite::Sprite()
        : location(0,0,0)
        , velocity(0,0)
        , color(1.f, 1.f, 1.f, 1.f)
        , rotation(0.f)
        , size(10.f)
      {}

      // Location of the sprite
      Nimble::Vector3f location;
      /// The velocity of the sprite
      /** The velocity information is used to implement motion blur/stretching. */
      Nimble::Vector2f velocity;
      /// The color of the sprite
      Nimble::Vector4f color;
      /// The rotation of the sprite
      float rotation;
      /// The size (diameter) of the particle.
      float size;
    };

    SpriteRenderer();
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

    const Luminous::Image & image() const;

    /// Create a blurry texture
    /// Creates a basic square texture with radial gradient pattern
    /// @param dim texture dimensions
    /// @param centerDotSize size of the opaque center dot
    /// @param haloweight weighting factor for the radial gradient
    /// @param halodescent factor for how fast the gradient drops to zero
    void createFuzzyTexture(int dim, float centerDotSize = 0.25f,
                            float haloweight = 0.75f, float halodescent = 1.0f);

    /// Selects the blending function used for the sprites
    void setBlendMode(const Luminous::BlendMode & mode);

    const BlendMode & blendMode() const;

    /// Sets the velocity scaling factor
    /** @param velscale The scaling factor to be used for stretching sprites along the
        velocity vector during rendering. Value zero inhibits the velocity stretching. Default value
        is zero. */
    void setVelocityScale(float velscale);

    float velocityScale() const;
  private:

    class D;
    D * m_d;
  };

}

#endif // SPRITERENDERER_HPP
