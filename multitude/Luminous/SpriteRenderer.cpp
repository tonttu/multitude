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
#include <Luminous/Shader.hpp>

#include <Radiant/ResourceLocator.hpp>

namespace Luminous {

  using namespace Radiant;

  SpriteRenderer::Sprite::Sprite()
    : m_location(0,0)
    , m_velocity(1,0)
    , m_color(1, 1, 1, 1)
    , m_rotation(0.f)
    , m_size(10)
  {}

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  class SpriteRenderer::GPUData : public Luminous::GLResource
  {
  public:
    GPUData(Luminous::RenderContext * res)
      : m_vbo(res)
    {}

    Luminous::VertexBuffer m_vbo;
  };

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  class SpriteRenderer::Internal
  {
  public:

    Internal()
      : m_blendFunc(Luminous::RenderContext::BLEND_ADDITIVE)
      , m_velocityScale(0.0f)
    {
      // Build a texture that will be used in this widget
      /* Maybe different widgets should share the texture to save a bit of resources.
       We can do this optimization later.
    */
      createFuzzyTexture(64, 0.5f, 0.5f, 0.5f);
    }


    void createFuzzyTexture(int dim, float centerDotSize,
                            float haloweight, float halodescent)
    {
      std::vector<uint8_t> blob;
      blob.resize(dim * dim * 2);

      uint8_t * ptr = & blob[0];

      Nimble::Vector2 center(dim * 0.5f, dim * 0.5f);
      float invscale = 1.0f / center.x;

      haloweight = Nimble::Math::Clamp(haloweight * 255.5f, 0.0f, 255.1f);

      for(int y = 0; y < dim; y++) {
        for(int x = 0; x < dim; x++) {
          float d = (Nimble::Vector2(x, y) - center).length() * invscale;
          *ptr++ = 255;
          if(d >= 1.0f) {
            *ptr = 0;
          }
          else {
            if(d < centerDotSize)
              *ptr = 255;
            else
              *ptr = (uint8_t) (haloweight *
                                powf((cosf(d * Nimble::Math::PI) * 0.5f + 0.5f), halodescent));
          }
          ptr++;
        }
      }

      m_texture.fromData( & blob[0], dim, dim, Luminous::PixelFormat::redGreenUByte());
    }
    Luminous::ImageTex m_texture;
    std::vector<SpriteRenderer::Sprite> m_sprites;

    Luminous::Shader m_shader;

    Luminous::ContextVariableT<SpriteRenderer::GPUData> m_gpuData;
    Luminous::RenderContext::BlendFunc m_blendFunc;
    float m_velocityScale;
  };

  /*
  class VertexAttribArrayStep : public Patterns::NotCopyable
  {
  public:
    VertexAttribArrayStep(int pos, int elems, GLenum type, size_t stride,
                          size_t offset)
                            : m_pos(pos)
    {
      glEnableVertexAttribArray(pos);
      glVertexAttribPointer(pos, elems, type, GL_FALSE,
                            (GLsizei) stride, ((GLubyte *) 0) + offset);
    }

    ~VertexAttribArrayStep ()
    {
      glDisableVertexAttribArray(m_pos);
    }

  private:
    int m_pos;
  };
  */

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////


  SpriteRenderer::SpriteRenderer()
    : m_data(new Internal())
  {
    QString shaderPath = Radiant::ResourceLocator::instance().locate("SpriteRenderer/Shaders");

    if(shaderPath.isEmpty()) {
      error("SpriteRenderer::SpriteRenderer # Could not locate shaders");
    }
    else {
      m_data->m_shader.loadFragmentShader(shaderPath + "/sprites.fs");
      m_data->m_shader.loadVertexShader(shaderPath + "/sprites.vs");
      m_data->m_shader.loadGeometryShader(shaderPath + "/sprites.gs");
    }
  }

  SpriteRenderer::~SpriteRenderer()
  {
    delete m_data;
  }

  void SpriteRenderer::resize(size_t n)
  {
    m_data->m_sprites.resize(n);
  }

  size_t SpriteRenderer::spriteCount() const
  {
    return m_data->m_sprites.size();
  }

  SpriteRenderer::Sprite * SpriteRenderer::sprites()
  {
    return m_data->m_sprites.empty() ? 0 : & m_data->m_sprites[0];
  }

  SpriteRenderer::SpriteVector & SpriteRenderer::spriteVector()
  {
    return m_data->m_sprites;
  }

  void SpriteRenderer::uploadSpritesToGPU(Luminous::RenderContext & r)
  {
    GPUData & gld = m_data->m_gpuData.ref(r.resources());

    gld.m_vbo.fill(sprites(), spriteCount() * sizeof(Sprite), Luminous::VertexBuffer::DYNAMIC_DRAW);
  }

  void SpriteRenderer::renderSprites(Luminous::RenderContext & r)
  {
    Luminous::CustomOpenGL cgl(r);

    GPUData & gld = m_data->m_gpuData.ref(r.resources());

    if(!gld.m_vbo.filled())
      return; // Nothing to render

    Luminous::GLSLProgramObject * prog = m_data->m_shader.program();

    if(!prog)
      return;

    if(!prog->isLinked()) {
      prog->setProgramParameter(GL_GEOMETRY_INPUT_TYPE, GL_POINTS);
      prog->setProgramParameter(GL_GEOMETRY_OUTPUT_TYPE, GL_TRIANGLE_STRIP);
      prog->setProgramParameter(GL_GEOMETRY_VERTICES_OUT, 4);

      if(!prog->link()) {
        error("When linking program: %s", prog->linkerLog());
        return;
      }
    }

    m_data->m_texture.bind(r.resources());

    // Set the GLSL program parameters
    prog->bind();

    prog->setUniformFloat("velocityscale", m_data->m_velocityScale * 0.05f);
    prog->setUniformMatrix3("modelmatrix2d", r.transform());

    gld.m_vbo.bind();

    int lpos = prog->getAttribLoc("location");
    int vpos = prog->getAttribLoc("velocity");
    int cpos = prog->getAttribLoc("color");
    int spos = prog->getAttribLoc("size");
    int rpos = prog->getAttribLoc("rotation");

    r.setBlendFunc(m_data->m_blendFunc);
    r.useCurrentBlendMode();

    {
      Sprite tmp;

      VertexAttribArrayStep sl(lpos, 2, GL_FLOAT, GL_FALSE, sizeof(Sprite), offsetBytes(tmp.m_location, tmp));
      VertexAttribArrayStep sv(vpos, 2, GL_FLOAT, GL_FALSE, sizeof(Sprite), offsetBytes(tmp.m_velocity, tmp));
      VertexAttribArrayStep sc(cpos, 4, GL_FLOAT, GL_FALSE, sizeof(Sprite), offsetBytes(tmp.m_color, tmp));
      VertexAttribArrayStep ss(spos, 1, GL_FLOAT, GL_FALSE, sizeof(Sprite), offsetBytes(tmp.m_size, tmp));
      VertexAttribArrayStep rs(rpos, 1, GL_FLOAT, GL_FALSE, sizeof(Sprite), offsetBytes(tmp.m_rotation, tmp));

      size_t n = gld.m_vbo.filled() / sizeof(Sprite);

      glDrawArrays(GL_POINTS, 0, (GLsizei) n);

      // info("%d sprites rendered", (int) n);
    }

    gld.m_vbo.unbind();

    prog->unbind();
    r.setBlendFunc(Luminous::RenderContext::BLEND_USUAL);
  }

  void SpriteRenderer::setTexture(const Luminous::Image & image)
  {
    m_data->m_texture = image;
  }

  void SpriteRenderer::createFuzzyTexture(int dim, float centerDotSize,
                                          float haloweight, float halodescent)
  {
    m_data->createFuzzyTexture(dim, centerDotSize, haloweight, halodescent);
  }

  void SpriteRenderer::setBlendFunc(Luminous::RenderContext::BlendFunc f)
  {
    m_data->m_blendFunc = f;
  }

  void SpriteRenderer::setVelocityScale(float velscale)
  {
    m_data->m_velocityScale = velscale;
  }
}
