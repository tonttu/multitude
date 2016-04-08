/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if !defined (LUMINOUS_SHADERUNIFORM_HPP)
#define LUMINOUS_SHADERUNIFORM_HPP

#include "Luminous/Luminous.hpp"
#include "Valuable/Attribute.hpp"
#include "Valuable/AttributeColor.hpp"
#include "Valuable/AttributeInt.hpp"
#include "Valuable/AttributeFloat.hpp"
#include "Valuable/AttributeVector.hpp"
#include "Valuable/AttributeMatrix.hpp"

#include "Nimble/Vector2.hpp"
#include "Nimble/Vector3.hpp"
#include "Nimble/Vector4.hpp"
#include "Nimble/Matrix2.hpp"
#include "Nimble/Matrix3.hpp"
#include "Nimble/Matrix4.hpp"

#include <algorithm>

namespace Luminous
{

  /// A shader uniform. Shader uniforms are used to pass uniform variables to
  /// OpenGL shaders.
  struct ShaderUniform
  {
    /// Type of this uniform
    enum Type
    {
      /// Unknown type, can't be applied to shader program
      Unknown,
      /// Signed integer, GLSL-type "int"
      Int,
      /// 2-dimensional vector of signed integers, GLSL-type "ivec2"
      Int2,
      /// 3-dimensional vector of signed integers, GLSL-type "ivec3"
      Int3,
      /// 4-dimensional vector of signed integers, GLSL-type "ivec4"
      Int4,
      /// Unsigned integers, GLSL-type "uint"
      UnsignedInt,
      /// 2-dimensional vector of unsigned integers, GLSL-type "uvec2"
      UnsignedInt2,
      /// 3-dimensional vector of unsigned integers, GLSL-type "uvec3"
      UnsignedInt3,
      /// 4-dimensional vector of unsigned integers, GLSL-type "uvec4"
      UnsignedInt4,
      /// Floating point number, GLSL-type "float"
      Float,
      /// 2-dimensional vector of floats, GLSL-type "vec2"
      Float2,
      /// 3-dimensional vector of floats, GLSL-type "vec3"
      Float3,
      /// 4-dimensional vector of floats, GLSL-type "vec4"
      Float4,
      /// 2x2-matrix of floats, GLSL-type "mat2"
      Float2x2,
      /// 3x3-matrix of floats, GLSL-type "mat3"
      Float3x3,
      /// 4x4-matrix of floats, GLSL-type "mat4"
      Float4x4
    };
    /// Default constructor
    /// Sets the Type of the uniform to Unknown
    ShaderUniform() { m_type = Unknown; };

    /// Constructor for singed integer
    /// Sets Type of this uniform to Int
    /// @param data Value of the uniform
    ShaderUniform(int data) { m_type = Int; m_data.i[0] = data; }

    /// Constructor for unsigned integer
    /// Sets Type of this uniform to UnsignedInt
    /// @param data Value of the uniform
    ShaderUniform(unsigned int data) { m_type = UnsignedInt; m_data.u[0] = data; }

    /// Constructor for floating point number
    /// Sets Type of this uniform to Float
    /// @param data Value of the uniform
    ShaderUniform(float data) { m_type = Float; m_data.f[0] = data; }

    /// Constructor for 2-dimensional signed integer vector
    /// Sets Type of this uniform to Int2
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Vector2i & data) { m_type = Int2; memcpy(m_data.i, &data, sizeof(data)); }

    /// Constructor for 3-dimensional signed integer vector
    /// Sets Type of this uniform to Int3
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Vector3i & data) { m_type = Int3; memcpy(m_data.i, &data, sizeof(data)); }

    /// Constructor for 4-dimensional signed integer vector
    /// Sets Type of this uniform to Int4
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Vector4i & data) { m_type = Int4; memcpy(m_data.i, &data, sizeof(data)); }

    /// Constructor for 2-dimensional unsigned integer vector
    /// Sets Type of this uniform to UnsignedInt2
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Vector2u & data) { m_type = UnsignedInt2; memcpy(m_data.u, &data, sizeof(data)); }

    /// Constructor for 3-dimensional unsigned integer vector
    /// Sets Type of this uniform to UnsignedInt3
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Vector3u & data) { m_type = UnsignedInt3; memcpy(m_data.u, &data, sizeof(data)); }

    /// Constructor for 4-dimensional unsigned integer vector
    /// Sets Type of this uniform to UnsignedInt4
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Vector4u & data) { m_type = UnsignedInt4; memcpy(m_data.u, &data, sizeof(data)); }

    /// Constructor for 2-dimensional floating point vector
    /// Sets Type of this uniform to Float2
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Vector2f & data) { m_type = Float2; memcpy(m_data.f, &data, sizeof(data)); }

    /// Constructor for 3-dimensional floating point vector
    /// Sets Type of this uniform to Float3
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Vector3f & data) { m_type = Float3; memcpy(m_data.f, &data, sizeof(data)); }

    /// Constructor for 4-dimensional floating point vector
    /// Sets Type of this uniform to Float4
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Vector4f & data) { m_type = Float4; memcpy(m_data.f, &data, sizeof(data)); }

    /// Constructor for 4-dimensional floating point vector (color)
    /// Sets Type of this uniform to Float4
    /// @param data Value of the uniform
    ShaderUniform(const Radiant::ColorPMA & data) { m_type = Float4; memcpy(m_data.f, data.data(), sizeof(data)); }

    /// Constructor for 2x2-dimensional matrix
    /// Sets Type of this uniform to Float2x2
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Matrix2f & data) { m_type = Float2x2; memcpy(m_data.f, &data, sizeof(data)); }

    /// Constructor for 3x3-dimensional matrix
    /// Sets Type of this uniform to Float3x3
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Matrix3f & data) { m_type = Float3x3; memcpy(m_data.f, &data, sizeof(data)); }

    /// Constructor for 4x4-dimensional matrix
    /// Sets Type of this uniform to Float4x4
    /// @param data Value of the uniform
    ShaderUniform(const Nimble::Matrix4f & data) { m_type = Float4x4; memcpy(m_data.f, &data, sizeof(data)); }

    /// Returns the pointer to the stored data
    /// @return The pointer to the stored value
    virtual const char * data() const { return reinterpret_cast<const char *>(&m_data); }

    /// Returns the type of the stored data
    /// @return The type of the stored value
    virtual Type type() const { return m_type; }
  
    /// @cond
    union {
      unsigned int u[16];
      int i[16];
      float f[16];
    } m_data;
    Type m_type;
    /// @endcond
  };
}

#endif // LUMINOUS_SHADERUNIFORM_HPP
