/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Attribute.hpp"
#include "Node.hpp"

#include "DOMElement.hpp"
#include "DOMDocument.hpp"
#include "XMLArchive.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/ThreadChecks.hpp>

#ifdef MULTI_DOCUMENTER
std::list<Valuable::Attribute::Doc> Valuable::Attribute::doc;
#endif

namespace Valuable
{
  Serializable::Serializable() noexcept
    : m_serializable(true)
  {}

  bool Serializable::deserializeXML(const DOMElement &element)
  {
    ArchiveElement ae = XMLArchiveElement::create(element);
    return deserialize(ae);
  }

  void Serializable::setSerializable(bool v)
  {
    m_serializable = v;
  }

  bool Serializable::isSerializable() const
  {
    return m_serializable;
  }

  Attribute::Attribute()
  : m_host(0),
    m_ownerShorthand(nullptr),
    m_listenersId(0)
  {}

  Attribute::Attribute(Node * host, const QByteArray & name)
    : m_host(0),
      m_ownerShorthand(nullptr),
      m_name(name),
      m_listenersId(0)
#ifdef ENABLE_THREAD_CHECKS
    , m_ownerThread(host ? host->m_ownerThread : nullptr)
#endif
  {
    REQUIRE_THREAD(m_ownerThread);

    if(host == this)
      Radiant::fatal("Attribute::Attribute # host = this! # check your code");

    if(host) {
      host->addAttribute(name, this);
#ifdef MULTI_DOCUMENTER
      doc.push_back(Doc());
      Doc & d = doc.back();
      d.class_name = Radiant::StringUtils::type(*host);
      d.vo = this;
      d.obj = host;
#endif
    }
  }

  Attribute::Attribute(Attribute && o) noexcept
    : m_host(nullptr)
    , m_ownerShorthand(o.m_ownerShorthand)
    , m_name(std::move(o.m_name))
    , m_listeners(std::move(o.m_listeners))
    , m_listenersId(o.m_listenersId)
#ifdef ENABLE_THREAD_CHECKS
    , m_ownerThread(o.m_ownerThread)
#endif
  {
    if (auto host = o.m_host) {
      o.removeHost();
      host->addAttribute(this);
    }
  }

  const Attribute & Attribute::operator=(Attribute && o) noexcept
  {
    removeHost();
    m_name = std::move(o.m_name);
    m_ownerShorthand = o.m_ownerShorthand;
    m_listeners = std::move(o.m_listeners);
    m_listenersId = o.m_listenersId;
#ifdef ENABLE_THREAD_CHECKS
    m_ownerThread = o.m_ownerThread;
#endif

    if (auto host = o.m_host) {
      o.removeHost();
      host->addAttribute(this);
    }

    return *this;
  }

  Attribute::~Attribute() noexcept
  {
    REQUIRE_THREAD(m_ownerThread);
    emitDelete();

    removeHost();

#ifdef MULTI_DOCUMENTER
    for(std::list<Doc>::iterator it = doc.begin(); it != doc.end();) {
      if(it->vo == this) it = doc.erase(it);
      else ++it;
    }
#endif
  }

  void Attribute::setName(const QByteArray & s)
  {
    if(host())
      host()->attributeRenamed(m_name, s);

    m_name = s;
  }

  QByteArray Attribute::path() const
  {
    if(m_host)
      return m_host->path() + "/" + m_name;

    return m_name;
  }

  void Attribute::eventProcess(const QByteArray &, Radiant::BinaryData & )
  {
    Radiant::error("Attribute::eventProcess # Unimplemented for %s",
                   Radiant::StringUtils::type(*this).data());
  }

  void Attribute::eventProcessString(const QByteArray & id, const QString & str)
  {
    Radiant::BinaryData bd;
    bd.writeString(str);
    bd.rewind();
    eventProcess(id, bd);
  }

  void Attribute::eventProcessFloat(const char * id, float v)
  {
    Radiant::BinaryData bd;
    bd.writeFloat32(v);
    bd.rewind();
    eventProcess(id, bd);
  }

  void Attribute::eventProcessInt(const char * id, int v)
  {
    Radiant::BinaryData bd;
    bd.writeInt32(v);
    bd.rewind();
    eventProcess(id, bd);
  }

  void Attribute::eventProcessVector2(const char * id, Nimble::Vector2 v)
  {
    Radiant::BinaryData bd;
    bd.writeVector2Float32(v);
    bd.rewind();
    eventProcess(id, bd);
  }

  void Attribute::eventProcessVector3(const char * id, Nimble::Vector3 v)
  {
    Radiant::BinaryData bd;
    bd.writeVector3Float32(v);
    bd.rewind();
    eventProcess(id, bd);
  }

  void Attribute::eventProcessVector4(const char * id, Nimble::Vector4 v)
  {
    Radiant::BinaryData bd;
    bd.writeVector4Float32(v);
    bd.rewind();
    eventProcess(id, bd);
  }

  float Attribute::asFloat(bool * const ok, Layer) const
  {
    if(ok) *ok = false;
    else Radiant::error("Attribute::asFloat # %s : conversion not available", m_name.data());
    return 0.0f;
  }

  int Attribute::asInt(bool * const ok, Layer) const
  {
    if(ok) *ok = false;
    else Radiant::error("Attribute::asInt # %s : conversion not available", m_name.data());
    return 0;
  }

  QString Attribute::asString(bool * const ok, Layer) const
  {
    if(ok) *ok = true;
    return QString().sprintf("Attribute: '%s' @ %p", name().constData(), this);
  }

  ArchiveElement Attribute::serialize(Archive & archive) const
  {
    REQUIRE_THREAD(m_ownerThread);
    Layer layer;
    if (!layerForSerialization(archive, layer))
      return ArchiveElement();

    ArchiveElement elem = archive.createElement(m_name.isEmpty() ? "Attribute" : m_name.data());
    if (QByteArray t = type(); !t.isEmpty())
      elem.add("type", t);
    elem.set(asString(nullptr, layer));

    return elem;
  }

  Attribute * Attribute::attribute(const QByteArray & ) const
  {
    return nullptr;
  }

  void Attribute::setTransitionParameters(TransitionParameters)
  {
    Radiant::warning("Attribute::setTransitionParameters # Class %s (%s) doesn't support transition animations",
                     Radiant::StringUtils::type(*this).data(), name().data());
  }

  void Attribute::emitChange()
  {
    REQUIRE_THREAD(m_ownerThread);
    // Radiant::trace("Attribute::emitChange # '%s'", m_name.data());
    // We use Q_FOREACH here because the callback functions might
    // remove themselves from the listeners. Since Q_FOREACH makes a
    // copy of the containers this doesn't present a problem
    Q_FOREACH(const AttributeListener & l, m_listeners) {
      if(l.role & CHANGE_ROLE) {
        l.func();
      }
    }
  }

  void Attribute::emitDelete()
  {
    //Radiant::trace("Attribute::emitDelete");
    // We use Q_FOREACH here because the callback functions might
    // remove themselves from the listeners. Since Q_FOREACH makes a
    // copy of the containers this doesn't present a problem
    Q_FOREACH(const AttributeListener & l, m_listeners) {
      if(l.role & DELETE_ROLE) {
        l.func();
      }
      if(l.listener) l.listener->m_attributeListening.remove(this);
    }
    m_listeners.clear();
  }

  void Attribute::emitHostChange()
  {
    // Radiant::trace("Attribute::emitChange # '%s'", m_name.data());
    // We use Q_FOREACH here because the callback functions might
    // remove themselves from the listeners. Since Q_FOREACH makes a
    // copy of the containers this doesn't present a problem
    Q_FOREACH(const AttributeListener & l, m_listeners) {
      if(l.role & HOST_CHANGE_ROLE) {
        l.func();
      }
    }
  }

  void Attribute::removeHost()
  {
    REQUIRE_THREAD(m_ownerThread);
    if(m_host) {
      m_host->removeAttribute(this, false);
      m_host = 0;
      emitHostChange();
    }
  }

  long Attribute::addListener(ListenerFunc func, int role)
  {
    return addListener(0, func, role);
  }

  long Attribute::addListener(Node * listener, ListenerFunc func, int role)
  {
    REQUIRE_THREAD(m_ownerThread);
    if (listener && listener->isBeingDestroyed()) {
      return -1;
    }

    long id = m_listenersId++;
    m_listeners[id] = AttributeListener(func, role, listener);
    if(listener) listener->m_attributeListening << this;
    return id;
  }

  void Attribute::removeListeners(int role)
  {
    removeListener(0, role);
  }

  bool Attribute::removeListener(Node * listener, int role)
  {
    REQUIRE_THREAD(m_ownerThread);
    bool erasedAnything = false;

    QList<Node*> listeners;
    for(QMap<long, AttributeListener>::iterator it = m_listeners.begin(); it != m_listeners.end(); ) {
      if((it->role & role) && (!listener || listener == it->listener)) {
        if(it->listener) listeners << it->listener;
        it = m_listeners.erase(it);
        erasedAnything = true;
      } else ++it;
    }

    for(Node * listener : listeners) {
      bool found = false;
      for(const AttributeListener & l : m_listeners)
        if(l.listener == listener) { found = true; break; }
      if(!found)
        listener->m_attributeListening.remove(this);
    }

    return erasedAnything;
  }

  bool Attribute::removeListener(long id)
  {
    REQUIRE_THREAD(m_ownerThread);
    QMap<long, AttributeListener>::iterator it = m_listeners.find(id);
    if(it == m_listeners.end()) return false;
    if(it->listener) it->listener->m_attributeListening.remove(this);
    m_listeners.erase(it);
    return true;
  }

  bool Attribute::hasListener(Node * listener, int role) const
  {
    REQUIRE_THREAD(m_ownerThread);

    for (const AttributeListener & attrListener: m_listeners) {
      if ((attrListener.role & role) && (!listener || listener == attrListener.listener)) {
        return true;
      }
    }

    return false;
  }

  bool Attribute::hasListener(long id) const
  {
    REQUIRE_THREAD(m_ownerThread);
    return m_listeners.find(id) != m_listeners.end();
  }

  bool Attribute::isChanged() const
  {
    return false;
  }

  void Attribute::clearValue(Layer)
  {
  }

  bool Attribute::handleShorthand(const StyleValue &,
                                  Radiant::ArrayMap<Attribute *, StyleValue> &)
  {
    return false;
  }

  bool Attribute::isValueDefinedOnLayer(Attribute::Layer) const
  {
    return false;
  }

  void Attribute::setOwnerShorthand(Attribute * owner)
  {
    m_ownerShorthand = owner;
  }

  Attribute * Attribute::ownerShorthand() const
  {
    return m_ownerShorthand;
  }

#ifdef ENABLE_THREAD_CHECKS
  void Attribute::setOwnerThread(Radiant::Thread::Id owner)
  {
    m_ownerThread = owner;
  }
#endif

  bool Attribute::set(float, Layer, ValueUnit)
  {
    Radiant::error("Attribute::set(float) # %s: conversion not available",
                   m_name.data());
    return false;
  }

  bool Attribute::set(int, Layer, ValueUnit)
  {
    Radiant::error("Attribute::set(int) # %s: conversion not available",
                   m_name.data());
    return false;
  }

  bool Attribute::set(const QString &, Layer, ValueUnit)
  {
    Radiant::error("Attribute::set(string) # %s: conversion not available",
                   m_name.data());
    return false;
  }

  bool Attribute::set(const Nimble::Vector2f &, Layer, QList<ValueUnit>)
  {
    Radiant::error("Attribute::set(Vector2f) # %s: conversion not available",
                   m_name.data());
    return false;
  }

  bool Attribute::set(const Nimble::Vector3f &, Layer, QList<ValueUnit>)
  {
    Radiant::error("Attribute::set(Vector3f) # %s: conversion not available",
                   m_name.data());
    return false;
  }

  bool Attribute::set(const Nimble::Vector4f &, Layer, QList<ValueUnit>)
  {
    Radiant::error("Attribute::set(Vector4f) # %s: conversion not available",
                   m_name.data());
    return false;
  }

  bool Attribute::set(const StyleValue &, Layer)
  {
    Radiant::error("Attribute::set(StyleValue) # %s: conversion not available",
                   m_name.data());
    return false;
  }

  void Attribute::copyValueFromLayer(Layer from, Layer to)
  {
    (void) from;
    (void) to;
    Radiant::error("Attribute::copyValueFromLayer(Layer from, Layer to) # %s: conversion not available",
                   m_name.data());
  }
}
