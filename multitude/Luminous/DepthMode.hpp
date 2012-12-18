#if !defined (LUMINOUS_DEPTHMODE_HPP)
#define LUMINOUS_DEPTHMODE_HPP

#include "Nimble/Range.hpp"
#include "Luminous/Luminous.hpp"

namespace Luminous
{
  class DepthMode
  {
  public:
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

    LUMINOUS_API DepthMode();

    void setFunction(Function function) { m_function = function; }
    Function function() const { return m_function; }

    void setRange(const Nimble::Rangef & range) { m_range = range; }
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
