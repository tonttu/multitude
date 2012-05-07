#if !defined (LUMINOUS_RENDERCONTEXT2_HPP)
#define LUMINOUS_RENDERCONTEXT2_HPP

#include "Luminous/Luminous.hpp"

namespace Luminous
{
  class RenderContext2
  {
  public:
    /// Returns the current frame
    virtual size_t frame() const = 0;
    /// Returns the framerate (frames per second)
    virtual float framerate() const = 0;

    /// Binds a set of vertexbuffers and their vertex descriptions
    virtual void bind(VertexAttributeBinding & binding) = 0;
    /// Issue a drawcall for the current state
    virtual void draw(PrimitiveType type, size_t offset, size_t primitiveCount) = 0;
  };
}
#endif // LUMINOUS_RENDERCONTEXT2_HPP