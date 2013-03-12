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
#include <Valuable/AttributeObject.hpp>

#include <QString>

namespace Valuable
{

  /// String value
  class VALUABLE_API AttributeString : public AttributeT<QString>
  {
    typedef AttributeT<QString> Base;

  public:
    using Base::operator =;

    /// The character type of this string class
    typedef QChar char_type;

    AttributeString();
    /// @copydoc Attribute::Attribute(Node *, const QByteArray &, bool transit)
    /// @param v The string to store in this object
    AttributeString(Node * host, const QByteArray & name,
                const QString & v = "", bool transit = false);

    virtual void processMessage(const QByteArray & id, Radiant::BinaryData & data) OVERRIDE;

    /// Concatenates two strings
    /// @param i The string to be appended to this string
    /// @return A new string that contains both this string, and the argument string.
    QString operator + (const AttributeString & i) const;

    /// Concatenates two strings
    QString operator + (const QString & i) const;

    /// Concatenates strings with UTF-8 encoded string
    QString operator + (const char * utf8) const;

    /// Compares if two strings are equal
    bool operator == (const QString & that) const;
    /// Compares if two strings are not equal
    bool operator != (const QString & that) const;

    /// Returns the value as float
    virtual float asFloat(bool * const ok = 0) const OVERRIDE;
    /// Returns the value as integer
    virtual int asInt(bool * const ok = 0) const OVERRIDE;
    /// Returns the value as string
    virtual QString asString(bool * const ok = 0) const OVERRIDE;

    virtual bool set(const QString & v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;

    /// Makes the string empty
    void clear();

    /// Returns the length of the string
    unsigned size() const;
  };
}

VALUABLE_API QString operator + (const QString & a, const Valuable::AttributeString & b);

#endif
