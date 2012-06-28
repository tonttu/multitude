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

#include <QString>
#include <vector>

namespace Luminous
{
  struct ShaderUniform
  {
    enum Type
    {
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

    ShaderUniform(const QString & name)
      : name(name), index(-1)
    {
    }

    virtual const char * data() const = 0;
    virtual Type type() const = 0;
  
    QString name;
    int index;        // Cached shader index location
  };

  // Empty base
  template <typename T>
  class ShaderUniformT : public ShaderUniform
  {
  public:
  };


#define UNIFORMCONST(TYPE, TYPENAME) \
  template <> \
  class ShaderUniformT<TYPE> : public ShaderUniform \
  { \
  public: \
    ShaderUniformT(const QString & name, const TYPE & value) : ShaderUniform(name), m_value(value) {} \
    virtual ShaderUniform::Type type() const { return TYPENAME; }; \
    virtual const char * data() const OVERRIDE { return (const char *)&m_value; } \
  private: \
    TYPE m_value; \
  };

#define UNIFORMATTR(ATTRTYPE, TYPENAME) \
  template <> \
  class ShaderUniformT< ATTRTYPE > : public ShaderUniform \
  { \
  public: \
    ShaderUniformT(const QString & name, const ATTRTYPE & value) : ShaderUniform(name), m_value(value) {} \
    virtual ShaderUniform::Type type() const { return TYPENAME; }; \
    virtual const char * data() const OVERRIDE { return (const char *)&m_value.value(); } \
  private: \
    const ATTRTYPE & m_value; \
  };

  // Only define the ones that are supported
  UNIFORMCONST(int, ShaderUniform::Int);
  UNIFORMCONST(unsigned int, ShaderUniform::UnsignedInt);
  UNIFORMCONST(float, ShaderUniform::Float);

  UNIFORMCONST(Nimble::Vector2i, ShaderUniform::Int2);
  UNIFORMCONST(Nimble::Vector3i, ShaderUniform::Int3);
  UNIFORMCONST(Nimble::Vector4i, ShaderUniform::Int4);

  UNIFORMCONST(Nimble::Vector2T<unsigned int>, ShaderUniform::UnsignedInt2);
  UNIFORMCONST(Nimble::Vector3T<unsigned int>, ShaderUniform::UnsignedInt3);
  UNIFORMCONST(Nimble::Vector4T<unsigned int>, ShaderUniform::UnsignedInt4);

  UNIFORMCONST(Nimble::Vector2f, ShaderUniform::Float2);
  UNIFORMCONST(Nimble::Vector3f, ShaderUniform::Float3);
  UNIFORMCONST(Nimble::Vector4f, ShaderUniform::Float4);
  UNIFORMCONST(Radiant::Color, ShaderUniform::Float4);

  UNIFORMCONST(Nimble::Matrix2f, ShaderUniform::Float2x2);
  UNIFORMCONST(Nimble::Matrix3f, ShaderUniform::Float3x3);
  UNIFORMCONST(Nimble::Matrix4f, ShaderUniform::Float4x4);

  /// Attributes
  UNIFORMATTR(Valuable::AttributeInt, ShaderUniform::Int);
  UNIFORMATTR(Valuable::AttributeUInt32, ShaderUniform::UnsignedInt);
  UNIFORMATTR(Valuable::AttributeFloat, ShaderUniform::Float);

  UNIFORMATTR(Valuable::AttributeVector2i, ShaderUniform::Int2);
  UNIFORMATTR(Valuable::AttributeVector3i, ShaderUniform::Int3);
  UNIFORMATTR(Valuable::AttributeVector4i, ShaderUniform::Int4);
  
  UNIFORMATTR(Valuable::AttributeVector2f, ShaderUniform::Float2);
  UNIFORMATTR(Valuable::AttributeVector3f, ShaderUniform::Float3);
  UNIFORMATTR(Valuable::AttributeVector4f, ShaderUniform::Float4);
  UNIFORMATTR(Valuable::AttributeColor, ShaderUniform::Float4);

  UNIFORMATTR(Valuable::AttributeMatrix2f, ShaderUniform::Float2x2);
  UNIFORMATTR(Valuable::AttributeMatrix3f, ShaderUniform::Float3x3);
  UNIFORMATTR(Valuable::AttributeMatrix4f, ShaderUniform::Float4x4);
  
#undef UNIFORMCONST
#undef UNIFORMATTR
}
#endif // LUMINOUS_SHADERUNIFORM_HPP
