#ifndef LUMINOUS_FILL_HPP
#define LUMINOUS_FILL_HPP

#include <Nimble/Rect.hpp>
#include <Nimble/Vector4.hpp>

do not include


#error

namespace Luminous
{

  class Fill
  {
  public:
    Fill();

    const Nimble::Vector4 & color () const { return m_color; }
    void setColor(Nimble::Vector4 & c) { m_color = c; }

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
#endif // FILL_HPP
