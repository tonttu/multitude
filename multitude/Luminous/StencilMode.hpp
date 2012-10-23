#if !defined (LUMINOUS_STENCILMODE_HPP)
#define LUMINOUS_STENCILMODE_HPP

#include "Luminous/Luminous.hpp"

namespace Luminous
{
  class StencilMode
  {
  public:
    enum Operation
    {
      Keep            = GL_KEEP,
      Zero            = GL_ZERO,
      Replace         = GL_REPLACE,
      Increment       = GL_INCR,
      IncrementWrap   = GL_INCR_WRAP,
      Decrement       = GL_DECR,
      DecrementWrap   = GL_DECR_WRAP,
      Invert          = GL_INVERT,
    };

    enum Function
    {
      Never         = GL_NEVER,
      Less          = GL_LESS,
      LessEqual     = GL_LEQUAL,
      Greater       = GL_GREATER,
      GreaterEqual  = GL_GEQUAL,
      Equal         = GL_EQUAL,
      NotEqual      = GL_NOTEQUAL,
      Always        = GL_ALWAYS,
    };

  public:
    static StencilMode Default() { return StencilMode(); }

    LUMINOUS_API StencilMode();

    void setFunction(Function function, int ref, unsigned int mask)
    {
      m_function = function;
      m_refValue = ref;
      m_maskValue = mask;
    }

    Function function() const { return m_function; }
    int refValue() const { return m_refValue; }
    unsigned int maskValue() const { return m_maskValue; }

    void setStencilFailOperation(Operation op) { m_stencilFail = op; }
    Operation stencilFailOperation() const { return m_stencilFail; }

    void setDepthFailOperation(Operation op) { m_depthFail = op; }
    Operation depthFailOperation() const { return m_depthFail; }

    void setPassOperation(Operation op) { m_pass = op;}
    Operation passOperation() const { return m_pass; }

    void setOperation(Operation stencilFail, Operation depthFail, Operation pass)
    {
      m_stencilFail = stencilFail;
      m_depthFail = depthFail;
      m_pass = pass;
    }

  private:
    Operation m_stencilFail;
    Operation m_depthFail;
    Operation m_pass;

    Function m_function;
    int m_refValue;
    unsigned int m_maskValue;
  };

  inline bool operator!=(const StencilMode & lhs, const StencilMode & rhs)
  {
    return
      lhs.stencilFailOperation() != rhs.stencilFailOperation()
      || lhs.depthFailOperation() != rhs.depthFailOperation()
      || lhs.passOperation() != rhs.passOperation()
      || lhs.function() != rhs.function()
      || lhs.refValue() != rhs.refValue()
      || lhs.maskValue() != rhs.maskValue();
  }

  inline bool operator==(const StencilMode & lhs, const StencilMode & rhs) { return !(lhs!=rhs); }
}
#endif // LUMINOUS_STENCILMODE_HPP
