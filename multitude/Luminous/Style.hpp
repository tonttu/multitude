#ifndef LUMINOUS_STYLE_HPP
#define LUMINOUS_STYLE_HPP

#include "Export.hpp"

#include <Nimble/Rect.hpp>
#include <Nimble/Vector4.hpp>

#include <Luminous/Program.hpp>

namespace Luminous
{
  class GLSLProgramObject;

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

    const Nimble::Vector4 & color() const { return m_color; }
    void setColor(const Nimble::Vector4f & c) { m_color = c; }

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
    Style() : m_translucency(Auto), m_texCoords(0, 0, 1, 1), m_texturing(0) {}

    Fill & fill() { return m_fill; }
    const Fill & fill() const { return m_fill; }

    /// Returns the color of the object to be drawn
    const Radiant::Color & fillColor () const { return m_fill.m_color; }
    /// Sets the color of the object to be drawn
    void setFillColor(const Nimble::Vector4 & c) { m_fill.m_color = c; }
    /// Sets the color of the object to be drawn
    void setFillColor(float r, float g, float b, float a) { m_fill.m_color.make(r, g, b, a); }

    Luminous::Program * fillProgram() const { return m_fill.m_program; }
    void setFillProgram(Luminous::Program & program) { m_fill.m_program = &program; }
    void setDefaultFillProgram() { m_fill.m_program = nullptr; m_fill.m_programGL = nullptr; }

    Luminous::ProgramGL * fillProgramGL() const { return m_fill.m_programGL; }
    void setFillProgram(Luminous::ProgramGL & program) { m_fill.m_programGL = &program; }

    void setTexture(Luminous::Texture & texture) { m_fill.setTexture(texture); }
    void setTexture(Luminous::TextureGL & texture) { m_fill.setTexture(texture); }
    void setTexture(const QByteArray & name, Luminous::Texture & texture) { m_fill.setTexture(name, texture); }
    void setTexture(const QByteArray & name, Luminous::TextureGL & texture) { m_fill.setTexture(name, texture); }

    void setTranslucency(Translucency translucency) { m_translucency = translucency; }
    Translucency translucency() const { return m_translucency; }

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
    Fill m_fill;
    Translucency m_translucency;

    Nimble::Rect m_texCoords;
    float m_texturing;
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
