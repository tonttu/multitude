/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_STRING_HPP
#define VALUABLE_VALUE_STRING_HPP

#include <Valuable/Export.hpp>
#include <Valuable/Attribute.hpp>

#include <QString>

namespace Valuable
{

  /// String value
  template <>
  class VALUABLE_API AttributeT<QString> : public AttributeBaseT<QString>
  {
    typedef AttributeBaseT<QString> Base;

  public:
    using Base::operator =;

    /// The character type of this string class
    typedef QChar char_type;

    AttributeT();
    /// @copydoc Attribute::Attribute(Node *, const QByteArray &)
    /// @param v The string to store in this object
    AttributeT(Node * host, const QByteArray & name,
               const QString & v = "");

    virtual void eventProcess(const QByteArray & id, Radiant::BinaryData & data) OVERRIDE;

    /// Concatenates two strings
    /// @param i The string to be appended to this string
    /// @return A new string that contains both this string, and the argument string.
    QString operator + (const AttributeT & i) const;

    /// Concatenates two strings
    QString operator + (const QString & i) const;

    /// Concatenates strings with UTF-8 encoded string
    QString operator + (const char * utf8) const;

    /// Compares if two strings are equal
    bool operator == (const QString & that) const;
    /// Compares if two strings are not equal
    bool operator != (const QString & that) const;

    /// Returns the value as float
    virtual float asFloat(bool * const ok = nullptr, Layer layer = CURRENT_VALUE) const OVERRIDE;
    /// Returns the value as integer
    virtual int asInt(bool * const ok = nullptr, Layer layer = CURRENT_VALUE) const OVERRIDE;
    /// Returns the value as string
    virtual QString asString(bool * const ok = nullptr, Layer layer = CURRENT_VALUE) const OVERRIDE;

    virtual bool set(const QString & v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;

    virtual bool set(const StyleValue & value, Layer layer = USER) OVERRIDE;

    /// Makes the string empty
    void clear();

    /// Returns the length of the string
    unsigned size() const;

    static inline QString interpolate(QString a, QString b, float m)
    {
      return m >= 0.5f ? b : a;
    }
  };
  typedef AttributeT<QString> AttributeString;
}

VALUABLE_API QString operator + (const QString & a, const Valuable::AttributeString & b);

#endif
