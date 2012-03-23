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

#include <Valuable/DOMDocument.hpp>
#include <Valuable/DOMElement.hpp>
#include <Valuable/Valuable.hpp>
#include <Valuable/Node.hpp>
#include <Valuable/Serializer.hpp>

#include <Radiant/Mutex.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/RefPtr.hpp>

#include <algorithm>
#include <typeinfo>
#include <cstring>

namespace
{
  struct QueueItem
  {
    QueueItem(Valuable::Node * sender_, Valuable::Node::ListenerFunc2 func_,
              const Radiant::BinaryData & data_)
      : sender(sender_)
      , func2(func_)
      , target()
      , data(data_)
    {}

    QueueItem(Valuable::Node * sender_, Valuable::Node::ListenerFunc func_)
      : sender(sender_)
      , func(func_)
      , func2()
      , target()
    {}

    QueueItem(Valuable::Node * sender_, Valuable::Node * target_,
              const QString & to_, const Radiant::BinaryData & data_)
      : sender(sender_)
      , func()
      , func2()
      , target(target_)
      , to(to_)
      , data(data_)
    {}

    Valuable::Node * sender;
    Valuable::Node::ListenerFunc func;
    Valuable::Node::ListenerFunc2 func2;
    Valuable::Node * target;
    const QString to;
    Radiant::BinaryData data;
  };

  // recursive because ~Node() might be called from processQueue()
  Radiant::Mutex s_queueMutex(true);
  QList<QueueItem*> s_queue;
  QSet<void *> s_queueOnce;

  void queueEvent(Valuable::Node * sender, Valuable::Node * target,
                  const QString & to, const Radiant::BinaryData & data,
                  void * once)
  {
    // make the new item before locking
    QueueItem * item = new QueueItem(sender, target, to, data);
    Radiant::Guard g(s_queueMutex);
    if(once) {
      if(s_queueOnce.contains(once)) return;
      s_queueOnce << once;
    }
    s_queue << item;
  }

  void queueEvent(Valuable::Node * sender, Valuable::Node::ListenerFunc func,
                  void * once)
  {
    QueueItem * item = new QueueItem(sender, func);
    Radiant::Guard g(s_queueMutex);
    if(once) {
      if(s_queueOnce.contains(once)) return;
      s_queueOnce << once;
    }
    s_queue << item;
  }

  void queueEvent(Valuable::Node * sender, Valuable::Node::ListenerFunc2 func,
                  const Radiant::BinaryData & data, void * once)
  {
    QueueItem * item = new QueueItem(sender, func, data);
    Radiant::Guard g(s_queueMutex);
    if(once) {
      if(s_queueOnce.contains(once)) return;
      s_queueOnce << once;
    }
    s_queue << item;
  }
}

namespace Valuable
{
  using namespace Radiant;

#ifdef MULTI_DOCUMENTER
  VALUABLE_API std::map<QString, std::set<QString> > s_eventSendNames;
  VALUABLE_API std::map<QString, std::set<QString> > s_eventListenNames;
#endif

  class Shortcut : public Attribute
  {
  public:
    Shortcut(Node * host, const QString & name)
      : Attribute(host, name)
    {}
    bool deserialize(const ArchiveElement &) { return false; }
    virtual bool shortcut() const { return true; }
    virtual const char * type() const { return "shortcut"; }
  };

  inline bool Node::ValuePass::operator == (const ValuePass & that) const
  {
    return m_valid && that.m_valid &&
        (m_listener == that.m_listener) && (m_from == that.m_from) &&
        (m_to == that.m_to)
    #ifdef MULTI_WITH_V8
        && (*m_funcv8 == *that.m_funcv8)
    #endif
        ;
  }

  Node::Node()
      : Attribute(),
      m_sender(0),
      m_eventsEnabled(true),
      m_id(this, "id", generateId()),
      m_frame(0)
  {}

  Node::Node(Node * host, const QString & name, bool transit)
      : Attribute(host, name, transit),
      m_eventsEnabled(true),
      m_id(this, "id", generateId()),
      m_frame(0)
  {
  }

  Node::~Node()
  {
    // Host of HasValues class member ValueObjects must be zeroed to avoid double-delete
    m_id.removeHost();

    while(!m_eventSources.empty()) {
      /* The eventRemoveListener call will also clear the relevant part from m_eventSources. */
      (*m_eventSources.begin())->eventRemoveListener(this);
    }

    for(Listeners::iterator it = m_elisteners.begin();
    it != m_elisteners.end(); it++) {
      if(it->m_valid) {
        if(it->m_listener)
          it->m_listener->eventRemoveSource(this);
#ifdef MULTI_WITH_V8
        else if(!it->m_funcv8.IsEmpty())
          it->m_funcv8.Dispose();
#endif
      }
    }

    foreach(Attribute* vo, m_valueListening) {
      for(QMap<long, AttributeListener>::iterator it = vo->m_listeners.begin(); it != vo->m_listeners.end(); ) {
        if(it->listener == this) {
          it = vo->m_listeners.erase(it);
        } else ++it;
      }
    }

    {
      Radiant::Guard g(s_queueMutex);
      for(QList<QueueItem*>::iterator it = s_queue.begin(); it != s_queue.end(); ++it) {
        QueueItem* item = *it;
        if(item->target == this)
          item->target = 0;
        if(item->sender == this)
          item->sender = 0;
      }
    }

    // Release memory for any value objects that are left (should be only
    // heap-allocated at this point)
    while(!m_values.empty())
      delete m_values.begin()->second;
  }

  Attribute * Node::getValue(const QString & name)
  {
    size_t slashIndex = name.indexOf('/');

    if(slashIndex == std::string::npos) {
      container::iterator it = m_values.find(name);

      return it == m_values.end() ? 0 : it->second;
    }
    else {
      const QString part1 = name.left(slashIndex);
      const QString part2 = name.mid(slashIndex + 1);

      Attribute * attribute = getValue(part1);
      if(attribute) {
        return attribute->getValue(part2);
      }
    }

    return 0;
  }

  bool Node::addValue(const QString & cname, Attribute * const value)
  {
    //    Radiant::trace("Node::addValue # adding %s", cname.c_str());

    // Check values
    if(m_values.find(cname) != m_values.end()) {
      Radiant::error(
          "Node::addValue # can not add value '%s' as '%s' "
          "already has a value with the same name.",
          cname.toUtf8().data(), m_name.toUtf8().data());
      return false;
    }

    // Unlink host if necessary
    Node * host = value->host();
    if(host) {
      Radiant::error(
          "Node::addValue # '%s' already has a host '%s'. "
          "Unlinking it to set new host.",
          cname.toUtf8().data(), host->name().toUtf8().data());
      value->removeHost();
    }

    // Change the value name
    value->setName(cname);

    m_values[value->name()] = value;
    value->m_host  = this;

    return true;
  }

  void Node::removeValue(Attribute * const value)
  {
    const QString & cname = value->name();

    container::iterator it = m_values.find(cname);
    if(it == m_values.end()) {
      Radiant::error(
          "Node::removeValue # '%s' is not a child value of '%s'.",
          cname.toUtf8().data(), m_name.toUtf8().data());
      return;
    }

    m_values.erase(it);
    value->m_host = 0;
  }

#ifdef MULTI_WITH_V8

  bool Node::setValue(const QString & name, v8::Handle<v8::Value> v)
  {
    using namespace v8;
    HandleScope handle_scope;
    if (v.IsEmpty()) {
      Radiant::error("Node::setValue # v8::Handle is empty");
      return false;
    }

    if (v->IsTrue()) return setValue(name, 1);
    if (v->IsFalse()) return setValue(name, 0);
    if (v->IsBoolean()) return setValue(name, v->ToBoolean()->Value() ? 1 : 0);
    if (v->IsInt32()) return setValue(name, int(v->ToInt32()->Value()));
    if (v->IsUint32()) return setValue(name, int(v->ToUint32()->Value()));
    if (v->IsString()) return setValue(name, QString::fromUtf16(*String::Value(v->ToString())));
    if (v->IsNumber()) return setValue(name, float(v->ToNumber()->NumberValue()));

    if (v->IsArray()) {
      Handle<Array> arr = v.As<Array>();
      assert(!arr.IsEmpty());
      if(arr->Length() == 2) {
        Local<Number> x = arr->Get(0)->ToNumber();
        Local<Number> y = arr->Get(1)->ToNumber();
        if (x.IsEmpty() || y.IsEmpty()) {
          Radiant::error("Node::setValue # v8::Value should be array of two numbers");
          return false;
        }
        return setValue(name, Nimble::Vector2f(x->Value(), y->Value()));
      } else if(arr->Length() == 4) {
        Local<Number> r = arr->Get(0)->ToNumber();
        Local<Number> g = arr->Get(1)->ToNumber();
        Local<Number> b = arr->Get(2)->ToNumber();
        Local<Number> a = arr->Get(3)->ToNumber();
        if (r.IsEmpty() || g.IsEmpty() || b.IsEmpty() || a.IsEmpty()) {
          Radiant::error("Node::setValue # v8::Value should be array of four numbers");
          return false;
        }
        return setValue(name, Nimble::Vector4f(r->Value(), g->Value(), b->Value(), a->Value()));
      }
      Radiant::error("Node::setValue # v8::Array with %d elements is not supported", arr->Length());
    } else if (v->IsRegExp()) {
      Radiant::error("Node::setValue # v8::Value type RegExp is not supported");
    } else if (v->IsDate()) {
      Radiant::error("Node::setValue # v8::Value type Date is not supported");
    } else if (v->IsExternal()) {
      Radiant::error("Node::setValue # v8::Value type External is not supported");
    } else if (v->IsObject()) {
      Radiant::error("Node::setValue # v8::Value type Object is not supported");
    } else if (v->IsFunction()) {
      Radiant::error("Node::setValue # v8::Value type Function is not supported");
    } else if (v->IsNull()) {
      Radiant::error("Node::setValue # v8::Value type Null is not supported");
    } else if (v->IsUndefined()) {
      Radiant::error("Node::setValue # v8::Value type Undefined is not supported");
    } else {
      Radiant::error("Node::setValue # v8::Value type is unknown");
    }
    return false;
  }
#endif

  bool Node::saveToFileXML(const QString & filename)
  {
    bool ok = Serializer::serializeXML(filename, this);
    if (!ok) {
      Radiant::error("Node::saveToFileXML # object failed to serialize (%s)", filename.toUtf8().data());
    }
    return ok;
  }

  bool Node::saveToMemoryXML(QByteArray & buffer)
  {
    XMLArchive archive;
    archive.setRoot(serialize(archive));

    return archive.writeToMem(buffer);
  }

  bool Node::loadFromFileXML(const QString & filename)
  {
    XMLArchive archive;

    if(!archive.readFromFile(filename))
      return false;

    return deserialize(archive.root());
  }

  ArchiveElement Node::serialize(Archive & archive) const
  {
    QString name = m_name.isEmpty() ? "Node" : m_name;

    ArchiveElement elem = archive.createElement(name.toUtf8().data());
    if(elem.isNull()) {
      Radiant::error(
          "Node::serialize # failed to create element");
      return ArchiveElement();
    }

    elem.add("type", type());

    for(container::const_iterator it = m_values.begin(); it != m_values.end(); it++) {
      Attribute * vo = it->second;

      if (!archive.checkFlag(Archive::ONLY_CHANGED) || vo->isChanged()) {
        ArchiveElement child = vo->serialize(archive);
        if(!child.isNull())
          elem.add(child);
      }
    }

    return elem;
  }

  bool Node::deserialize(const ArchiveElement & element)
  {
    // Name
    m_name = element.name();

    // Children
    for(ArchiveElement::Iterator it = element.children(); it; ++it) {
      ArchiveElement elem = *it;

      QString name = elem.name();

      Attribute * vo = getValue(name);

      // If the value exists, just deserialize it. Otherwise, pass the element
      // to readElement()
      bool ok = false;
      if(vo)
        ok = vo->deserialize(elem);
      if(!ok)
        ok = readElement(elem);
      if(!ok) {
        Radiant::error(
            "Node::deserialize # (%s) don't know how to handle element '%s'", type(), name.toUtf8().data());
        return false;
      }
    }

    return true;
  }

  void Node::debugDump() {
    Radiant::trace(Radiant::DEBUG, "%s {", m_name.toUtf8().data());

    for(container::iterator it = m_values.begin(); it != m_values.end(); it++) {
      Attribute * vo = it->second;

      Node * hv = dynamic_cast<Node *> (vo);
      if(hv) hv->debugDump();
      else {
        QString s = vo->asString();
        Radiant::trace(Radiant::DEBUG, "\t%s = %s", vo->name().toUtf8().data(), s.toUtf8().data());
      }
    }

    Radiant::trace(Radiant::DEBUG, "}");
  }

  void Node::eventAddListener(const QString & from,
                              const QString & to,
                              Valuable::Node * obj,
                              ListenerType listenerType,
                              const Radiant::BinaryData * defaultData)
  {
    ValuePass vp;
    vp.m_listener = obj;
    vp.m_from = from;
    vp.m_to = to;
    vp.m_frame = m_frame;
    vp.m_type = listenerType;

    if(!m_eventSendNames.contains(from)) {
      warning("Node::eventAddListener # Adding listener to unexistent event '%s'", from.toUtf8().data());
    }

    if(!obj->m_eventListenNames.contains(to)) {
      const QString & klass = Radiant::StringUtils::demangle(typeid(*obj).name());
      warning("Node::eventAddListener # %s (%s %p) doesn't accept event '%s'",
              klass.toUtf8().data(), obj->name().toUtf8().data(), obj, to.toUtf8().data());
    }

    if(defaultData)
      vp.m_defaultData = *defaultData;

    if(std::find(m_elisteners.begin(), m_elisteners.end(), vp) !=
       m_elisteners.end())
      debugValuable("Widget::eventAddListener # Already got item %s -> %s (%p)",
            from.toUtf8().data(), to.toUtf8().data(), obj);
    else {
      m_elisteners.push_back(vp);
      obj->eventAddSource(this);
    }
  }

#ifdef MULTI_WITH_V8
  void Node::eventAddListener(const QString & from,
                              const QString & to,
                              v8::Persistent<v8::Function> func,
                              const Radiant::BinaryData * defaultData)
  {
    ValuePass vp;
    vp.m_funcv8 = func;
    vp.m_from = from;
    vp.m_to = to;

    if(!m_eventSendNames.contains(from)) {
      warning("Node::eventAddListener # Adding listener to unexistent event '%s'", from.toUtf8().data());
    }

    if(defaultData)
      vp.m_defaultData = *defaultData;

    if(std::find(m_elisteners.begin(), m_elisteners.end(), vp) !=
       m_elisteners.end())
      debug("Widget::eventAddListener # Already got item %s -> %s",
            from.toUtf8().data(), to.toUtf8().data());
    else {
      m_elisteners.push_back(vp);
    }
  }
#endif

  void Node::eventAddListener(const QString & from, ListenerFunc func,
                              ListenerType listenerType)
  {
    ValuePass vp;
    vp.m_func = func;
    vp.m_from = from;
    vp.m_type = listenerType;

    if(!m_eventSendNames.contains(from)) {
      warning("Node::eventAddListener # Adding listener to unexistent event '%s'", from.toUtf8().data());
    }

    // No duplicate check, since there is no way to compare std::function objects
    m_elisteners.push_back(vp);
  }

  void Node::eventAddListenerBd(const QString & from, ListenerFunc2 func,
                                ListenerType listenerType)
  {
    ValuePass vp;
    vp.m_func2 = func;
    vp.m_from = from;
    vp.m_type = listenerType;

    if(!m_eventSendNames.contains(from)) {
      warning("Node::eventAddListenerBd # Adding listener to unexistent event '%s'", from.toUtf8().data());
    }

    // No duplicate check, since there is no way to compare std::function objects
    m_elisteners.push_back(vp);
  }

  int Node::eventRemoveListener(const QString & from, const QString & to, Valuable::Node * obj)
  {
    int removed = 0;

    QSet<Node *> nodes;
    for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end(); it++) {

      // match obj if specified
      if((!obj || it->m_listener == obj) && it->m_valid) {
        // match from & to if specified
        if((from.isNull() || it->m_from == from) &&
           (to.isNull() || it->m_to == to)) {
          it->m_valid = false;
          nodes << it->m_listener;
          /* We cannot erase the list iterator, since that might invalidate iterators
             elsewhere. */
          removed++;
        }
      }
    }

    if(removed) {

      // Count number of references left to the objects
      QMap<Node *, size_t> count;
      for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end(); it++) {
        if(nodes.contains(it->m_listener) && it->m_valid)
          ++count[it->m_listener];
      }

      // If nothing references the object, remove the source
      foreach(Node * node, nodes)
        if(node && count.value(node) == 0)
          node->eventRemoveSource(this);
    }

    return removed;
  }

  void Node::eventAddSource(Valuable::Node * source)
  {
    m_eventSources.insert(source);
  }

  void Node::eventRemoveSource(Valuable::Node * source)
  {
    Sources::iterator it = m_eventSources.find(source);

    if(it != m_eventSources.end())
      m_eventSources.erase(it);
  }

  void Node::processMessage(const QString & id, Radiant::BinaryData & data)
  {
    // info("Node::processMessage # %s %s", typeid(*this).name(), id);

    int idx = id.indexOf('/');
    QString n = idx == -1 ? id : id.left(idx);

    // info("Node::processMessage # Child id = %s", key.c_str());

    Attribute * vo = getValue(n);

    if(vo) {
      // info("Node::processMessage # Sending message \"%s\" to %s",
      // id + skip, typeid(*vo).name());
      vo->processMessage(idx == -1 ? "" : id.mid(idx + 1), data);
    } else {
      if(!m_eventListenNames.contains(n)) {
        /*warning("Node::processMessage # %s (%s %p) doesn't accept event '%s'",
                  klass.c_str(), name().c_str(), this, id);*/
      } else {
        const QString klass = Radiant::StringUtils::demangle(typeid(*this).name());
        warning("Node::processMessage # %s (%s %p): unhandled event '%s'",
                klass.toUtf8().data(), name().toUtf8().data(), this, id.toUtf8().data());
      }
    }
  }

  // Must be outside function definition to be thread-safe
  static Radiant::Mutex s_generateIdMutex;

  Node::Uuid Node::generateId()
  {
    Radiant::Guard g(s_generateIdMutex);
    static Uuid s_id = static_cast<Uuid>(Radiant::TimeStamp::getTime());
    return s_id++;
  }

  Node::Uuid Node::id() const
  {
    return m_id;
  }

  void Node::eventAddOut(const QString & id)
  {
    if (m_eventSendNames.contains(id)) {
      warning("Node::eventAddSend # Trying to register event '%s' that is already registered", id.toUtf8().data());
    } else {
      m_eventSendNames.insert(id);
#ifdef MULTI_DOCUMENTER
      s_eventSendNames[Radiant::StringUtils::demangle(typeid(*this).name())].insert(id);
#endif
    }
  }

  void Node::eventAddIn(const QString & id)
  {
    if (m_eventListenNames.contains(id)) {
      warning("Node::eventAddListen # Trying to register duplicate event handler for event '%s'", id.toUtf8().data());
    } else {
      m_eventListenNames.insert(id);
#ifdef MULTI_DOCUMENTER
      s_eventListenNames[Radiant::StringUtils::demangle(typeid(*this).name())].insert(id);
#endif
    }
  }

  bool Node::acceptsEvent(const QString & id) const
  {
    return m_eventListenNames.contains(id);
  }

#ifdef MULTI_WITH_V8
  long Node::addListener(const QString & name, v8::Persistent<v8::Function> func, int role)
  {
    Attribute * attr = getValue(name);
    if(!attr) {
      warning("Node::addListener # Failed to find attribute %s", name.toUtf8().data());
      return -1;
    }
    return attr->addListener(func, role);
  }
#endif

  int Node::processQueue()
  {
    /// The queue must be locked during the whole time when calling the callback
    Radiant::Guard g(s_queueMutex);
    foreach(QueueItem* item, s_queue) {
      if(item->target) {
        std::swap(item->target->m_sender, item->sender);
        item->target->processMessage(item->to, item->data);
        std::swap(item->target->m_sender, item->sender);
      } else if(item->func) {
        item->func();
      } else if(item->func2) {
        item->func2(item->data);
      }
      // can't call "delete item" here, because that processMessage call could
      // call some destructors that iterate s_queue
    }
    foreach(QueueItem* item, s_queue)
      delete item;
    int r = s_queue.size();
    s_queue.clear();
    s_queueOnce.clear();
    return r;
  }

  bool Node::copyValues(const Node & from, Node & to)
  {
    XMLArchive archive;
    ArchiveElement e = Valuable::Serializer::serialize(archive, from);
    if(!e.isNull())
      return to.deserialize(e);
    return false;
  }

  void Node::eventSend(const QString & id, Radiant::BinaryData & bd)
  {
    if(!m_eventsEnabled)
      return;

    if(!m_eventSendNames.contains(id)) {
      error("Node::eventSend # Sending unknown event '%s'", id.toUtf8().data());
    }

    m_frame++;

    for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end();) {
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
          if(vp.m_type == AFTER_UPDATE_ONCE) {
            queueEvent(this, vp.m_listener, vp.m_to, bdsend, &vp);
          } else if(vp.m_type == AFTER_UPDATE) {
            queueEvent(this, vp.m_listener, vp.m_to, bdsend, 0);
          } else {
            // m_sender is valid only at the beginning of processMessage call
            Node * sender = this;
            std::swap(vp.m_listener->m_sender, sender);
            vp.m_listener->processMessage(vp.m_to, bdsend);
            vp.m_listener->m_sender = sender;
          }
#ifdef MULTI_WITH_V8
        } else if(!vp.m_funcv8.IsEmpty()) {
          /// @todo what is the correct receiver ("this" in the callback)?
          /// @todo should we set m_sender or something similar?
          /// @todo queueEvent?
          v8::HandleScope handle_scope;
          v8::Local<v8::Value> argv[10];
          argv[0] = v8::String::New(vp.m_to.utf16());
          int argc = 9;
          bdsend.readTo(argc, argv + 1);
          vp.m_funcv8->Call(v8::Context::GetCurrent()->Global(), argc+1, argv);
#endif
        } else if(vp.m_func) {
          if(vp.m_type == AFTER_UPDATE_ONCE) {
            queueEvent(this, vp.m_func, &vp);
          } else if(vp.m_type == AFTER_UPDATE) {
            queueEvent(this, vp.m_func, 0);
          } else {
            vp.m_func();
          }
        } else if(vp.m_func2) {
          if(vp.m_type == AFTER_UPDATE_ONCE) {
            queueEvent(this, vp.m_func2, bdsend, &vp);
          } else if(vp.m_type == AFTER_UPDATE) {
            queueEvent(this, vp.m_func2, bdsend, 0);
          } else {
            vp.m_func2(bdsend);
          }
        }
      }
      ++it;
    }
  }

  void Node::eventSend(const QString & id)
  {
    Radiant::BinaryData tmp;
    eventSend(id, tmp);
  }

  void Node::defineShortcut(const QString & name)
  {
    new Shortcut(this, name);
  }

  void Node::valueRenamed(const QString & was, const QString & now)
  {
    // Check that the value does not exist already
    iterator it = m_values.find(now);
    if(it != m_values.end()) {
      error("Node::valueRenamed # Value '%s' already exist", now.toUtf8().data());
      return;
    }

    it = m_values.find(was);
    if(it == m_values.end()) {
      error("Node::valueRenamed # No such value: %s", was.toUtf8().data());
      return;
    }

    Attribute * vo = (*it).second;
    m_values.erase(it);
    m_values[now] = vo;
  }

  bool Node::readElement(const ArchiveElement &)
  {
    return false;
  }

  // Template functions must be instantiated to be exported
  template VALUABLE_API bool Node::setValue<float>(const QString & name, const float &);
  template VALUABLE_API bool Node::setValue<Nimble::Vector2T<float> >(const QString & name, const Nimble::Vector2T<float> &);
  template VALUABLE_API bool Node::setValue<Nimble::Vector4T<float> >(const QString & name, const Nimble::Vector4T<float> &);


}
