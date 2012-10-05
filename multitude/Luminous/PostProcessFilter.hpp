#ifndef POSTPROCESSOR_HPP
#define POSTPROCESSOR_HPP

#include <Luminous/RenderContext.hpp>
#include <Luminous/RenderTarget.hpp>
#include <Luminous/Style.hpp>

namespace Luminous
{
  /// This class represents one filter in the post process queue that is
  /// applied to the scene before rendering the final image.
  ///
  /// To create your own custom post processing filter by using a custom shader
  /// you should derive this class and override the appropriate functions. For
  /// most cases overriding style() is sufficient. By default the the filter uses
  /// the style from style() to render a context-sized quad to the bound render
  /// target.
  class LUMINOUS_API PostProcessFilter : public Patterns::NotCopyable
  {
  public:
    /// Creates a new post process filter
    PostProcessFilter();
    virtual ~PostProcessFilter();

    /// Initializes the filter. By default attaches a color and depth attachments
    /// to the render target and resizes the render target and attachments.
    virtual void initialize(Luminous::RenderContext & rc);

    /// Returns the style that is used to render the scene
    /// Override this function if you want to specify a custom style,
    /// ie. for using a custom shader.
    virtual Luminous::Style style() const;

    /// Clears the render target
    virtual void begin(Luminous::RenderContext & rc);
    /// Draws a context sized quad with style from calling style().
    virtual void apply(Luminous::RenderContext & rc);

    /// If the filter is disabled, it is skipped
    /// @return true if enabled, false otherwise
    bool enabled() const;

    /// Sets the enabled state of the filter
    void setEnabled(bool enabled);

    int order() const;
    void setOrder(int order);

    Luminous::RenderTarget & renderTarget();
    const Luminous::RenderTarget & renderTarget() const;

    /// Color buffer texture used for rendering the scene.
    /// Use this texture as the source for the post processing filter.
    /// @return color buffer texture
    const Luminous::Texture & texture() const;

    /// Depth buffer used for rendering the scene.
    const Luminous::RenderBuffer & depthBuffer() const;

  private:
    class D;
    D * m_d;
  };
}
#endif // POSTPROCESSOR_HPP
