#ifndef VALUABLE_ATTRIBUTESIZE_HPP
#define VALUABLE_ATTRIBUTESIZE_HPP

#include "AttributeFloat.hpp"

#include <Nimble/Size.hpp>

#include <QStringList>

#include <array>

namespace Valuable {

  class AttributeSize : public Valuable::Attribute
  {
  public:
    AttributeSize(Node * host, const QByteArray & name, const Nimble::SizeF & size = Nimble::SizeF(), bool transit = false)
      : Attribute(host, name, transit)
      , m_inChangeTransaction(false)
      , m_emitChangedAfterTransaction(false)
    {
      m_values[0] = new AttributeFloat(host, "width", size.width(), transit);
      m_values[1] = new AttributeFloat(host, "height", size.height(), transit);

      for(int i = 0; i < 2; ++i) {
        m_values[i]->addListener(std::bind(&AttributeSize::valuesChanged, this));
        m_values[i]->setSerializable(false);
      }
    }

    ~AttributeSize()
    {
      for(int i = 0; i < 2; ++i)
        delete m_values[i];
    }

    virtual QString asString(bool * const ok = 0) const OVERRIDE
    {
      if(ok) *ok = true;
      return QString("%1 %2").arg(*m_values[0]).arg(*m_values[1]);
    }

    virtual bool deserialize(const ArchiveElement &element) OVERRIDE
    {
      beginChangeTransaction();

      QStringList lst = element.get().split(QRegExp("\\s"), QString::SkipEmptyParts);

      if(lst.size() == 2) {

        Nimble::Vector2f size;

        for(int i = 0; i < 2; ++i) {
          bool ok = false;
          size[i] = lst[i].toFloat(&ok);
          if(!ok) {
            endChangeTransaction();
            return false;
          }
        }

        *this = size;
        endChangeTransaction();

        return true;
      }

      endChangeTransaction();

      return false;
    }

    virtual bool set(const Nimble::Vector2f &v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      beginChangeTransaction();

      for(int i = 0; i < 2; ++i)
        m_values[i]->set(v[i], layer, units[i]);

      endChangeTransaction();

      return true;
    }

    virtual const char * type() const OVERRIDE
    {
      return "AttributeSize";
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

    void setSrc(Nimble::Vector2f src)
    {
      beginChangeTransaction();

      for(int i = 0; i << 2; ++i)
        m_values[i]->setSrc(src[i]);

      endChangeTransaction();
    }

    AttributeSize & operator=(const AttributeSize & size)
    {
      beginChangeTransaction();

      for(int i = 0; i < 2; ++i)
        *m_values[i] = *size.m_values[i];

      endChangeTransaction();

      return *this;
    }

    AttributeSize & operator=(Nimble::Vector2f vec)
    {
      beginChangeTransaction();

      for(int i = 0; i < 2; ++i)
        *m_values[i] = vec[i];

      endChangeTransaction();

      return *this;
    }

    AttributeSize & operator=(const Nimble::SizeF & size)
    {
      beginChangeTransaction();

      *m_values[0] = size.width();
      *m_values[1] = size.height();

      endChangeTransaction();

      return *this;
    }

    Nimble::SizeF operator*() const
    {
      return value();
    }

    Nimble::SizeF value() const
    {
      return Nimble::SizeF(*m_values[0], *m_values[1]);
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

    std::array<AttributeFloat*, 2> m_values;

    bool m_inChangeTransaction;
    bool m_emitChangedAfterTransaction;
  };

} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESIZE_HPP
