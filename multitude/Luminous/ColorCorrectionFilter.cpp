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

    Luminous::Program m_shader;

    // This is only valid in begin/apply
    const MultiHead::Area * m_currentArea;
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

  void ColorCorrectionFilter::begin(Luminous::RenderContext & rc)
  {
    PostProcessContext::begin(rc);

    m_d->m_currentArea = rc.area();
  }

  Luminous::Style ColorCorrectionFilter::style() const
  {
    Luminous::Style s = PostProcessContext::style();

    const RGBCube * cube = nullptr;

    // First test if RGBCube is used
    if(m_d->m_currentArea->rgbCube().isDefined()) {
      cube = &m_d->m_currentArea->rgbCube();
    }
    // Next use spline CC
    else if(!m_d->m_currentArea->colorCorrection().isIdentity()) {
      const RGBCube & tmp = m_d->m_currentArea->colorCorrection().asRGBCube();
      if(tmp.isDefined())
        cube = &tmp;
    }

    if(cube != nullptr) {
      s.setFillProgram(m_d->m_shader);
      s.setTexture("lut", cube->asTexture());
    } else {
      debugLuminous("ColorCorrectionFilter # No RGBCube defined for current area. "
                    "Using default shader");
    }

    return s;
  }
}
