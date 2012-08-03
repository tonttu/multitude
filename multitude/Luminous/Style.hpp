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
    Stroke() : m_color(0.f, 0.f, 0.f, 0.f), m_width(0.f) {}

    void setWidth(float width) { m_width = width; }
    float width() const { return m_width; }

    void setColor(const Radiant::Color & color) { m_color = color; }
    const Radiant::Color & color() const { return m_color; }
  private:
    Radiant::Color m_color;
    float m_width;
  };

  class Fill
  {
  public:
    /// This class is because we would like to do n std::map lookups in one frame instead of 2n
    /// @todo Does this make sense?
    struct Texture
    {
      Luminous::Texture * texture;
      Luminous::TextureGL * textureGL;
    };

  public:
    Fill() : m_color(1.f, 1.f, 1.f, 1.f), m_programGL(nullptr), m_program(nullptr) {}

    const Radiant::Color & color() const { return m_color; }
    void setColor(const Radiant::Color & c) { m_color = c; }

    Luminous::Program * program() const { return m_program; }
    void setProgram(Luminous::Program & program) { m_program = &program; }
    void setDefaultProgram() { m_program = nullptr; m_programGL = nullptr; }

    Luminous::ProgramGL * programGL() const { return m_programGL; }
    void setProgram(Luminous::ProgramGL & program) { m_programGL = &program; }

    inline Luminous::Texture * texture(const QByteArray & name);
    inline Luminous::TextureGL * textureGL(const QByteArray & name);
    inline void findTexture(const QByteArray & name, Luminous::TextureGL *& textureGL, Luminous::Texture *& texture);

    inline void setTexture(Luminous::Texture & texture) { setTexture("tex", texture); }
    inline void setTexture(Luminous::TextureGL & texture) { setTexture("tex", texture); }
    inline void setTexture(const QByteArray & name, Luminous::Texture & texture);
    inline void setTexture(const QByteArray & name, Luminous::TextureGL & texture);

    const std::map<QByteArray, Texture> & textures() const { return m_textures; }

  private:
    Radiant::Color m_color;
    Luminous::ProgramGL * m_programGL;
    Luminous::Program * m_program;

    /// @todo We could have our own map class that could store n bytes of data
    ///       to pre-allocated buffer, and then convert to std::map if it runs
    ///       out of space. Since it would have only couple of values normally,
    ///       it could be just an array of pairs with unique keys
    std::map<QByteArray, Texture> m_textures;

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

    Luminous::Program * fillProgram() const { return m_fill.program(); }
    void setFillProgram(Luminous::Program & program) { m_fill.setProgram(program); }
    void setDefaultFillProgram() { m_fill.setDefaultProgram(); }

    void setStrokeColor(float r, float g, float b, float a) { m_stroke.setColor(Radiant::Color(r, g, b, a)); }
    void setStrokeColor(const Radiant::Color & color) { m_stroke.setColor(color); }
    const Radiant::Color & strokeColor() const { return m_stroke.color(); }

    void setStrokeWidth(float width) { m_stroke.setWidth(width); }
    float strokeWidth() const { return m_stroke.width(); }

    Luminous::ProgramGL * fillProgramGL() const { return m_fill.programGL(); }
    void setFillProgram(Luminous::ProgramGL & program) { m_fill.setProgram(program); }

    void setTexture(Luminous::Texture & texture) { m_fill.setTexture(texture); }
    void setTexture(Luminous::TextureGL & texture) { m_fill.setTexture(texture); }
    void setTexture(const QByteArray & name, Luminous::Texture & texture) { m_fill.setTexture(name, texture); }
    void setTexture(const QByteArray & name, Luminous::TextureGL & texture) { m_fill.setTexture(name, texture); }

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
    return it == m_textures.end() ? nullptr : it->second.texture;
  }

  Luminous::TextureGL * Fill::textureGL(const QByteArray & name)
  {
    auto it = m_textures.find(name);
    return it == m_textures.end() ? nullptr : it->second.textureGL;
  }

  void Fill::findTexture(const QByteArray & name, Luminous::TextureGL *& textureGL, Luminous::Texture *& texture)
  {
    auto it = m_textures.find(name);
    if(it == m_textures.end()) {
      textureGL = nullptr;
      texture = nullptr;
    } else {
      textureGL = it->second.textureGL;
      texture = it->second.texture;
    }
  }

  void Fill::setTexture(const QByteArray & name, Luminous::Texture & texture)
  {
    auto & t = m_textures[name];
    t.texture = &texture;
    t.textureGL = nullptr;
  }

  void Fill::setTexture(const QByteArray & name, Luminous::TextureGL & texture)
  {
    auto & t = m_textures[name];
    t.textureGL = &texture;
    t.texture = nullptr;
  }
}
#endif // LUMINOUS_STYLE_HPP
