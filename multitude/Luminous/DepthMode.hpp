/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

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
      /// Never pass comparison of depth test
      NEVER         = GL_NEVER,
      /// Pass the incoming value if it is less than the stored value
      LESS          = GL_LESS,
      /// Pass if the values are equal
      EQUAL         = GL_EQUAL,
      /// Pass if the incoming value is less or equal
      LESS_EQUAL    = GL_LEQUAL,
      /// Pass the incoming value if it is greeater than the stored value
      GREATER       = GL_GREATER,
      /// Pass if the values are not equal
      NOT_EQUAL     = GL_NOTEQUAL,
      /// Pass if the incoming value is greater or equal
      GREATER_EQUAL = GL_GEQUAL,
      /// Pass always
      ALWAYS        = GL_ALWAYS
    };

  public:
    /// Default depth mode
    /// @return DepthMode with default settings
    static DepthMode Default() { return DepthMode(); }

    /// Construct a new depth mode.
    /// @param function depth test function
    /// @param range depth range
    LUMINOUS_API DepthMode(Function function = LESS_EQUAL,
                           Nimble::Rangef range = Nimble::Rangef(0.f, 1.f));

    /// Set function for depth comparisons
    /// @param function Function to use in comparisons
    void setFunction(Function function) { m_function = function; }
    /// Returns the function used in depth comparisons
    /// @return Function to use in comparisons
    Function function() const { return m_function; }

    /// Specify the mapping of depth values from normalized device coordinates to window coordinates.
    /// See http://www.opengl.org/sdk/docs/man3/xhtml/glDepthRange.xml for details.
    /// @param range Depth range to use in window coordinates
    void setRange(const Nimble::Rangef & range) { m_range = range; }
    /// Get the mapping of depth values
    /// @sa setRange
    /// @return Current range used for depth values
    const Nimble::Rangef & range() const { return m_range; }

  private:
    Function m_function;
    Nimble::Rangef m_range;
  };

  /// Inequality comparison of DepthMode objects.
  /// @param lhs Left side operand
  /// @param rhs Right side operand
  /// @return true if the modes are not equal; otherwise false
  inline bool operator!=(const DepthMode & lhs, const DepthMode & rhs)
  {
    return
      lhs.function() != rhs.function()
      || lhs.range() != rhs.range();
  }

  /// Equality comparison of DepthMode objects.  For depth modes to be equal,
  /// both the function and range must be the same.
  /// @param lhs Left side operand
  /// @param rhs Right side operand
  /// @return true if the modes are equal; otherwise false
  inline bool operator==(const DepthMode & lhs, const DepthMode & rhs) { return !(lhs!=rhs); }
}

#endif // LUMINOUS_DEPTHMODE_HPP
