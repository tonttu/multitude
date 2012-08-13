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
      Never         = GL_NEVER,
      Less          = GL_LESS,
      Equal         = GL_EQUAL,
      LessEqual     = GL_LEQUAL,
      Greater       = GL_GREATER,
      NotEqual      = GL_NOTEQUAL,
      GreaterEqual  = GL_GEQUAL,
      Always        = GL_ALWAYS,
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
}
#endif // LUMINOUS_DEPTHMODE_HPP