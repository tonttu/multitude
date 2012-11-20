#include "ColorCorrectionFilter.hpp"

#include <Luminous/RGBCube.hpp>
#include <Luminous/Shader.hpp>
#include <Luminous/Texture2.hpp>
#include <Luminous/VertexDescription.hpp>

namespace Luminous
{
  class ColorCorrectionFilter::D
  {
  public:
    D()
      : m_rgbCube(nullptr)
    {
      m_shader.loadShader("Luminous/GLSL150/tex.vs", Luminous::ShaderGLSL::Vertex);
      m_shader.loadShader("Luminous/GLSL150/cc_rgb.fs", Luminous::ShaderGLSL::Fragment);

      Luminous::VertexDescription desc;

      desc.addAttribute<Nimble::Vector2f>("vertex_position");
      desc.addAttribute<Nimble::Vector2>("vertex_uv");

      m_shader.setVertexDescription(desc);
    }

    virtual ~D()
    {}

    const Luminous::RGBCube * m_rgbCube;
    Luminous::Program m_shader;
  };

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  ColorCorrectionFilter::ColorCorrectionFilter()
    : m_d(new D())
  {
  }

  ColorCorrectionFilter::~ColorCorrectionFilter()
  {
    delete m_d;
  }

  const Luminous::RGBCube * ColorCorrectionFilter::rgbCube() const
  {
    return m_d->m_rgbCube;
  }

  void ColorCorrectionFilter::begin(Luminous::RenderContext & rc)
  {
    PostProcessFilter::begin(rc);

    // Get the rgbCube for the current area
    m_d->m_rgbCube = &rc.area()->rgbCube();
  }

  Luminous::Style ColorCorrectionFilter::style() const
  {
    Luminous::Style s = PostProcessFilter::style();
    const Luminous::RGBCube * cube = rgbCube();

    if(cube != nullptr && cube->isDefined()) {
      s.setFillProgram(m_d->m_shader);
      s.setTexture("lut", cube->asTexture());
    } else {
      debugLuminous("ColorCorrectionFilter # No RGBCube defined for current area. "
                    "Using default shader");
    }

    return s;
  }
}
