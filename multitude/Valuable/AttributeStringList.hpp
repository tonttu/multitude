/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_ATTRIBUTESTRINGLIST_HPP
#define VALUABLE_ATTRIBUTESTRINGLIST_HPP

#include "Export.hpp"
#include "Attribute.hpp"

#include <QStringList>

namespace Valuable
{
  /// This class provides a QStringList attribute.
  class VALUABLE_API AttributeStringList : public AttributeT<QStringList>
  {
    typedef AttributeT<QStringList> Base;

  public:
    using Base::operator =;

    AttributeStringList();
    AttributeStringList(Node * host, const QByteArray & name,
                        const QStringList & v = QStringList(), bool transit = false);

    /// Returns the value as string
    virtual QString asString(bool * const ok, Layer layer) const OVERRIDE;

    virtual bool set(const QString & v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(const StyleValue & v, Layer layer = USER) OVERRIDE;
  };
} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESTRINGLIST_HPP
