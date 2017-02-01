/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_ATTRIBUTESIZE_HPP
#define VALUABLE_ATTRIBUTESIZE_HPP

#include "Attribute.hpp"
#include "AttributeTuple.hpp"
#include "StyleValue.hpp"

#include <Nimble/Size.hpp>

namespace Valuable
{
  template <typename T>
  struct IsSizeT { static constexpr bool value = false; };

  template <typename E>
  struct IsSizeT<Nimble::SizeT<E>> { static constexpr bool value = true; };

  /// This class defines an attribute that stores a Nimble::SizeT object.
  template <typename T>
  class AttributeT<T, typename std::enable_if<IsSizeT<T>::value>::type>
      : public Valuable::AttributeTuple<T, AttributeT<T, typename std::enable_if<IsSizeT<T>::value>::type>>
  {
  public:
    typedef AttributeTuple<T, AttributeT<T, typename std::enable_if<IsSizeT<T>::value>::type>> Base;
    using Base::operator=;

    typedef AttributeT<T, typename std::enable_if<IsSizeT<T>::value>::type> AttributeType;
    typedef typename AttributeTuple<T, AttributeType>::ElementType ElementType;

    AttributeT()
      : AttributeTuple<T, AttributeType>(nullptr, QByteArray())
    {
      AttributeTuple<T ,AttributeType>::m_values[0]->setName("width");
      AttributeTuple<T, AttributeType>::m_values[1]->setName("height");
    }

    /// Constructor
    /// @param host host node
    /// @param name name of the size attribute
    /// @param widthName name for attribute alias to width of the size
    /// @param heightName name for attribute alias to height of the size
    /// @param size initial value
    /// @param transit (ignored)
    AttributeT(Node * host, const QByteArray & name, const QByteArray & widthName, const QByteArray & heightName, const T & size = T())
      : AttributeTuple<T, AttributeType>(host, name, size)
    {
      AttributeTuple<T ,AttributeType>::m_values[0]->setName(widthName);
      AttributeTuple<T, AttributeType>::m_values[1]->setName(heightName);
    }

    bool setWidth(ElementType w, Attribute::Layer layer = Attribute::USER,
                  Attribute::ValueUnit unit = Attribute::VU_PXS)
    {
      AttributeTuple<T,AttributeType>::beginChangeTransaction();
      AttributeTuple<T,AttributeType>::m_values[0]->set(w, layer, unit);
      AttributeTuple<T,AttributeType>::endChangeTransaction();

      return true;
    }

    bool setHeight(ElementType h, Attribute::Layer layer = Attribute::USER,
                   Attribute::ValueUnit unit = Attribute::VU_PXS)
    {
      AttributeTuple<T,AttributeType>::beginChangeTransaction();
      AttributeTuple<T,AttributeType>::m_values[1]->set(h, layer, unit);
      AttributeTuple<T,AttributeType>::endChangeTransaction();

      return true;
    }

    ElementType width() const
    {
      return *AttributeTuple<T,AttributeType>::m_values[0];
    }

    ElementType height() const
    {
      return *AttributeTuple<T,AttributeType>::m_values[1];
    }

    virtual void eventProcess(const QByteArray &, Radiant::BinaryData & data) OVERRIDE
    {
      bool ok = true;
      Nimble::Vector2f s = data.readVector2Float32(&ok);
      if (ok) {
        AttributeTuple<T,AttributeType>::set(s);
      } else {
        Radiant::warning("AttributeSizeT::eventProcess # Failed to parse data");
      }
    }

    using Base::setValue;

    bool setValue(const Nimble::SizeF & v, Attribute::Layer layer,
                  Attribute::ValueUnit widthUnit, Attribute::ValueUnit heightUnit)
    {
      AttributeTuple<T,AttributeType>::beginChangeTransaction();

      AttributeTuple<T,AttributeType>::m_values[0]->set(v.width(), layer, widthUnit);
      AttributeTuple<T,AttributeType>::m_values[1]->set(v.height(), layer, heightUnit);

      AttributeTuple<T,AttributeType>::endChangeTransaction();

      return true;
    }

    virtual void priv_setWrapped(T& v, int index, ElementType elem) const OVERRIDE
    {
      if(index == 0) v.setWidth(elem);
      else v.setHeight(elem);
    }


  };

  typedef AttributeT<Nimble::SizeF> AttributeSizeF;
  typedef AttributeT<Nimble::Size> AttributeSize;

} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESIZE_HPP
