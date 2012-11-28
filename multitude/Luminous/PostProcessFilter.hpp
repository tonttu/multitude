#ifndef POSTPROCESSFILTER_HPP
#define POSTPROCESSFILTER_HPP

#include <Valuable/Node.hpp>

#include <Luminous/Style.hpp>

namespace Luminous
{
  class PostProcessContext;
  class RenderContext;

  /** Post-processing filters are tools that can be used to process the final
      output image of a rendered scene. Each filter describes one post-processing
      pass that is applied on the whole window context. The post-processing
      framework works by rendering the whole scene into an off-screen render
      target and then passing the rendered image as a texture to first filter
      in the post-process chain. The image is then used as a source for rendering
      for the next filter with a custom shader.

      To create a custom post-processing filter the typical way is to derive from
      this class and write a shader that performs the desired effect. To use
      the custom shader you should override the filter() function, set the shader
      and corresponding parameters to the style parameter and then call the
      base implementation that does the actual work.
  */
  class LUMINOUS_API PostProcessFilter : public Valuable::Node
  {
  public:
    /// Creates a new post processing filter
    PostProcessFilter(Valuable::Node * host = 0, const QByteArray & name = "");
    virtual ~PostProcessFilter();

    /// Called by the application when the scene is updated.
    virtual void update(float dt);

    /// Initialize is called when a context is created for the filter. Override
    /// this if you need to specify additional parameter to the context, such as
    /// extra buffer attachments.
    /// Note: this function needs to be thread-safe.
    /// @param rc the current render context
    /// @param ctx context for the render side post-processing
    virtual void initialize(Luminous::RenderContext & rc,
                            Luminous::PostProcessContext & ctx) const;

    /// Performs the filtering operation by rendering the scene using the
    /// specified style.
    /// Note: this function needs to be thread-safe.
    /// @param rc the current render context
    /// @param ctx context for the render side post-processing
    /// @param style The style used for rendering ie. the filtering
    virtual void filter(Luminous::RenderContext & rc,
                        Luminous::PostProcessContext & ctx,
                        Luminous::Style style = Luminous::Style()) const;

    /// Checks if the filter is enabled, disabled filters will be skipped.
    /// @return true if enabled, false otherwise.
    bool enabled() const;
    /// Sets the enabled state of the filter.
    /// @param enabled true to enable, false to disable.
    void setEnabled(bool enabled);

  private:
    class D;
    D * m_d;
  };

  typedef std::shared_ptr<PostProcessFilter> PostProcessFilterPtr;
  typedef std::map<unsigned, PostProcessFilterPtr> PostProcessFilters;
}
#endif // POSTPROCESSFILTER_HPP
