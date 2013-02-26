/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
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
      m_image.allocate(dim, dim, Luminous::PixelFormat::rgbaUByte());
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
          m_image.setPixel(x,y, pixel);
        }
      }

      m_texture["tex"] = &m_image.texture();
    }

    std::map<QByteArray, const Texture *> m_texture;
    Luminous::Image m_image;

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
    QStringList shaderPaths = Radiant::ResourceLocator::instance()->locate("Luminous/GLSL150");

    if(shaderPaths.isEmpty()) {
      Radiant::error("SpriteRenderer::SpriteRenderer # Could not locate shaders");
    }
    else {
      const QString shaderPath = shaderPaths.front();
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

    m_d->m_depthMode.setFunction(DepthMode::LESS_EQUAL);
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
    /// Nothing to render
    if (m_d->m_sprites.empty())
      return;

    /// @todo If this gets too slow we can always convert this to use a persistent vertex buffer instead of always creating this anew
    bool translucent = true;

    rc.setBlendMode(m_d->m_blendMode);
    rc.setDepthMode(m_d->m_depthMode);

    auto b = rc.render<Sprite, D::SpriteUniform>(translucent, Luminous::PRIMITIVE_POINT, 0, spriteCount(), 1.f,
                                                  m_d->m_program, &m_d->m_texture);

    b.uniform->velocityScale = m_d->m_velocityScale;
    b.uniform->depth = b.depth;

    auto v = b.vertex;

    // Copy vertex data
    std::copy(m_d->m_sprites.begin(), m_d->m_sprites.end(), v);

    rc.setBlendMode(Luminous::BlendMode::Default());
    rc.setDepthMode(Luminous::DepthMode::Default());
  }

  void SpriteRenderer::setImage(const Luminous::Image & image)
  {
    m_d->m_image = image;
    m_d->m_texture["tex"] = & m_d->m_image.texture();
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
