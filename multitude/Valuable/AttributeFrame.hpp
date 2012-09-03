/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef VALUABLE_ATTRIBUTE_FRAME_HPP
#define VALUABLE_ATTRIBUTE_FRAME_HPP

#include <Nimble/Frame4.hpp>

#include "AttributeFloat.hpp"
#include "AttributeVector.hpp"

#include <QStringList>

#include <array>

namespace Valuable
{
  class AttributeFrame : public Attribute
  {
  public:
    AttributeFrame(Node * host, const QString & name,
                   const Nimble::Frame4f & v = Nimble::Frame4f(), bool transit = false)
      : Attribute(host, name, transit)
    {
      m_values[0] = new AttributeFloat(host, name + "-top", v.top(), transit);
      m_values[1] = new AttributeFloat(host, name + "-right", v.right(), transit);
      m_values[2] = new AttributeFloat(host, name + "-bottom", v.bottom(), transit);
      m_values[3] = new AttributeFloat(host, name + "-left", v.left(), transit);

      for (int i = 0; i < 4; ++i)
        m_values[i]->addListener(std::bind(&AttributeFrame::emitChange, this));
    }

    virtual QString asString(bool * const ok = 0) const OVERRIDE
    {
      if (ok) *ok = true;
      return QString("%1 %2 %3 %4").arg(*m_values[0]).arg(*m_values[1]).arg(*m_values[2]).arg(*m_values[3]);
    }

    virtual bool deserialize(const ArchiveElement & element) OVERRIDE
    {
      QStringList lst = element.get().split(QRegExp("\\s"), QString::SkipEmptyParts);
      if (lst.size() == 4) {
        Nimble::Frame4f frame;
        for (int i = 0; i < 4; ++i) {
          bool ok = false;
          frame[i] = lst[i].toFloat(&ok);
          if (!ok) return false;
        }
        *this = frame;
        return true;
      }
      return false;
    }

    virtual bool set(float v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE
    {
      for (int i = 0; i < 4; ++i)
        m_values[i]->set(v, layer, unit);
      return true;
    }

    virtual bool set(int v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN) OVERRIDE
    {
      for (int i = 0; i < 4; ++i)
        m_values[i]->set(v, layer, unit);
      return true;
    }

    virtual bool set(const Nimble::Vector2f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      for (int i = 0; i < 4; ++i)
        m_values[i]->set(v[i % 2], layer, units[i % 2]);
      return true;
    }

    virtual bool set(const Nimble::Vector3f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      for (int i = 0; i < 4; ++i)
        m_values[i]->set(v[i % 3], layer, units[i % 3]);
      return true;
    }

    virtual bool set(const Nimble::Vector4f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>()) OVERRIDE
    {
      for (int i = 0; i < 4; ++i)
        m_values[i]->set(v[i], layer, units[i]);
      return true;
    }

    virtual const char * type() const OVERRIDE
    {
      return "AttributeFrame";
    }

    virtual bool isChanged() const OVERRIDE
    {
      for (int i = 0; i < 4; ++i)
        if (m_values[i]->isChanged())
          return true;
      return false;
    }

    virtual void clearValue(Layer layout) OVERRIDE
    {
      for (int i = 0; i < 4; ++i)
        m_values[i]->clearValue(layout);
    }

    virtual void setAsDefaults() OVERRIDE
    {
      for (int i = 0; i < 4; ++i)
        m_values[i]->setAsDefaults();
    }

    void setSrc(float src)
    {
      for (int i = 0; i < 4; ++i)
        m_values[i]->setSrc(src);
    }

    void setSrc(const Nimble::Vector4f & src)
    {
      for (int i = 0; i < 4; ++i)
        m_values[i]->setSrc(src[i]);
    }

    AttributeFrame & operator=(const AttributeFrame & frame)
    {
      for (int i = 0; i < 4; ++i)
        *m_values[i] = **frame.m_values[i];
      return *this;
    }

    AttributeFrame & operator=(const Nimble::Frame4f & frame)
    {
      for (int i = 0; i < 4; ++i)
        *m_values[i] = frame[i];
      return *this;
    }

    Nimble::Frame4f operator*() const
    {
      return value();
    }

    Nimble::Frame4f value() const
    {
      return Nimble::Frame4f(*m_values[0], *m_values[1], *m_values[2], *m_values[3]);
    }

  private:
    std::array<AttributeFloat *, 4> m_values;
  };
}

#endif // VALUABLE_ATTRIBUTE_FRAME_HPP
