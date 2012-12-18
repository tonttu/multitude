#ifndef VALUABLE_ATTRIBUTE_ALIAS_HPP
#define VALUABLE_ATTRIBUTE_ALIAS_HPP

#include "AttributeObject.hpp"

namespace Valuable
{
  /// Alias to another Attribute object
  /// @todo this class shouldn't be serialized. Add isSerializable() in Attribute
  class VALUABLE_API AttributeAlias : public Attribute
  {
  public:
    AttributeAlias(Node * host, const QByteArray & name, Attribute * attribute);
    virtual ~AttributeAlias();

    /// Calls the target processMessage-function
    virtual void processMessage(const QByteArray & id, Radiant::BinaryData & data);

    /// Converts the target object to a floating point number
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a float
    virtual float asFloat(bool * const ok = 0) const;
    /// Converts the target object to an integer
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a int
    virtual int asInt(bool * const ok = 0) const;
    /// Converts the target value to a string
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a string
    virtual QString asString(bool * const ok = 0) const;

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

    virtual ArchiveElement serialize(Archive & archive) const;
    virtual bool deserialize(const ArchiveElement & element);

    /// Get the target Attribute where this class points to
    Attribute * attribute() const;

    /// Set the target Attribute where this class points to
    void setAttribute(Attribute * attribute);

    /// Returns true if the current value of the target object is different from the original value.
    virtual bool isChanged() const;

    virtual void clearValue(Layer layout);

    virtual bool handleShorthand(const StyleValue & value, QMap<Attribute *, StyleValue> & expanded);

    /// Gets an Attribute with the given name
    /// @param name Attribute name to search for
    /// @return Null if no object can be found
    virtual Attribute * getAttribute(const QByteArray &name);

  private:
    Attribute * m_attribute;
    long m_event;
  };
}

#endif // VALUABLE_ATTRIBUTE_ALIAS_HPP
