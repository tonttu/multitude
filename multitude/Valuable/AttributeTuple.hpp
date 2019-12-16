/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_ATTRIBUTE_TUPLE_HPP
#define VALUABLE_ATTRIBUTE_TUPLE_HPP

#include "Attribute.hpp"
#include "AttributeFloat.hpp"
#include "AttributeInt.hpp"
#include "Node.hpp"

#include <QStringList>

#include <array>

namespace Valuable
{

  /// The list of supported types is below
  template <typename T>
  struct Elements { enum { value = -1 }; };

  template<typename T>
  struct Elements<Nimble::Vector2T<T> > { enum { value = 2 }; };
  template<typename T>
  struct Elements<Nimble::Vector3T<T> > { enum { value = 3 }; };
  template<typename T>
  struct Elements<Nimble::Vector4T<T> > { enum { value = 4 }; };

  template<typename T>
  struct Elements<Nimble::SizeT<T> > { enum { value = 2 }; };

  template<>
  struct Elements<Nimble::Frame4f> { enum { value = 4 }; };


  /// This class provides an attribute that stores a tuple of one dimensional
  /// attributes. This will also enable access to the individual components. For
  /// example AttributeSize, AttributeLocation etc can be easily implemented using
  /// this as a base implementation.
  ///
  /// For example AttributeSizeF inherits from AttributeTuple<Nimble::SizeF, AttributeT<Nimble::SizeF>>
  template <typename WrappedValue, typename AttributeType>
  class AttributeTuple : public Attribute
  {
  public:
    typedef typename std::remove_cv<typename std::remove_reference<
                     decltype(WrappedValue()[0])>::type>::type ElementType;
    const static int N = Elements<WrappedValue>::value;

    AttributeTuple(Node* host, const QByteArray& name,
                   const WrappedValue& v = WrappedValue::null());

    AttributeTuple & operator=(const AttributeTuple& tuple);
    AttributeTuple & operator=(const WrappedValue& tuple);

    ~AttributeTuple();

    const WrappedValue operator*() const { return value(); }
    operator const WrappedValue () const { return value(); }
    const WrappedValue value() const;
    const WrappedValue value(Layer layer) const;

    const WrappedValue defaultValue() const;
    Layer currentLayer() const;

    virtual bool isValueDefinedOnLayer(Layer layer) const OVERRIDE;
    virtual QString asString(bool * const ok=nullptr, Layer layer=CURRENT_VALUE) const OVERRIDE;

    virtual bool deserialize(const ArchiveElement &element) OVERRIDE;

    virtual bool set(float v, Layer layer=USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;
    virtual bool set(int v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE;

    virtual bool set(const Nimble::Vector2f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE;
    virtual bool set(const Nimble::Vector3f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE;
    virtual bool set(const Nimble::Vector4f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE;
    virtual bool set(const StyleValue& value, Layer layer = USER) override;

    virtual QByteArray type() const override;

    inline void setValue(const WrappedValue & t, Layer layer = USER);

    virtual bool isChanged() const OVERRIDE;
    virtual void clearValue(Layer layer) OVERRIDE;
    virtual void setAsDefaults() OVERRIDE;

    virtual void setTransitionParameters(TransitionParameters params) OVERRIDE;

    template<typename T> void setSrc(T src);
    template<typename T> void setSrc(const Nimble::Vector2T<T>& v);
    template<typename T> void setSrc(const Nimble::Vector3T<T>& v);
    template<typename T> void setSrc(const Nimble::Vector4T<T>& v);

    virtual bool handleShorthand(const StyleValue &value,
                                 Radiant::ArrayMap<Attribute *, StyleValue> &expanded) OVERRIDE;



    QString elementName(int tupleIndex, QString baseName) const;

    /// This function changes index used in in tuple to the index in the given range:
    /// [0, range-1] in WrappedValue or similar (most notably Nimble::Vectors).
    /// This can be overriden for shuffling the indices. For example css margin values
    /// can be specified by 3 values, where the order of values is not straightforward.
    int t2r(int tupleIndex, int range=N) const;

    /// Getter of value from wrapped value
    ElementType unwrap(const WrappedValue& v, int index) const;
    /// Setter of value from wrapped value
    void setWrapped(WrappedValue& v, int index, ElementType elem) const;

  protected:
    /// CRTP implementation. override in subclass
    static QString priv_elementName(int tupleIndex, QString baseName);
    virtual int priv_t2r(int tupleIndex, int range) const;
    virtual ElementType priv_unwrap(const WrappedValue& v, int index) const;
    /// This needs to be always overriden
    virtual void priv_setWrapped(WrappedValue& v, int index, ElementType elem) const;

    void valuesChanged();
    void beginChangeTransaction();
    void endChangeTransaction();

    std::array<AttributeT<ElementType>*, N> m_values;
    std::array<bool, N> m_animated;

    bool m_inChangeTransaction;
    bool m_emitChangedAfterTransaction;
  };


  template <typename T, typename A>
  QString AttributeTuple<T, A>::elementName(int tupleIndex, QString baseName) const
  {
    return A::priv_elementName(tupleIndex, baseName);
  }

  template <typename T, typename A>
  QString AttributeTuple<T, A>::priv_elementName(int tupleIndex, QString baseName)
  {
    static const char *suffixes[] = {"-x", "-y", "-z", "-w"};
    return baseName.append(suffixes[tupleIndex]);
  }

  template <typename T, typename A>
  int AttributeTuple<T,A>::t2r(int tupleIndex, int range) const
  {
    return static_cast<const A*>(this)->priv_t2r(tupleIndex, range);
  }

  template <typename T, typename A>
  int AttributeTuple<T,A>::priv_t2r(int tupleIndex, int range) const
  {
    return tupleIndex % range;
  }

  template <typename T, typename A>
  typename AttributeTuple<T,A>::ElementType
  AttributeTuple<T,A>::unwrap(const T &v, int index) const
  {
    return static_cast<const A*>(this)->priv_unwrap(v, index);
  }

  template <typename T, typename A>
  typename AttributeTuple<T,A>::ElementType
  AttributeTuple<T,A>::priv_unwrap(const T &v, int index) const
  {
    return v[index];
  }

  template <typename T, typename A>
  void AttributeTuple<T,A>::setWrapped(T &v, int index, typename AttributeTuple<T,A>::ElementType elem) const
  {
    static_cast<const A*>(this)->priv_setWrapped(v, index, elem);
  }

  template <typename T, typename A>
  void AttributeTuple<T,A>::priv_setWrapped(T &v, int index, typename AttributeTuple<T,A>::ElementType elem) const
  {
    (void) v; (void) index; (void) elem;
    /// This needs to be overloaded in every class
    assert(0);
  }

  template <typename T, typename A>
  AttributeTuple<T,A>::AttributeTuple(Node* host, const QByteArray& name,
                                    const T& v)
    : Attribute(host, name),
      m_inChangeTransaction(false),
      m_emitChangedAfterTransaction(false)
  {
    for(int i = 0; i < N; ++i) {
      ElementType e = unwrap(v, t2r(i));
      m_values[i] = new AttributeT<ElementType>(host, elementName(i, name).toUtf8(), e);
    }

    for(int i = 0; i < N; ++i) {
      m_values[i]->addListener([this] { valuesChanged(); });
      m_values[i]->setOwnerShorthand(this);
    }
    setSerializable(false);
  }

  template <typename T, typename A>
  AttributeTuple<T,A>& AttributeTuple<T,A>::operator=(const AttributeTuple<T,A>& tuple)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i) {
      *m_values[i] = **tuple.m_values[i];
    }

    endChangeTransaction();
    return *this;
  }

  template <typename T, typename A>
  AttributeTuple<T,A>& AttributeTuple<T,A>::operator=(const T& tuple)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i) {
      *m_values[i] = unwrap(tuple, t2r(i));
    }

    endChangeTransaction();
    return *this;
  }

  template <typename T, typename A>
  AttributeTuple<T,A>::~AttributeTuple()
  {
    for(int i = 0; i < N; ++i)
      delete m_values[i];
  }

  template <typename T, typename A>
  const T AttributeTuple<T,A>::defaultValue() const
  {
    return value(Attribute::DEFAULT);
  }

  template <typename T, typename A>
  Attribute::Layer AttributeTuple<T,A>::currentLayer() const
  {
    for(int l = int(Attribute::LAYER_COUNT) - 1; l  > Attribute::DEFAULT; --l) {
      Attribute::Layer layer = Attribute::Layer(l);
      if(isValueDefinedOnLayer(layer))
        return layer;
    }
    return Attribute::DEFAULT;
  }

  template <typename T, typename A>
  const T AttributeTuple<T,A>::value() const {
    T tmp;
    for(int i = 0; i < N; ++i) {
      setWrapped(tmp, i, m_values[i]->value());
    }
    return tmp;
  }

  template <typename T, typename A>
  const T AttributeTuple<T,A>::value(Attribute::Layer layer) const {
    T tmp;
    for(int i = 0; i < N; ++i)
      setWrapped(tmp, i, m_values[i]->value(layer));
    return tmp;
  }

  template <typename T, typename A>
  bool AttributeTuple<T,A>::isValueDefinedOnLayer(Attribute::Layer layer) const
  {
    bool isDefined = false;
    for(int i = 0; i < N && !isDefined; ++i)
      isDefined = isDefined || m_values[i]->isValueDefinedOnLayer(layer);
    return isDefined;
  }

  template <typename T, typename A>
  QString AttributeTuple<T,A>::asString(bool * const ok, Layer layer) const
  {
    if(ok) *ok = true;
    QString str;
    for(int i = 0; i < N; ++i) {
      str.append(QString("%1%2").arg(Radiant::StringUtils::toString(m_values[i]->value(layer)),
                                     (i+1 < N)? " ": ""));
    }
    return str;
  }

  template <typename T, typename A>
  bool AttributeTuple<T,A>::deserialize(const Valuable::ArchiveElement& elem)
  {
    QStringList lst = elem.get().split(QRegExp("\\s"), QString::SkipEmptyParts);

    if(lst.size() == N) {
      T tmp;
      for(int i = 0; i < N; ++i) {
        bool ok = false;
        setWrapped(tmp, i, lst[i].toFloat(&ok));
        if(!ok)
          return false;
      }

      *this = tmp;
      return true;
    }
    return false;
  }

  template <typename T, typename A>
  bool AttributeTuple<T,A>::set(float v, Layer layer, ValueUnit unit)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i)
      m_values[i]->set(v, layer, unit);

    endChangeTransaction();
    return true;
  }

  template <typename T, typename A>
  bool AttributeTuple<T,A>::set(int v, Layer layer, ValueUnit unit)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i)
      m_values[i]->set(v, layer, unit);

    endChangeTransaction();
    return true;
  }


  template <typename T, typename A>
  bool AttributeTuple<T,A>::set(const Nimble::Vector2f& v, Layer layer, QList<ValueUnit> units)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i) {
      int index = t2r(i, 2);
      ElementType elem = v[index];
      ValueUnit unit = units.isEmpty() ? VU_UNKNOWN : units[t2r(i, units.size())];
      m_values[i]->set(elem, layer, unit);
    }

    endChangeTransaction();
    return true;
  }

  template <typename T, typename A>
  bool AttributeTuple<T,A>::set(const Nimble::Vector3f& v, Layer layer, QList<ValueUnit> units)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i) {
      int index = t2r(i, 3);
      ElementType elem = v[index];
      ValueUnit unit = units.isEmpty() ? VU_UNKNOWN : units[t2r(i, units.size())];
      m_values[i]->set(elem, layer, unit);
    }

    endChangeTransaction();
    return true;
  }

  template <typename T, typename A>
  bool AttributeTuple<T,A>::set(const Nimble::Vector4f& v, Layer layer, QList<ValueUnit> units)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i) {
      int index = t2r(i, 4);
      ElementType elem = v[index];
      ValueUnit unit = units.isEmpty() ? VU_UNKNOWN : units[t2r(i, units.size())];
      m_values[i]->set(elem, layer, unit);
    }

    endChangeTransaction();
    return true;
  }

  template <typename T, typename A>
  bool AttributeTuple<T,A>::set(const StyleValue& sv, Layer layer)
  {
    if (sv.isEmpty())
      return false;

    beginChangeTransaction();

    bool ok = true;
    for(int i = 0; i < N; ++i) {
      int index = t2r(i, sv.size());
      StyleValue s(sv[index]);
      if (!m_values[i]->set(s, layer))
        ok = false;
    }

    endChangeTransaction();
    return ok;
  }

  template <typename T, typename A>
  QByteArray AttributeTuple<T, A>::type() const
  {
    return Radiant::StringUtils::type<T>();
  }

  template <typename T, typename A>
  inline void AttributeTuple<T,A>::setValue(const T & tuple, Layer layer)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i) {
      m_values[i]->setValue(unwrap(tuple, t2r(i)), layer);
    }

    endChangeTransaction();
  }

  template <typename T, typename A>
  bool AttributeTuple<T,A>::isChanged() const
  {
    for(int i = 0; i < N; ++i)
      if(m_values[i]->isChanged())
        return true;
    return false;
  }

  template <typename T, typename A>
  void AttributeTuple<T,A>::clearValue(Layer layer)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i)
      m_values[i]->clearValue(layer);

    endChangeTransaction();
  }

  template <typename T, typename A>
  void AttributeTuple<T,A>::setAsDefaults()
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i)
      m_values[i]->setAsDefaults();

    endChangeTransaction();
  }

  template <typename T, typename A>
  template <typename U>
  void AttributeTuple<T,A>::setSrc(U src)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i)
      m_values[i]->setSrc(src);

    endChangeTransaction();
  }

  template <typename T, typename A>
  template <typename U>
  void AttributeTuple<T,A>::setSrc(const Nimble::Vector2T<U>& src)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i) {
      U value = src[t2r(i, 2)];
      m_values[i]->setSrc(value);
    }

    endChangeTransaction();
  }

  template <typename T, typename A>
  template <typename U>
  void AttributeTuple<T,A>::setSrc(const Nimble::Vector3T<U>& src)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i) {
      U value = src[t2r(i, 3)];
      m_values[i]->setSrc(value);
    }

    endChangeTransaction();
  }

  template <typename T, typename A>
  template <typename U>
  void AttributeTuple<T,A>::setSrc(const Nimble::Vector4T<U>& src)
  {
    beginChangeTransaction();

    for(int i = 0; i < N; ++i) {
      U value = src[t2r(i, 4)];
      m_values[i]->setSrc(value);
    }

    endChangeTransaction();
  }

  template <typename T, typename A>
  bool AttributeTuple<T,A>::handleShorthand(const StyleValue& value,
                                          Radiant::ArrayMap<Attribute*, StyleValue> &expanded)
  {
    if(value.isEmpty() || value.size() > N)
      return false;
    for(int i = 0; i < N; ++i) {
      expanded[m_values[i]] = value[t2r(i, value.size())];
    }
    return true;
  }

  template <typename T, typename A>
  void AttributeTuple<T,A>::setTransitionParameters(TransitionParameters params)
  {
    for(int i = 0; i < N; ++i) {
      m_values[i]->setTransitionParameters(params);
    }
  }

  template <typename T, typename A>
  void AttributeTuple<T,A>::valuesChanged()
  {
    if(m_inChangeTransaction)
      m_emitChangedAfterTransaction = true;
    else
      Attribute::emitChange();
  }

  template <typename T, typename A>
  void AttributeTuple<T,A>::beginChangeTransaction()
  {
    assert(m_inChangeTransaction == false);
    m_inChangeTransaction = true;
  }

  template <typename T, typename A>
  void AttributeTuple<T,A>::endChangeTransaction()
  {
    assert(m_inChangeTransaction);

    m_inChangeTransaction = false;

    if(m_emitChangedAfterTransaction) {
      m_emitChangedAfterTransaction = false;
      Attribute::emitChange();
    }
  }

}

#endif // VALUABLE_ATTRIBUTE_TUPLE_HPP
