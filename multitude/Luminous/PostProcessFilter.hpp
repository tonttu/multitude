#ifndef POSTPROCESSOR_HPP
#define POSTPROCESSOR_HPP

#include <Luminous/RenderContext.hpp>
#include <Luminous/RenderTarget.hpp>
#include <Luminous/Style.hpp>

namespace Luminous
{
  /// This class represents one filter in the post process queue that is
  /// applied to the scene before rendering the final image.
  class LUMINOUS_API PostProcessFilter : public Patterns::NotCopyable
  {
  public:
    PostProcessFilter();
    virtual ~PostProcessFilter();

    virtual void initialize(Luminous::RenderContext & rc);

    /// Returns the style that is used to render the scene
    virtual Luminous::Style style() const;

    /// Clears the render target
    virtual void begin(Luminous::RenderContext & rc);
    /// Draws a context sized quad with style from calling style().
    virtual void apply(Luminous::RenderContext & rc);

    /// @todo allow user to add own uniform block
    //template<typename UniformBlock>
    //virtual void applyUniforms(UniformBlock * b);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    int order() const { return m_order; }
    void setOrder(int order) { m_order = order; }

    const Luminous::RenderTarget & renderTarget() const { return m_renderTarget; }

  protected:
    Luminous::RenderTarget m_renderTarget;

    Luminous::Texture m_framebuffer;
    Luminous::RenderBuffer m_depthBuffer;

  private:
    bool m_enabled;
    int m_order;
  };
}
#endif // POSTPROCESSOR_HPP
