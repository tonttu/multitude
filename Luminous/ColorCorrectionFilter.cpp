/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ColorCorrectionFilter.hpp"

#include <Luminous/RenderContext.hpp>
#include <Luminous/RGBCube.hpp>
#include <Luminous/Texture.hpp>
#include <Luminous/VertexDescription.hpp>

namespace Luminous
{
  class ColorCorrectionFilter::D
  {
  public:
    D()
    {
      m_shader.loadShader("cornerstone:Luminous/GLSL150/tex.vs", Luminous::Shader::Vertex);
      m_shader.loadShader("cornerstone:Luminous/GLSL150/cc_rgb.fs", Luminous::Shader::Fragment);

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
