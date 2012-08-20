#include "AttributeStringList.hpp"

#include "StyleValue.hpp"

namespace Valuable
{
  AttributeStringList::AttributeStringList()
  {
  }

  AttributeStringList::AttributeStringList(Node * host, const QString & name,
                                           const QStringList & v, bool transit)
    : Base(host, name, v, transit)
  {
  }

  QString AttributeStringList::asString(bool * const ok) const
  {
    if (ok) *ok = true;
    return value().join(" ");
  }

  bool AttributeStringList::set(const QString & v, Attribute::Layer layer, Attribute::ValueUnit)
  {
    setValue(QStringList(v), layer);
    return true;
  }

  bool AttributeStringList::set(const StyleValue & v, Attribute::Layer layer)
  {
    foreach (auto u, v.units())
      if (u != VU_UNKNOWN)
        return false;

    QStringList lst;
    lst.reserve(v.size());
    foreach (const QVariant & var, v.values())
      lst << var.toString();
    setValue(lst, layer);
    return true;
  }

  bool AttributeStringList::deserialize(const ArchiveElement & element)
  {
    /// @todo doesn't handle whitespace properly
    setValue(element.get().split(" "));
    return true;
  }
} // namespace Valuable
