/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef VALUABLE_ATTRIBUTESTRINGMAP_HPP
#define VALUABLE_ATTRIBUTESTRINGMAP_HPP

#include "Export.hpp"
#include "Attribute.hpp"

#include <QMap>

namespace Valuable
{
  /// This class provides a QMap<QString, QString> attribute.
  /// In CSS map entries are pairs of strings separated by colons. For example in
  /// CSS one would define the values in following way:
  /// @code
  /// attribute : "key" "value", "key with spaces" "value with spaces";
  /// @endcode
  class VALUABLE_API AttributeStringMap : public AttributeT<QMap<QString, QString> >
  {
    typedef QMap<QString, QString> StringMap;
    typedef AttributeT<StringMap> Base;

  public:
    using Base::operator =;

    AttributeStringMap();
    AttributeStringMap(Node * host, const QByteArray & name,
                       const StringMap & v = StringMap(), bool transit = false);

    /// Returns the value as string
    virtual QString asString(bool * const ok = 0) const OVERRIDE;

    virtual bool set(const QString & v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(const StyleValue & v, Layer layer = USER) OVERRIDE;
  };
} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESTRINGLIST_HPP