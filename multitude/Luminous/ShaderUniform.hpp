#if !defined (LUMINOUS_SHADERUNIFORM_HPP)
#define LUMINOUS_SHADERUNIFORM_HPP

#include "Luminous/Luminous.hpp"
#include "Valuable/AttributeObject.hpp"
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
  struct ShaderUniform
  {
    enum Type
    {
      Unknown,
      Int,
      Int2,
      Int3,
      Int4,
      UnsignedInt,
      UnsignedInt2,
      UnsignedInt3,
      UnsignedInt4,
      Float,
      Float2,
      Float3,
      Float4,
      Float2x2,
      Float3x3,
      Float4x4,
    };
    ShaderUniform() { m_type = Unknown; };

    ShaderUniform(int data) { m_type = Int; m_data.i[0] = data; }
    ShaderUniform(unsigned int data) { m_type = UnsignedInt; m_data.u[0] = data; }
    ShaderUniform(float data) { m_type = Float; m_data.f[0] = data; }

    ShaderUniform(const Nimble::Vector2i & data) { m_type = Int2; memcpy(m_data.i, &data, sizeof(data)); }
    ShaderUniform(const Nimble::Vector3i & data) { m_type = Int3; memcpy(m_data.i, &data, sizeof(data)); }
    ShaderUniform(const Nimble::Vector4i & data) { m_type = Int4; memcpy(m_data.i, &data, sizeof(data)); }
    ShaderUniform(const Nimble::Vector2u & data) { m_type = UnsignedInt2; memcpy(m_data.u, &data, sizeof(data)); }
    ShaderUniform(const Nimble::Vector3u & data) { m_type = UnsignedInt3; memcpy(m_data.u, &data, sizeof(data)); }
    ShaderUniform(const Nimble::Vector4u & data) { m_type = UnsignedInt4; memcpy(m_data.u, &data, sizeof(data)); }
    ShaderUniform(const Nimble::Vector2f & data) { m_type = Float2; memcpy(m_data.i, &data, sizeof(data)); }
    ShaderUniform(const Nimble::Vector3f & data) { m_type = Float3; memcpy(m_data.i, &data, sizeof(data)); }
    ShaderUniform(const Nimble::Vector4f & data) { m_type = Float4; memcpy(m_data.i, &data, sizeof(data)); }
    ShaderUniform(const Nimble::Matrix2f & data) { m_type = Float2x2; memcpy(m_data.i, &data, sizeof(data)); }
    ShaderUniform(const Nimble::Matrix3f & data) { m_type = Float3x3; memcpy(m_data.i, &data, sizeof(data)); }
    ShaderUniform(const Nimble::Matrix4f & data) { m_type = Float4x4; memcpy(m_data.i, &data, sizeof(data)); }

    virtual const char * data() const { return reinterpret_cast<const char *>(&m_data); }
    virtual Type type() const { return m_type; }
  
    union {
      unsigned int u[16];
      int i[16];
      float f[16];
    } m_data;
    Type m_type;
  };
}
#endif // LUMINOUS_SHADERUNIFORM_HPP
