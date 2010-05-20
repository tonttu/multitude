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

#include <Valuable/HasValues.hpp>
#include <Valuable/Serializer.hpp>

#include <Radiant/TimeStamp.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/RefPtr.hpp>

#include <algorithm>
#include <typeinfo>
#include <cstring>

namespace Valuable
{
  using namespace Radiant;

  HasValues::HasValues()
      : ValueObject(),
      m_eventsEnabled(true),
      m_id(this, "id", generateId())
  {}

  HasValues::HasValues(HasValues * parent, const std::string & name, bool transit)
      : ValueObject(parent, name, transit),
      m_eventsEnabled(true),
      m_id(this, "id", generateId())
  {
  }

  HasValues::~HasValues()
  {
    for(Sources::iterator it = m_eventSources.begin(); it != m_eventSources.end(); it++) {
      (*it)->eventRemoveListener(this);
    }

    for(Listeners::iterator it = m_elisteners.begin();
    it != m_elisteners.end(); it++) {
      (*it).m_listener->eventRemoveSource(this);
    }

  }

  ValueObject * HasValues::getValue(const std::string & name)
  {
    container::iterator it = m_children.find(name);

    return it == m_children.end() ? 0 : it->second;
  }

  bool HasValues::addValue(const std::string & cname, ValueObject * const value)
  {
    //    Radiant::trace("HasValues::addValue # adding %s", cname.c_str());

    // Check children
    if(m_children.find(cname) != m_children.end()) {
      Radiant::error(
          "HasValues::addValue # can not add child '%s' as '%s' "
          "already has a child with the same name.",
          cname.c_str(), m_name.c_str());
      return false;
    }

    // Unlink parent if necessary
    HasValues * parent = value->parent();
    if(parent) {
      Radiant::error(
          "HasValues::addValue # '%s' already has a parent '%s'. "
          "Unlinking it to set new parent.",
          cname.c_str(), parent->name().c_str());
      value->removeParent();
    }

    // Change the value name
    value->setName(cname);

    m_children[value->name()] = value;
    value->m_parent  = this;

    return true;
  }

  void HasValues::removeValue(ValueObject * const value)
  {
    const std::string & cname = value->name();

    container::iterator it = m_children.find(cname);
    if(it == m_children.end()) {
      Radiant::error(
          "HasValues::removeValue # '%s' is not a child of '%s'.",
          cname.c_str(), m_name.c_str());
      return;
    }

    m_children.erase(it);
    value->m_parent = 0;
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

  ArchiveElement & HasValues::serialize(Archive & archive)
  {
    if(m_name.empty()) {
      Radiant::error(
          "HasValues::serialize # attempt to serialize object with no name");
      return archive.emptyElement();
    }

    ArchiveElement & elem = archive.createElement(m_name.c_str());
    if(elem.isNull()) {
      Radiant::error(
          "HasValues::serialize # failed to create element");
      return archive.emptyElement();
    }

    elem.add("type", type());

    for(container::iterator it = m_children.begin(); it != m_children.end(); it++) {
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

    for(container::iterator it = m_children.begin(); it != m_children.end(); it++) {
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

    if(defaultData)
      vp.m_defaultData = *defaultData;

    if(std::find(m_elisteners.begin(), m_elisteners.end(), vp) !=
       m_elisteners.end())
      debug("Widget::eventAddListener # Already got item %s -> %s (%p)",
            from, to, obj);
    else {
      m_elisteners.push_back(vp);
      obj->eventAddSource(this);
    }
  }
  void eventAddListener(const char * from,
                        const char * to,
                        Valuable::HasValues * obj,
                        const Radiant::BinaryData & defaultData );

  int HasValues::eventRemoveListener(Valuable::HasValues * obj, const char * from, const char * to)
  {
    int removed = 0;
    for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end();){
      if((*it).m_listener == obj) {
        // match from & to if specified
        if ( (!from || it->m_from == from) &&
             (!to || it->m_to == to) ) {
          it = m_elisteners.erase(it);
          ++removed;
        }
      }
      else
        it++;
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
      skip = key.size();

    // info("HasValues::processMessage # Child id = %s", key.c_str());

    ValueObject * vo = getValue(key);

    if(vo) {
      // info("HasValues::processMessage # Sending message \"%s\" to %s",
      // id + skip, typeid(*vo).name());
      vo->processMessage(id + skip, data);
    }
  }

  HasValues::Uuid HasValues::generateId()
  {
    static Uuid id = static_cast<Uuid>(Radiant::TimeStamp::getTime());
    return id++;
  }

  HasValues::Uuid HasValues::id() const
  {
    return m_id;
  }

  void HasValues::eventSend(const std::string & id, Radiant::BinaryData & bd)
  {
    eventSend(id.c_str(), bd);
  }

  void HasValues::eventSend(const char * id, Radiant::BinaryData & bd)
  {
    if(!id || !m_eventsEnabled)
      return;

    for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end(); it++) {
      ValuePass & vp = *it;
      if(vp.m_from == id) {

        BinaryData & bdsend = vp.m_defaultData.total() ? vp.m_defaultData : bd;

        bdsend.rewind();

        vp.m_listener->processMessage(vp.m_to.c_str(), bdsend);

      }
    }
  }

  void HasValues::eventSend(const char * id)
  {
    Radiant::BinaryData tmp;
    eventSend(id, tmp);
  }

  void HasValues::childRenamed(const std::string & was, const std::string & now)
  {
    iterator it = m_children.find(was);
    if(it == m_children.end()) {
      error("HasValues::childRenamed # No such child: %s", was.c_str());
      return;
    }
    ValueObject * vo = (*it).second;
    m_children.erase(it);
    m_children[now] = vo;
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
