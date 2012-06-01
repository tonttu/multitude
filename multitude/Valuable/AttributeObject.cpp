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

#include "AttributeObject.hpp"
#include "Node.hpp"
#include "ChangeMap.hpp"

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
  using namespace Radiant;

  bool Serializable::deserializeXML(const DOMElement &element)
  {
    ArchiveElement ae = XMLArchiveElement::create(element);
    return deserialize(ae);
  }

  Attribute::Attribute()
  : m_host(0),
    m_changed(false),
    m_transit(false),
    m_listenersId(0)
  {}

  Attribute::Attribute(Node * host, const QString & name, bool transit)
    : m_host(0),
      m_changed(false),
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
    , m_changed(false)
    , m_listenersId(0)
  {
    *this = o;
  }

  const Attribute & Attribute::operator = (const Attribute & o)
  {
    // Do not copy the name or listeners. It will break stuff.
    m_transit = o.m_transit;
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

  void Attribute::setName(const QString & s)
  {
    if(host())
      host()->valueRenamed(m_name, s);

    m_name = s;
  }

  QString Attribute::path() const
  {
    if(m_host)
      return m_host->path() + "/" + m_name;

    return "/" + m_name;
  }

  void Attribute::processMessage(const QString &, Radiant::BinaryData & )
  {
    Radiant::error("Attribute::processMessage # Unimplemented for %s",
                   typeid(*this).name());
  }

  void Attribute::processMessageString(const char * id, const char * str)
  {
    Radiant::BinaryData bd;
    bd.writeString(str);
    bd.rewind();
    processMessage(id, bd);
  }

  void Attribute::processMessageFloat(const char * id, float v)
  {
    Radiant::BinaryData bd;
    bd.writeFloat32(v);
    bd.rewind();
    processMessage(id, bd);
  }

  void Attribute::processMessageInt(const char * id, int v)
  {
    Radiant::BinaryData bd;
    bd.writeInt32(v);
    bd.rewind();
    processMessage(id, bd);
  }

  void Attribute::processMessageVector2(const char * id, Nimble::Vector2 v)
  {
    Radiant::BinaryData bd;
    bd.writeVector2Float32(v);
    bd.rewind();
    processMessage(id, bd);
  }

  void Attribute::processMessageVector3(const char * id, Nimble::Vector3 v)
  {
    Radiant::BinaryData bd;
    bd.writeVector3Float32(v);
    bd.rewind();
    processMessage(id, bd);
  }

  void Attribute::processMessageVector4(const char * id, Nimble::Vector4 v)
  {
    Radiant::BinaryData bd;
    bd.writeVector4Float32(v);
    bd.rewind();
    processMessage(id, bd);
  }

  float Attribute::asFloat(bool * ok) const
  {
    if(ok) *ok = false;
    Radiant::error(
"Attribute::asFloat # %s : conversion not available", m_name.toUtf8().data());
    return 0.0f;
  }

  int Attribute::asInt(bool * ok) const
  {
    if(ok) *ok = false;
    Radiant::error(
"Attribute::asInt # %s : conversion not available", m_name.toUtf8().data());
    return 0;
  }

  QString Attribute::asString(bool * ok) const
  {
    if(ok) *ok = false;
    Radiant::error(
"Attribute::asString # %s : conversion not available", m_name.toUtf8().data());
    return "";
  }

  ArchiveElement Attribute::serialize(Archive & archive) const
  {
    ArchiveElement elem = archive.createElement(m_name.isEmpty() ? "Attribute" : m_name.toUtf8().data());
    elem.add("type", type());
    elem.set(asString());

    return elem;
  }

  Attribute * Attribute::getValue(const QString & )
  {
    return 0;
  }

  void Attribute::emitChange()
  {
//    Radiant::trace("Attribute::emitChange # '%s'", m_name.toUtf8().data());
    m_changed = true;
    foreach(const AttributeListener & l, m_listeners) {
      if(l.role & CHANGE_ROLE) {
        if(!l.func) {
#ifdef MULTI_WITH_V8
          /// @todo what is the correct receiver ("this" in the callback)?
          /// @todo is this legal without v8::HandleScope handle_scope;
          /// @todo is it legal to give null pointer to argv parameter?
          l.scriptFunc->Call(v8::Context::GetCurrent()->Global(), 0, 0);
#endif
        } else l.func();
      }
    }
    ChangeMap::addChange(this);
  }

  void Attribute::emitDelete()
  {
    //Radiant::trace("Attribute::emitDelete");
    foreach(const AttributeListener & l, m_listeners) {
      if(l.role & DELETE_ROLE) {
        if(!l.func) {
#ifdef MULTI_WITH_V8
          /// @todo what is the correct receiver ("this" in the callback)?
          l.scriptFunc->Call(v8::Context::GetCurrent()->Global(), 0, 0);
#endif
        } else l.func();
      }
      if(l.listener) l.listener->m_valueListening.remove(this);
    }
    m_listeners.clear();
    ChangeMap::addDelete(this);
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

#ifdef MULTI_WITH_V8
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

    foreach(Node * listener, listeners) {
      bool found = false;
      foreach(const AttributeListener & l, m_listeners)
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
    return m_changed;
  }

  void Attribute::clearValue(Layer)
  {
  }

  bool Attribute::shortcut() const
  {
    return false;
  }

  bool Attribute::set(float, Layer, ValueUnit)
  {
    Radiant::error("Attribute::set(float) # %s: conversion not available",
                   m_name.toUtf8().data());
    return false;
  }

  bool Attribute::set(int, Layer, ValueUnit)
  {
    Radiant::error("Attribute::set(int) # %s: conversion not available",
                   m_name.toUtf8().data());
    return false;
  }

  bool Attribute::set(const QString &, Layer, ValueUnit)
  {
    Radiant::error("Attribute::set(string) # %s: conversion not available",
                   m_name.toUtf8().data());
    return false;
  }

  bool Attribute::set(const Nimble::Vector2f &, Layer, QList<ValueUnit>)
  {
    Radiant::error("Attribute::set(Vector2f) # %s: conversion not available",
                   m_name.toUtf8().data());
    return false;
  }

  bool Attribute::set(const Nimble::Vector3f &, Layer, QList<ValueUnit>)
  {
    Radiant::error("Attribute::set(Vector3f) # %s: conversion not available",
                   m_name.toUtf8().data());
    return false;
  }

  bool Attribute::set(const Nimble::Vector4f &, Layer, QList<ValueUnit>)
  {
    Radiant::error("Attribute::set(Vector4f) # %s: conversion not available",
                   m_name.toUtf8().data());
    return false;
  }

  bool Attribute::set(const QVariantList &, QList<ValueUnit>, Layer)
  {
    Radiant::error("Attribute::set(QVariantList) # %s: conversion not available",
                   m_name.toUtf8().data());
    return false;
  }
}
