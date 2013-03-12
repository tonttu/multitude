/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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
    /// A separator in a string
    enum Separator
    {
      WhiteSpace,
      Comma,
      Slash
    };

    /// Utility class used when parsing lists separated by tokens
    struct Group
    {
      QVariantList values;
      QList<Attribute::ValueUnit> units;
      QList<Separator> separators;
    };

  public:
    StyleValue();
    StyleValue(float v, Attribute::ValueUnit unit = Attribute::VU_UNKNOWN);
    StyleValue(int v);
    StyleValue(QVariant v, Attribute::ValueUnit unit = Attribute::VU_UNKNOWN);
    StyleValue(const QMap<QString, QString> & map);
    virtual ~StyleValue();

    virtual int asInt(int idx = 0) const;
    virtual float asFloat(int idx = 0) const;
    virtual QString asString(int idx = 0) const;
    virtual Radiant::Color asColor(int idx = 0) const;

    virtual int type(int idx = 0) const;

    Attribute::ValueUnit unit(int idx = 0) const;

    void append(const StyleValue & v, Separator sep);

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

    const QVariantList & values() const { return m_values; }
    const QList<Attribute::ValueUnit> & units() const { return m_units; }
    const QList<Separator> & separators() const { return m_separators; }

    inline bool operator!=(const StyleValue & v) const { return !operator==(v); }
    bool operator==(const StyleValue & v) const;

    QList<Group> groups(Separator sep) const;

    /// CSS value "aaa bbb, ccc ddd eee, fff" will be converted to map:
    /// "aaa" => "bbb", "ccc" => "ddd eee", "fff" => ""
    QMap<QString, QString> asMap() const;

  private:
    bool m_uniform;
    /// QVariants in the list can be ints, floats, strings and colors
    QVariantList m_values;
    QList<Attribute::ValueUnit> m_units;
    QList<Separator> m_separators;
  };

  VALUABLE_API std::ostream & operator<<(std::ostream & os, const StyleValue & value);
  /// This needs full CSS parser to work, so its implemented in Stylish
  STYLISH_API std::istream & operator>>(std::istream & is, StyleValue & value);
}

#endif
