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

#include "SpriteRenderer.hpp"

#include <Luminous/Image.hpp>
#include <Luminous/Program.hpp>
#include <Luminous/VertexDescription.hpp>

#include <Radiant/ResourceLocator.hpp>

namespace Luminous {

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  class SpriteRenderer::D
  {
  public:
    struct SpriteUniform
    {
      Nimble::Matrix4f projMatrix;
      Nimble::Matrix4f modelMatrix;
      float velocityScale;
      float depth;
    };

    D()
      : m_blendMode(Luminous::BlendMode::Default())
      , m_velocityScale(0.0f)
    {
      // Build a texture that will be used in this widget
      /// @todo Share default texture between instances of the renderer
      createFuzzyTexture(64, 0.5f, 0.5f, 0.5f);
    }

    void createFuzzyTexture(int dim, float centerDotSize,
                            float haloweight, float halodescent)
    {
      m_defaultImage.allocate(dim, dim, Luminous::PixelFormat::rgbaUByte());
      Nimble::Vector2 center(dim * 0.5f, dim * 0.5f);
      float invscale = 1.0f / center.x;

      haloweight = Nimble::Math::Clamp(haloweight * 255.5f, 0.0f, 255.1f);

      Nimble::Vector4f pixel(1,1,1,0);

      for(int y = 0; y < dim; y++) {
        for(int x = 0; x < dim; x++) {
          float d = (Nimble::Vector2(x, y) - center).length() * invscale;
          if(d >= 1.0f) {
            pixel.w = 0.f;
          }
          else {
            if(d < centerDotSize)
              pixel.w = 1.f;
            else
              pixel.w = ((haloweight *
                powf((cosf(d * Nimble::Math::PI) * 0.5f + 0.5f), halodescent))) / 255.f;
          }
          m_defaultImage.setPixel(x,y, pixel);
        }
      }

      m_texture["tex"] = &m_defaultImage.texture();
    }

    std::map<QByteArray, const Texture *> m_texture;
    Luminous::Image m_defaultImage;

    SpriteRenderer::SpriteVector m_sprites;

    Luminous::Program m_program;
    Luminous::VertexDescription m_vdescr;
    Luminous::Buffer m_vbo;

    Luminous::BlendMode m_blendMode;
    Luminous::DepthMode m_depthMode;
    float m_velocityScale;
  };
  
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  SpriteRenderer::SpriteRenderer()
    : m_d(new D())
  {
    QString shaderPath = Radiant::ResourceLocator::instance().locate("Luminous/GLSL150");

    if(shaderPath.isEmpty()) {
      Radiant::error("SpriteRenderer::SpriteRenderer # Could not locate shaders");
    }
    else {
      m_d->m_program.loadShader(shaderPath + "/sprites.fs", Luminous::ShaderGLSL::Fragment);
      m_d->m_program.loadShader(shaderPath + "/sprites.vs", Luminous::ShaderGLSL::Vertex);
      m_d->m_program.loadShader(shaderPath + "/sprites.gs", Luminous::ShaderGLSL::Geometry);

      m_d->m_vdescr.addAttribute<Nimble::Vector2f>("vertex_position");
      m_d->m_vdescr.addAttribute<Nimble::Vector2f>("vertex_velocity");
      m_d->m_vdescr.addAttribute<Nimble::Vector4f>("vertex_color");
      m_d->m_vdescr.addAttribute<float>("vertex_rotation");
      m_d->m_vdescr.addAttribute<float>("vertex_size");
      m_d->m_program.setVertexDescription(m_d->m_vdescr);
    }

    m_d->m_depthMode.setFunction(DepthMode::LessEqual);
  }

  SpriteRenderer::~SpriteRenderer()
  {
    delete m_d;
  }

  void SpriteRenderer::resize(size_t n)
  {
    m_d->m_sprites.resize(n);
  }

  size_t SpriteRenderer::spriteCount() const
  {
    return m_d->m_sprites.size();
  }

  SpriteRenderer::SpriteVector & SpriteRenderer::sprites()
  {
    return m_d->m_sprites;
  }
  
  void SpriteRenderer::render(Luminous::RenderContext & rc) const
  {
    /// @todo If this gets too slow we can always convert this to use a persistent vertex buffer instead of always creating this anew
    bool translucent = true;

    auto b = rc.render<Sprite, D::SpriteUniform>(translucent, Luminous::PrimitiveType_Point, 0, spriteCount(), 1.f,
    m_d->m_program, m_d->m_texture);

    b.uniform->velocityScale = m_d->m_velocityScale;
    b.uniform->depth = b.depth;

    b.command->blendMode = m_d->m_blendMode;
    b.command->depthMode = m_d->m_depthMode;
    auto v = b.vertex;

    // Copy vertex data
    std::copy(m_d->m_sprites.begin(), m_d->m_sprites.end(), v);
  }

  void SpriteRenderer::setImage(const Luminous::Image & image)
  {
    m_d->m_texture["tex"] = &image.texture();
  }

  void SpriteRenderer::createFuzzyTexture(int dim, float centerDotSize,
                                          float haloweight, float halodescent)
  {
    m_d->createFuzzyTexture(dim, centerDotSize, haloweight, halodescent);
  }

  void SpriteRenderer::setBlendMode(const Luminous::BlendMode & mode)
  {
    m_d->m_blendMode = mode;
  }

  const Luminous::BlendMode & SpriteRenderer::blendMode() const
  {
    return m_d->m_blendMode;
  }

  void SpriteRenderer::setVelocityScale(float velscale)
  {
    m_d->m_velocityScale = velscale;
  }

  float SpriteRenderer::velocityScale() const
  {
    return m_d->m_velocityScale;
  }
}
