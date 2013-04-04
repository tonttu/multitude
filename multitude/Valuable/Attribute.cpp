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

#ifdef MULTI_DOCUMENTER
std::list<Valuable::Attribute::Doc> Valuable::Attribute::doc;
#endif

namespace Valuable
{

  bool Serializable::deserializeXML(const DOMElement &element)
  {
    ArchiveElement ae = XMLArchiveElement::create(element);
    return deserialize(ae);
  }

  Attribute::Attribute()
  : m_host(0),
    m_serializable(true),
    m_transit(false),
    m_listenersId(0)
  {}

  Attribute::Attribute(Node * host, const QByteArray & name, bool transit)
    : m_host(0),
      m_serializable(true),
      m_name(name),
      m_transit(transit),
      m_listenersId(0)
  {
    if(host) {
      host->addValue(name, this);
#ifdef MULTI_DOCUMENTER
      doc.push_back(Doc());
      Doc & d = doc.back();
      d.class_name = Radiant::StringUtils::demangle(typeid(*host).name());
      d.vo = this;
      d.obj = host;
#endif
    }
  }

  Attribute::Attribute(const Attribute & o)
    : m_host(0)
    , m_serializable(true)
    , m_listenersId(0)
  {
    *this = o;
  }

  const Attribute & Attribute::operator = (const Attribute & o)
  {
    // Do not copy the name or listeners. It will break stuff.
    m_transit = o.m_transit;
    return *this;
  }

  Attribute::~Attribute()
  {
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
      host()->valueRenamed(m_name, s);

    m_name = s;
  }

  QByteArray Attribute::path() const
  {
    if(m_host)
      return m_host->path() + "/" + m_name;

    return "/" + m_name;
  }

  void Attribute::eventProcess(const QByteArray &, Radiant::BinaryData & )
  {
    Radiant::error("Attribute::eventProcess # Unimplemented for %s",
                   typeid(*this).name());
  }

  void Attribute::eventProcessString(const char * id, const QString & str)
  {
    Radiant::BinaryData bd;
    bd.writeString(str);
    bd.rewind();
    eventProcess(id, bd);
  }

  void Attribute::eventProcessString(const char *id, const char *str)
  {
    eventProcessString(id, QString(str));
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

  float Attribute::asFloat(bool * ok) const
  {
    if(ok) *ok = false;
    Radiant::error(
          "Attribute::asFloat # %s : conversion not available", m_name.data());
    return 0.0f;
  }

  int Attribute::asInt(bool * ok) const
  {
    if(ok) *ok = false;
    Radiant::error(
"Attribute::asInt # %s : conversion not available", m_name.data());
    return 0;
  }

  QString Attribute::asString(bool * ok) const
  {
    if(ok) *ok = false;
    Radiant::error(
"Attribute::asString # %s : conversion not available", m_name.data());
    return "";
  }

  ArchiveElement Attribute::serialize(Archive & archive) const
  {
    ArchiveElement elem = archive.createElement(m_name.isEmpty() ? "Attribute" : m_name.data());
    elem.add("type", type());
    elem.set(asString());

    return elem;
  }

  Attribute * Attribute::getValue(const QByteArray & name) const
  {
    return getAttribute(name);
  }

  Attribute * Attribute::getAttribute(const QByteArray & ) const
  {
    return nullptr;
  }

  void Attribute::emitChange()
  {
    // Radiant::trace("Attribute::emitChange # '%s'", m_name.data());
    // We use foreach here because the callback functions might
    // remove themselves from the listeners. Since foreach makes a
    // copy of the containers this doesn't present a problem
    foreach(const AttributeListener & l, m_listeners) {
      if(l.role & CHANGE_ROLE) {
        if(!l.func) {
#ifdef CORNERSTONE_JS
          /// @todo what is the correct receiver ("this" in the callback)?
          /// @todo is this legal without v8::HandleScope handle_scope;
          l.scriptFunc->Call(v8::Context::GetCurrent()->Global(), 0, 0);
#endif
        } else l.func();
      }
    }
  }

  void Attribute::emitDelete()
  {
    //Radiant::trace("Attribute::emitDelete");
    // We use foreach here because the callback functions might
    // remove themselves from the listeners. Since foreach makes a
    // copy of the containers this doesn't present a problem
    foreach(const AttributeListener & l, m_listeners) {
      if(l.role & DELETE_ROLE) {
        if(!l.func) {
#ifdef CORNERSTONE_JS
          /// @todo what is the correct receiver ("this" in the callback)?
          l.scriptFunc->Call(v8::Context::GetCurrent()->Global(), 0, 0);
#endif
        } else l.func();
      }
      if(l.listener) l.listener->m_valueListening.remove(this);
    }
    m_listeners.clear();
  }

  void Attribute::removeHost()
  {
    if(m_host) {
      m_host->removeValue(this);
      m_host = 0;
    }
  }

  long Attribute::addListener(ListenerFunc func, int role)
  {
    return addListener(0, func, role);
  }

  long Attribute::addListener(Node * listener, ListenerFunc func, int role)
  {
    long id = m_listenersId++;
    m_listeners[id] = AttributeListener(func, role, listener);
    if(listener) listener->m_valueListening << this;
    return id;
  }

#ifdef CORNERSTONE_JS
  long Attribute::addListener(v8::Persistent<v8::Function> func, int role)
  {
    long id = m_listenersId++;
    m_listeners[id] = AttributeListener(func, role);
    return id;
  }
#endif
  void Attribute::removeListeners(int role)
  {
    removeListener(0, role);
  }

  void Attribute::removeListener(Node * listener, int role)
  {
    QList<Node*> listeners;
    for(QMap<long, AttributeListener>::iterator it = m_listeners.begin(); it != m_listeners.end(); ) {
      if((it->role & role) && (!listener || listener == it->listener)) {
        if(it->listener) listeners << it->listener;
        it = m_listeners.erase(it);
      } else ++it;
    }

    for(Node * listener : listeners) {
      bool found = false;
      for(const AttributeListener & l : m_listeners)
        if(l.listener == listener) { found = true; break; }
      if(!found)
        listener->m_valueListening.remove(this);
    }
  }

  void Attribute::removeListener(long id)
  {
    QMap<long, AttributeListener>::iterator it = m_listeners.find(id);
    if(it == m_listeners.end()) return;
    if(it->listener) it->listener->m_valueListening.remove(this);
    m_listeners.erase(it);
  }

  bool Attribute::isChanged() const
  {
    return true;
  }

  void Attribute::clearValue(Layer)
  {
  }

  bool Attribute::handleShorthand(const StyleValue &,
                                  QMap<Attribute *, StyleValue> &)
  {
    return false;
  }

  bool Attribute::isValueDefinedOnLayer(Attribute::Layer) const
  {
    return false;
  }

  void Attribute::setSerializable(bool v)
  {
    m_serializable = v;
  }

  bool Attribute::serializable() const
  {
    return m_serializable;
  }

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
}
