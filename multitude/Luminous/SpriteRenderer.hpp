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

#include <Luminous/RenderContext.hpp>

namespace Luminous {

  /** Optimized 2D sprite renderer.

      This class can be used to draw a great number of things on the screen.


  */
  LUMINOUS_API class SpriteRenderer : public Patterns::NotCopyable
  {
  public:

    class Sprite
    {
    public:
      Sprite();

      Nimble::Vector2 m_location;
      Nimble::Vector2 m_velocity;
      Nimble::Vector4 m_color;
      float m_size;

    };

    SpriteRenderer();
    ~SpriteRenderer();

    void resize(size_t n);
    size_t spriteCount();
    Sprite * sprites();

    typedef std::vector<Sprite> SpriteVector;

    SpriteVector & spriteVector();

    void uploadSpritesToGPU(Luminous::RenderContext & r);
    void renderSprites(Luminous::RenderContext & r);

    void createFuzzyTexture(int dim, float centerDotSize = 0.25f,
                            float haloweight = 0.75f, float halodescent = 1.0f);

    void setBlendFunc(Luminous::RenderContext::BlendFunc f);
    void setVelocityScale(float velscale);
  private:

    class GPUData;
    class Internal;
    Internal * m_data;
  };

}

#endif // SPRITERENDERER_HPP
