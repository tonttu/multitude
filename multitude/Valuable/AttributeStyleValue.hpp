#ifndef VALUABLE_ATTRIBUTESTYLEVALUE_HPP
#define VALUABLE_ATTRIBUTESTYLEVALUE_HPP

#include "AttributeObject.hpp"
#include "StyleValue.hpp"

namespace Valuable
{
  class AttributeStyleValue : public AttributeT<StyleValue>
  {
  public:
    using AttributeT<StyleValue>::operator=;

    AttributeStyleValue() {}
    AttributeStyleValue(Node * host, const QString & name,
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

    virtual bool set(const Nimble::Vector2f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      StyleValue sv(v[0], units[0]);
      for (int i = 1; i < 2; ++i)
        sv.append(StyleValue(v[i], units[i]), StyleValue::WhiteSpace);
      setValue(sv, layer);
      return true;
    }

    virtual bool set(const Nimble::Vector3f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      StyleValue sv(v[0], units[0]);
      for (int i = 1; i < 3; ++i)
        sv.append(StyleValue(v[i], units[i]), StyleValue::WhiteSpace);
      setValue(sv, layer);
      return true;
    }

    virtual bool set(const Nimble::Vector4f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      StyleValue sv(v[0], units[0]);
      for (int i = 1; i < 4; ++i)
        sv.append(StyleValue(v[i], units[i]), StyleValue::WhiteSpace);
      setValue(sv, layer);
      return true;
    }

    virtual bool set(const StyleValue & value, Layer layer = USER) OVERRIDE
    {
      setValue(value, layer);
      return true;
    }

    virtual bool deserialize(const ArchiveElement &) OVERRIDE
    {
      /// @todo implement?
      return false;
    }

    virtual const char * type() const OVERRIDE { return "style-value"; }
  };
} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESTYLEVALUE_HPP
