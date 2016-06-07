/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if !defined(LUMINOUS_BLENDMODE_HPP)
#define LUMINOUS_BLENDMODE_HPP

#include <Radiant/Color.hpp>
#include <Luminous/Luminous.hpp>

namespace Luminous
{

  /// This class defines a blending mode used during rendering.
  class BlendMode
  {
  public:
    /// The blending equation. See
    /// http://www.opengl.org/sdk/docs/man3/xhtml/glBlendEquation.xml for details.
    enum Equation
    {
      /// Add colors
      ADD                       = GL_FUNC_ADD,
      /// Subtract colors
      SUBTRACT                  = GL_FUNC_SUBTRACT,
      /// Subtract colors reversily
      REVERSE_SUBTRACT          = GL_FUNC_REVERSE_SUBTRACT,
      /// Minimum of colors
      MIN                       = GL_MIN,
      /// Maximum of colors
      MAX                       = GL_MAX
    };

    /// Defines the pixel arithmetic used during blending. See
    /// http://www.opengl.org/sdk/docs/man3/xhtml/glBlendFunc.xml for details.
    enum Function
    {
      /// Ignore the color
      ZERO                      = GL_ZERO,
      /// Do not scale color in any way
      ONE                       = GL_ONE,
      /// Scale according to source color
      SOURCE_COLOR               = GL_SRC_COLOR,
      /// Scale according to source alpha
      SOURCE_ALPHA               = GL_SRC_ALPHA,
      /// Scale according to inverse of source color factors
      ONE_MINUS_SOURCE_COLOR       = GL_ONE_MINUS_SRC_COLOR,
      /// Scale according to inverse of source color alpha
      ONE_MINUS_SOURCE_ALPHA       = GL_ONE_MINUS_SRC_ALPHA,

      /// Scale according to destination color
      DESTINATION_COLOR          = GL_DST_COLOR,
      /// Scale according to destionation alpha
      DESTINATION_ALPHA          = GL_DST_ALPHA,
      /// Scale according to inverse of destination color factors
      ONE_MINUS_DESTINATION_COLOR  = GL_ONE_MINUS_DST_COLOR,
      /// Scale according to inverse of destination color alpha
      ONE_MINUS_DESTINATION_ALPHA  = GL_ONE_MINUS_DST_ALPHA,

      /// Use blend color set by setConstantColor
      CONSTANT_COLOR             = GL_CONSTANT_COLOR,
      /// Use blend alpha set by setConstantColor
      CONSTANT_ALPHA             = GL_CONSTANT_ALPHA,
      /// Use inverse of blend color set by setConstantColor
      ONE_MINUS_CONSTANT_COLOR     = GL_ONE_MINUS_CONSTANT_COLOR,
      /// Use inverse of blend alpha set by setConstantColor
      ONE_MINUS_CONSTANT_ALPHA     = GL_ONE_MINUS_CONSTANT_ALPHA,
      /// Saturate alpha
      ALPHA_SATURATE             = GL_SRC_ALPHA_SATURATE,

      // GL_SRC1_COLOR
      // GL_ONE_MINUS_SRC1_COLOR
      // GL_SRC1_ALPHA
      // GL_ONE_MINUS_SRC1_ALPHA
    };

  public:
    /// Returns default blend mode
    /// @return BlendMode having default values
    static BlendMode Default() { return BlendMode(); }

    /// Returns additive blending mode
    /// @return BlendMode having additive blending
    LUMINOUS_API static BlendMode Additive();
    /// Returns subtractive blending mode
    /// @return BlendMode having subtractive blending
    LUMINOUS_API static BlendMode Subtractive();

    /// Constructs new blend mode object
    LUMINOUS_API BlendMode();
    /// Constructs new blend mode object
    /// @param equation blending equation
    /// @param srcFunc source color blending function
    /// @param dstFunc destination color blending function
    LUMINOUS_API BlendMode(Equation equation, Function srcFunc, Function dstFunc);

    /// Set constant blending color
    /// @param color Color for constant blending color.
    void setConstantColor(const Radiant::Color & color) { m_color = color; }
    /// Returns blending color.
    /// @return Constant blending color
    const Radiant::Color & constantColor() const { return m_color; }

    /// Set blending equation
    /// @param eq Equation to use for blending
    void setEquation(Equation eq) { m_equation = eq; }
    /// Returns blending equation
    /// @return Equation to use for blending
    Equation equation() const { return m_equation; }

    /// Sets function to use when blending source
    /// @param func Function to use for blending source
    void setSourceFunction(Function func) { m_srcFunction = func; }
    /// Returns the function to use when blending source
    /// @return Function to use for blending source
    Function sourceFunction() const { return m_srcFunction; }

    /// Sets function to use when blending destination
    /// @param func Function to use for blending destination
    void setDestFunction(Function func) { m_dstFunction = func; }
    /// Sets function to use when blending destination
    /// @return Function to use for blending destination
    Function destFunction() const { return m_dstFunction; }

  private:
    Radiant::Color m_color;
    Equation m_equation;
    Function m_srcFunction;
    Function m_dstFunction;
  };

  /// Inequality comparison of two BlendMode objects
  /// lhs Blending mode on the left
  /// rhs Blending mode on the right
  inline bool operator!=(const BlendMode & lhs, const BlendMode & rhs)
  {
    return
      lhs.equation() != rhs.equation()
      || lhs.sourceFunction() != rhs.sourceFunction()
      || lhs.destFunction() != rhs.destFunction()
      || lhs.constantColor() != rhs.constantColor();
  }

  /// Equality comparison of two BlendMode objects
  /// lhs Blending mode on the left
  /// rhs Blending mode on the right
  inline bool operator==(const BlendMode & lhs, const BlendMode & rhs) { return !(lhs!=rhs); }
}
#endif // LUMINOUS_BLENDMODE_HPP
