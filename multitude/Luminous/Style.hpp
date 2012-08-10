#ifndef LUMINOUS_STYLE_HPP
#define LUMINOUS_STYLE_HPP

#include "Export.hpp"

#include <Nimble/Rect.hpp>
#include <Nimble/Vector4.hpp>

#include <Luminous/Program.hpp>

namespace Luminous
{
  class GLSLProgramObject;

  class Stroke
  {
  public:
    Stroke() : m_color(0.f, 0.f, 0.f, 0.f), m_program(nullptr), m_width(0.f) {}

    Luminous::Program * program() const { return m_program; }
    void setProgram(Luminous::Program & program) { m_program = &program; }
    void setDefaultProgram() { m_program = nullptr; }

    void setWidth(float width) { m_width = width; }
    float width() const { return m_width; }

    void setColor(const Radiant::Color & color) { m_color = color; }
    const Radiant::Color & color() const { return m_color; }
  private:
    Radiant::Color m_color;
    Luminous::Program * m_program;

    float m_width;
  };

  class Fill
  {
  public:
    Fill() : m_color(0.f, 0.f, 0.f, 0.f), m_program(nullptr) {}

    const Radiant::Color & color() const { return m_color; }
    void setColor(const Radiant::Color & c) { m_color = c; }

    const Luminous::Program * program() const { return m_program; }
    void setProgram(const Luminous::Program & program) { m_program = &program; }
    void setDefaultProgram() { m_program = nullptr; }

    inline Luminous::Texture * texture(const QByteArray & name);
    inline Luminous::TextureGL * textureGL(const QByteArray & name);
    inline void findTexture(const QByteArray & name, Luminous::Texture *& texture);

    inline void setTexture(Luminous::Texture & texture) { setTexture("tex", texture); }
    inline void setTexture(const QByteArray & name, Luminous::Texture & texture);

    const std::map<QByteArray, Texture *> & textures() const { return m_textures; }

  private:
    Radiant::Color m_color;
    const Luminous::Program * m_program;

    /// @todo We could have our own map class that could store n bytes of data
    ///       to pre-allocated buffer, and then convert to std::map if it runs
    ///       out of space. Since it would have only couple of values normally,
    ///       it could be just an array of pairs with unique keys
    std::map<QByteArray, Texture *> m_textures;

    friend class Style;
  };

  /// Style object for giving rendering parameters to the RenderContext
  class Style
  {
  public:
    enum Translucency
    {
      Translucent,
      Opaque,
      Auto
    };

  public:
    Style() : m_translucency(Auto) {}

    Stroke & stroke() { return m_stroke; }
    const Stroke & stroke() const { return m_stroke; }

    Fill & fill() { return m_fill; }
    const Fill & fill() const { return m_fill; }

    /// Returns the color of the object to be drawn
    const Radiant::Color & fillColor() const { return m_fill.color(); }
    /// Sets the color of the object to be drawn
    void setFillColor(const Nimble::Vector4 & c) { m_fill.setColor(c); }
    /// Sets the color of the object to be drawn
    void setFillColor(float r, float g, float b, float a) { m_fill.setColor(Radiant::Color(r, g, b, a)); }

    const Luminous::Program * fillProgram() const { return m_fill.program(); }
    void setFillProgram(const Luminous::Program & program) { m_fill.setProgram(program); }
    void setDefaultFillProgram() { m_fill.setDefaultProgram(); }

    Luminous::Program * strokeProgram() const { return m_stroke.program(); }
    void setStrokeProgram(Luminous::Program & program) { m_stroke.setProgram(program); }
    void setDefaultStrokeProgram() { m_stroke.setDefaultProgram(); }
    
    void setStrokeColor(float r, float g, float b, float a) { m_stroke.setColor(Radiant::Color(r, g, b, a)); }
    void setStrokeColor(const Radiant::Color & color) { m_stroke.setColor(color); }
    const Radiant::Color & strokeColor() const { return m_stroke.color(); }

    void setStrokeWidth(float width) { m_stroke.setWidth(width); }
    float strokeWidth() const { return m_stroke.width(); }

    void setTexture(Luminous::Texture & texture) { m_fill.setTexture(texture); }
    void setTexture(const QByteArray & name, Luminous::Texture & texture) { m_fill.setTexture(name, texture); }

    void setTranslucency(Translucency translucency) { m_translucency = translucency; }
    Translucency translucency() const { return m_translucency; }

  private:
    Fill m_fill;
    Stroke m_stroke;
    Translucency m_translucency;
  };

  /////////////////////////////////////////////////////////////////////////////

  Luminous::Texture * Fill::texture(const QByteArray & name)
  {
    auto it = m_textures.find(name);
    return it == m_textures.end() ? nullptr : it->second;
  }

  void Fill::findTexture(const QByteArray & name, Luminous::Texture *& texture)
  {
    auto it = m_textures.find(name);
    if(it == m_textures.end()) {
      texture = nullptr;
    } else {
      texture = it->second;
    }
  }

  void Fill::setTexture(const QByteArray & name, Luminous::Texture & texture)
  {
    m_textures[name] = &texture;
  }
}
#endif // LUMINOUS_STYLE_HPP
