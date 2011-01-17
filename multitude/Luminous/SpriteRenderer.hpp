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
  LUMINOUS_API class SpriteRenderer : public Patterns::NotCopyable
  {
  public:

    /// Individual sprite
    class Sprite
    {
    public:
      Sprite();
      /// The location of the sprite
      Nimble::Vector2 m_location;
      /// The velocity of the sprite
      /** The velocity information is used to implement motion blur/stretching. */
      Nimble::Vector2 m_velocity;
      /// The color of the sprite
      Nimble::Vector4 m_color;
      /// The size (diameter) of the particle.
      float m_size;
    };

    SpriteRenderer();
    ~SpriteRenderer();

    /// Resize the sprite buffer
    void resize(size_t n);
    /// Returns the number of allocated sprites
    size_t spriteCount();
    /// A pointer to the sprites
    Sprite * sprites();

    /// The container type where the sprites are stored
    typedef std::vector<Sprite> SpriteVector;

    /// Return the vector containing the sprites
    SpriteVector & spriteVector();

    /// Uploads the current sprites to the GPU
    void uploadSpritesToGPU(Luminous::RenderContext & r);
    /// Renders the sprites
    void renderSprites(Luminous::RenderContext & r);
    /// Sets the texture that is used in the rendering process
    void setTexture(const Luminous::Image &);
    void createFuzzyTexture(int dim, float centerDotSize = 0.25f,
                            float haloweight = 0.75f, float halodescent = 1.0f);

    /// Selects the blending function used for the sprites
    void setBlendFunc(Luminous::RenderContext::BlendFunc f);
    /// Sets the velocity scaling factor
    /** @param velscale The scaling factor to be used for stretching sprites along the
        velocity vector during rendering. Value zero inhibits the velocity stretching. Default value
        is zero. */
    void setVelocityScale(float velscale);
  private:

    class GPUData;
    class Internal;
    Internal * m_data;
  };

}

#endif // SPRITERENDERER_HPP
