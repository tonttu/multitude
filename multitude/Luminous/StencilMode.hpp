/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if !defined (LUMINOUS_STENCILMODE_HPP)
#define LUMINOUS_STENCILMODE_HPP

#include "Luminous.hpp"
#include "RenderDefines.hpp"

#include <map>

namespace Luminous
{

  /// This class defines the stencil buffer operation mode.
  /// Back and front faces of the primitives have both their separate and own stencil
  /// operations.
  class StencilMode
  {
  public:

    /// Enumeration for stencil actions
    /// See http://www.opengl.org/sdk/docs/man/xhtml/glStencilOp.xml for details
    enum Operation
    {
      /// Keep the current value
      Keep            = GL_KEEP,
      /// Set stencil value to 0
      Zero            = GL_ZERO,
      /// replace current value
      Replace         = GL_REPLACE,
      /// Increment current value.
      Increment       = GL_INCR,
      /// Increment current value. Wrap in case of overflow
      IncrementWrap   = GL_INCR_WRAP,
      /// Decrement current value
      Decrement       = GL_DECR,
      /// Decrement current value. Wrap in case of underflow
      DecrementWrap   = GL_DECR_WRAP,
      /// Bitvise invert current value
      Invert          = GL_INVERT,
    };

    /// Enumeration for stencil test
    /// See http://www.opengl.org/sdk/docs/man/xhtml/glStencilFunc.xml for details
    enum Function
    {
      /// Never pass the test
      Never         = GL_NEVER,
      /// Pass if the reference value is less than stencil value
      Less          = GL_LESS,
      /// Pass if the reference value is less than or equal to stencil value
      LessEqual     = GL_LEQUAL,
      /// Pass if the reference value is greater than stencil value
      Greater       = GL_GREATER,
      /// Pass if the reference value is greater than or equal to stencil value
      GreaterEqual  = GL_GEQUAL,
      /// Pass if the reference value is equal to stencil value
      Equal         = GL_EQUAL,
      /// Pass if the refence value is inequal to stencil value
      NotEqual      = GL_NOTEQUAL,
      /// Always pass
      Always        = GL_ALWAYS,
    };

  public:
    /// Returns the default StencilMode object
    /// @return Default stencil mode
    static StencilMode Default() { return StencilMode(); }

    /// Construct new StencilMode object
    LUMINOUS_API StencilMode();

    /// Set test functionality for stencil operations
    /// @param face Facing of the primitives for this stencil test
    /// @param function Test function to use
    /// @param ref Reference value to use in stencil tests
    /// @param mask Mask to use in stencil operations
    LUMINOUS_API void setFunction(Face face, Function function, int ref, unsigned int mask);

    /// Set stencil operations
    /// @param face Facing of the primitives for this stencil operation
    /// @param stencilFail Action to take when stencil test fails
    /// @param depthFail Action to take if stencil test passes and depth test fails
    /// @param pass Action to take when both stencil and depth test passes
    LUMINOUS_API void setOperation(Face face, Operation stencilFail, Operation depthFail, Operation pass);

    /// Returns test function used for front facing primitives
    /// @return Test function for front faces
    Function frontFunction() const { return m_frontFunction; }
    /// Returns reference value used in stencil tests of front facing primitives
    /// @return Reference value used for front faces
    int frontRefValue() const { return m_frontRefValue; }
    /// Returns mask value used in stencil tests of front facing primitives
    /// @return mask Mask used in stencil tests of front faces
    unsigned int frontMaskValue() const { return m_frontMaskValue; }

    /// Returns stencil operation when front facing primitive doesn't pass stencil test
    /// @return Operation on stencil test fail
    Operation frontStencilFailOp() const { return m_frontStencilFail; }
    /// Returns stencil operation when front facing primitive doesn't pass depth test
    /// @return Operation on depth test fail
    Operation frontDepthFailOp() const { return m_frontDepthFail; }
    /// Returns stencil operation when front facing primitive passes both stencil and depth test
    /// @return Operation on stencil and depth test pass
    Operation frontPassOp() const { return m_frontPass; }

    /// Returns test function used for back facing primitives
    /// @return Test function for backfaces
    Function backFunction() const { return m_backFunction; }
    /// Returns reference value used in stencil tests of back facing primitives
    /// @return Reference value used for back faces
    int backRefValue() const { return m_backRefValue; }
    /// Returns mask value used in stencil tests of back facing primitives
    /// @return mask Mask used in stencil tests of back faces
    unsigned int backMaskValue() const { return m_backMaskValue; }

    /// Returns stencil operation when back facing primitive doesn't pass stencil test
    /// @return Operation on stencil test fail
    Operation backStencilFailOp() const { return m_backStencilFail; }
    /// Returns stencil operation when back facing primitive doesn't pass depth test
    /// @return Operation on depth test fail
    Operation backDepthFailOp() const { return m_backDepthFail; }
    /// Returns stencil operation when back facing primitive passes both stencil and depth test
    /// @return Operation on stencil and depth test pass
    Operation backPassOp() const { return m_backPass; }

    /// Equality comparison of stencil modes
    /// @param o Other StencilMode
    /// @return True if the modes were equal
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

  /// Inequality comparison between two StencilMode operators
  /// @param lhs Left hand operand
  /// @param rhs Right hand operand
  /// @return Were the objects inequal
  inline bool operator!=(const StencilMode & lhs, const StencilMode & rhs)
  {
    return !lhs.equal(rhs);
  }

  /// Equality comparison between two StencilMode operators
  /// @param lhs Left hand operand
  /// @param rhs Right hand operand
  /// @return Were the objects equal
  inline bool operator==(const StencilMode & lhs, const StencilMode & rhs) { return lhs.equal(rhs); }
}

#endif // LUMINOUS_STENCILMODE_HPP
