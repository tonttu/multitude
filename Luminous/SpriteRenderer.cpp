/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "SpriteRenderer.hpp"

#include <Luminous/Image.hpp>
#include <Luminous/Program.hpp>
#include <Luminous/VertexDescription.hpp>

#include <QFileInfo>

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

      m_program.loadShader("cornerstone:Luminous/GLSL150/sprites.fs", Luminous::Shader::Fragment);
      m_program.loadShader("cornerstone:Luminous/GLSL150/sprites.vs", Luminous::Shader::Vertex);
      m_program.loadShader("cornerstone:Luminous/GLSL150/sprites.gs", Luminous::Shader::Geometry);

      Luminous::VertexDescription vdescr;
      vdescr.addAttribute<Nimble::Vector2f>("vertex_position");
      vdescr.addAttribute<Nimble::Vector2f>("vertex_velocity");
      vdescr.addAttribute<Nimble::Vector4f>("vertex_color");
      vdescr.addAttribute<float>("vertex_rotation");
      vdescr.addAttribute<float>("vertex_size");
      m_program.setVertexDescription(vdescr);
      m_varray.addBinding(m_vbo, vdescr);

      // Particles should always pass Z-test since they're drawn on the same depth
      m_depthMode.setFunction(DepthMode::ALWAYS);
    }

    void updateData()
    {
      m_vbo.setData(m_sprites.data(), m_sprites.size() * sizeof(Sprite), Buffer::DYNAMIC_DRAW);
    }

    void createFuzzyTexture(int dim, float centerDotSize,
                            float haloweight, float halodescent)
    {
      m_image.allocate(dim, dim, Luminous::PixelFormat::rgbaUByte());
      Nimble::Vector2 center(dim * 0.5f, dim * 0.5f);
      float invscale = 1.0f / center.x;

      haloweight = Nimble::Math::Clamp(haloweight * 255.5f, 0.0f, 255.1f);

      // Specify the texture in post-multiplied format
      Radiant::Color pixel(1,1,1,0);

      for(int y = 0; y < dim; y++) {
        for(int x = 0; x < dim; x++) {
          float d = (Nimble::Vector2(x, y) - center).length() * invscale;
          if(d >= 1.0f) {
            pixel.setAlpha(0.f);
          }
          else {
            if(d < centerDotSize)
              pixel.setAlpha(1.f);
            else
              pixel.setAlpha(((haloweight *
                powf((cosf(d * Nimble::Math::PI) * 0.5f + 0.5f), halodescent))) / 255.f);
          }
          m_image.setPixel(x,y, pixel.toVector());
        }
      }

      // Convert the image to pre-multiplied format for texturing
      m_image.toPreMultipliedAlpha();
      m_texture["tex"] = &m_image.texture();
    }

    std::map<QByteArray, const Texture *> m_texture;
    Luminous::Image m_image;

    SpriteRenderer::SpriteVector m_sprites;

    Luminous::Program m_program;
    Luminous::VertexArray m_varray;
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

    rc.setBlendMode(m_d->m_blendMode);
    rc.setDepthMode(m_d->m_depthMode);

    bool transparent = m_d->m_texture["tex"]->dataFormat().hasAlpha();

    auto b = rc.render<Sprite, D::SpriteUniform>(transparent, Luminous::PRIMITIVE_POINT, 0, static_cast<int>(spriteCount()), 1.f,
                                                  m_d->m_varray, m_d->m_program, &m_d->m_texture);
    b.uniform->velocityScale = m_d->m_velocityScale;
    b.uniform->depth = b.depth;

    rc.viewTransform().transpose(b.uniform->projMatrix);
    rc.transform().transpose(b.uniform->modelMatrix);

    rc.setBlendMode(Luminous::BlendMode::Default());
    rc.setDepthMode(Luminous::DepthMode::Default());
  }

  void SpriteRenderer::uploadData()
  {
    m_d->updateData();
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
