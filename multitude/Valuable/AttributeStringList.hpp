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
  template <>
  class VALUABLE_API AttributeT<QStringList> : public AttributeBaseT<QStringList>
  {
    typedef AttributeBaseT<QStringList> Base;

  public:
    using Base::operator =;

    AttributeT();
    AttributeT(Node * host, const QByteArray & name,
               const QStringList & v = QStringList());

    /// Returns the value as string
    virtual QString asString(bool * const ok = nullptr, Layer layer = USER) const OVERRIDE;

    virtual bool set(const QString & v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(const StyleValue & v, Layer layer = USER) OVERRIDE;

    static inline QStringList interpolate(QStringList a, QStringList b, float m)
    {
      return m >= 0.5f ? b : a;
    }
  };
  typedef AttributeT<QStringList> AttributeStringList;
} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESTRINGLIST_HPP
