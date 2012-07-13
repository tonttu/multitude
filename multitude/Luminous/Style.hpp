#ifndef LUMINOUS_STYLE_HPP
#define LUMINOUS_STYLE_HPP

#include "Export.hpp"

#include <Nimble/Rect.hpp>
#include <Nimble/Vector4.hpp>

#include <Luminous/ShaderProgram.hpp>

namespace Luminous
{
  class GLSLProgramObject;

  struct Fill
  {
    Fill() : color(1, 1, 1, 1), shader(nullptr) {}
    /// @todo accessors
    Nimble::Vector4f color;
    Luminous::ShaderProgram * shader;
    std::map<QByteArray, Luminous::Texture*> tex;
  };

  /// Style object for giving rendering parameters to the RenderContext
  class Style
  {
  public:
    LUMINOUS_API Style();

    /// @todo accessors
    Fill fill;

    /// Returns the color of the object to be drawn
    const Nimble::Vector4 & color () const { return m_color; }
    /// Sets the color of the object to be drawn
    void setColor(const Nimble::Vector4 & c) { m_color = c; }
    /// Sets the color of the object to be drawn
    void setColor(float r, float g, float b, float a) { m_color.make(r, g, b, a); }

    /// Returns the texture coordinates to use
    const Nimble::Rect & texCoords () const { return m_texCoords; }
    /// Sets the texture coordinates to be use
    void setTexCoords(const Nimble::Rect &tc) { m_texCoords = tc; }

    /// Control the amount of texturing
    /** @param texturing Variable control of the texture weight. 1 gives full weight to the texture
        while zero make non-textured objects. */
    void setTexturing(float texturing) { m_texturing = texturing; }
    /// Returns the amount of texturing
    float texturing() const { return m_texturing; }

    /// Flips the y-coordinates used for texturing
    /** This function is handy if you find that your image is upside-down. */
    void flipTextureYCoordinates()
    {
      float tmp = m_texCoords.low().y;
      m_texCoords.low().y = m_texCoords.high().y;
      m_texCoords.high().y = tmp;
    }

    /// Sets the GLSLProgramObject to be used.
    /** This feature is seldom used, as the underlying system generally
        selects the right program automatically.*/
    void setProgram(GLSLProgramObject * prog) { m_program = prog; }

    /// Returns a custom GLSLProgramObject for rendering
    GLSLProgramObject * program() const { return m_program; }
  private:

    Nimble::Vector4 m_color;
    Nimble::Rect m_texCoords;
    float m_texturing;
    GLSLProgramObject * m_program;
  };

}
#endif // LUMINOUS_STYLE_HPP
