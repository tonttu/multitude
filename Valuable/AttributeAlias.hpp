/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_ATTRIBUTE_ALIAS_HPP
#define VALUABLE_ATTRIBUTE_ALIAS_HPP

#include "Attribute.hpp"

namespace Valuable
{
  /// Alias to another Attribute object
  class VALUABLE_API AttributeAlias : public Attribute
  {
  public:
    AttributeAlias(Node * host, const QByteArray & name, Attribute * attribute);
    virtual ~AttributeAlias();

    /// Calls the target eventProcess-function
    virtual void eventProcess(const QByteArray & id, Radiant::BinaryData & data);

    /// Converts the target object to a floating point number
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a float
    virtual float asFloat(bool * const ok = 0, Layer layer = CURRENT_VALUE) const;
    /// Converts the target object to an integer
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a int
    virtual int asInt(bool * const ok = 0, Layer layer = CURRENT_VALUE) const;
    /// Converts the target value to a string
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a string
    virtual QString asString(bool * const ok = 0, Layer layer = CURRENT_VALUE) const;

    /// Sets the value of the target object
    virtual bool set(float v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN);
    /// Sets the value of the target object
    virtual bool set(int v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN);
    /// Sets the value of the target object
    virtual bool set(const QString & v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN);
    /// Sets the value of the target object
    virtual bool set(const Nimble::Vector2f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>());
    /// Sets the value of the target object
    virtual bool set(const Nimble::Vector3f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>());
    /// Sets the value of the target object
    virtual bool set(const Nimble::Vector4f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>());
    /// Sets the value of the target object
    virtual bool set(const StyleValue & v, Layer layer = USER);

    virtual QByteArray type() const;

    virtual ArchiveElement serialize(Archive & archive) const;
    virtual bool deserialize(const ArchiveElement & element);

    /// Get the target Attribute where this class points to
    Attribute * attribute() const;

    /// Set the target Attribute where this class points to
    void setAttribute(Attribute * attribute);

    /// Returns true if the current value of the target object is different from the original value.
    virtual bool isChanged() const;

    virtual void clearValue(Layer layout);

    virtual bool handleShorthand(const StyleValue & value, Radiant::ArrayMap<Attribute *, StyleValue> & expanded);

    virtual bool isValueDefinedOnLayer(Layer layer) const;

    /// Gets an Attribute with the given name
    /// @param name Attribute name to search for
    /// @return Null if no object can be found
    virtual Attribute * attribute(const QByteArray & name) const;

    virtual void setAsDefaults();

  private:
    Attribute * m_attribute;
    long m_eventDelete, m_eventChange;
  };
}

#endif // VALUABLE_ATTRIBUTE_ALIAS_HPP
