#include "ValueBool.hpp"
#include "DOMElement.hpp"
#include "Radiant/StringUtils.hpp"

namespace Valuable
{
  ValueBool::ValueBool(HasValues * parent, const std::string & name,
                       bool value, bool transit)
    : ValueObjectT<bool>(parent, name, value, transit)
  {}

  ValueBool::~ValueBool() {}

  bool ValueBool::deserialize(ArchiveElement & e)
  {
    m_value = (bool)Radiant::StringUtils::fromString<int32_t>(e.get().c_str());
    return true;
  }

  void ValueBool::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    int32_t v = data.readInt32(&ok);
    if(ok) *this = (bool)v;
  }

  std::string ValueBool::asString(bool * const ok) const
  {
    if(ok) *ok = true;
    return Radiant::StringUtils::stringify((int32_t)m_value);
  }
}
