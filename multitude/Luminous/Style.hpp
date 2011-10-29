#ifndef LUMINOUS_STYLE_HPP
#define LUMINOUS_STYLE_HPP

#include "Export.hpp"

#include <Nimble/Rect.hpp>
#include <Nimble/Vector4.hpp>

namespace Luminous
{

  /// Style object for giving rendering parameters to the RenderContext
  class LUMINOUS_API Style
  {
  public:
    Style();

    /// Returns the color of the object to be drawn
    const Nimble::Vector4 & color () const { return m_color; }
    /// Sets the color of the object to be drawn
    void setColor(const Nimble::Vector4 & c) { m_color = c; }

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

  private:

    Nimble::Vector4 m_color;
    Nimble::Rect m_texCoords;
    float m_texturing;
  };

}
#endif // LUMINOUS_STYLE_HPP
