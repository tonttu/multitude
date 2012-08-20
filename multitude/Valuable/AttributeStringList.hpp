#ifndef VALUABLE_ATTRIBUTESTRINGLIST_HPP
#define VALUABLE_ATTRIBUTESTRINGLIST_HPP

#include "Export.hpp"
#include "AttributeObject.hpp"

#include <QStringList>

namespace Valuable
{
  class VALUABLE_API AttributeStringList : public AttributeT<QStringList>
  {
    typedef AttributeT<QStringList> Base;

  public:
    using Base::operator =;

    AttributeStringList();
    AttributeStringList(Node * host, const QString & name,
                        const QStringList & v = QStringList(), bool transit = false);

    /// Returns the value as string
    virtual QString asString(bool * const ok = 0) const OVERRIDE;

    virtual bool set(const QString & v, Layer layer = MANUAL, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(const StyleValue & v, Layer layer = MANUAL) OVERRIDE;

    virtual const char * type() const OVERRIDE { return "stringlist"; }

    virtual bool deserialize(const ArchiveElement & element) OVERRIDE;
  };
} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESTRINGLIST_HPP
