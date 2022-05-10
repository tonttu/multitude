/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "AttributeAlias.hpp"

namespace Valuable
{
  AttributeAlias::AttributeAlias(Node * host, const QByteArray & name, Attribute * attribute)
    : Attribute(host, name)
    , m_attribute(nullptr)
    , m_eventDelete(0)
    , m_eventChange(0)
  {
    setAttribute(attribute);
    setSerializable(false);
  }

  AttributeAlias::~AttributeAlias()
  {
    setAttribute(nullptr);
  }

  void AttributeAlias::eventProcess(const QByteArray & id, Radiant::BinaryData & data)
  {
    if(m_attribute)
      m_attribute->eventProcess(id, data);
  }

  float AttributeAlias::asFloat(bool * const ok, Layer layer) const
  {
    if(!m_attribute) {
      if(ok) *ok = false;
      return .0f;
    }
    return m_attribute->asFloat(ok, layer);
  }

  int AttributeAlias::asInt(bool * const ok, Layer layer) const
  {
    if(!m_attribute) {
      if(ok) *ok = false;
      return 0;
    }
    return m_attribute->asInt(ok, layer);
  }

  QString AttributeAlias::asString(bool * const ok, Layer layer) const
  {
    if(!m_attribute) {
      if(ok) *ok = false;
      return QString();
    }
    return m_attribute->asString(ok, layer);
  }

  bool AttributeAlias::set(float v, Attribute::Layer layer, Attribute::ValueUnit unit)
  {
    if(!m_attribute)
      return false;
    return m_attribute->set(v, layer, unit);
  }

  bool AttributeAlias::set(int v, Attribute::Layer layer, Attribute::ValueUnit unit)
  {
    if(!m_attribute)
      return false;
    return m_attribute->set(v, layer, unit);
  }

  bool AttributeAlias::set(const QString & v, Attribute::Layer layer, Attribute::ValueUnit unit)
  {
    if(!m_attribute)
      return false;
    return m_attribute->set(v, layer, unit);
  }

  bool AttributeAlias::set(const Nimble::Vector2f & v, Attribute::Layer layer, QList<Attribute::ValueUnit> units)
  {
    if(!m_attribute)
      return false;
    return m_attribute->set(v, layer, units);
  }

  bool AttributeAlias::set(const Nimble::Vector3f & v, Attribute::Layer layer, QList<Attribute::ValueUnit> units)
  {
    if(!m_attribute)
      return false;
    return m_attribute->set(v, layer, units);
  }

  bool AttributeAlias::set(const Nimble::Vector4f & v, Attribute::Layer layer, QList<Attribute::ValueUnit> units)
  {
    if(!m_attribute)
      return false;
    return m_attribute->set(v, layer, units);
  }

  bool AttributeAlias::set(const StyleValue & v, Attribute::Layer layer)
  {
    if(!m_attribute)
      return false;
    return m_attribute->set(v, layer);
  }

  QByteArray AttributeAlias::type() const
  {
    return m_attribute ? m_attribute->type() : QByteArray();
  }

  ArchiveElement AttributeAlias::serialize(Archive & archive) const
  {
    if(!m_attribute)
      return ArchiveElement();
    return m_attribute->serialize(archive);
  }

  bool AttributeAlias::deserialize(const ArchiveElement & element)
  {
    if(!m_attribute)
      return false;
    return m_attribute->deserialize(element);
  }

  Attribute * AttributeAlias::attribute() const
  {
    return m_attribute;
  }

  void AttributeAlias::setAttribute(Attribute * attribute)
  {
    if(m_attribute == attribute)
      return;

    if(m_attribute) {
      m_attribute->removeListener(m_eventDelete);
      m_attribute->removeListener(m_eventChange);
      m_eventDelete = 0;
      m_eventChange = 0;
    }

    m_attribute = attribute;
    if(m_attribute) {
      m_eventDelete = m_attribute->addListener([=] { setAttribute(nullptr); }, DELETE_ROLE);
      m_eventChange = m_attribute->addListener([=] { emitChange(); }, CHANGE_ROLE);
    }

    // This serves a double purpose. First, since the target attribute has changed, the value is probably
    // different and it would be nice to notify listeners. Second, it indirectly allows listeners to detect
    // when the attribute target has changed or has become non-null.
    emitChange();
  }

  bool AttributeAlias::isChanged() const
  {
    if(!m_attribute)
      return false;
    return m_attribute->isChanged();
  }

  void AttributeAlias::clearValue(Attribute::Layer layout)
  {
    if(m_attribute)
      m_attribute->clearValue(layout);
  }

  bool AttributeAlias::handleShorthand(const StyleValue & value, Radiant::ArrayMap<Attribute *, StyleValue> & expanded)
  {
    if (m_attribute)
      return m_attribute->handleShorthand(value, expanded);
    return false;
  }

  bool AttributeAlias::isValueDefinedOnLayer(Attribute::Layer layer) const
  {
    return m_attribute ? m_attribute->isValueDefinedOnLayer(layer) : false;
  }

  Attribute * AttributeAlias::attribute(const QByteArray & name) const
  {
    if(!m_attribute)
      return nullptr;
    return m_attribute->attribute(name);
  }

  void AttributeAlias::setAsDefaults()
  {
    if (m_attribute)
      m_attribute->setAsDefaults();
  }
}
