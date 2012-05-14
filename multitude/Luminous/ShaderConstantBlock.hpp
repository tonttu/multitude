#if !defined (LUMINOUS_SHADERCONSTANTBLOCK_HPP)
#define LUMINOUS_SHADERCONSTANTBLOCK_HPP

#include "Luminous/RenderResource.hpp"
#include "Luminous/ShaderConstant.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector3.hpp>
#include <Nimble/Vector4.hpp>
#include <Nimble/Matrix2.hpp>
#include <Nimble/Matrix3.hpp>
#include <Nimble/Matrix4.hpp>
#include <QString>

namespace Luminous
{
  class ShaderConstantBlock
    : public RenderResource
  {
  public:
    ShaderConstantBlock(RenderResource::Id id, RenderDriver & driver);
    LUMINOUS_API ~ShaderConstantBlock();

    LUMINOUS_API bool addConstant(const ShaderConstant & constant);

    template <typename T>
    LUMINOUS_API bool addConstant(const QString & name, const T & value);

    LUMINOUS_API void clear();

    //LUMINOUS_API const ShaderConstant & constant(size_t index) const;
    //LUMINOUS_API size_t constantCount() const;

    LUMINOUS_API const char * data() const;
    LUMINOUS_API size_t size() const;

  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_SHADERCONSTANTBLOCK_HPP
