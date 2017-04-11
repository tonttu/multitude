/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_NODE_HPP
#define VALUABLE_NODE_HPP

#include "Export.hpp"
#include "AttributeInt.hpp"
#include "Attribute.hpp"

#include <Patterns/NotCopyable.hpp>

#include <Radiant/Color.hpp>
#include <Radiant/IntrusivePtr.hpp>
#include <Radiant/ThreadChecks.hpp>
#include <Radiant/Trace.hpp>

#ifdef CORNERSTONE_JS
#include <Valuable/v8.hpp>
#endif

#include <map>
#include <set>
#include <atomic>

#include <QString>
#include <QSet>

namespace Valuable
{
  /** Base class for objects that include member variables with automatic IO.

      This base class has a list of #Valuable::Attribute child objects (aka
      member variables) that are named with unique string.

      Deleting the child objects is the responsibility of the inherited
      classes, Node simply maintains a list of children.
  */
  /// @todo Examples
  class VALUABLE_API Node : public Attribute, public Patterns::NotCopyable
  {
  public:
    /// Unique identifier type
    typedef int64_t Uuid;

    /// Default listener function type
    typedef std::function<void ()> ListenerFuncVoid;
    /// Listener function type that takes a BinaryData reference as a parameter,
    /// similar to @ref eventProcess
    typedef std::function<void (Radiant::BinaryData &)> ListenerFuncBd;

    /// Types of event listeners
    enum ListenerType
    {
      /// Listener will activate immediately when an event is sent. Listener is
      /// executed in the thread the event is sent.
      DIRECT=1,

      /// Listener activation is delayed to happen after update. Listener is
      /// executed in the main thread.
      AFTER_UPDATE=2,
      /// Listener activation is delayed to happen after update. Duplicate
      /// events are merged, so the listener activates only once even if multiple
      /// events were sent. Listener is executed in the main thread.
      AFTER_UPDATE_ONCE=4
    };

    Node();
    /** Constructs a new Node and adds it under the given host.
      This method is "explicit", so that you cannot cast from Node pointer
      to Node object accidentally.
      @param host Host of this node. Parent in node-hierarchy
      @param name Name of this object.
    */
    explicit Node(Node * host, const QByteArray &name = "");
    virtual ~Node();

    /// @cond

    /// @todo document
    void merge(Node && node);

    /// @endcond

    /// Moves a node, including all its attributes, events etc
    /// @param node node to move
    Node(Node && node);
    /// Moves a node, replacing this
    /// @param node node to move
    /// @returns reference to this
    Node & operator=(Node && node);

    /// Checks if the node is a descendant from a given ancestor.
    /// Node descends from another node if it is a direct child of the
    /// another node or if there is a direct parent child path between them.
    /// @param ancestorCandidate The potential ancestor of this ndoe.
    bool hasAncestor(const Node & ancestorCandidate) const;

    /// Adds a new Attribute to the list of attribute objects.
    /// Copies the name of the attribute from the given object.
    /// After successful adding of attribute to node, the node handle memory
    /// management of attribute.
    /// @param attribute Attribute to be added
    /// @return True if attribute was successfully added, false otherwise
    bool addAttribute(Attribute * const attribute);

    /// Adds a new Attribute to the list of attribute objects.
    bool addAttribute(const QByteArray &name, Attribute * const attribute);

    /// @copydoc addAttribute
    template<typename Widget>
    bool addAttribute(const QByteArray &name, const Radiant::IntrusivePtr<Widget>& attribute)
    {
      return addAttribute(name, &*attribute);
    }

    /// Gets an Attribute with the given name
    /// @param name Attribute name to search for
    /// @return Null if no object can be found
    virtual Attribute * attribute(const QByteArray & name) const OVERRIDE;

    /// @copydoc attribute
    /// @return Null if no object can be found or the type is wrong
    template <typename T>
    AttributeT<T> * attribute(const QByteArray & name) const
    {
      return dynamic_cast<AttributeT<T> *>(attribute(name));
    }

    /// Removes an Attribute from the list of attribute objects.
    void removeAttribute(Attribute * const attribute, bool emitChange = true);

    /// Clears all Attribute values of the given layer
    /// @param layer layer to clear
    void clearValues(Layer layer);

    /// Uses a query string to find a Attribute, and sets a new value to that if found.
    /// @param name The path to the Attribute. This is a '/'-separated list
    ///        of Attribute names, forming a path inside a Attribute tree.
    ///        ".." can be used to refer to host element. For example
    ///        setValue("../foo/bar", 4.0f) sets 4.0f to Attribute named "bar"
    ///        under Attribute "foo" that is sibling of this object.
    /// @param v The new value
    /// @return True if object was found and the value was set successfully.
    /// @todo implement similar to getValue (to avoid dynamic_cast)
    template<class T>
    bool setValue(const QByteArray & name, const T & v)
    {
      REQUIRE_THREAD(m_ownerThread);
      int cut = name.indexOf("/");
      QByteArray next, rest;
      if(cut > 0) {
        next = name.left(cut);
        rest = name.mid(cut + 1);

        if(next == QByteArray("..")) {
          if(!m_host) {
            Radiant::error(
                "Node::setValue # node '%s' has no host", m_name.data());
            return false;
          }

          return m_host->setValue(rest, v);
        }
      } else {
        next = name;
      }

      container::iterator it = m_attributes.find(next);
      if(it == m_attributes.end()) {
        Radiant::error(
            "Node::setValue # property '%s' not found", next.data());
        return false;
      }

      if(cut > 0) {
        Node * hv = dynamic_cast<Node *> (it->second);
        if(hv) return hv->setValue(rest, v);
      }

      Attribute * vo = it->second;
      return vo->set(v);
    }

#ifdef CORNERSTONE_JS
    /// Set attribute value from JavaScript
    /// @param name attribute name
    /// @param v attribute value. Supported types: boolean, number, string,
    ///          list of two, three or four numbers
    /// @returns true if conversion from JavaScript was successful, and attribute value was set
    bool setValue(const QByteArray & name, v8::Handle<v8::Value> v);

    /// @cond

    inline bool setValue(const QByteArray & name, const v8::Local<v8::Value> & v) {
      return setValue(name, v8::Handle<v8::Value>(v));
    }

    /// @endcond

#endif

    /// Saves this object (and its children) to an XML file
    bool saveToFileXML(const QString & filename, unsigned int opts = SerializationOptions::DEFAULTS) const;
    /// Saves this object (and its children) to binary data buffer
    bool saveToMemoryXML(QByteArray & buffer, unsigned int opts = SerializationOptions::DEFAULTS) const;

    /// Reads this object (and its children) from an XML file
    bool loadFromFileXML(const QString & filename);

    /// Reads this object (and its children) from a memory buffer
    bool loadFromMemoryXML(const QByteArray & buffer);

    /// Serializes this object (and its children) to a DOM node
    virtual ArchiveElement serialize(Archive &doc) const OVERRIDE;
    /// De-serializes this object (and its children) from a DOM node
    virtual bool deserialize(const ArchiveElement & element) OVERRIDE;

    /// Handles a serialization element that lacks automatic handlers.
    /// @param element The element to be deserialized
    /// @return true on success
    virtual bool readElement(const ArchiveElement & element);

    /// Prints the contents of this Attribute to the terminal
    void debugDump();

    /// Container for attributes, key is the attribute name
    typedef Radiant::ArrayMap<QByteArray, Attribute *> container;
    /// Iterator for the container
    typedef container::iterator iterator;
    typedef container::const_iterator const_iterator;

    /// @returns attribute container
    const container & attributes() const { return m_attributes; }

    /** Add an event listener to this object.
        This function is part of the event passing framework. After calling
        this, @a listener will get the messages with id @a messageId whenever this
        object has events with @a eventId. This function can fail if the Node is being
        destroyed while calling this function.
        @param eventId the event id to match when in the @ref eventSend. Corresponds
                       to the first parameter in @ref eventSend
        @param messageId the event id to use when delivering the event to listener.
                         This is the first parameter in @ref eventProcess
        @param listener the listening widget. Receives events and handles them
                        in @ref eventProcess -function
        @param listenerType defines when to send events to listener
        @param defaultData the default binary data to be used when delivering
                           the message, used only if @ref eventSend doesn't
                           include BinaryData
        @returns event id or -1 on failure
        @sa eventRemoveListener
    */
    template <typename Widget>
    long eventAddListener(const QByteArray &eventId,
                          const QByteArray &messageId,
                          Radiant::IntrusivePtr<Widget>& listener,
                          ListenerType listenerType = DIRECT,
                          const Radiant::BinaryData *defaultData = 0)
    {
      return eventAddListener(eventId, messageId, &*listener, listenerType, defaultData);
    }

    /// @todo the raw pointers in these should be fixed!
    /** Add an event listener to this object.
        This function is part of the event passing framework. After calling
        this, @a listener will get the messages with id @a messageId whenever this
        object has events with @a eventId. This function can fail if the Node is being
        destroyed while calling this function.
        @param eventId the event id to match when in the @ref eventSend. Corresponds
                       to the first parameter in @ref eventSend
        @param messageId the event id to use when delivering the event to listener.
                         This is the first parameter in @ref eventProcess
        @param listener the listening object. Receives events and handles them
                        in @ref eventProcess -function
        @param listenerType defines when to send events to listener
        @param defaultData the default binary data to be used when delivering
                           the message, used only if @ref eventSend doesn't
                           include BinaryData
        @returns event id or -1 on failure
        @sa eventRemoveListener
    */
    long eventAddListener(const QByteArray & eventId,
                          const QByteArray & messageId,
                          Valuable::Node * listener,
                          ListenerType listenerType = DIRECT,
                          const Radiant::BinaryData * defaultData = 0);

    /** Add an event listener to this object.
        This function is part of the event passing framework. After calling
        this, @a func will be called whenever this object has events with @a eventId.
        This function can fail if the Node is being destroyed while calling this
        function.
        @param eventId the event id to match when in the @ref eventSend. Corresponds
                       to the first parameter in @ref eventSend
        @param func the listener callback
        @param listenerType defines when to call the callback
        @returns event id or -1 on failure
        @sa eventRemoveListener
    */
    long eventAddListener(const QByteArray & eventId, ListenerFuncVoid func,
                          ListenerType listenerType = DIRECT);

    long eventAddListener(const QByteArray & eventId, Node * dstNode, ListenerFuncVoid func, ListenerType listenerType = DIRECT);
    long eventAddListenerBd(const QByteArray & eventId, Node * dstNode, ListenerFuncBd func, ListenerType listenerType = DIRECT);

    /** Add an event listener to this object.
        This function is part of the event passing framework. After calling this, @a
        func will be called whenever this object has events with @a eventId. This
        function can fail if the Node is being destroyed while calling this function.
        @param eventId the event id to match when in the @ref eventSend. Corresponds
                       to the first parameter in @ref eventSend
        @param func the listener callback that will get event BinaryData as a parameter
        @param listenerType defines when to call the callback
        @returns event id or -1 on failure
        @sa eventRemoveListener
    */
    long eventAddListenerBd(const QByteArray & eventId, ListenerFuncBd func,
                            ListenerType listenerType = DIRECT);

    /// Removes all events from this object to given listener
    /// @param listener event listener
    /// @returns number of events removed
    template <typename Widget>
    int eventRemoveListener(Radiant::IntrusivePtr<Widget>& listener)
    {
      return eventRemoveListener(QByteArray(), QByteArray(), &*listener);
    }

    /// Removes events from this object that match the parameters.
    /// @param eventId event id or null QByteArray to match all event ids
    /// @param messageId message id or null QByteArray to match all message ids
    /// @param listener event listener or nullptr to match all listeners
    /// @returns number of events removed
    template <typename Widget>
    int eventRemoveListener(const QByteArray & eventId = QByteArray(),
                            const QByteArray & messageId = QByteArray(),
                            Radiant::IntrusivePtr<Widget>& listener=Radiant::IntrusivePtr<Widget>())
    {
      return eventRemoveListener(eventId, messageId, &*listener);
    }

    /** Removes event listeners from this object.

      @code
      // Remove all event links between two widgets:
      myWidget1->eventRemoveListener(myWidget2);

      // Remove selected event links between two widgets:
      myWidget1->eventRemoveListener("interaction-begin", myWidget3);
      myWidget1->eventRemoveListener(QByteArray(), "clear", myWidget4);

      // Remove all selected events to any other widgets
      myWidget1->eventRemoveListener("single-tap");
      @endcode


      @param eventId The name of the originating event that should be cleared. If this parameter
      is null (QByteArray()), then all originating events are matched.

      @param messageId The name of of the destination event that should be cleared. If this parameter
      is null (QByteArray()), then all destination events are matched.

      @param listener The target object for which the events should be cleared. If
                 this parameter is null, then all objects are matched.

      @return number of event listener links removed
      */
    int eventRemoveListener(const QByteArray & eventId = QByteArray(), const QByteArray & messageId = QByteArray(), Valuable::Node * listener = 0);

    /// Removes all events from this object to given listener
    /// @param listener event listener
    /// @returns number of events removed
    int eventRemoveListener(Valuable::Node * listener)
    {
      return eventRemoveListener(QByteArray(), QByteArray(), listener);
    }

    /// Removes event listener with given id
    /// @returns true if listener was found and removed
    bool eventRemoveListener(long listenerId);

    /// Returns the number of event sources
    unsigned eventSourceCount() const
    {
      Radiant::Guard g(m_eventsMutex);
      return (unsigned) m_eventSources.size();
    }
    /// Returns the number of event listeners
    unsigned eventListenerCount() const
    {
      Radiant::Guard g(m_eventsMutex);
      return (unsigned) m_elisteners.size();
    }

    /// Control whether events are passed
    void eventPassingEnable(bool enable) { m_eventsEnabled = enable; }

    /// @cond
    virtual void eventProcess(const QByteArray & messageId, Radiant::BinaryData & data) OVERRIDE;

    /// @endcond

    /// Generates a unique identifier
    /// @return New unique id
    static Uuid generateId();
    /// Returns the unique id
    Uuid id() const;
    /// Sets the unique id
    void setId(Uuid newId);

    /// Registers a new event this class can send with eventSend
    void eventAddOut(const QByteArray & eventId);

    /// Registers a new event that this class handles in eventProcess
    void eventAddIn(const QByteArray & messageId);

    /// Unregisters an existing event this class can send with eventSend
    void eventRemoveOut(const QByteArray & eventId);

    /// Unregisters an existing event that this class handles in eventProcess
    void eventRemoveIn(const QByteArray & messageId);

    /// Register a deprecated event that is automatically converted to new
    /// event id and a warning is issued when it is used.
    /// @param deprecatedId deprecated event id
    /// @param newId replacing event id
    void eventAddDeprecated(const QByteArray & deprecatedId, const QByteArray & newId);

    /// Returns true if this object accepts event 'id' in eventProcess
    bool acceptsEvent(const QByteArray & messageId) const;

    /// Returns set of all registered OUT events
    const QSet<QByteArray> & eventOutNames() const { return m_eventSendNames; }

    /// Returns set of all registered IN events
    const QSet<QByteArray> & eventInNames() const { return m_eventListenNames; }

#ifdef CORNERSTONE_JS
    /// Add a JavaScript attribute listener to attribute belonging this Node.
    /// @param attribute Attribute name
    /// @param func JavaScript function to call whenever attribute is changed/deleted
    /// @param role when should the listener function be called
    /// @returns listener id that can be used to remove the listener with Attribute::removeListener
    long addListener(const QByteArray & attribute, v8::Persistent<v8::Function> func,
                     int role = Attribute::CHANGE_ROLE);
    using Attribute::addListener;
#endif
    /// Triggers any pending AFTER_UPDATE-events.
    /// This is called automatically from MultiWidgets::Application every frame.
    /// If you don't have running application instance, or if you want to block
    /// main thread for a longer period of time, this should be called periodically
    /// manually to make sure all events are dispatched.
    /// @returns number of processes items
    static int processQueue();

    /// Disables all AFTER_UPDATE event processing, no new events will be
    /// accepted and the current queue will be cleared.
    static void disableQueue();

    /// Enables queue after calling disableQueue. Does nothing if the queue
    /// is already enabled.
    static void reEnableQueue();

    /// Copies attribute values from one node to another
    /// @param from source node
    /// @param to target node
    /// @returns true if copying was successful
    static bool copyValues(const Node & from, Node & to);

    /// Queue function to be called in the main thread after the next update()
    /// @param function function to be called
    static void invokeAfterUpdate(ListenerFuncVoid function);

    virtual void setAsDefaults() OVERRIDE;

    virtual bool isChanged() const OVERRIDE;

    /// Sends an event and bd to all listeners on this eventId
    void eventSend(const QByteArray & eventId, Radiant::BinaryData & bd);
    /// Sends an event to all listeners on this eventId
    void eventSend(const QByteArray & eventId);

    /// Sends an event to all listeners on this eventId.
    /// Builds Radiant::BinaryData automatically based on the function parameters
    /// @param eventId event id
    /// @param p1 event parameter
    template <typename P1>
    void eventSend(const QByteArray & eventId, const P1 & p1)
    {
      Radiant::BinaryData bd;
      bd.write(p1);
      eventSend(eventId, bd);
    }

    /// Sends an event to all listeners on this eventId.
    /// Builds Radiant::BinaryData automatically based on the function parameters
    /// @param eventId event id
    /// @param p1,p2 event parameters
    template <typename P1, typename P2>
    void eventSend(const QByteArray & eventId, const P1 & p1, const P2 & p2)
    {
      Radiant::BinaryData bd;
      bd.write(p1);
      bd.write(p2);
      eventSend(eventId, bd);
    }

    /// Sends an event to all listeners on this eventId.
    /// Builds Radiant::BinaryData automatically based on the function parameters
    /// @param eventId event id
    /// @param p1,p2,p3 event parameters
    template <typename P1, typename P2, typename P3>
    void eventSend(const QByteArray & eventId, const P1 & p1, const P2 & p2, const P3 & p3)
    {
      Radiant::BinaryData bd;
      bd.write(p1);
      bd.write(p2);
      bd.write(p3);
      eventSend(eventId, bd);
    }

    /// Sends an event to all listeners on this eventId.
    /// Builds Radiant::BinaryData automatically based on the function parameters
    /// @param eventId event id
    /// @param p1,p2,p3,p4 event parameters
    template <typename P1, typename P2, typename P3, typename P4>
    void eventSend(const QByteArray & eventId, const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4)
    {
      Radiant::BinaryData bd;
      bd.write(p1);
      bd.write(p2);
      bd.write(p3);
      bd.write(p4);
      eventSend(eventId, bd);
    }

    /// Sends an event to all listeners on this eventId.
    /// Builds Radiant::BinaryData automatically based on the function parameters
    /// @param eventId event id
    /// @param p1,p2,p3,p4,p5 event parameters
    template <typename P1, typename P2, typename P3, typename P4, typename P5>
    void eventSend(const QByteArray & eventId, const P1 & p1, const P2 & p2, const P3 & p3, const P4 & p4, const P5 & p5)
    {
      Radiant::BinaryData bd;
      bd.write(p1);
      bd.write(p2);
      bd.write(p3);
      bd.write(p4);
      bd.write(p5);
      eventSend(eventId, bd);
    }

    /// Returns true if we are about to delete this object. This flag needs to
    /// be set manually using setBeginDestroyed, not all classes inheriting
    /// from this might set it. If Node is being destroyed, it won't receive
    /// any events. You also can't set any new event listeners to it.
    bool isBeingDestroyed() const { return m_isBeingDestroyed; }

#ifdef ENABLE_THREAD_CHECKS
    virtual void setOwnerThread(Radiant::Thread::id_t owner) override;
#endif

  protected:
    /// Sets 'isBeginDestroyed' flag to true and removes all event listeners
    /// to this object. This is set at least in Widget::preDestroy and
    /// Operator::~Operator.
    void setBeingDestroyed();

    /// Get the sender of the event, only valid in DIRECT events
    /// @returns the sender of the event, can be read in eventProcess()
    Node * sender() { return m_sender; }

    /// This is called when new attribute is added to Node
    /// @param attribute added attribute
    virtual void attributeAdded(Attribute * attribute);
    /// This is called when attribute is removed from Node
    /// @param attribute removed attribute
    virtual void attributeRemoved(Attribute * attribute);

  private:
    /// Adds an event source
    void eventAddSource(Valuable::Node * source);
    /// Removes an event source
    void eventRemoveSource(Valuable::Node * source);

    /// Check that the given event is registered. Also convert deprecated
    /// events to new ids and issue warnings.
    QByteArray validateEvent(const QByteArray & from);

    /// Renamed 'internal' so that it won't be confused with Attribute::removeListeners
    void internalRemoveListeners();

  private:

    Node * m_sender;

    friend class Attribute; // So that Attribute can call the function below.

    void attributeRenamed(const QByteArray & was, const QByteArray & now);

    container m_attributes;

    class ValuePass {
    public:
      ValuePass(long id) : m_listener(0), m_func(), m_func2(), m_type(DIRECT), m_listenerId(id) {}

      inline bool operator == (const ValuePass & that) const;

      Valuable::Node * m_listener;
      ListenerFuncVoid m_func;
      ListenerFuncBd m_func2;
      Radiant::BinaryData   m_defaultData;
      QByteArray m_from;
      QByteArray m_to;
      ListenerType m_type;
      long m_listenerId;
    };

    typedef std::list<ValuePass> Listeners;
    Listeners m_elisteners; // Event listeners

    typedef std::map<Valuable::Node *, int> Sources;
    Sources m_eventSources;

    // Mutable so that we can access read-only properties of m_elisteners in const-functions
    // Protects m_elisteners and m_eventSources
    mutable Radiant::Mutex m_eventsMutex;

    bool m_eventsEnabled;
    bool m_isBeingDestroyed = false;

    // set of all attributes that this Node is listening to
    QSet<Attribute*> m_attributeListening;

    Valuable::AttributeT<Uuid> m_id;

    std::atomic<long> m_listenersId;

    QSet<QByteArray> m_eventSendNames;
    QSet<QByteArray> m_eventListenNames;

    QMap<QByteArray, QByteArray> m_deprecatedEventCompatibility;
  };

  typedef Valuable::AttributeT<Node::Uuid> AttributeUuid;
}

#endif
