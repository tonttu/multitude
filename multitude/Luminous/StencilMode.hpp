#if !defined (LUMINOUS_STENCILMODE_HPP)
#define LUMINOUS_STENCILMODE_HPP

#include "Luminous/Luminous.hpp"

#include <map>

namespace Luminous
{
  class StencilMode
  {
  public:
    enum Face
    {
      Front = GL_FRONT,
      Back = GL_BACK,
      FrontAndBack = GL_FRONT_AND_BACK
    };

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

    LUMINOUS_API void setFunction(Face face, Function function, int ref, unsigned int mask);

    LUMINOUS_API void setOperation(Face face, Operation stencilFail, Operation depthFail, Operation pass);

    Function frontFunction() const { return m_frontFunction; }
    int frontRefValue() const { return m_frontRefValue; }
    unsigned int frontMaskValue() const { return m_frontMaskValue; }

    Operation frontStencilFailOp() const { return m_frontStencilFail; }
    Operation frontDepthFailOp() const { return m_frontDepthFail; }
    Operation frontPassOp() const { return m_frontPass; }

    Function backFunction() const { return m_backFunction; }
    int backRefValue() const { return m_backRefValue; }
    unsigned int backMaskValue() const { return m_backMaskValue; }

    Operation backStencilFailOp() const { return m_backStencilFail; }
    Operation backDepthFailOp() const { return m_backDepthFail; }
    Operation backPassOp() const { return m_backPass; }

    LUMINOUS_API bool equal(const StencilMode & o) const;

  private:
    Operation m_frontStencilFail;
    Operation m_frontDepthFail;
    Operation m_frontPass;

    Function m_frontFunction;
    int m_frontRefValue;
    unsigned int m_frontMaskValue;

    Operation m_backStencilFail;
    Operation m_backDepthFail;
    Operation m_backPass;

    Function m_backFunction;
    int m_backRefValue;
    unsigned int m_backMaskValue;
  };

  inline bool operator!=(const StencilMode & lhs, const StencilMode & rhs)
  {
    return !lhs.equal(rhs);
  }

  inline bool operator==(const StencilMode & lhs, const StencilMode & rhs) { return lhs.equal(rhs); }
}

#endif // LUMINOUS_STENCILMODE_HPP
