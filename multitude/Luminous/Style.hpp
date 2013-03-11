/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

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
    Stroke(Stroke && s) : m_color(s.m_color), m_program(s.m_program), m_uniforms(std::move(s.m_uniforms)), m_width(s.m_width) {}
    Stroke(const Stroke & s) : m_color(s.m_color), m_program(s.m_program), m_width(s.m_width)
    {
      if (s.m_uniforms)
        m_uniforms.reset(new std::map<QByteArray, ShaderUniform>(*s.m_uniforms));
    }

    Stroke & operator=(Stroke && s)
    {
      m_color = s.m_color;
      m_program = s.m_program;
      m_width = s.m_width;
      m_uniforms = std::move(s.m_uniforms);
      return *this;
    }

    Stroke & operator=(const Stroke & s)
    {
      m_color = s.m_color;
      m_program = s.m_program;
      m_width = s.m_width;
      if (s.m_uniforms)
        m_uniforms.reset(new std::map<QByteArray, ShaderUniform>(*s.m_uniforms));
      return *this;
    }

    /// Shader program to be used for stroke
    const Luminous::Program * program() const { return m_program; }

    /// Sets the shader program
    /// @param program Program to use
    void setProgram(Luminous::Program & program) { m_program = &program; }

    /// Sets the stroke program to defautl
    void setDefaultProgram() { m_program = nullptr; }

    /// Sets the width of the stroke
    void setWidth(float width) { m_width = width; }

    /// Returns the width of the stroke
    float width() const { return m_width; }

    /// Sets the color of the stroke
    void setColor(const Radiant::Color & color) { m_color = color; }
    /// Returns the color of the stroke
    const Radiant::Color & color() const { return m_color; }

    // Shader uniforms
    /// Add shader uniform
    /// @param name Name for the uniform
    /// @param value Value of the uniform
    template <typename T> void setShaderUniform(const QByteArray & name, const T & value) {
      if (!m_uniforms)
        m_uniforms.reset(new std::map<QByteArray, ShaderUniform>());
      (*m_uniforms)[name] = ShaderUniform(value);
    }

    /// Remove shader uniform
    /// @param name Name of the uniform to be removed
    void removeShaderUniform(const QByteArray & name) { if (m_uniforms) m_uniforms->erase(name); }

    /// Returns the mapping from names to shader uniforms
    const std::map<QByteArray, ShaderUniform> * uniforms() const { return m_uniforms.get(); }

    void clear()
    {
      m_color.make(0.f, 0.f, 0.f, 0.f);
      m_program = nullptr;
      m_uniforms.reset();
      m_width = 0.f;
    }

  private:
    Radiant::Color m_color;
    Luminous::Program * m_program;
    std::unique_ptr<std::map<QByteArray, ShaderUniform> > m_uniforms;
    float m_width;
  };

  /// Defines the fill parameters for drawn objects.
  /// @sa Stroke
  class Fill
  {
  public:
    Fill() : m_color(0.f, 0.f, 0.f, 0.f), m_program(nullptr), m_translucentTextures(false) {}
    Fill(Fill && f) : m_color(f.m_color), m_program(f.m_program), m_textures(std::move(f.m_textures)),
      m_uniforms(std::move(f.m_uniforms)), m_translucentTextures(f.m_translucentTextures) {}
    Fill(const Fill & f) : m_color(f.m_color), m_program(f.m_program), m_translucentTextures(f.m_translucentTextures)
    {
      if (f.m_uniforms)
        m_uniforms.reset(new std::map<QByteArray, ShaderUniform>(*f.m_uniforms));
      if (f.m_textures)
        m_textures.reset(new std::map<QByteArray, const Texture *>(*f.m_textures));
    }

    Fill & operator=(Fill && f)
    {
      m_color = f.m_color;
      m_program = f.m_program;
      m_translucentTextures = f.m_translucentTextures;
      m_uniforms = std::move(f.m_uniforms);
      m_textures = std::move(f.m_textures);
      return *this;
    }

    Fill & operator=(const Fill & f)
    {
      m_color = f.m_color;
      m_program = f.m_program;
      m_translucentTextures = f.m_translucentTextures;
      if (f.m_uniforms)
        m_uniforms.reset(new std::map<QByteArray, ShaderUniform>(*f.m_uniforms));
      if (f.m_textures)
        m_textures.reset(new std::map<QByteArray, const Texture *>(*f.m_textures));
      return *this;
    }

    /// Returns the color of the fill
    const Radiant::Color & color() const { return m_color; }
    /// Sets the color of the fill
    void setColor(const Radiant::Color & c) { m_color = c; }

    /// Shader program to be used for fill
    const Luminous::Program * program() const { return m_program; }

    /// Sets the shader program
    /// @param program Program to use
    void setProgram(const Luminous::Program & program) { m_program = &program; }

    /// Sets the fill program to default
    void setDefaultProgram() { m_program = nullptr; }

    /// Returns the texture tied to given name
    /// @param name Name to search from textures
    /// @return Pointer to texture. nullptr if not found.
    inline const Luminous::Texture * texture(const QByteArray & name);

    /// Sets default fill texture.
    /// Internally this ties the given texture to the identifier "tex".
    /// @param texture Texture to be used for filling
    inline void setTexture(const Luminous::Texture & texture) { setTexture("tex", texture); }

    /// Sets fill texture with name
    /// @param name Name to use in shader to refer to texture
    /// @param texture Texture to be tied with the name
    inline void setTexture(const QByteArray & name, const Texture &texture);

    /// Returns the mapping from names to fill textures
    const std::map<QByteArray, const Texture *> * textures() const { return m_textures.get(); }

    /// Does the object contain translucent textures
    bool hasTranslucentTextures() const { return m_translucentTextures; }

    bool hasTextures() const { return m_textures && !m_textures->empty(); }

    // Shader uniforms
    /// Add shader uniform
    /// @param name Name for the uniform
    /// @param value Value of the uniform
    template <typename T> void setShaderUniform(const QByteArray & name, const T & value) {
      if (!m_uniforms)
        m_uniforms.reset(new std::map<QByteArray, ShaderUniform>());
      (*m_uniforms)[name] = ShaderUniform(value);
    }

    /// Remove shader uniform
    /// @param name Name of the uniform to be removed
    void removeShaderUniform(const QByteArray & name) { if (m_uniforms) m_uniforms->erase(name); }

    /// Returns the mapping from names to shader uniforms
    const std::map<QByteArray, ShaderUniform> * uniforms() const { return m_uniforms.get(); }

    void clear()
    {
      m_color.make(0.f, 0.f, 0.f, 0.f);
      m_program = nullptr;
      m_textures.reset();
      m_uniforms.reset();
      m_translucentTextures = false;
    }

  private:
    Radiant::Color m_color;
    const Luminous::Program * m_program;

    std::unique_ptr<std::map<QByteArray, const Texture *> > m_textures;
    std::unique_ptr<std::map<QByteArray, ShaderUniform> > m_uniforms;
    bool m_translucentTextures;

    friend class Style;
  };

  /// Style object for giving rendering parameters to the RenderContext
  /// Style objects acts as a collection of fill and stroke parameters and
  /// shader uniforms.
  class Style
  {
  public:
    Style() {}
    Style(Style && s) : m_fill(std::move(s.m_fill)), m_stroke(std::move(s.m_stroke)) {}
    Style(const Style & s) : m_fill(s.m_fill), m_stroke(s.m_stroke) {}

    Style & operator=(Style && s)
    {
      m_fill = std::move(s.m_fill);
      m_stroke = std::move(s.m_stroke);
      return *this;
    }

    Style & operator=(const Style & s)
    {
      m_fill = s.m_fill;
      m_stroke = s.m_stroke;
      return *this;
    }

    /// Stroke object of the style
    Stroke & stroke() { return m_stroke; }
    /// Stroke object of the style
    const Stroke & stroke() const { return m_stroke; }

    /// Fill object of the style
    Fill & fill() { return m_fill; }
    /// Fill object of the style
    const Fill & fill() const { return m_fill; }

    /// Returns the fill color
    const Radiant::Color & fillColor() const { return m_fill.color(); }

    /// Sets the fill color
    /// @param c Fill color
    void setFillColor(const Radiant::Color & c) { m_fill.setColor(c); }

    /// Sets the fill color
    /// @param r Red intensity
    /// @param g Green intensity
    /// @param b Blue intensity
    /// @param a Opacity (alpha)
    void setFillColor(float r, float g, float b, float a) { m_fill.setColor(Radiant::Color(r, g, b, a)); }

    /// Returns the shader program used for fill
    const Luminous::Program * fillProgram() const { return m_fill.program(); }

    /// Sets the shader program used for fill
    void setFillProgram(const Luminous::Program & program) { m_fill.setProgram(program); }

    /// Sets the fill program to default
    void setDefaultFillProgram() { m_fill.setDefaultProgram(); }

    /// Sets fill shader uniform
    /// @param name Name of the uniform
    /// @param value Value of the uniform
    template <typename T> void setFillShaderUniform(const QByteArray & name, const T & value) { m_fill.setShaderUniform(name, value); }

    /// Sets stroke shader uniform
    /// @param name Name of the uniform
    /// @param value Value of the uniform
    template <typename T> void setStrokeShaderUniform(const QByteArray & name, const T & value) { m_stroke.setShaderUniform(name, value); }

    /// Returns the shader program used for stroke
    const Luminous::Program * strokeProgram() const { return m_stroke.program(); }

    /// Sets the shader program used for stroke
    void setStrokeProgram(Luminous::Program & program) { m_stroke.setProgram(program); }

    /// Sets the stroke program to default
    void setDefaultStrokeProgram() { m_stroke.setDefaultProgram(); }

    /// Sets the stroke color
    /// @param r Red intensity
    /// @param g Green intensity
    /// @param b Blue intensity
    /// @param a Opacity (alpha)
    void setStrokeColor(float r, float g, float b, float a) { m_stroke.setColor(Radiant::Color(r, g, b, a)); }

    /// Sets the stroke color
    /// @param color Stroke color
    void setStrokeColor(const Radiant::Color & color) { m_stroke.setColor(color); }

    /// Returns the stroke color of the
    const Radiant::Color & strokeColor() const { return m_stroke.color(); }

    /// Sets the width of the stroke
    /// @param width The stroke width
    void setStrokeWidth(float width) { m_stroke.setWidth(width); }
    /// Returns the width of the stroke
    float strokeWidth() const { return m_stroke.width(); }

    /// @copydoc Fill::setTexture
    void setTexture(const Luminous::Texture & texture) { m_fill.setTexture(texture); }

    /// Sets fill texture with name
    /// @param name Name to use in shader to refer to texture
    /// @param texture Texture to be tied with the name
    void setTexture(const QByteArray & name, const Luminous::Texture & texture) { m_fill.setTexture(name, texture); }

  private:
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

    void setPointSize(float size) { m_font.setPointSizeF(size); }

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
    if (!m_textures)
      return nullptr;
    auto it = m_textures->find(name);
    return it == m_textures->end() ? nullptr : it->second;
  }

  void Fill::setTexture(const QByteArray & name, const Luminous::Texture & texture)
  {
    if (!m_textures)
      m_textures.reset(new std::map<QByteArray, const Texture *>());
    (*m_textures)[name] = &texture;
    m_translucentTextures = m_translucentTextures || texture.translucent();
  }
}
#endif // LUMINOUS_STYLE_HPP
