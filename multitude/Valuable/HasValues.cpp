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

#include <Valuable/HasValuesImpl.hpp>
#include <Valuable/DOMDocument.hpp>
#include <Valuable/DOMElement.hpp>
#include <Valuable/Valuable.hpp>
#include <Valuable/HasValues.hpp>
#include <Valuable/Serializer.hpp>

#include <Radiant/Mutex.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/RefPtr.hpp>

#include <algorithm>
#include <typeinfo>
#include <cstring>

namespace Valuable
{
  using namespace Radiant;


  inline bool HasValues::ValuePass::operator == (const ValuePass & that) const
  {
    return m_valid && that.m_valid &&
        (m_listener == that.m_listener) && (m_from == that.m_from) &&
        (m_to == that.m_to);
  }

  HasValues::HasValues()
      : ValueObject(),
      m_eventsEnabled(true),
      m_id(this, "id", generateId()),
      m_frame(0)
  {}

  HasValues::HasValues(HasValues * host, const std::string & name, bool transit)
      : ValueObject(host, name, transit),
      m_eventsEnabled(true),
      m_id(this, "id", generateId()),
      m_frame(0)
  {
  }

  HasValues::~HasValues()
  {
    while(!m_eventSources.empty()) {
      /* The eventRemoveListener call will also clear the relevant part from m_eventSources. */
      (*m_eventSources.begin())->eventRemoveListener(this);
    }

    for(Listeners::iterator it = m_elisteners.begin();
    it != m_elisteners.end(); it++) {
      if(it->m_valid)
        (*it).m_listener->eventRemoveSource(this);
    }

  }

  ValueObject * HasValues::getValue(const std::string & name)
  {
    container::iterator it = m_values.find(name);

    return it == m_values.end() ? 0 : it->second;
  }

  bool HasValues::addValue(const std::string & cname, ValueObject * const value)
  {
    //    Radiant::trace("HasValues::addValue # adding %s", cname.c_str());

    // Check values
    if(m_values.find(cname) != m_values.end()) {
      Radiant::error(
          "HasValues::addValue # can not add value '%s' as '%s' "
          "already has a value with the same name.",
          cname.c_str(), m_name.c_str());
      return false;
    }

    // Unlink host if necessary
    HasValues * host = value->host();
    if(host) {
      Radiant::error(
          "HasValues::addValue # '%s' already has a host '%s'. "
          "Unlinking it to set new host.",
          cname.c_str(), host->name().c_str());
      value->removeHost();
    }

    // Change the value name
    value->setName(cname);

    m_values[value->name()] = value;
    value->m_host  = this;

    return true;
  }

  void HasValues::removeValue(ValueObject * const value)
  {
    const std::string & cname = value->name();

    container::iterator it = m_values.find(cname);
    if(it == m_values.end()) {
      Radiant::error(
          "HasValues::removeValue # '%s' is not a child value of '%s'.",
          cname.c_str(), m_name.c_str());
      return;
    }

    m_values.erase(it);
    value->m_host = 0;
  }

  bool HasValues::saveToFileXML(const char * filename)
  {
    bool ok = Serializer::serializeXML(filename, this);
    if (!ok) {
      Radiant::error("HasValues::saveToFileXML # object failed to serialize");
    }
    return ok;
  }

  bool HasValues::saveToMemoryXML(std::vector<char> & buffer)
  {
    XMLArchive archive;
    archive.setRoot(serialize(archive));

    return archive.writeToMem(buffer);
  }

  bool HasValues::loadFromFileXML(const char * filename)
  {
    XMLArchive archive;

    if(!archive.readFromFile(filename))
      return false;

    return deserialize(archive.root());
  }

  ArchiveElement & HasValues::serialize(Archive & archive) const
  {
    const char * n = m_name.empty() ? "HasValues" : m_name.c_str();

    ArchiveElement & elem = archive.createElement(n);
    if(elem.isNull()) {
      Radiant::error(
          "HasValues::serialize # failed to create element");
      return archive.emptyElement();
    }

    elem.add("type", type());

    for(container::const_iterator it = m_values.begin(); it != m_values.end(); it++) {
      ValueObject * vo = it->second;

      if (!archive.checkFlag(Archive::ONLY_CHANGED) || vo->isChanged()) {
        ArchiveElement & child = vo->serialize(archive);
        if(!child.isNull())
          elem.add(child);
      }
    }

    return elem;
  }

  bool HasValues::deserialize(ArchiveElement & element)
  {
    // Name
    m_name = element.name();

    // Children
    for(ArchiveElement::Iterator & it = element.children(); it; ++it) {
      ArchiveElement & elem = *it;

      std::string name = elem.name();

      ValueObject * vo = getValue(name);

      // If the value exists, just deserialize it. Otherwise, pass the element
      // to readElement()
      if(vo)
        vo->deserialize(elem);
      else if(!elem.xml() || !readElement(*elem.xml())) {
        Radiant::error(
            "HasValues::deserialize # (%s) don't know how to handle element '%s'", type(), name.c_str());
        return false;
      }
    }

    return true;
  }

  void HasValues::debugDump() {
    Radiant::trace(Radiant::DEBUG, "%s {", m_name.c_str());

    for(container::iterator it = m_values.begin(); it != m_values.end(); it++) {
      ValueObject * vo = it->second;

      HasValues * hv = dynamic_cast<HasValues *> (vo);
      if(hv) hv->debugDump();
      else {
        std::string s = vo->asString();
        Radiant::trace(Radiant::DEBUG, "\t%s = %s", vo->name().c_str(), s.c_str());
      }
    }

    Radiant::trace(Radiant::DEBUG, "}");
  }

  void HasValues::eventAddListener(const char * from,
                                   const char * to,
                                   Valuable::HasValues * obj,
                                   const Radiant::BinaryData * defaultData)
  {
    ValuePass vp;
    vp.m_listener = obj;
    vp.m_from = from;
    vp.m_to = to;
    vp.m_frame = m_frame;

    if(m_eventSendNames.find(from) == m_eventSendNames.end()) {
      warning("HasValues::eventAddListener # Adding listener to unexistent event '%s'", from);
    }

    if(obj->m_eventListenNames.find(to) == obj->m_eventListenNames.end()) {
      std::string klass = Radiant::StringUtils::demangle(typeid(*obj).name());
      warning("HasValues::eventAddListener # %s (%s %p) doesn't accept event '%s'",
              klass.c_str(), obj->name().c_str(), obj, to);
    }

    if(defaultData)
      vp.m_defaultData = *defaultData;

    if(std::find(m_elisteners.begin(), m_elisteners.end(), vp) !=
       m_elisteners.end())
      debugValuable("Widget::eventAddListener # Already got item %s -> %s (%p)",
            from, to, obj);
    else {
      // m_elisteners.push_back(vp);
      m_elisteners.push_back(vp);
      obj->eventAddSource(this);
    }
  }

  int HasValues::eventRemoveListener(Valuable::HasValues * obj, const char * from, const char * to)
  {
    int removed = 0;

    for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end(); it++) {

      if(it->m_listener == obj && it->m_valid) {
        // match from & to if specified
        if ( (!from || it->m_from == from) &&
             (!to || it->m_to == to) ) {
          it->m_valid = false;
          /* We cannot erase the list iterator, since that might invalidate iterators
             elsewhere. */
          removed++;
        }
      }
    }

    if(removed) {

      // Count number of references left to the object
      size_t count = 0;
      for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end(); it++) {
        if(it->m_listener == obj && it->m_valid)
          count++;
      }

      // If nothing references the object, remove the source
      if(count == 0)
        obj->eventRemoveSource(this);
    }

    return removed;
  }

  void HasValues::eventAddSource(Valuable::HasValues * source)
  {
    m_eventSources.insert(source);
  }

  void HasValues::eventRemoveSource(Valuable::HasValues * source)
  {
    Sources::iterator it = m_eventSources.find(source);

    if(it != m_eventSources.end())
      m_eventSources.erase(it);
  }

  void HasValues::processMessage(const char * id, Radiant::BinaryData & data)
  {
    // info("HasValues::processMessage # %s %s", typeid(*this).name(), id);

    if(!id)
      return;

    const char * delim = strchr(id, '/');

    std::string key(id);
    int skip;

    if(delim) {
      skip = delim - id;
      key.erase(key.begin() + skip, key.end());
      skip++;
    }
    else
      skip = (int) key.size();

    // info("HasValues::processMessage # Child id = %s", key.c_str());

    ValueObject * vo = getValue(key);

    if(vo) {
      // info("HasValues::processMessage # Sending message \"%s\" to %s",
      // id + skip, typeid(*vo).name());
      vo->processMessage(id + skip, data);
    } else {
      if(m_eventListenNames.find(id) == m_eventListenNames.end()) {
        /*warning("HasValues::processMessage # %s (%s %p) doesn't accept event '%s'",
                  klass.c_str(), name().c_str(), this, id);*/
      } else {
        std::string klass = Radiant::StringUtils::demangle(typeid(*this).name());
        warning("HasValues::processMessage # %s (%s %p): unhandled event '%s'",
                  klass.c_str(), name().c_str(), this, id);
      }
    }
  }

  // Must be outside function definition to be thread-safe
  static Radiant::Mutex s_generateIdMutex;

  HasValues::Uuid HasValues::generateId()
  {
    Radiant::Guard g(s_generateIdMutex);
    static Uuid s_id = static_cast<Uuid>(Radiant::TimeStamp::getTime());
    return s_id++;
  }

  HasValues::Uuid HasValues::id() const
  {
    return m_id;
  }

  void HasValues::eventAddSend(const std::string & id)
  {
    if (m_eventSendNames.find(id) != m_eventSendNames.end()) {
      warning("HasValues::eventAddSend # Trying to register event '%s' that is already registered", id.c_str());
    } else m_eventSendNames.insert(id);
  }

  void HasValues::eventAddListen(const std::string & id)
  {
    if (m_eventListenNames.find(id) != m_eventListenNames.end()) {
      warning("HasValues::eventAddListen # Trying to register duplicate event handler for event '%s'", id.c_str());
    } else m_eventListenNames.insert(id);
  }

  bool HasValues::acceptsEvent(const std::string & id) const
  {
    return m_eventListenNames.find(id) != m_eventListenNames.end();
  }

  void HasValues::eventSend(const std::string & id, Radiant::BinaryData & bd)
  {
    eventSend(id.c_str(), bd);
  }

  void HasValues::eventSend(const char * id, Radiant::BinaryData & bd)
  {
    if(!id || !m_eventsEnabled)
      return;

    if (m_eventSendNames.find(id) == m_eventSendNames.end()) {
      error("HasValues::eventSend # Sending unknown event '%s'", id);
    }

    m_frame++;

    for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end(); ) {
      ValuePass & vp = *it;

      if(!vp.m_valid) {
        it = m_elisteners.erase(it);
      }
      else if(vp.m_frame == m_frame) {
        /* The listener was added during this function call. Lets not call it yet. */
        it++;
      }
      else if(vp.m_from == id) {

        BinaryData & bdsend = vp.m_defaultData.total() ? vp.m_defaultData : bd;

        bdsend.rewind();

        vp.m_listener->processMessage(vp.m_to.c_str(), bdsend);
        it++;
      }
      else
        it++;
    }
  }

  void HasValues::eventSend(const char * id)
  {
    Radiant::BinaryData tmp;
    eventSend(id, tmp);
  }

  void HasValues::valueRenamed(const std::string & was, const std::string & now)
  {
    // Check that the value does not exist already
    iterator it = m_values.find(now);
    if(it != m_values.end()) {
      error("HasValues::valueRenamed # Value '%s' already exist", now.c_str());
      return;
    }

    it = m_values.find(was);
    if(it == m_values.end()) {
      error("HasValues::valueRenamed # No such value: %s", was.c_str());
      return;
    }
    ValueObject * vo = (*it).second;
    m_values.erase(it);
    m_values[now] = vo;
  }

  bool HasValues::readElement(DOMElement )
  {
    return false;
  }

  // Template functions must be instantiated to be exported
  template VALUABLE_API bool HasValues::setValue<float>(const std::string & name, const float &);
  template VALUABLE_API bool HasValues::setValue<Nimble::Vector2T<float> >(const std::string & name, const Nimble::Vector2T<float> &);
  template VALUABLE_API bool HasValues::setValue<Nimble::Vector4T<float> >(const std::string & name, const Nimble::Vector4T<float> &);


}
