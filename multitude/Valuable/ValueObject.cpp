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

#include <Radiant/Trace.hpp>

#include <typeinfo>

namespace Valuable
{
  using namespace Radiant;

  ValueObject::ValueObject()
  : m_parent(0),
    m_transit(false)
  {}

  ValueObject::ValueObject(HasValues * parent, const std::string & name, bool transit)
    : m_parent(0),
      m_name(name),
      m_transit(transit)
  {
    if(parent)
      parent->addValue(name, this);
  }

  ValueObject::ValueObject(const ValueObject & o)
    : m_parent(0)
  {
    m_name = o.m_name;
    m_transit = o.m_transit;
  }

  ValueObject::~ValueObject()
  {
    emitDelete();
  }

  void ValueObject::setName(const std::string & s)
  {
    if(parent())
      parent()->childRenamed(m_name, s);

    m_name = s;
  }

  std::string ValueObject::path() const
  {
    if(m_parent)
      return m_parent->path() + "/" + m_name;

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

  DOMElement ValueObject::serializeXML(DOMDocument * doc)
  {
    if(m_name.empty()) {
      Radiant::error(
"ValueObject::serializeXML # attempt to serialize object with no name");
      return DOMElement();
    }

    DOMElement elem = doc->createElement(m_name.c_str());
    elem.setAttribute("type", type());
    elem.setTextContent(asString());

    return elem;
  }

  void ValueObject::emitChange()
  {
//    Radiant::trace("ValueObject::emitChange # '%s'", m_name.c_str());
    m_listeners.emitChange(this);
    ChangeMap::addChange(this);
  }

  void ValueObject::emitDelete()
  {
    //Radiant::trace("ValueObject::emitDelete");
    m_listeners.emitDelete(this);
    ChangeMap::addDelete(this);
  }

  void ValueObject::removeParent()
  {
    if(m_parent) {
      m_parent->removeValue(this);
      m_parent = 0;
    }
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
