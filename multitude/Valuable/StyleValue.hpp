/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef VALUABLE_STYLE_VALUE_HPP
#define VALUABLE_STYLE_VALUE_HPP

#include "Export.hpp"
#include "AttributeObject.hpp"

#include <QString>
#include <Radiant/Color.hpp>
#include <QVariant>

namespace Valuable
{
  /// The base interface for different value types
  class VALUABLE_API StyleValue
  {
  public:
    StyleValue(float v, Attribute::ValueUnit unit = Attribute::VU_UNKNOWN);
    StyleValue(int v);
    StyleValue(QVariant v);
    virtual ~StyleValue();

    virtual int asInt(int idx = 0) const;
    virtual float asFloat(int idx = 0) const;
    virtual QString asString(int idx = 0) const;
    virtual Radiant::Color asColor(int idx = 0) const;

    virtual int type(int idx = 0) const;

    Attribute::ValueUnit unit(int idx = 0) const;

    void append(const StyleValue & v);

    /// Size of this vector
    int size() const;
    /// Are all the values in the vector ~same type (int and float values are ok)
    bool uniform() const;

    /// String representation that can be used in a CSS
    QString stringify() const;

    bool isNumber(int idx = 0) const;

    /// Makes a copy of one individual value
    /// @param idx 0 <= idx < size()
    StyleValue operator[](int idx) const;

    QVariantList values() const { return m_values; }
    QList<Attribute::ValueUnit> units() const { return m_units; }

  private:
    bool m_uniform;
    /// QVariants in the list can be ints, floats, strings and colors
    QVariantList m_values;
    QList<Attribute::ValueUnit> m_units;
  };

}

#endif
