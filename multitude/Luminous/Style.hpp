/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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
#include <Luminous/Texture.hpp>
#include <Luminous/ShaderUniform.hpp>
#include <Luminous/TextLayout.hpp>

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
    /// Construct default stroke (black, zero width)
    Stroke() : m_color(0.f, 0.f, 0.f, 0.f), m_program(nullptr), m_width(0.f) {}
    /// Move constructor
    /// @param s stroke to move
    Stroke(Stroke && s) : m_color(s.m_color), m_program(s.m_program), m_uniforms(std::move(s.m_uniforms)), m_width(s.m_width) {}
    /// Copy constructor
    /// @param s stroke to copy
    Stroke(const Stroke & s) : m_color(s.m_color), m_program(s.m_program), m_width(s.m_width)
    {
      if (s.m_uniforms)
        m_uniforms.reset(new std::map<QByteArray, ShaderUniform>(*s.m_uniforms));
    }

    /// Move assignment operator
    /// @param s stroke to move
    Stroke & operator=(Stroke && s)
    {
      m_color = s.m_color;
      m_program = s.m_program;
      m_width = s.m_width;
      m_uniforms = std::move(s.m_uniforms);
      return *this;
    }

    /// Assignment operator
    /// @param s stroke to copy
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
    /// @return program used for the stroke
    const Luminous::Program * program() const { return m_program; }

    /// Sets the shader program
    /// @param program Program to use
    void setProgram(Luminous::Program & program) { m_program = &program; }

    /// Sets the stroke program to default. This functions sets the stroke to
    /// use the default stroke shader for Cornerstone.
    void setDefaultProgram() { m_program = nullptr; }

    /// Sets the width of the stroke
    /// @param width stroke width
    void setWidth(float width) { m_width = width; }

    /// Returns the width of the stroke
    /// @return stroke width
    float width() const { return m_width; }

    /// Sets the color of the stroke
    /// @param color stroke color
    void setColor(const Radiant::ColorPMA & color) { m_color = color; }
    /// Returns the color of the stroke
    /// @return stroke color
    const Radiant::ColorPMA & color() const { return m_color; }

    /// Set the value of a shader uniform for the stroke program
    /// @param name Name for the uniform
    /// @param value Value of the uniform
    template <typename T> void setShaderUniform(const QByteArray & name, const T & value) {
      if (!m_uniforms)
        m_uniforms.reset(new std::map<QByteArray, ShaderUniform>());
      (*m_uniforms)[name] = ShaderUniform(value);
    }

    /// Remove a shader uniform
    /// @param name Name of the uniform to be removed
    void removeShaderUniform(const QByteArray & name) { if (m_uniforms) m_uniforms->erase(name); }

    /// Returns the mapping from names to shader uniforms
    const std::map<QByteArray, ShaderUniform> * uniforms() const { return m_uniforms.get(); }

    /// Clear the stroke to default state (black, zero width)
    void clear()
    {
      m_color.setRGBA(0.f, 0.f, 0.f, 0.f);
      m_program = nullptr;
      m_uniforms.reset();
      m_width = 0.f;
    }

  private:
    Radiant::ColorPMA m_color;
    Luminous::Program * m_program;
    std::unique_ptr<std::map<QByteArray, ShaderUniform> > m_uniforms;
    float m_width;
  };

  /// Defines the fill parameters for drawn objects.
  /// @sa Stroke
  class Fill
  {
  public:
    /// Create a default fill (black)
    Fill() : m_color(0.f, 0.f, 0.f, 0.f), m_program(nullptr), m_translucentTextures(false) {}
    /// Move constructor
    /// @param f fill to move
    Fill(Fill && f) : m_color(f.m_color), m_program(f.m_program), m_textures(std::move(f.m_textures)),
      m_uniforms(std::move(f.m_uniforms)), m_translucentTextures(f.m_translucentTextures) {}
    /// Copy constructor
    /// @param f fill to copy
    Fill(const Fill & f) : m_color(f.m_color), m_program(f.m_program), m_translucentTextures(f.m_translucentTextures)
    {
      if (f.m_uniforms)
        m_uniforms.reset(new std::map<QByteArray, ShaderUniform>(*f.m_uniforms));
      if (f.m_textures)
        m_textures.reset(new std::map<QByteArray, const Texture *>(*f.m_textures));
    }

    /// Move assignment operator
    /// @param f fill to move
    /// @return reference to this
    Fill & operator=(Fill && f)
    {
      m_color = f.m_color;
      m_program = f.m_program;
      m_translucentTextures = f.m_translucentTextures;
      m_uniforms = std::move(f.m_uniforms);
      m_textures = std::move(f.m_textures);
      return *this;
    }

    /// Assignment operator
    /// @param f fill to copy
    /// @return reference to this
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

    /// Get the fill color
    /// @return fill color
    const Radiant::ColorPMA & color() const { return m_color; }
    /// Sets the color of the fill
    /// @param c fill color
    void setColor(const Radiant::ColorPMA & c) { m_color = c; }

    /// Shader program to be used for fill
    /// @return fill shader program
    const Luminous::Program * program() const { return m_program; }

    /// Sets the shader program
    /// @param program Program to use
    void setProgram(const Luminous::Program & program) { m_program = &program; }

    /// Sets the fill program to default. This function sets the fill shader to
    /// the default Cornerstone shader.
    void setDefaultProgram() { m_program = nullptr; }

    /// Returns the texture tied to given name
    /// @param name Name to search from textures
    /// @return Pointer to texture or nullptr if not found.
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

    /// Does the style contain translucent textures. Translucent textures cause
    /// that any drawing operations using the style will not be re-ordered for
    /// performance.
    /// @return true if the style has translucent textures; otherwise false
    bool hasTranslucentTextures() const { return m_translucentTextures; }

    /// Does the style contain any textures.
    /// @return true if the style has textures; otherwise false
    bool hasTextures() const { return m_textures && !m_textures->empty(); }

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

    /// Clear the style to default values (black)
    void clear()
    {
      m_color.setRGBA(0.f, 0.f, 0.f, 0.f);
      m_program = nullptr;
      m_textures.reset();
      m_uniforms.reset();
      m_translucentTextures = false;
    }

  private:
    Radiant::ColorPMA m_color;
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
    const Radiant::ColorPMA & fillColor() const { return m_fill.color(); }

    /// Sets the fill color
    /// @param c Fill color
    void setFillColor(const Radiant::ColorPMA & c) { m_fill.setColor(c); }

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
    void setStrokeColor(const Radiant::ColorPMA & color) { m_stroke.setColor(color); }

    /// Returns the stroke color of the
    const Radiant::ColorPMA & strokeColor() const { return m_stroke.color(); }

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

    bool hasFill() const
    {
      return !m_fill.color().isZero();
    }

    bool hasStroke() const
    {
      return m_stroke.width() > 0.f && !m_stroke.color().isZero();
    }

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
    /// You can modify the returned reference, but typically you should use
    /// setFontPixelSize to set the font size instead of using QFont directly
    QFont & font() { return m_font; }
    /// @copydoc font
    const QFont & font() const { return m_font; }
    /// Set the font for the style
    void setFont(const QFont & font) { m_font = font; }
    /// Set the font size in pixels
    inline void setFontPixelSize(float sizeInPixels);
    /// Returns the font size in pixels
    inline float fontPixelSize() const;

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
    const Radiant::ColorPMA & glowColor() const { return m_glowColor; }
    /// Set text glow color
    void setGlowColor(const Radiant::ColorPMA & glowColor) { m_glowColor = glowColor; }
    /// Set text glow color in non-premultiplied format
    void setGlowColor(float r, float g, float b, float a) { m_glowColor = Radiant::Color(r, g, b, a); }

    /// Get text sharpness
    float textSharpness() const { return m_textSharpness; }
    /// Set text sharpness. Can be used to blur the text.
    void setTextSharpness(float textSharpness) { m_textSharpness = textSharpness; }

    /// Get the amount of drop-shadow blur
    float dropShadowBlur() const { return m_dropShadowBlur; }
    /// Set the amount of drop-shadow blur. Values from 0 to 1.
    void setDropShadowBlur(float blur) { m_dropShadowBlur = blur; }

    /// Get the drop-shadow color
    const Radiant::ColorPMA & dropShadowColor() const { return m_dropShadowColor; }
    /// Set the drop-shadow color
    void setDropShadowColor(const Radiant::ColorPMA & dropShadowColor) { m_dropShadowColor = dropShadowColor; }
    /// Set the drop-shadow color in non-premultiplied format
    void setDropShadowColor(float r, float g, float b, float a) { m_dropShadowColor = Radiant::Color(r, g, b, a); }

    /// Get the drop-shadow offset
    const Nimble::Vector2f & dropShadowOffset() const { return m_dropShadowOffset; }
    /// Set the drop-shadow offset
    void setDropShadowOffset(const Nimble::Vector2f & offset) { m_dropShadowOffset = offset; }

  private:
    float m_fontRenderWidth;

    /// from 0 to 1
    float m_glow;
    Radiant::ColorPMA m_glowColor;

    float m_textSharpness;

    /// from 0 to 1
    float m_dropShadowBlur;
    Radiant::ColorPMA m_dropShadowColor;
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

  void TextStyle::setFontPixelSize(float sizeInPixels)
  {
    m_font.setPointSizeF(TextLayout::pixelToPointSize(sizeInPixels));
  }

  float TextStyle::fontPixelSize() const
  {
    return m_font.pixelSize() < 0
        ? TextLayout::pointToPixelSize(static_cast<float>(m_font.pointSizeF()))
        : static_cast<float>(m_font.pixelSize());
  }
}
#endif // LUMINOUS_STYLE_HPP
