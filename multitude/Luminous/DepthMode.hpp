#if !defined (LUMINOUS_DEPTHMODE_HPP)
#define LUMINOUS_DEPTHMODE_HPP

#include "Nimble/Range.hpp"
#include "Luminous/Luminous.hpp"

namespace Luminous
{
  /// This class defines the depth comparison mode used during rendering.
  class DepthMode
  {
  public:
    /// Specifies the value used for depth buffer comparisons.
    /// See http://www.opengl.org/sdk/docs/man3/xhtml/glDepthFunc.xml for details.
    enum Function
    {
      NEVER         = GL_NEVER,
      LESS          = GL_LESS,
      EQUAL         = GL_EQUAL,
      LESS_EQUAL    = GL_LEQUAL,
      GREATER       = GL_GREATER,
      NOT_EQUAL     = GL_NOTEQUAL,
      GREATER_EQUAL = GL_GEQUAL,
      ALWAYS        = GL_ALWAYS
    };

  public:
    static DepthMode Default() { return DepthMode(); }

    /// Construct a default depth mode. The default mode consists of LESS
    /// function with range set to (0, 1).
    LUMINOUS_API DepthMode();

    void setFunction(Function function) { m_function = function; }
    Function function() const { return m_function; }

    /// Specify the mapping of depth values from normalized device coordinates to window coordinates.
    /// See http://www.opengl.org/sdk/docs/man3/xhtml/glDepthRange.xml for details.
    void setRange(const Nimble::Rangef & range) { m_range = range; }
    /// Get the mapping of depth values
    /// @sa setRange
    const Nimble::Rangef & range() const { return m_range; }

  private:
    Function m_function;
    Nimble::Rangef m_range;
  };

  inline bool operator!=(const DepthMode & lhs, const DepthMode & rhs)
  {
    return
      lhs.function() != rhs.function()
      || lhs.range() != rhs.range();
  }

  inline bool operator==(const DepthMode & lhs, const DepthMode & rhs) { return !(lhs!=rhs); }
}

#endif // LUMINOUS_DEPTHMODE_HPP
