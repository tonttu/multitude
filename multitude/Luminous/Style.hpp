#ifndef LUMINOUS_STYLE_HPP
#define LUMINOUS_STYLE_HPP

#include <Nimble/Rect.hpp>
#include <Nimble/Vector4.hpp>

namespace Luminous
{

  class Style
  {
  public:
    Style();

    const Nimble::Vector4 & color () const { return m_color; }
    void setColor(const Nimble::Vector4 & c) { m_color = c; }

    const Nimble::Rect & texCoords () const { return m_texCoords; }
    void setTexCoords(const Nimble::Rect &tc) { m_texCoords = tc; }

    void setTexturing(float texturing) { m_texturing = texturing; }
    float texturing() const { return m_texturing; }

  private:

    Nimble::Vector4 m_color;
    Nimble::Rect m_texCoords;
    float m_texturing;
  };

}
#endif // LUMINOUS_STYLE_HPP
