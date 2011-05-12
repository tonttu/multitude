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
        (m_to == that.m_to) && (*m_func == *that.m_func);
  }

  HasValues::HasValues()
      : ValueObject(),
      m_eventsEnabled(true),
      m_id(this, "id", generateId()),
      m_frame(0)
  {}

  HasValues::HasValues(HasValues * parent, const QString & name, bool transit)
      : ValueObject(parent, name, transit),
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
      if(it->m_valid) {
        if(it->m_listener)
          it->m_listener->eventRemoveSource(this);
        else
          it->m_func.Dispose();
      }
    }

    foreach(ValueObject* vo, m_valueListening) {
      for(QList<ValueListener>::iterator it = vo->m_listeners.begin(); it != vo->m_listeners.end(); ) {
        if(it->listener == this) {
          it = vo->m_listeners.erase(it);
        } else ++it;
      }
    }
  }

  ValueObject * HasValues::getValue(const QString & name)
  {
    container::iterator it = m_children.find(name);

    return it == m_children.end() ? 0 : it->second;
  }

  bool HasValues::addValue(const QString & cname, ValueObject * const value)
  {
    //    Radiant::trace("HasValues::addValue # adding %s", cname.c_str());

    // Check children
    if(m_children.find(cname) != m_children.end()) {
      Radiant::error(
          "HasValues::addValue # can not add child '%s' as '%s' "
          "already has a child with the same name.",
          cname.toUtf8().data(), m_name.toUtf8().data());
      return false;
    }

    // Unlink parent if necessary
    HasValues * parent = value->parent();
    if(parent) {
      Radiant::error(
          "HasValues::addValue # '%s' already has a parent '%s'. "
          "Unlinking it to set new parent.",
          cname.toUtf8().data(), parent->name().toUtf8().data());
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
    const QString & cname = value->name();

    container::iterator it = m_children.find(cname);
    if(it == m_children.end()) {
      Radiant::error(
          "HasValues::removeValue # '%s' is not a child of '%s'.",
          cname.toUtf8().data(), m_name.toUtf8().data());
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

  ArchiveElement & HasValues::serialize(Archive & archive) const
  {
    QString name;
    if(m_name.isEmpty()) {
      if(parent()) {
        Radiant::error(
          "HasValues::serialize # attempt to serialize object with no name");
        return archive.emptyElement();
      } else name = "ValueObject";
    } else name = m_name;

    ArchiveElement & elem = archive.createElement(name.toUtf8().data());
    if(elem.isNull()) {
      Radiant::error(
          "HasValues::serialize # failed to create element");
      return archive.emptyElement();
    }

    elem.add("type", type());

    for(container::const_iterator it = m_children.begin(); it != m_children.end(); it++) {
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

      QString name = elem.name();

      ValueObject * vo = getValue(name);

      // If the value exists, just deserialize it. Otherwise, pass the element
      // to readElement()
      if(vo)
        vo->deserialize(elem);
      else if(!elem.xml() || !readElement(*elem.xml())) {
        Radiant::error(
            "HasValues::deserialize # (%s) don't know how to handle element '%s'", type(), name.toUtf8().data());
        return false;
      }
    }

    return true;
  }

  void HasValues::debugDump() {
    Radiant::trace(Radiant::DEBUG, "%s {", m_name.toUtf8().data());

    for(container::iterator it = m_children.begin(); it != m_children.end(); it++) {
      ValueObject * vo = it->second;

      HasValues * hv = dynamic_cast<HasValues *> (vo);
      if(hv) hv->debugDump();
      else {
        QString s = vo->asString();
        Radiant::trace(Radiant::DEBUG, "\t%s = %s", vo->name().toUtf8().data(), s.toUtf8().data());
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

  void HasValues::eventAddListener(const char * from,
                                   const char * to,
                                   v8::Persistent<v8::Function> func,
                                   const Radiant::BinaryData * defaultData)
  {
    ValuePass vp;
    vp.m_func = func;
    vp.m_from = from;
    vp.m_to = to;

    if(defaultData)
      vp.m_defaultData = *defaultData;

    if(std::find(m_elisteners.begin(), m_elisteners.end(), vp) !=
       m_elisteners.end())
      debug("Widget::eventAddListener # Already got item %s -> %s",
            from, to);
    else {
      m_elisteners.push_back(vp);
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

    ValueObject * vo = getValue(QString::fromUtf8(key.c_str()));

    if(vo) {
      // info("HasValues::processMessage # Sending message \"%s\" to %s",
      // id + skip, typeid(*vo).name());
      vo->processMessage(id + skip, data);
    }
  }

  HasValues::Uuid HasValues::generateId()
  {
    static Radiant::Mutex s_mutex;
    Radiant::Guard g(s_mutex);
    static Uuid s_id = static_cast<Uuid>(Radiant::TimeStamp::getTime());
    return s_id++;
  }

  HasValues::Uuid HasValues::id() const
  {
    return m_id;
  }

  void HasValues::eventSend(const QString & id, Radiant::BinaryData & bd)
  {
    eventSend(id.toUtf8().data(), bd);
  }

  void HasValues::eventSend(const char * id, Radiant::BinaryData & bd)
  {
    if(!id || !m_eventsEnabled)
      return;

    m_frame++;

    for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end(); ++it) {
      ValuePass & vp = *it;

      if(!vp.m_valid) {
        it = m_elisteners.erase(it);
        continue;
      }
      else if(vp.m_frame == m_frame) {
        /* The listener was added during this function call. Lets not call it yet. */
      }
      else if(vp.m_from == id) {

        BinaryData & bdsend = vp.m_defaultData.total() ? vp.m_defaultData : bd;

        bdsend.rewind();

        if(vp.m_listener) {
          vp.m_listener->processMessage(vp.m_to.toUtf8().data(), bdsend);
        } else {
          /// @todo wrap bdsend
          /// @todo what is the correct receiver?
          v8::Local<v8::Value> argv[] = {v8::String::New(vp.m_to.toUtf8().data())};
          vp.m_func->Call(v8::Context::GetCurrent()->Global(), 1, argv);
        }
      }
    }
  }

  void HasValues::eventSend(const char * id)
  {
    Radiant::BinaryData tmp;
    eventSend(id, tmp);
  }

  void HasValues::childRenamed(const QString & was, const QString & now)
  {
    // Check that the value does not exist already
    iterator it = m_children.find(now);
    if(it != m_children.end()) {
      error("HasValues::childRenamed # Child '%s' already exist", now.toUtf8().data());
      return;
    }

    it = m_children.find(was);
    if(it == m_children.end()) {
      error("HasValues::childRenamed # No such child: %s", was.toUtf8().data());
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
  template VALUABLE_API bool HasValues::setValue<float>(const QString & name, const float &);
  template VALUABLE_API bool HasValues::setValue<Nimble::Vector2T<float> >(const QString & name, const Nimble::Vector2T<float> &);
  template VALUABLE_API bool HasValues::setValue<Nimble::Vector4T<float> >(const QString & name, const Nimble::Vector4T<float> &);


}
