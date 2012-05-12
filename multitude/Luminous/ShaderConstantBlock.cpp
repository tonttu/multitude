#include "Luminous/ShaderConstantBlock.hpp"
#include "Luminous/GLUtils.hpp"
#include "Luminous/Luminous.hpp"

#include <vector>
#include <algorithm>

namespace Luminous
{
  class ShaderConstantBlock::Impl
  {
  public:
    // List of constants
    typedef std::vector<ShaderConstant> ConstantList;
    ConstantList constants;

    std::vector<char> data;
  };

  ShaderConstantBlock::ShaderConstantBlock(RenderResource::Id id)
    : RenderResource(id, RT_VertexArray)
    , m_impl(new ShaderConstantBlock::Impl())
  {
  }

  ShaderConstantBlock::~ShaderConstantBlock()
  {
    delete m_impl;
  }

  bool ShaderConstantBlock::addConstant(const ShaderConstant & constant)
  {
    // Check for duplicates
    Impl::ConstantList::iterator it = std::find(m_impl->constants.begin(), m_impl->constants.end(), constant.name);
    if ( it != m_impl->constants.end())
        return false;

    // Store constant
    m_impl->constants.push_back(constant);

    // Make space and add raw data
    size_t location = m_impl->data.size();
    size_t dataSize = Utils2::getDataSize(constant.type) * constant.count;
    m_impl->data.resize(location + dataSize);
    memcpy(&(m_impl->data[location]), &constant.value.f, dataSize);
    
    invalidate();
    return true;
  }

#define ADDCONSTANTIMPL(TYPE, DATATYPE, COUNT, VALUE) \
  bool ShaderConstantBlock::addConstant(const QString & name, const TYPE & value) { \
    ShaderConstant constant; \
    constant.name = name; \
    constant.type = DATATYPE; \
    constant.count = COUNT; \
    memcpy(&constant.VALUE, &value, sizeof(value)); \
    return addConstant(constant); \
  }
  ADDCONSTANTIMPL(short, DT_Short, 1, value.s);
  ADDCONSTANTIMPL(int, DT_Int, 1, value.i);
  ADDCONSTANTIMPL(float, DT_Float, 1, value.f);
  ADDCONSTANTIMPL(double, DT_Double, 1, value.d);

  ADDCONSTANTIMPL(Nimble::Vector2f, DT_Float, 2, value.f);
  ADDCONSTANTIMPL(Nimble::Vector3f, DT_Float, 3, value.f);
  ADDCONSTANTIMPL(Nimble::Vector4f, DT_Float, 4, value.f);
  ADDCONSTANTIMPL(Nimble::Vector2d, DT_Double, 2, value.d);
  ADDCONSTANTIMPL(Nimble::Vector3d, DT_Double, 3, value.d);
  ADDCONSTANTIMPL(Nimble::Vector4d, DT_Double, 4, value.d);

  ADDCONSTANTIMPL(Nimble::Matrix2f, DT_Float, 4, value.f);
  ADDCONSTANTIMPL(Nimble::Matrix3f, DT_Float, 9, value.f);
  ADDCONSTANTIMPL(Nimble::Matrix4f, DT_Float, 16, value.f);
  ADDCONSTANTIMPL(Nimble::Matrix2d, DT_Double, 4, value.d);
  ADDCONSTANTIMPL(Nimble::Matrix3d, DT_Double, 9, value.d);
  ADDCONSTANTIMPL(Nimble::Matrix4d, DT_Double, 16, value.d);
#undef ADDCONSTANTIMPL

  void ShaderConstantBlock::clear()
  {
    m_impl->constants.clear();
    m_impl->data.clear();
    // Trigger buffer reallocation
    invalidate();
  }

  const char * ShaderConstantBlock::data() const
  {
    return m_impl->data.data();
  }

  size_t ShaderConstantBlock::size() const
  {
    return m_impl->data.size();
  }
}