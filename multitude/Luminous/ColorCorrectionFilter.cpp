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

  void ColorCorrectionFilter::filter(Luminous::RenderContext & rc,
                                     Luminous::PostProcessContext & pc,
                                     Luminous::Style style) const
  {
    const RGBCube * cube = nullptr;

    // First test if RGBCube is used
    if(rc.area()->rgbCube().isDefined()) {
      cube = &rc.area()->rgbCube();
    }
    // Next use spline CC
    else if(!rc.area()->colorCorrection().isIdentity()) {
      const RGBCube & tmp = rc.area()->colorCorrection().asRGBCube();
      if(tmp.isDefined())
        cube = &tmp;
    }

    if(cube != nullptr) {
      style.setFillProgram(m_d->m_shader);
      style.setTexture("lut", cube->asTexture());
    } else {
      debugLuminous("ColorCorrectionFilter # No RGBCube defined for current area. "
                    "Using default shader");
    }

    PostProcessFilter::filter(rc, pc, style);
  }
}
