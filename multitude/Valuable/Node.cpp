/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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
#include <memory>

#ifdef ENABLE_PUNCTUAL
#include <Punctual/TaskScheduler.hpp>
#endif

#include <algorithm>
#include <typeinfo>
#include <cstring>

namespace
{
  struct QueueItem
  {
    QueueItem(Valuable::Node * sender_, Valuable::Node * target_,
              Valuable::Node::ListenerFuncBd func_,
              const Radiant::BinaryData & data_)
      : sender(sender_)
      , func2(func_)
      , target(target_)
      , data(data_)
    {}

    QueueItem(Valuable::Node * sender_,  Valuable::Node * target_,
              Valuable::Node::ListenerFuncVoid func_)
      : sender(sender_)
      , func(func_)
      , func2()
      , target(target_)
    {}

    QueueItem(Valuable::Node * sender_, Valuable::Node * target_,
              const QByteArray & to_, const Radiant::BinaryData & data_)
      : sender(sender_)
      , func()
      , func2()
      , target(target_)
      , to(to_)
      , data(data_)
    {}

    /// @todo use WeakNodePtrT here so that we don't need to eliminate these manually
    Valuable::Node * sender;
    Valuable::Node::ListenerFuncVoid func;
    Valuable::Node::ListenerFuncBd func2;
    Valuable::Node * target;
    const QByteArray to;
    Radiant::BinaryData data;
  };

  // recursive because ~Node() might be called from processQueue()
  Radiant::Mutex s_queueMutex(true);
  std::list<std::unique_ptr<QueueItem> > s_queue;
  QSet<void *> s_queueOnce;

  Radiant::Mutex s_processingQueueMutex;
  bool s_processingQueue = false;
  bool s_processingQueueDisabled = false;
  std::list<std::unique_ptr<QueueItem> > s_queueTmp;
  QSet<void *> s_queueOnceTmp;

  void enableQueue()
  {
    Radiant::Guard g(s_processingQueueMutex);
    s_processingQueueDisabled = false;
  }

  void disableQueue()
  {
    std::list<std::unique_ptr<QueueItem> > queue;
    std::list<std::unique_ptr<QueueItem> > queueTmp;

    {
      Radiant::Guard g(s_processingQueueMutex);
      Radiant::Guard g2(s_queueMutex);

      s_processingQueueDisabled = true;

      s_queueOnce.clear();
      s_queueOnceTmp.clear();
      std::swap(queue, s_queue);
      std::swap(queueTmp, s_queueTmp);
    }
  }

  void queueEvent(std::unique_ptr<QueueItem> item, void * once)
  {
    s_processingQueueMutex.lock();

    if (s_processingQueueDisabled) {
      s_processingQueueMutex.unlock();
      return;
    }

    {
      if (s_processingQueue) {
        if (once) {
          if (s_queueOnceTmp.contains(once)) {
            s_processingQueueMutex.unlock();
            return;
          }
          s_queueOnceTmp << once;
        }
        s_queueTmp.push_back(std::move(item));
        s_processingQueueMutex.unlock();
        return;
      }
    }

    Radiant::Guard g(s_queueMutex);
    s_processingQueueMutex.unlock();

    if (once) {
      if (s_queueOnce.contains(once)) return;
      s_queueOnce << once;
    }
    s_queue.push_back(std::move(item));
  }

  void queueEvent(Valuable::Node * sender, Valuable::Node * target,
                  const QByteArray & to, const Radiant::BinaryData & data,
                  void * once)
  {
    queueEvent(std::unique_ptr<QueueItem>(new QueueItem(sender, target, to, data)), once);
  }

  void queueEvent(Valuable::Node * sender, Valuable::Node * target,
                  Valuable::Node::ListenerFuncVoid func, void * once)
  {
    queueEvent(std::unique_ptr<QueueItem>(new QueueItem(sender, target, func)), once);
  }

  void queueEvent(Valuable::Node * sender, Valuable::Node * target,
                  Valuable::Node::ListenerFuncBd func,
                  const Radiant::BinaryData & data, void * once)
  {
    queueEvent(std::unique_ptr<QueueItem>(new QueueItem(sender, target, func, data)), once);
  }
}

namespace Valuable
{
#ifdef MULTI_DOCUMENTER
  VALUABLE_API std::map<QString, std::set<QString> > s_eventSendNames;
  VALUABLE_API std::map<QString, std::set<QString> > s_eventListenNames;
#endif

  inline bool Node::ValuePass::operator == (const ValuePass & that) const
  {
    return (m_listener == that.m_listener) && (m_from == that.m_from) &&
           (m_to == that.m_to);
  }

  Node::Node()
      : Attribute(),
      m_sender(nullptr),
      m_eventsEnabled(true),
      m_id(nullptr, "id", generateId()),
      m_listenersId(0)
  {
    eventAddOut("attribute-added");
    eventAddOut("attribute-removed");
    addAttribute("id", &m_id);
  }

  Node::Node(Node * host, const QByteArray & name)
      : Attribute(host, name),
      m_sender(nullptr),
      m_eventsEnabled(true),
      m_id(nullptr, "id", generateId()),
      m_listenersId(0)
  {
    eventAddOut("attribute-added");
    eventAddOut("attribute-removed");
    addAttribute("id", &m_id);
  }

  Node::~Node()
  {
    REQUIRE_THREAD(m_ownerThread);

    // Host of Node class member Attributes must be zeroed to avoid double-delete
    m_id.removeHost();

    // Removes listeners and WeakNodePtrs that point to this object
    setBeingDestroyed();

    // Remove event listeners. No need to lock m_eventsMutex, since you
    // shouldn't be calling any event functions to this object anyway from
    // other threads since we are being deleted.
    for (auto listener: m_elisteners) {
      if (listener.m_listener) {
        listener.m_listener->eventRemoveSource(this);
      }
    }
    m_elisteners.clear();

    // Release memory for any attributes that are left (should be only
    // heap-allocated at this point)
    while(!m_attributes.empty())
      delete m_attributes.begin()->second;
  }

  Node::Node(Node && node)
    : Attribute(std::move(*this))
    , m_sender(std::move(node.m_sender))
    , m_attributes(std::move(node.m_attributes))
    , m_elisteners(std::move(node.m_elisteners))
    , m_eventSources(std::move(node.m_eventSources))
    , m_eventsEnabled(std::move(node.m_eventsEnabled))
    , m_attributeListening(std::move(node.m_attributeListening))
    , m_id(nullptr, "id", node.m_id)
    , m_listenersId((long)node.m_listenersId)
    , m_eventSendNames(std::move(node.m_eventSendNames))
    , m_eventListenNames(std::move(node.m_eventListenNames))
  {
    node.m_id.m_host = nullptr;
    m_id.m_host = this;
    m_attributes[m_id.name()] = &m_id;
  }

  Node & Node::operator=(Node && node)
  {
    Attribute::operator=(std::move(*this));
    m_sender = std::move(node.m_sender);
    m_attributes = std::move(node.m_attributes);
    m_elisteners = std::move(node.m_elisteners);
    m_eventSources = std::move(node.m_eventSources);
    m_eventsEnabled = std::move(node.m_eventsEnabled);
    m_attributeListening = std::move(node.m_attributeListening);
    m_listenersId = (long)node.m_listenersId;
    m_eventSendNames = std::move(node.m_eventSendNames);
    m_eventListenNames = std::move(node.m_eventListenNames);

    node.m_id.m_host = nullptr;
    m_id = node.m_id.value();
    m_id.setAsDefaults();
    m_id.m_host = nullptr;
    m_id.setName("id");
    m_id.m_host = this;
    m_attributes[m_id.name()] = &m_id;
    return *this;
  }

  bool Node::hasAncestor(const Node &ancestorCandidate) const
  {
    const Node * test = host();
    while(test) {
      if(test == &ancestorCandidate) return true;
      test = test->host();
    }
    return false;
  }

  void Node::merge(Node && node)
  {
    auto & src = node.m_attributes.vector();
    auto & target = m_attributes.vector();
    target.reserve(target.size() + src.size() - 1);
    for (auto & p: src) {
      if (p.second != &node.m_id) {
        p.second->m_host = this;
        target.push_back(std::move(p));
      }
    }
    node.m_attributes.clear();
    node.m_id.m_host = nullptr;

    for (auto & l: node.m_elisteners) {
      m_elisteners.emplace_back(std::move(l));
    }
    node.m_elisteners.clear();

    for (auto & p: node.m_eventSources)
      m_eventSources[p.first] += p.second;

    m_attributeListening |= std::move(node.m_attributeListening);
    m_listenersId += node.m_listenersId;
    m_eventSendNames |= std::move(node.m_eventSendNames);
    m_eventListenNames |= std::move(node.m_eventListenNames);
  }

  Attribute * Node::attribute(const QByteArray & name) const
  {
    REQUIRE_THREAD(m_ownerThread);
    const int slashIndex = name.indexOf('/');

    if(slashIndex == -1) {
      return m_attributes.value(name);
    }
    else {
      const QByteArray part1 = name.left(slashIndex);
      const QByteArray part2 = name.mid(slashIndex + 1);

      Attribute * attr;
      if(part1 == "..") {
        attr = host();
      } else {
        attr = attribute(part1);
      }

      if(attr) {
        return attr->attribute(part2);
      }
    }

    return nullptr;
  }

  bool Node::addAttribute(Attribute * const attribute)
  {
    return Node::addAttribute(attribute->name(), attribute);
  }

  bool Node::addAttribute(const QByteArray & cname, Attribute * const attribute)
  {
    REQUIRE_THREAD(m_ownerThread);
    // Check attributes
    if(m_attributes.find(cname) != m_attributes.end()) {
      Radiant::error(
          "Node::addAttribute # can not add attribute '%s' as '%s' "
          "already has an attribute with the same name.",
          cname.data(), m_name.data());
      return false;
    }

    // Unlink host if necessary
    Node * host = attribute->host();
    if(host) {
      /* Do not call removeHost, since that would call emitHostChange. We call
         emiHostChange anyhow. */
      host->removeAttribute(attribute, false);
    }

    // Change the attribute name
    attribute->setName(cname);

    /// m_attributes[cname] = attribute; would iterate m_attributes again,
    /// optimize the insertion by adding the new attribute directly to the
    /// implementation vector. This is safe, since we just checked that it
    /// doesn't already exist.
    m_attributes.vector().emplace_back(std::make_pair(cname, attribute));
#ifdef ENABLE_THREAD_CHECKS
    attribute->setOwnerThread(m_ownerThread);
#endif
    attribute->m_host  = this;
    eventSend("attribute-added", cname);
    attributeAdded(attribute);

    attribute->emitHostChange();

    return true;
  }

  void Node::removeAttribute(Attribute * const attribute, bool emitChange)
  {
    for (auto it = m_attributes.begin(), end = m_attributes.end(); it != end; ++it) {
      if (it->second == attribute) {
        m_attributes.erase(it);
#ifdef ENABLE_THREAD_CHECKS
        attribute->setOwnerThread(nullptr);
#endif
        attribute->m_host = nullptr;
        eventSend("attribute-removed", attribute->name());
        attributeRemoved(attribute);
        if(emitChange)
          attribute->emitHostChange();
        return;
      }
    }

    Radiant::error("Node::removeAttribute # '%s' is not a child attribute of '%s'.",
                   attribute->name().data(), m_name.data());
  }

  static Valuable::Node *findDescendantNodeInternal(Valuable::Node &n, const QByteArray& name,
                                                    std::set<Valuable::Node::Uuid>& visited)
  {
    if(visited.find(n.id()) != visited.end())
      return nullptr;
    else if (n.name() == name)
      return dynamic_cast<Valuable::Node*>(&n);
    else {
      visited.insert(n.id());
      const Valuable::Node::container& attributes = n.attributes();
      for(const auto& p : attributes) {
        auto node = dynamic_cast<Valuable::Node*>(p.second);
        if(!node)
          continue;
        auto res = findDescendantNodeInternal(*node, name, visited);
        if(res) return res;
      }
      return nullptr;
    }
  }

  Node *Node::findDescendantNode(const QByteArray& name)
  {
    std::set<Valuable::Node::Uuid> tmp;
    return findDescendantNodeInternal(*this, name, tmp);
  }

  bool Node::saveToFileXML(const QString & filename, unsigned int opts) const
  {
    bool ok = Serializer::serializeXML(filename, this, opts);
    if (!ok) {
      Radiant::error("Node::saveToFileXML # object failed to serialize (%s)", filename.toUtf8().data());
    }
    return ok;
  }

  bool Node::saveToMemoryXML(QByteArray & buffer, unsigned int opts) const
  {
    XMLArchive archive(opts);
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

  bool Node::loadFromMemoryXML(const QByteArray & buffer)
  {
    XMLArchive archive;

    if(!archive.readFromMem(buffer))
      return false;

    return deserialize(archive.root());
  }

  ArchiveElement Node::serialize(Archive & archive) const
  {
    REQUIRE_THREAD(m_ownerThread);
    QString name = m_name.isEmpty() ? "Node" : m_name;

    ArchiveElement elem = archive.createElement(name.toUtf8().data());
    if(elem.isNull()) {
      Radiant::error(
          "Node::serialize # failed to create element");
      return ArchiveElement();
    }

    elem.add("type", type());

    for(container::const_iterator it = m_attributes.begin(); it != m_attributes.end(); ++it) {
      Attribute * vo = it->second;

      /// @todo need to add new flag to Archive that controls how Attribute::isSerializable works
      if (!vo->isSerializable())
        continue;

      ArchiveElement child = vo->serialize(archive);
      if (!child.isNull())
        elem.add(child);
    }

    return elem;
  }

  bool Node::deserialize(const ArchiveElement & element)
  {
    REQUIRE_THREAD(m_ownerThread);
    // Name
    m_name = element.name().toUtf8();

    bool allChildrenOk = true;
    // Children
    for(ArchiveElement::Iterator it = element.children(); it; ++it) {
      ArchiveElement elem = *it;

      QByteArray name = elem.name().toUtf8();

      Attribute * a = attribute(name);

      // If the attribute exists, just deserialize it. Otherwise, pass the element
      // to readElement()
      if(a) {
        if(!a->deserialize(elem)) {
          Radiant::error("Node::deserialize # (%s) deserialize failed for element '%s'",
                         typeid(*this).name(), name.data());
          allChildrenOk = false;
        }
      }
      else {
        if(!readElement(elem)) {
          Radiant::error("Node::deserialize # (%s) readElement failed for element '%s'",
                         typeid(*this).name(), name.data());
          allChildrenOk = false;
        }
      }
    }

    return allChildrenOk;
  }

  void Node::debugDump() {
    Radiant::debug("%s {", m_name.data());

    for(container::iterator it = m_attributes.begin(); it != m_attributes.end(); ++it) {
      Attribute * vo = it->second;

      Node * hv = dynamic_cast<Node *> (vo);
      if(hv) hv->debugDump();
      else {
        QString s = vo->asString();
        Radiant::debug("\t%s = %s", vo->name().data(), s.toUtf8().data());
      }
    }

    Radiant::debug("}");
  }

  long Node::eventAddListener(const QByteArray & fromIn,
                              const QByteArray & to,
                              Valuable::Node * obj,
                              ListenerType listenerType,
                              const Radiant::BinaryData * defaultData)
  {
    if (isBeingDestroyed())
      return -1;

    const QByteArray from = validateEvent(fromIn);

    ValuePass vp(++m_listenersId);
    vp.m_listener = obj;
    vp.m_from = from;
    vp.m_to = to;
    vp.m_type = listenerType;

    if(!obj->m_eventListenNames.contains(to)) {
      if(!obj->attribute(to)) {
        /* If the to attribute is not a known listener, or an attribute we output a warning.
          */
        /** @todo We could still check that the "to" is not a hierarchical path, for example
           "widget1/color".
        */

        const QByteArray & klass = Radiant::StringUtils::demangle(typeid(*obj).name());

#ifdef RADIANT_DEBUG
          Radiant::fatal("Node::eventAddListener # %s (%s %p) doesn't accept event '%s'",
                           klass.data(), obj->name().data(), obj, to.data());
#else
          Radiant::warning("Node::eventAddListener # %s (%s %p) doesn't accept event '%s'",
                           klass.data(), obj->name().data(), obj, to.data());
#endif
      }
    }

    if(defaultData)
      vp.m_defaultData = *defaultData;

    {
      Radiant::Guard g(m_eventsMutex);
      if (std::find(m_elisteners.begin(), m_elisteners.end(), vp) !=
         m_elisteners.end()) {
        debugValuable("Widget::eventAddListener # Already got item %s -> %s (%p)",
                      from.data(), to.data(), obj);
      } else {
        m_elisteners.push_back(vp);
        obj->eventAddSource(this);
      }
    }
    return vp.m_listenerId;
  }

  long Node::eventAddListener(const QByteArray & fromIn, ListenerFuncVoid func,
                              ListenerType listenerType)
  {
    if (isBeingDestroyed())
      return -1;

    const QByteArray from = validateEvent(fromIn);

    ValuePass vp(++m_listenersId);
    vp.m_func = func;
    vp.m_from = from;
    vp.m_type = listenerType;

    {
      Radiant::Guard g(m_eventsMutex);
      // No duplicate check, since there is no way to compare std::function objects
      m_elisteners.push_back(vp);
    }
    return vp.m_listenerId;
  }

  long Node::eventAddListener(const QByteArray&eventId, Node*dstNode, Node::ListenerFuncVoid func, Node::ListenerType listenerType)
  {
    if (isBeingDestroyed())
      return -1;

    const QByteArray from = validateEvent(eventId);

    ValuePass vp(++m_listenersId);
    vp.m_func = func;
    vp.m_from = from;
    vp.m_type = listenerType;
    vp.m_listener = dstNode;

    if(dstNode)
      dstNode->eventAddSource(this);

    {
      Radiant::Guard g(m_eventsMutex);
      m_elisteners.push_back(vp);
    }
    return vp.m_listenerId;
  }

  long Node::eventAddListenerBd(const QByteArray&eventId, Node*dstNode, Node::ListenerFuncBd func, Node::ListenerType listenerType)
  {
    if (isBeingDestroyed())
      return -1;

    const QByteArray from = validateEvent(eventId);

    ValuePass vp(++m_listenersId);
    vp.m_func2 = func;
    vp.m_from = from;
    vp.m_type = listenerType;
    vp.m_listener = dstNode;

    if(dstNode)
      dstNode->eventAddSource(this);

    {
      Radiant::Guard g(m_eventsMutex);
      m_elisteners.push_back(vp);
    }
    return vp.m_listenerId;
  }

  long Node::eventAddListenerBd(const QByteArray & fromIn, ListenerFuncBd func,
                                ListenerType listenerType)
  {
    if (isBeingDestroyed())
      return -1;

    const QByteArray from = validateEvent(fromIn);

    ValuePass vp(++m_listenersId);
    vp.m_func2 = func;
    vp.m_from = from;
    vp.m_type = listenerType;

    {
      Radiant::Guard g(m_eventsMutex);
      // No duplicate check, since there is no way to compare std::function objects
      m_elisteners.push_back(vp);
    }
    return vp.m_listenerId;
  }

  int Node::eventRemoveListener(const QByteArray & from, const QByteArray & to, Valuable::Node * obj)
  {
    Radiant::Guard g(m_eventsMutex);
    int removed = 0;

    for(Listeners::iterator it = m_elisteners.begin(); it != m_elisteners.end(); ) {

      // match obj if specified
      if(!obj || it->m_listener == obj) {
        // match from & to if specified
        if((from.isNull() || it->m_from == from) &&
           (to.isNull() || it->m_to == to)) {

          if (it->m_listener)
            it->m_listener->eventRemoveSource(this);

          it = m_elisteners.erase(it);
          removed++;
          continue;
        }
      }
      ++it;
    }

    return removed;
  }

  bool Node::eventRemoveListener(long listenerId)
  {
    Radiant::Guard g(m_eventsMutex);
    for (auto it = m_elisteners.begin(); it != m_elisteners.end(); ++it) {
      if (it->m_listenerId == listenerId) {
        if (it->m_listener)
          it->m_listener->eventRemoveSource(this);
        it = m_elisteners.erase(it);
        return true;
      }
    }
    return false;
  }

  void Node::setBeingDestroyed()
  {
    if (!m_isBeingDestroyed) {
      m_self.reset();
      internalRemoveListeners();
      m_isBeingDestroyed = true;
    }
  }

  void Node::attributeAdded(Attribute *)
  {
  }

  void Node::attributeRemoved(Attribute *)
  {
  }

  void Node::eventAddSource(Valuable::Node * source)
  {
    // Do not keep references to self, they are not needed, since the event
    // listeners are deleted anyway in internalRemoveListeners. This also
    // removes deadlock when trying to lock m_eventsMutex twice.
    if (source == this)
      return;

    if (isBeingDestroyed())
      return;

    Radiant::Guard g(m_eventsMutex);
    ++m_eventSources[source];
  }

  void Node::eventRemoveSource(Valuable::Node * source)
  {
    if (source == this)
      return;

    Radiant::Guard g(m_eventsMutex);
    Sources::iterator it = m_eventSources.find(source);

    if (it != m_eventSources.end() && --it->second <= 0)
      m_eventSources.erase(it);
  }

  void Node::eventProcess(const QByteArray & id, Radiant::BinaryData & data)
  {
    // Radiant::info("Node::eventProcess # %s %s", typeid(*this).name(), id);

    int idx = id.indexOf('/');
    QByteArray n = idx == -1 ? id : id.left(idx);

    // Radiant::info("Node::eventProcess # Child id = %s", key.c_str());

    Attribute * vo = attribute(n);

    if(vo) {
      // Radiant::info("Node::eventProcess # Sending message \"%s\" to %s",
      // id + skip, typeid(*vo).name());
      vo->eventProcess(idx == -1 ? "" : id.mid(idx + 1), data);
    } else {
      if(!m_eventListenNames.contains(n)) {
        /*warning("Node::eventProcess # %s (%s %p) doesn't accept event '%s'",
                  klass.c_str(), name().c_str(), this, id);*/
      } else {
        const QByteArray klass = Radiant::StringUtils::demangle(typeid(*this).name());
        Radiant::warning("Node::eventProcess # %s (%s %p): unhandled event '%s'",
                klass.data(), name().data(), this, id.data());
      }
    }
  }

  // Must be outside function definition to be thread-safe
  static Radiant::Mutex s_generateIdMutex;

  Node::Uuid Node::generateId()
  {
    Radiant::Guard g(s_generateIdMutex);
    static Uuid s_id = static_cast<Uuid>(Radiant::TimeStamp::currentTime().value());
    return s_id++;
  }

  Node::Uuid Node::id() const
  {
    return m_id;
  }

  void Node::setId(Node::Uuid newId)
  {
    m_id = newId;
  }

  void Node::eventAddOut(const QByteArray & id)
  {
    if (m_eventSendNames.contains(id)) {
      Radiant::warning("Node::eventAddOut # Trying to register event '%s' that is already registered", id.data());
    } else {
      m_eventSendNames.insert(id);
#ifdef MULTI_DOCUMENTER
      s_eventSendNames[Radiant::StringUtils::demangle(typeid(*this).name())].insert(id);
#endif
    }
  }

  void Node::eventAddIn(const QByteArray &id)
  {
    if (m_eventListenNames.contains(id)) {
      Radiant::warning("Node::eventAddIn # Trying to register duplicate event handler for event '%s'", id.data());
    } else {
      m_eventListenNames.insert(id);
#ifdef MULTI_DOCUMENTER
      s_eventListenNames[Radiant::StringUtils::demangle(typeid(*this).name())].insert(id);
#endif
    }
  }

  void Node::eventRemoveOut(const QByteArray & eventId)
  {
    if (m_eventSendNames.contains(eventId)) {
      m_eventSendNames.remove(eventId);
    } else {
      Radiant::warning("Node::eventRemoveOut # Couldn't find event '%s'", eventId.data());
    }

  }

  void Node::eventRemoveIn(const QByteArray & messageId)
  {
    if (m_eventListenNames.contains(messageId)) {
      m_eventListenNames.remove(messageId);
    } else {
      Radiant::warning("Node::eventRemoveIn # Couldn't find event '%s'", messageId.data());
    }
  }

  bool Node::acceptsEvent(const QByteArray & id) const
  {
    return m_eventListenNames.contains(id);
  }

  int Node::processQueue()
  {
#ifdef ENABLE_PUNCTUAL
    /// Which should be first?
    auto exec = Punctual::TaskScheduler::instance()->afterUpdate();
    /// @todo should we have some 'max' parameter for manual executor?
    exec->run();
#endif

    {
      Radiant::Guard g(s_processingQueueMutex);
      s_processingQueue = true;
    }

    /// The queue must be locked during the whole time when calling the callback
    Radiant::Guard g(s_queueMutex);

    // Can not use range-based loop here because it doesn't iterate all
    // elements when the QList gets modified inside the loop.
    for(auto i = s_queue.begin(); i != s_queue.end(); ++i) {
      auto & item = *i;
      if(item->func) {
        item->func();
      } else if(item->func2) {
        item->func2(item->data);
      } else if(item->target) {
        std::swap(item->target->m_sender, item->sender);
        item->target->eventProcess(item->to, item->data);
        std::swap(item->target->m_sender, item->sender);
      }
      // can't call "delete item" here, because that eventProcess call could
      // call some destructors that iterate s_queue
    }

    int r = static_cast<int>(s_queue.size());

    {
      // Make a temporary copy to prevent weird callback recursion bugs
      auto tempQueue = std::move(s_queue);
      s_queue.clear();
    }

    // Since we are locking two mutexes at the same time also in queueEvent,
    // it's important that the lock order is right. Always lock
    // s_processingQueueMutex before s_queueMutex. That is why we need to
    // release the lock, otherwise we will get deadlock if queueEvent has
    // already locked s_processingQueueMutex and is waiting for s_queueMutex.
    // Also remember to clear s_queue, otherwise ~Node() could be reading old
    // deleted values from it
    s_queueMutex.unlock();
    Radiant::Guard g2(s_processingQueueMutex);
    s_queueMutex.lock();

    s_queue = std::move(s_queueTmp);
    s_queueOnce = s_queueOnceTmp;
    s_queueTmp.clear();
    s_queueOnceTmp.clear();
    s_processingQueue = false;
    return r;
  }

  void Node::disableQueue()
  {
    ::disableQueue();
  }

  void Node::reEnableQueue()
  {
    enableQueue();
  }

  bool Node::copyValues(const Node & from, Node & to)
  {
    XMLArchive archive;
    ArchiveElement e = Valuable::Serializer::serialize(archive, from);
    Uuid toId = to.id();

    if(!e.isNull()) {
      bool ok = to.deserialize(e);
      to.m_id = toId;
      return ok;
    }

    return false;
  }

  void Node::invokeAfterUpdate(Node::ListenerFuncVoid function)
  {
    queueEvent(nullptr, nullptr, function, nullptr);
  }

  void Node::eventSend(const QByteArray & id, Radiant::BinaryData & bd)
  {
    if(!m_eventsEnabled)
      return;

    if(!m_eventSendNames.contains(id)) {
      Radiant::error("Node::eventSend # Sending unknown event '%s'", id.data());
    }

    Listeners listenerCopy;
    {
      Radiant::Guard g(m_eventsMutex);
      listenerCopy = m_elisteners;
    }

    for(Listeners::iterator it = listenerCopy.begin(); it != listenerCopy.end();) {
      ValuePass & vp = *it;

      if(vp.m_from == id) {

        Radiant::BinaryData & bdsend = vp.m_defaultData.total() ? vp.m_defaultData : bd;

        bdsend.rewind();

        if(vp.m_func) {
          if(vp.m_type == AFTER_UPDATE_ONCE) {
            queueEvent(this, vp.m_listener, vp.m_func, &vp);
          } else if(vp.m_type == AFTER_UPDATE) {
            queueEvent(this, vp.m_listener, vp.m_func, 0);
          } else {
            vp.m_func();
          }
        } else if(vp.m_func2) {
          if(vp.m_type == AFTER_UPDATE_ONCE) {
            queueEvent(this, vp.m_listener, vp.m_func2, bdsend, &vp);
          } else if(vp.m_type == AFTER_UPDATE) {
            queueEvent(this, vp.m_listener, vp.m_func2, bdsend, 0);
          } else {
            vp.m_func2(bdsend);
          }
        } else if(vp.m_listener) {
          if(vp.m_type == AFTER_UPDATE_ONCE) {
            queueEvent(this, vp.m_listener, vp.m_to, bdsend, &vp);
          } else if(vp.m_type == AFTER_UPDATE) {
            queueEvent(this, vp.m_listener, vp.m_to, bdsend, 0);
          } else {
            // m_sender is valid only at the beginning of eventProcess call
            Node * sender = this;
            std::swap(vp.m_listener->m_sender, sender);
            vp.m_listener->eventProcess(vp.m_to, bdsend);
            vp.m_listener->m_sender = sender;
          }
        }
      }
      ++it;
    }
  }

  void Node::eventSend(const QByteArray & id)
  {
    Radiant::BinaryData tmp;
    eventSend(id, tmp);
  }

#ifdef ENABLE_THREAD_CHECKS
  void Node::setOwnerThread(Radiant::Thread::id_t owner)
  {
    m_ownerThread = owner;
    for (auto attr: m_attributes) {
      attr.second->setOwnerThread(owner);
    }
  }
#endif

  void Node::attributeRenamed(const QByteArray & was, const QByteArray & now)
  {
    REQUIRE_THREAD(m_ownerThread);
    // Check that the attribute does not exist already
    iterator it = m_attributes.find(now);
    if(it != m_attributes.end()) {
      Radiant::error("Node::attributeRenamed # Attribute '%s' already exist", now.data());
      return;
    }

    it = m_attributes.find(was);
    if(it == m_attributes.end()) {
      Radiant::error("Node::attributeRenamed # No such attribute: %s", was.data());
      return;
    }

    Attribute * vo = (*it).second;
    m_attributes.erase(it);
    m_attributes[now] = vo;
  }

  const std::shared_ptr<Node> & Node::sharedPtr()
  {
    if (!m_self && !m_isBeingDestroyed)
      m_self.reset(this, [] (Node*) {});
    return m_self;
  }

  bool Node::readElement(const ArchiveElement &)
  {
    return false;
  }

  void Node::clearValues(Layer layer)
  {
    for(auto i = m_attributes.begin(); i != m_attributes.end(); ++i)
      i->second->clearValue(layer);
  }

  void Node::setAsDefaults()
  {
    for(auto i = m_attributes.begin(); i != m_attributes.end(); ++i)
      i->second->setAsDefaults();
  }

  bool Node::isChanged() const
  {
    for (auto i = m_attributes.begin(); i != m_attributes.end(); ++i)
      if (i->second->isChanged())
        return true;
    return false;
  }

  void Node::eventAddDeprecated(const QByteArray &deprecatedId, const QByteArray &newId)
  {
    m_deprecatedEventCompatibility[deprecatedId] = newId;
  }

  QByteArray Node::validateEvent(const QByteArray &from)
  {
    // Issue warning if the original requested event is not registered
    if(!m_eventSendNames.contains(from)) {

      // Check for event conversions
      if(m_deprecatedEventCompatibility.contains(from)) {

        const QByteArray & converted = m_deprecatedEventCompatibility[from];
        Radiant::warning("The event '%s' is deprecated. Use '%s' instead.", from.data(), converted.data());

        return converted;
      }


#ifdef RADIANT_DEBUG
        Radiant::fatal("Node::validateEvent # event '%s' does not exist for this class", from.data());
#else
        Radiant::warning("Node::validateEvent # event '%s' does not exist for this class (%s)",
                         from.data(), Radiant::StringUtils::type(*this).data());
#endif
    }

    return from;
  }

  void Node::internalRemoveListeners()
  {
    Sources sources;
    {
      Radiant::Guard g(m_eventsMutex);
      sources = m_eventSources;
    }
    for (auto p: sources) {
      // The eventRemoveListener call will also clear the relevant part from
      // m_eventSources. Can't hold m_eventsMutex while doing this, otherwise
      // we will have deadlock in eventRemoveSource.
      p.first->eventRemoveListener(this);
    }

    Q_FOREACH(Attribute* vo, m_attributeListening) {
      for(QMap<long, AttributeListener>::iterator it = vo->m_listeners.begin(); it != vo->m_listeners.end(); ) {
        if(it->listener == this) {
          it = vo->m_listeners.erase(it);
        } else ++it;
      }
    }

    {
      Radiant::Guard g(s_queueMutex);
      for(auto it = s_queue.begin(); it != s_queue.end(); ++it) {
        auto & item = *it;
        if(item->target == this) {
          item->target = nullptr;
          item->func = ListenerFuncVoid();
          item->func2 = ListenerFuncBd();
        }
        if(item->sender == this)
          item->sender = nullptr;
      }
      for(auto it = s_queueTmp.begin(); it != s_queueTmp.end(); ++it) {
        auto & item = *it;
        if(item->target == this) {
          item->target = nullptr;
          item->func = ListenerFuncVoid();
          item->func2 = ListenerFuncBd();
        }
        if(item->sender == this)
          item->sender = nullptr;
      }
    }
  }

}
