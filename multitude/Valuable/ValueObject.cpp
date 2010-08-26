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

#ifdef VALUEOBJECT_MEMCHECK
#ifdef __GNUC__
#include <execinfo.h>
#include <cxxabi.h>
#endif
#endif

namespace Valuable
{
  using namespace Radiant;

#ifdef VALUEOBJECT_MEMCHECK
#ifdef __GNUC__
  typedef std::map<Serializable*, std::pair<void **, size_t> > MemMap;
  static MemMap s_map;
  static Radiant::MutexStatic s_mutex;

  inline void print_bt(void ** data, size_t size) {
    char ** strings = backtrace_symbols(data, size);

    int status;
    for(size_t i = 0; i < size; i++) {
      char * a = index(strings[i], '(');
      char * b = index(strings[i], '+');
      char * c = index(strings[i], ')');

      if(a && b && c && b > a + 1) {
        *b = '\0';
        char * tmp = abi::__cxa_demangle(a + 1, 0, 0, &status);
        if(tmp) {
          *a = '\0';
          std::cerr << tmp << ' ' << strings[i] << std::endl;
          free(tmp);
        } else {
          std::cerr << strings[i] << '+' << (b+1) << std::endl;
        }
      } else {
        std::cerr << strings[i] << std::endl;
      }
    }
    free (strings);
  }

  Serializable::Serializable()
  {
    void ** bt = new void*[50];
    size_t size = backtrace(bt, 50);

    Radiant::Guard g(s_mutex);
    s_map[this] = std::make_pair(bt, size);
  }

  Serializable::Serializable(const Serializable &)
  {
    void ** bt = new void*[50];
    size_t size = backtrace(bt, 50);

    Radiant::Guard g(s_mutex);
    s_map[this] = std::make_pair(bt, size);
  }

  Serializable & Serializable::operator=(const Serializable &)
  {
    void ** bt = new void*[50];
    size_t size = backtrace(bt, 50);

    Radiant::Guard g(s_mutex);
    s_map[this] = std::make_pair(bt, size);

    return *this;
  }

  Serializable::~Serializable() {
    Radiant::Guard g(s_mutex);
    MemMap::iterator it = s_map.find(this);
    if(it == s_map.end()) {
      std::cerr << "~Serializable: Couldn't find object " << this << std::endl;
      void ** bt = new void*[50];
      size_t size = backtrace(bt, 50);
      print_bt(bt, size);
      delete[] bt;
    } else {
      delete[] it->second.first;
      s_map.erase(it);
    }
  }

  class Checker {
  public:
    virtual ~Checker() {
      MemMap::iterator it;
      Radiant::GuardStatic g(s_mutex);
      if(s_map.empty())
        Radiant::info("All Serializables were released correctly");

      for(it = s_map.begin(); it != s_map.end(); ++it) {
        std::cerr << "Unreleased: " << it->first << std::endl;
        print_bt(it->second.first, it->second.second);
      }
    }
  };

  static Checker s_checker;
#endif
#endif

  bool Serializable::deserializeXML(DOMElement &element)
  {
    XMLArchiveElement ae(element);
    return deserialize(ae);
  }

  ValueObject::ValueObject()
  : m_parent(0),
    m_changed(false),
    m_transit(false)
  {}

  ValueObject::ValueObject(HasValues * parent, const std::string & name, bool transit)
    : m_parent(0),
      m_changed(false),
      m_name(name),
      m_transit(transit)
  {
    if(parent)
      parent->addValue(name, this);
  }

  ValueObject::ValueObject(const ValueObject & o) :
#ifdef VALUEOBJECT_MEMCHECK
    Serializable(o),
#endif
    m_parent(0),
    m_changed(false)
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

  ArchiveElement & ValueObject::serialize(Archive &archive)
  {
    if(m_name.empty()) {
      Radiant::error(
"ValueObject::serialize # attempt to serialize object with no name");
      return archive.emptyElement();
    }

    ArchiveElement & elem = archive.createElement(m_name.c_str());
    elem.add("type", type());
    elem.set(asString());

    return elem;
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

  void ValueObject::removeParent()
  {
    if(m_parent) {
      m_parent->removeValue(this);
      m_parent = 0;
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
