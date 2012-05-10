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
    ShaderConstantBlock(RenderResource::Id id);
    ~ShaderConstantBlock();

    bool addConstant(const ShaderConstant & constant);

    bool addConstant(const QString & name, const short & value);
    bool addConstant(const QString & name, const int & value);
    bool addConstant(const QString & name, const float & value);
    bool addConstant(const QString & name, const double & value);
    bool addConstant(const QString & name, const Nimble::Vector2f & value);
    bool addConstant(const QString & name, const Nimble::Vector3f & value);
    bool addConstant(const QString & name, const Nimble::Vector4f & value);
    bool addConstant(const QString & name, const Nimble::Matrix2f & value);
    bool addConstant(const QString & name, const Nimble::Matrix3f & value);
    bool addConstant(const QString & name, const Nimble::Matrix4f & value);
    bool addConstant(const QString & name, const Nimble::Vector2d & value);
    bool addConstant(const QString & name, const Nimble::Vector3d & value);
    bool addConstant(const QString & name, const Nimble::Vector4d & value);
    bool addConstant(const QString & name, const Nimble::Matrix2d & value);
    bool addConstant(const QString & name, const Nimble::Matrix3d & value);
    bool addConstant(const QString & name, const Nimble::Matrix4d & value);
    void clear();

    const ShaderConstant & constant(size_t index) const;
    size_t constantCount() const;

    const char * data() const;
    size_t size() const;

  private:
    class Impl;
    Impl * m_impl;
  };
}
#endif // LUMINOUS_SHADERCONSTANTBLOCK_HPP