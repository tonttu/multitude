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

#include "ValueObject.hpp"
#include "HasValues.hpp"
#include "ChangeMap.hpp"

#include "DOMElement.hpp"
#include "DOMDocument.hpp"
#include "XMLArchive.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/Mutex.hpp>

#ifdef MULTI_DOCUMENTER
std::list<Valuable::ValueObject::Doc> Valuable::ValueObject::doc;
#endif

namespace Valuable
{
  using namespace Radiant;

  bool Serializable::deserializeXML(const DOMElement &element)
  {
    ArchiveElement ae = XMLArchiveElement::create(element);
    return deserialize(ae);
  }

  ValueObject::ValueObject()
  : m_host(0),
    m_changed(false),
    m_transit(false)
  {}

  ValueObject::ValueObject(HasValues * host, const std::string & name, bool transit)
    : m_host(0),
      m_changed(false),
      m_name(name),
      m_transit(transit)
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

  ValueObject::ValueObject(const ValueObject & o)
    : Serializable(), // GCC wants this
    m_host(0),
    m_changed(false)
  {
    m_name = o.m_name;
    m_transit = o.m_transit;
  }

  ValueObject::~ValueObject()
  {
    emitDelete();
#ifdef MULTI_DOCUMENTER
    for(std::list<Doc>::iterator it = doc.begin(); it != doc.end();) {
      if(it->vo == this) it = doc.erase(it);
      else ++it;
    }
#endif
  }

  void ValueObject::setName(const std::string & s)
  {
    if(host())
      host()->valueRenamed(m_name, s);

    m_name = s;
  }

  std::string ValueObject::path() const
  {
    if(m_host)
      return m_host->path() + "/" + m_name;

    return "/" + m_name;
  }

  void ValueObject::processMessage(const char *, Radiant::BinaryData & )
  {
    Radiant::error("ValueObject::processMessage # Unimplemented for %s",
                   typeid(*this).name());
  }

  void ValueObject::processMessageString(const char * id, const char * str)
  {
    Radiant::BinaryData bd;
    bd.writeString(str);
    bd.rewind();
    processMessage(id, bd);
  }

  void ValueObject::processMessageFloat(const char * id, float v)
  {
    Radiant::BinaryData bd;
    bd.writeFloat32(v);
    bd.rewind();
    processMessage(id, bd);
  }

  void ValueObject::processMessageInt(const char * id, int v)
  {
    Radiant::BinaryData bd;
    bd.writeInt32(v);
    bd.rewind();
    processMessage(id, bd);
  }

  void ValueObject::processMessageVector2(const char * id, Nimble::Vector2 v)
  {
    Radiant::BinaryData bd;
    bd.writeVector2Float32(v);
    bd.rewind();
    processMessage(id, bd);
  }

  void ValueObject::processMessageVector3(const char * id, Nimble::Vector3 v)
  {
    Radiant::BinaryData bd;
    bd.writeVector3Float32(v);
    bd.rewind();
    processMessage(id, bd);
  }

  void ValueObject::processMessageVector4(const char * id, Nimble::Vector4 v)
  {
    Radiant::BinaryData bd;
    bd.writeVector4Float32(v);
    bd.rewind();
    processMessage(id, bd);
  }

  float ValueObject::asFloat(bool * ok) const
  {
    if(ok) *ok = false;
    Radiant::error(
"ValueObject::asFloat # %s : conversion not available", m_name.c_str());
    return 0.0f;
  }

  int ValueObject::asInt(bool * ok) const
  {
    if(ok) *ok = false;
    Radiant::error(
"ValueObject::asInt # %s : conversion not available", m_name.c_str());
    return 0;
  }

  std::string ValueObject::asString(bool * ok) const
  {
    if(ok) *ok = false;
    Radiant::error(
"ValueObject::asString # %s : conversion not available", m_name.c_str());
    return "";
  }

  ArchiveElement ValueObject::serialize(Archive & archive) const
  {
    const char * n = m_name.empty() ? "ValueObject" : m_name.c_str();

    ArchiveElement elem = archive.createElement(n);
    elem.add("type", type());
    elem.set(asString());

    return elem;
  }

  ValueObject * ValueObject::getValue(const std::string & name)
  {
    return 0;
  }

  void ValueObject::emitChange()
  {
//    Radiant::trace("ValueObject::emitChange # '%s'", m_name.c_str());
    m_changed = true;
    m_listeners.emitChange(this);
    ChangeMap::addChange(this);
  }

  void ValueObject::emitDelete()
  {
    //Radiant::trace("ValueObject::emitDelete");
    m_listeners.emitDelete(this);
    ChangeMap::addDelete(this);
  }

  void ValueObject::removeHost()
  {
    if(m_host) {
      m_host->removeValue(this);
      m_host = 0;
    }
  }

  bool ValueObject::isChanged() const
  {
    return m_changed;
  }

  bool ValueObject::set(float )
  {
    Radiant::error(
"ValueObject::set(float) # conversion not available");
    return false;
  }

  bool ValueObject::set(int )
  {
    Radiant::error(
"ValueObject::set(int) # conversion not available");
    return false;
  }

  bool ValueObject::set(const std::string & )
  {
    Radiant::error(
"ValueObject::set(string) # conversion not available");
    return false;
  }

  bool ValueObject::set(const Nimble::Vector2f & )
  {
    Radiant::error(
"ValueObject::set(Vector2f) # conversion not available");
    return false;
  }

  bool ValueObject::set(const Nimble::Vector4f & )
  {
    Radiant::error(
"ValueObject::set(Vector4f) # conversion not available");
    return false;
  }

}
