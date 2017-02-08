/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef POSTPROCESSCONTEXT_HPP
#define POSTPROCESSCONTEXT_HPP

#include <Luminous/RenderContext.hpp>
#include <Luminous/FrameBuffer.hpp>
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
    explicit PostProcessContext(const PostProcessFilterPtr filter);
    virtual ~PostProcessContext();

    /// Initializes the filter. By default attaches a color and depth attachments
    /// to the frame buffer and resizes the frame buffer and attachments.
    /// @param rc Context of the current render thread
    void initialize(Luminous::RenderContext & rc, Nimble::Size size);

    /// Performs the filtering operation by setting the style values and
    /// calling PostProcessFilter::filter()
    /// @param rc Context of the current render thread
    void doFilter(Luminous::RenderContext & rc, Nimble::Matrix3f textureMatrix = Nimble::Matrix3f::IDENTITY);

    /// Checks if the filter is enabled, disabled filters will be skipped.
    /// @return true if enabled, false otherwise.
    bool enabled() const;

    /// Returns the order of the filter corresponding to this context.
    /// @return order index
    unsigned int order() const;

    /// Returns a reference to the filter that owns this context
    /// @return filter pointer to post-process filter
    const PostProcessFilterPtr & filter() const;

    /// Returns the frame buffer used for rendering the scene
    Luminous::FrameBuffer & frameBuffer();
    /// @copydoc frameBuffer
    const Luminous::FrameBuffer & frameBuffer() const;

    /// Color buffer texture used for rendering the scene.
    /// Use this texture as the source for the post processing filter.
    /// @return color buffer texture
    const Luminous::Texture & texture() const;

    /// Combined depth and stencil buffer used for rendering the scene.
    const Luminous::RenderBuffer & depthStencilBuffer() const;

  private:
    class D;
    D * m_d;
  };

  /// A shared pointer to PostProcessContext
  typedef std::shared_ptr<PostProcessContext> PostProcessContextPtr;
}
#endif // POSTPROCESSCONTEXT_HPP
