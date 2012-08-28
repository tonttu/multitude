#ifndef POSTPROCESSOR_HPP
#define POSTPROCESSOR_HPP

#include <Luminous/RenderContext.hpp>
#include <Luminous/RenderTarget.hpp>
#include <Luminous/Style.hpp>

namespace Luminous
{
  class LUMINOUS_API PostProcessFilter : public Patterns::NotCopyable
  {
  public:
    PostProcessFilter();
    virtual ~PostProcessFilter();

    virtual void initialize(RenderContext & rc);

    virtual Luminous::Style style() const;

    virtual void begin(RenderContext & rc);
    virtual bool end(RenderContext & rc);

    /// @todo allow user to add own uniform block
    //template<typename UniformBlock>
    //virtual void applyUniforms(UniformBlock * b);

    bool enabled() const { return m_enabled; }

    const RenderTarget & renderTarget() const { return m_renderTarget; }

    int order;

  protected:
    bool m_enabled;

    Luminous::RenderTarget m_renderTarget;

    Luminous::Texture m_framebuffer;
    Luminous::RenderBuffer m_depthBuffer;
  };
}
#endif // POSTPROCESSOR_HPP
