#ifndef VALUABLE_ATTRIBUTESTYLEVALUE_HPP
#define VALUABLE_ATTRIBUTESTYLEVALUE_HPP

#include "AttributeObject.hpp"
#include "StyleValue.hpp"

namespace Valuable
{
  /// This class provides a StyleValue attribute.
  class AttributeStyleValue : public AttributeT<StyleValue>
  {
  public:
    using AttributeT<StyleValue>::operator=;

    AttributeStyleValue() {}
    AttributeStyleValue(Node * host, const QByteArray & name,
                        const StyleValue & v = StyleValue(), bool transit = false)
      : AttributeT<StyleValue>(host, name, v, transit)
    {}

    virtual bool set(float v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE
    {
      setValue(StyleValue(v, unit), layer);
      return true;
    }

    virtual bool set(int v, Layer layer = USER, ValueUnit = VU_UNKNOWN) OVERRIDE
    {
      setValue(StyleValue(v), layer);
      return true;
    }

    virtual bool set(const QString & v, Layer layer = USER, ValueUnit = VU_UNKNOWN) OVERRIDE
    {
      setValue(StyleValue(v), layer);
      return true;
    }

    virtual bool set(const Nimble::Vector2f &, Layer, QList<ValueUnit>) OVERRIDE
    {
      return false;
    }

    virtual bool set(const Nimble::Vector3f &, Layer, QList<ValueUnit>) OVERRIDE
    {
      return false;
    }

    virtual bool set(const Nimble::Vector4f &, Layer, QList<ValueUnit>) OVERRIDE
    {
      return false;
    }

    virtual bool set(const StyleValue & value, Layer layer = USER) OVERRIDE
    {
      setValue(value, layer);
      return true;
    }

    virtual QString asString(bool * const ok) const OVERRIDE
    {
      if (ok) *ok = true;
      return value().stringify();
    }

    virtual const char * type() const OVERRIDE { return "style-value"; }
  };
} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESTYLEVALUE_HPP
