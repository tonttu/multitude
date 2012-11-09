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

  /// @todo only reason we override this is to get rgbCube from RC,
  /// do it some other way..
  void ColorCorrectionFilter::apply(RenderContext & rc)
  {
    Luminous::Style style;
    style.setFillColor(1.0f, 1.0f, 1.0f, 1.0f);
    style.setTexture("tex", texture());

    if(rc.area()->rgbCube().isDefined()) {
      style.setFillProgram(m_d->m_shader);
      style.setTexture("lut", rc.area()->rgbCube().asTexture());
    } else {
      debugLuminous("ColorCorrectionFilter # No RGBCube defined for current area. "
                    "Using default shader");
      style.setFillProgram(rc.texShader());
    }

    const Nimble::Vector2f size = rc.contextSize();
    const Luminous::Program & program = *style.fillProgram();

    auto b = rc.drawPrimitiveT<Luminous::BasicVertexUV, Luminous::BasicUniformBlock>(
          Luminous::PrimitiveType_TriangleStrip, 0, 4, program, style.fillColor(), 1.f, style);

    b.vertex[0].location.make(0, 0);
    b.vertex[0].texCoord.make(0, 0);
    b.vertex[1].location.make(size.x, 0);
    b.vertex[1].texCoord.make(1, 0);
    b.vertex[2].location.make(0, size.y);
    b.vertex[2].texCoord.make(0, 1);
    b.vertex[3].location.make(size.x, size.y);
    b.vertex[3].texCoord.make(1, 1);
  }
}
