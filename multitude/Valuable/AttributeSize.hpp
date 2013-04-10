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

#include "AttributeFloat.hpp"
#include "AttributeInt.hpp"

#include <Nimble/Size.hpp>

#include <QStringList>

#include <array>

namespace Valuable {

  /// This class defines an attribute that stores a Nimble::Size(F) object.
  template<typename ElementType, class SizeClass, class ElementAttribute>
  class AttributeSizeT : public Valuable::Attribute
  {
  public:
    /// Constructor
    /// @param host host node
    /// @param name name of the size attribute
    /// @param widthName name for attribute alias to width of the size
    /// @param heightName name for attribute alias to height of the size
    /// @param size initial value
    /// @param transit (ignored)
    AttributeSizeT(Node * host, const QByteArray & name, const QByteArray & widthName, const QByteArray & heightName, const SizeClass & size = SizeClass(), bool transit = false)
      : Attribute(host, name, transit)
      , m_inChangeTransaction(false)
      , m_emitChangedAfterTransaction(false)
    {
      m_values[0] = new ElementAttribute(host, widthName, size.width(), transit);
      m_values[1] = new ElementAttribute(host, heightName, size.height(), transit);

#ifndef CLANG_XML
      for(int i = 0; i < 2; ++i) {
        m_values[i]->addListener(std::bind(&AttributeSizeT::valuesChanged, this));
        m_values[i]->setSerializable(false);
      }
#endif
    }

    ~AttributeSizeT()
    {
      for(int i = 0; i < 2; ++i)
        delete m_values[i];
    }

    bool setWidth(ElementType w, Layer layer = USER, ValueUnit unit = VU_PXS)
    {
      beginChangeTransaction();
      m_values[0]->set(w, layer, unit);
      endChangeTransaction();

      return true;
    }

    bool setHeight(ElementType h, Layer layer = USER, ValueUnit unit = VU_PXS)
    {
      beginChangeTransaction();
      m_values[1]->set(h, layer, unit);
      endChangeTransaction();

      return true;
    }

    virtual QString asString(bool * const ok = 0) const OVERRIDE
    {
      if(ok) *ok = true;
      return QString("%1 %2").arg(*m_values[0]).arg(*m_values[1]);
    }

    virtual bool deserialize(const ArchiveElement &element) OVERRIDE
    {
      QStringList lst = element.get().split(QRegExp("\\s"), QString::SkipEmptyParts);

      if(lst.size() == 2) {

        Nimble::Vector2T<ElementType> size;

        for(int i = 0; i < 2; ++i) {
          bool ok = false;
          size[i] = lst[i].toFloat(&ok);
          if(!ok)
            return false;
        }

        *this = size;
        return true;
      }

      return false;
    }

    virtual void eventProcess(const QByteArray &, Radiant::BinaryData & data) OVERRIDE
    {
      bool ok = true;
      Nimble::Vector2 s = data.readVector2Float32(&ok);
      if (ok) {
        set(s);
      } else {
        Radiant::warning("AttributeSizeT::eventProcess # Failed to parse data");
      }
    }

    virtual bool set(ElementType v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE
    {
      beginChangeTransaction();

      for(int i = 0; i < 2; ++i)
        m_values[i]->set(v, layer, unit);

      endChangeTransaction();

      return true;
    }


    virtual bool set(const Nimble::Vector2f &v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      beginChangeTransaction();

      for(int i = 0; i < 2; ++i)
        m_values[i]->set(static_cast<ElementType>(v[i]), layer, i >= units.size() ? VU_UNKNOWN : units[i]);

      endChangeTransaction();

      return true;
    }

    virtual bool isChanged() const OVERRIDE
    {
      for(int i = 0; i < 2; ++i)
        if(m_values[i]->isChanged())
          return true;
      return false;
    }

    virtual void clearValue(Layer layout) OVERRIDE
    {
      beginChangeTransaction();

      for(int i = 0; i < 2; ++i)
        m_values[i]->clearValue(layout);

      endChangeTransaction();
    }

    virtual void setAsDefaults() OVERRIDE
    {
      beginChangeTransaction();

      for(int i = 0; i < 2; ++i)
        m_values[i]->setAsDefaults();

      endChangeTransaction();
    }

    void setSrc(Nimble::Vector2T<ElementType> src)
    {
      beginChangeTransaction();

      for(int i = 0; i < 2; ++i)
        m_values[i]->setSrc(src[i]);

      endChangeTransaction();
    }

    const SizeClass operator * () const { return value(); }
    operator const SizeClass () const { return value(); }

    AttributeSizeT & operator=(const AttributeSizeT & size)
    {
      beginChangeTransaction();

      for(int i = 0; i < 2; ++i)
        *m_values[i] = *size.m_values[i];

      endChangeTransaction();

      return *this;
    }

    AttributeSizeT & operator=(Nimble::Vector2T<ElementType> vec)
    {
      beginChangeTransaction();

      for(int i = 0; i < 2; ++i)
        *m_values[i] = vec[i];

      endChangeTransaction();

      return *this;
    }

    AttributeSizeT & operator=(const SizeClass & size)
    {
      beginChangeTransaction();

      *m_values[0] = size.width();
      *m_values[1] = size.height();

      endChangeTransaction();

      return *this;
    }

    SizeClass value() const
    {
      return SizeClass(*m_values[0], *m_values[1]);
    }

  private:
    void valuesChanged()
    {
      if(m_inChangeTransaction)
        m_emitChangedAfterTransaction = true;
      else
        Attribute::emitChange();
    }

    void beginChangeTransaction()
    {
      assert(m_inChangeTransaction == false);
      m_inChangeTransaction = true;
    }

    void endChangeTransaction()
    {
      assert(m_inChangeTransaction);

      m_inChangeTransaction = false;

      if(m_emitChangedAfterTransaction) {
        m_emitChangedAfterTransaction = false;
        Attribute::emitChange();
      }
    }

    std::array<ElementAttribute*, 2> m_values;

    bool m_inChangeTransaction;
    bool m_emitChangedAfterTransaction;
  };

  typedef AttributeSizeT<float, Nimble::SizeF, AttributeFloat> AttributeSizeF;
  typedef AttributeSizeT<int, Nimble::Size, AttributeInt> AttributeSize;

} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESIZE_HPP
