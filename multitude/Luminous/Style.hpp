#ifndef LUMINOUS_STYLE_HPP
#define LUMINOUS_STYLE_HPP

#include "Export.hpp"

#include <Nimble/Rect.hpp>
#include <Nimble/Vector4.hpp>

#include <Luminous/Program.hpp>
#include <Luminous/BlendMode.hpp>
#include <Luminous/DepthMode.hpp>
#include <Luminous/StencilMode.hpp>
#include <Luminous/Texture2.hpp>
#include <Luminous/ShaderUniform.hpp>

#include <QFont>
#include <QTextOption>

#include <map>

namespace Luminous
{

  /// Defines the stroke parameters for drawn objects.
  /// @sa Fill
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

    // Shader uniforms
    template <typename T> void setShaderUniform(const QByteArray & name, const T & value) { m_uniforms[name] = ShaderUniform(value); }
    void removeShaderUniform(const QByteArray & name) { m_uniforms.erase(name); }
    const std::map<QByteArray, ShaderUniform> & uniforms() const { return m_uniforms; }

  private:
    Radiant::Color m_color;
    Luminous::Program * m_program;
    std::map<QByteArray, ShaderUniform> m_uniforms;
    float m_width;
  };

  /// Defines the fill parameters for drawn objects.
  /// @sa Stroke
  class Fill
  {
  public:
    Fill() : m_color(0.f, 0.f, 0.f, 0.f), m_program(nullptr), m_translucentTextures(false) {}

    const Radiant::Color & color() const { return m_color; }
    void setColor(const Radiant::Color & c) { m_color = c; }

    const Luminous::Program * program() const { return m_program; }
    void setProgram(const Luminous::Program & program) { m_program = &program; }
    void setDefaultProgram() { m_program = nullptr; }

    inline const Luminous::Texture * texture(const QByteArray & name);
    inline void findTexture(const QByteArray & name, const Texture *&texture);

    inline void setTexture(const Luminous::Texture & texture) { setTexture("tex", texture); }
    inline void setTexture(const QByteArray & name, const Texture &texture);

    const std::map<QByteArray, const Texture *> & textures() const { return m_textures; }

    bool hasTranslucentTextures() const { return m_translucentTextures; }

    // Shader uniforms
    template <typename T> void setShaderUniform(const QByteArray & name, const T & value) { m_uniforms[name] = ShaderUniform(value); }
    void removeShaderUniform(const QByteArray & name) { m_uniforms.erase(name); }
    const std::map<QByteArray, ShaderUniform> & uniforms() const { return m_uniforms; }

  private:
    Radiant::Color m_color;
    const Luminous::Program * m_program;

    std::map<QByteArray, const Texture *> m_textures;
    std::map<QByteArray, ShaderUniform> m_uniforms;
    bool m_translucentTextures;

    friend class Style;
  };

  /// Style object for giving rendering parameters to the RenderContext
  class Style
  {
  public:
    Style() {}

    Stroke & stroke() { return m_stroke; }
    const Stroke & stroke() const { return m_stroke; }

    Fill & fill() { return m_fill; }
    const Fill & fill() const { return m_fill; }

    /// Returns the color of the object to be drawn
    const Radiant::Color & fillColor() const { return m_fill.color(); }
    /// Sets the color of the object to be drawn
    void setFillColor(const Radiant::Color & c) { m_fill.setColor(c); }
    /// Sets the color of the object to be drawn
    void setFillColor(float r, float g, float b, float a) { m_fill.setColor(Radiant::Color(r, g, b, a)); }

    const Luminous::Program * fillProgram() const { return m_fill.program(); }
    void setFillProgram(const Luminous::Program & program) { m_fill.setProgram(program); }
    void setDefaultFillProgram() { m_fill.setDefaultProgram(); }

    template <typename T> void setFillShaderUniform(const QByteArray & name, const T & value) { m_fill.setShaderUniform(name, value); }
    template <typename T> void setStrokeShaderUniform(const QByteArray & name, const T & value) { m_stroke.setShaderUniform(name, value); }

    Luminous::Program * strokeProgram() const { return m_stroke.program(); }
    void setStrokeProgram(Luminous::Program & program) { m_stroke.setProgram(program); }
    void setDefaultStrokeProgram() { m_stroke.setDefaultProgram(); }

    void setStrokeColor(float r, float g, float b, float a) { m_stroke.setColor(Radiant::Color(r, g, b, a)); }
    void setStrokeColor(const Radiant::Color & color) { m_stroke.setColor(color); }
    const Radiant::Color & strokeColor() const { return m_stroke.color(); }

    void setStrokeWidth(float width) { m_stroke.setWidth(width); }
    float strokeWidth() const { return m_stroke.width(); }

    void setTexture(const Luminous::Texture & texture) { m_fill.setTexture(texture); }
    void setTexture(const QByteArray & name, const Luminous::Texture & texture) { m_fill.setTexture(name, texture); }

  private:
    typedef std::vector< std::shared_ptr<ShaderUniform> > UniformList;
    UniformList m_uniforms;

    Fill m_fill;
    Stroke m_stroke;
  };


  //////////////////////////////////////////////////////////////////////////

  /// Control how text content overflow is handled.
  enum Overflow
  {
    /// "visible", content is not clipped, i.e., it may be rendered outside the content box
    /// (default value)
    OverflowVisible,
    /// "hidden", content is clipped, no scrolling mechanism should be provided
    OverflowHidden,
    /// "scroll", content is clipped, scrolling mechanism is always visible
    OverflowScroll,
    /// "auto", content is clipped, scrolling mechanism is visible when needed
    OverflowAuto
  };

  /// This class stores the style information needed to render text.
  class TextStyle : public Style
  {
  public:
    TextStyle()
      : m_fontRenderWidth(0.0f)
      , m_glow(0)
      , m_glowColor(1.0f, 1.0f, 1.0f, 1.0f)
      , m_textSharpness(1.0f)
      , m_dropShadowBlur(0.0f)
      , m_dropShadowColor(0.0f, 0.0f, 0.0f, 0.0f)
      , m_dropShadowOffset(0, 0)
      , m_textOverflow(OverflowVisible)
    {}

    /// Get the font for the style
    QFont & font() { return m_font; }
    /// @copydoc font
    const QFont & font() const { return m_font; }
    /// Set the font for the style
    void setFont(const QFont & font) { m_font = font; }

    /// Get the text options for the style. The options are used to define things like wrapping and alignment.
    QTextOption & textOption() { return m_textOption; }
    /// @copydoc textOption
    const QTextOption & textOption() const { return m_textOption; }

    /// Get the text overflow for the style
    Overflow textOverflow() const { return m_textOverflow; }
    /// Set the text overflow for the style
    void setTextOverflow(Overflow overflow) { m_textOverflow = overflow; }

    /// Get the relative width of the rendered font
    float fontRenderWidth() const { return m_fontRenderWidth; }
    /// Set the relative width of the rendered font
    void setFontRenderWidth(float offset) { m_fontRenderWidth = offset; }

    /// Get the text glow
    float glow() const { return m_glow; }
    /// Set the text glow amount. Values should be from 0 to 1.
    void setGlow(float glow) { m_glow = glow; }

    /// Get the text glow color
    const Radiant::Color & glowColor() const { return m_glowColor; }
    /// Set text glow color
    void setGlowColor(const Radiant::Color & glowColor) { m_glowColor = glowColor; }
    /// Set text glow color
    void setGlowColor(float r, float g, float b, float a) { m_glowColor.make(r, g, b, a); }

    /// Get text sharpness
    float textSharpness() const { return m_textSharpness; }
    /// Set text sharpness. Can be used to blur the text.
    void setTextSharpness(float textSharpness) { m_textSharpness = textSharpness; }

    /// Get the amount of drop-shadow blur
    float dropShadowBlur() const { return m_dropShadowBlur; }
    /// Set the amount of drop-shadow blur. Values from 0 to 1.
    void setDropShadowBlur(float blur) { m_dropShadowBlur = blur; }

    /// Get the drop-shadow color
    const Radiant::Color & dropShadowColor() const { return m_dropShadowColor; }
    /// Set the drop-shadow color
    void setDropShadowColor(const Radiant::Color & dropShadowColor) { m_dropShadowColor = dropShadowColor; }
    /// Set the drop-shadow color
    void setDropShadowColor(float r, float g, float b, float a) { m_dropShadowColor.make(r, g, b, a); }

    /// Get the drop-shadow offset
    const Nimble::Vector2f & dropShadowOffset() const { return m_dropShadowOffset; }
    /// Set the drop-shadow offset
    void setDropShadowOffset(const Nimble::Vector2f & offset) { m_dropShadowOffset = offset; }

  private:
    float m_fontRenderWidth;

    /// from 0 to 1
    float m_glow;
    Radiant::Color m_glowColor;

    float m_textSharpness;

    /// from 0 to 1
    float m_dropShadowBlur;
    Radiant::Color m_dropShadowColor;
    Nimble::Vector2f m_dropShadowOffset;

    QFont m_font;
    QTextOption m_textOption;
    Overflow m_textOverflow;
  };

  /////////////////////////////////////////////////////////////////////////////

  const Texture *Fill::texture(const QByteArray & name)
  {
    auto it = m_textures.find(name);
    return it == m_textures.end() ? nullptr : it->second;
  }

  void Fill::findTexture(const QByteArray & name, const Luminous::Texture *& texture)
  {
    auto it = m_textures.find(name);
    if(it == m_textures.end()) {
      texture = nullptr;
    } else {
      texture = it->second;
    }
  }

  void Fill::setTexture(const QByteArray & name, const Luminous::Texture & texture)
  {
    m_textures[name] = &texture;
    m_translucentTextures = m_translucentTextures || texture.translucent();
  }
}
#endif // LUMINOUS_STYLE_HPP
