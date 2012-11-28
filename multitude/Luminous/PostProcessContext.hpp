#ifndef POSTPROCESSCONTEXT_HPP
#define POSTPROCESSCONTEXT_HPP

#include <Luminous/RenderContext.hpp>
#include <Luminous/RenderTarget.hpp>
#include <Luminous/Style.hpp>
#include <Luminous/PostProcessFilter.hpp>

namespace Luminous
{
  /// PostProcessContext is the render-thread specific context that is used
  /// for post processing to work in a multi-context environment.
  /// Use PostProcesFilter for defining custom filters.
  class LUMINOUS_API PostProcessContext : public Patterns::NotCopyable
  {
  public:
    /// Creates a new post process filter
    /// @param filter filter that this context belongs to
    PostProcessContext(const PostProcessFilterPtr filter);
    virtual ~PostProcessContext();

    /// Initializes the filter. By default attaches a color and depth attachments
    /// to the render target and resizes the render target and attachments.
    /// @param rc Context of the current render thread
    void initialize(Luminous::RenderContext & rc);

    /// Performs the filtering operation by setting the style values and
    /// calling PostProcessFilter::filter()
    /// @param rc Context of the current render thread
    void doFilter(Luminous::RenderContext & rc);

    /// Checks if the filter is enabled, disabled filters will be skipped.
    /// @return true if enabled, false otherwise.
    bool enabled() const;

    /// Returns the render target used for rendering the scene
    Luminous::RenderTarget & renderTarget();
    /// @copydoc renderTarget
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

  typedef std::shared_ptr<PostProcessContext> PostProcessContextPtr;
}
#endif // POSTPROCESSCONTEXT_HPP
