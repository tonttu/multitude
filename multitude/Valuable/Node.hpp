
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

#ifndef VALUABLE_HASVALUES_HPP
#define VALUABLE_HASVALUES_HPP

#include "Export.hpp"
#include "AttributeInt.hpp"
#include "AttributeObject.hpp"

#include <Patterns/NotCopyable.hpp>

#include <Radiant/Color.hpp>
#include <Radiant/IntrusivePtr.hpp>
#include <Radiant/Trace.hpp>

#ifdef CORNERSTONE_JS
#include <v8.h>
#endif

#include <map>
#include <set>

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
    /// Universally unique identifier type
    typedef int64_t Uuid;

    typedef std::function<void ()> ListenerFunc;
    typedef std::function<void (Radiant::BinaryData &)> ListenerFunc2;
    typedef std::function<void(Valuable::Node*)> CallbackType;

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
    /** Constructs a new Node and adds it under the given host
      @param host host
      @param name name of the object
      @param transit should the object changes be transmitted
    */
    Node(Node * host, const QByteArray &name = "", bool transit = false);
    virtual ~Node();

    Node(Node && node);
    Node & operator=(Node && node);

    /// Adds a new Attribute to the list of attribute objects.
    /// Copies the name of the attribute from the given object.
    bool addAttribute(Attribute * const attribute);
    /// @deprecated This function will be removed in Cornerstone 2.1. Use addAttribute instead.
    bool addValue(Attribute * const value);

    /// Adds a new Attribute to the list of attribute objects.
    bool addAttribute(const QByteArray &name, Attribute * const attribute);
    /// @deprecated This function will be removed in Cornerstone 2.1. Use addAttribute instead.
    bool addValue(const QByteArray &name, Attribute * const value);

    /// @copydoc addAttribute
    template<typename Widget>
    bool addAttribute(const QByteArray &name, const Radiant::IntrusivePtr<Widget>& attribute)
    {
      return addAttribute(name, &*attribute);
    }

    virtual bool isReady(bool recursive) const;

    /// Callback is called when the node is ready or immediately if the node is already ready
    /// This is needed for ensuring that callbacks get called. For example the following
    /// code is not thread-safe:
    /// if(n.isReady()) {
    ///   /// do something
    /// } else {
    ///   n.eventAddListener("ready", "do smth", ..);
    /// }
    template <typename T>
    void onReady(std::function<void(T*)> readyCallback, bool once = false, ListenerType type = AFTER_UPDATE)
    {
      onReady([readyCallback](Valuable::Node* n) { readyCallback(static_cast<T*>(n)); }, once, type);
    }

    template <typename T, template <typename> class Ptr >
    void onReady(std::function<void(Ptr<T>)> readyCallback, bool once = false, ListenerType type = AFTER_UPDATE)
    {
      onReady([readyCallback](Valuable::Node* n) {
            T* ptr = static_cast<T*>(n);
            readyCallback(Ptr<T>(ptr));
            }, once, type);
    }

    void clearReadyCallbacks();

    /// Same as other onReady-functions but supports functions taking none parameters
    //  In perfect world compiler could deduce the types more correctly but at least g++ 4.6.3
    //  had serious problems with type inference when function below was named "onReady"
    void onReadyVoid(std::function<void(void)> readyCallback, bool once = false, ListenerType type = AFTER_UPDATE);
    void onReady(CallbackType readyCallback, bool once = false, ListenerType type = AFTER_UPDATE);

    /// Gets an Attribute with the given name
    /// @param name Attribute name to search for
    /// @return Null if no object can be found
    virtual Attribute * getAttribute(const QByteArray & name) const;
    /// @deprecated This function will be removed in Cornerstone 2.1. Use getAttribute instead.
    virtual Attribute * getValue(const QByteArray & name) const;

    /// Removes an Attribute from the list of attribute objects.
    void removeAttribute(Attribute * const attribute);
    /// @deprecated This function will be removed in Cornerstone 2.1. Use removeAttribute instead.
    void removeValue(Attribute * const value);

    /// Clears all Attribute values of the given layer
    /// @param layer layer to clear
    void clearValues(Layer layer);

    /// @todo add 'shortcut' API
    // float getAttributeFloat(const QString & name, bool * ok = 0, float default = 0.f)
    // ...

    /// Uses a query string to find a Attribute, and sets a new value to that if found.
    /// @param query The path to the Attribute. This is a '/'-separated list
    ///        of Attribute names, forming a path inside a Attribute tree.
    ///        ".." can be used to refer to host element. For example
    ///        setValue("../foo/bar", 4.0f) sets 4.0f to Attribute named "bar"
    ///        under Attribute "foo" that is sibling of this object.
    /// @param value The new value
    /// @return True if object was found and the value was set successfully.
    /// @todo implement similar to getValue (to avoid dynamic_cast)
    template<class T>
    bool setValue(const QByteArray & name, const T & v)
    {
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

      container::iterator it = m_values.find(next);
      if(it == m_values.end()) {
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

    bool setValue(const QByteArray & name, v8::Handle<v8::Value> v);
    bool setValue(const QByteArray & name, v8::Local<v8::Value> v) {
      return setValue(name, static_cast<v8::Handle<v8::Value> >(v));
    }
#endif

    /// Saves this object (and its children) to an XML file
    bool saveToFileXML(const QString & filename, unsigned int opts = SerializationOptions::DEFAULTS);
    /// Saves this object (and its children) to binary data buffer
    bool saveToMemoryXML(QByteArray & buffer, unsigned int opts = SerializationOptions::DEFAULTS);

    /// Reads this object (and its children) from an XML file
    bool loadFromFileXML(const QString & filename);

    /// Reads this object (and its children) from a memory buffer
    bool loadFromMemoryXML(const QByteArray & buffer);

    /// Serializes this object (and its children) to a DOM node
    virtual ArchiveElement serialize(Archive &doc) const;
    /// De-serializes this object (and its children) from a DOM node
    virtual bool deserialize(const ArchiveElement & element);

    /// Handles a serialization element that lacks automatic handlers.
    /// @param element The element to be deserialized
    /// @return true on success
    virtual bool readElement(const ArchiveElement & element);

    /// Prints the contents of this Attribute to the terminal
    void debugDump();

    /// Container for key-value object pairs
    typedef std::map<QByteArray, Attribute *> container;
    /// Iterator for the container
    typedef container::iterator iterator;
    typedef container::const_iterator const_iterator;

    /// Returns an iterator to the beginning of the values
    iterator valuesBegin() { return m_values.begin(); }
    const_iterator valuesBegin() const { return m_values.begin(); }
    /// Returns an iterator to the end of the values
    iterator valuesEnd() { return m_values.end(); }
    const_iterator valuesEnd() const { return m_values.end(); }

    const container & values() { return m_values; }

    /** Add an event listener to this object.

        This function is part of the experimental event passing
        framework. After calling this, @a listener will get the messages
        with id @a messageId whenever this object has events with @a eventId.

        @param eventId The event id to match when in the @ref eventSend. Corresponds
                       to the first parameter in @ref eventSend.

        @param messageId The event id to use when delivering the event to listener.
                         This is the first parameter in @ref processMessage.

        @param listener The listening object. Receives events and handles them
                        in @ref processMessage -function.

        @param listenerType Defines when to send events to listener.

        @param defaultData The default binary data to be used when
        delivering the message.

    */
    template <typename Widget>
    void eventAddListener(const QByteArray &eventId,
                          const QByteArray &messageId,
                          Radiant::IntrusivePtr<Widget>& listener,
                          ListenerType listenerType,
                          const Radiant::BinaryData *defaultData = 0)
    {
      eventAddListener(eventId, messageId, &*listener, listenerType, defaultData);
    }


    /// Add an event listener to this object.
    template <typename Widget>
    void eventAddListener(const QByteArray &eventId,
                          const QByteArray &messageId,
                          Radiant::IntrusivePtr<Widget>& listener,
                          const Radiant::BinaryData *defaultData = 0)
    {
      eventAddListener(eventId, messageId, &*listener, DIRECT, defaultData);
    }

    /// @todo the raw pointers in these should be fixed!
    void eventAddListener(const QByteArray & eventId,
                          const QByteArray & messageId,
                          Valuable::Node * listener,
                          ListenerType listenerType,
                          const Radiant::BinaryData * defaultData = 0);
    void eventAddListener(const QByteArray & eventId,
                          const QByteArray & messageId,
                          Valuable::Node * listener,
                          const Radiant::BinaryData * defaultData = 0)
    {
      eventAddListener(eventId, messageId, listener, DIRECT, defaultData);
    }

#ifdef CORNERSTONE_JS
    void eventAddListener(const QByteArray & eventId,
                          const QByteArray & messageId,
                          v8::Persistent<v8::Function> func,
                          const Radiant::BinaryData * defaultData = 0);
    void eventAddListener(const QByteArray & eventId,
                          v8::Persistent<v8::Function> func,
                          const Radiant::BinaryData * defaultData = 0)
    {
      eventAddListener(eventId, eventId, func, defaultData);
    }
#endif

    void eventAddListener(const QByteArray & eventId, ListenerFunc func,
                          ListenerType listenerType = DIRECT);

    void eventAddListenerBd(const QByteArray & eventId, ListenerFunc2 func,
                            ListenerType listenerType = DIRECT);

    template <typename Widget>
    int eventRemoveListener(Radiant::IntrusivePtr<Widget>& listener)
    {
      return eventRemoveListener(QByteArray(), QByteArray(), &*listener);
    }

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
      myWidget1->eventRemoveListener("interactionbegin", myWidget3);
      myWidget1->eventRemoveListener(QByteArray(), "clear", myWidget4);

      // Remove all selected events to any other widgets
      myWidget1->eventRemoveListener("singletap");
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
    int eventRemoveListener(Valuable::Node * listener)
    {
      return eventRemoveListener(QByteArray(), QByteArray(), listener);
    }

    /// Adds an event source
    template <typename Widget>
    void eventAddSource(Radiant::IntrusivePtr<Widget>& source)
    {
      eventAddSource(&*source);
    }

    /// Removes an event source
    template <typename Widget>
    void eventRemoveSource(Radiant::IntrusivePtr<Widget>& source)
    {
      eventRemoveSource(&*source);
    }

    /// Adds an event source
    void eventAddSource(Valuable::Node * source);
    /// Removes an event source
    void eventRemoveSource(Valuable::Node * source);

    /// Returns the number of event sources
    unsigned eventSourceCount() const {  return (unsigned) m_eventSources.size(); }
    /// Returns the number of event listeners
    unsigned eventListenerCount() const { return (unsigned) m_elisteners.size(); }

    /// Control whether events are passed
    void eventPassingEnable(bool enable) { m_eventsEnabled = enable; }

    /// @cond
    virtual void processMessage(const QByteArray & messageId, Radiant::BinaryData & data);

    /// @endcond

    /// Generates a unique identifier
    /// @return New unique id
    static Uuid generateId();
    /// Returns the unique id
    Uuid id() const;

    /// Registers a new event this class can send with eventSend
    void eventAddOut(const QByteArray & eventId);

    /// Registers a new event that this class handles in processMessage
    void eventAddIn(const QByteArray & messageId);

    /// Returns true if this object accepts event 'id' in processMessage
    bool acceptsEvent(const QByteArray & messageId) const;

    /// Returns set of all registered OUT events
    const QSet<QByteArray> & eventOutNames() const { return m_eventSendNames; }

    /// Returns set of all registered IN events
    const QSet<QByteArray> & eventInNames() const { return m_eventListenNames; }

#ifdef CORNERSTONE_JS
    long addListener(const QByteArray & name, v8::Persistent<v8::Function> func,
                     int role = Attribute::CHANGE_ROLE);
#endif
    static int processQueue();

    static bool copyValues(const Node & from, Node & to);

    virtual void setAsDefaults() OVERRIDE;

  protected:

    /// Sends an event and bd to all listeners on this eventId
    void eventSend(const QByteArray & eventId, Radiant::BinaryData & bd);
    /// Sends an event to all listeners on this eventId
    void eventSend(const QByteArray & eventId);

    template <typename P1>
    void eventSend(const QByteArray & eventId, const P1 & p1)
    {
      Radiant::BinaryData bd;
      bd.write(p1);
      eventSend(eventId, bd);
    }

    template <typename P1, typename P2>
    void eventSend(const QByteArray & eventId, const P1 & p1, const P2 & p2)
    {
      Radiant::BinaryData bd;
      bd.write(p1);
      bd.write(p2);
      eventSend(eventId, bd);
    }

    template <typename P1, typename P2, typename P3>
    void eventSend(const QByteArray & eventId, const P1 & p1, const P2 & p2, const P3 & p3)
    {
      Radiant::BinaryData bd;
      bd.write(p1);
      bd.write(p2);
      bd.write(p3);
      eventSend(eventId, bd);
    }

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

    /// The sender of the event, can be read in processMessage()
    Node * sender() { return m_sender; }


  private:

    Node * m_sender;

    friend class Attribute; // So that Attribute can call the function below.

    void valueRenamed(const QByteArray &was, const QByteArray &now);

    /// @todo rename to m_attributes
    container m_values;

    class ValuePass {
    public:
      ValuePass() : m_listener(0), m_func(), m_func2(), m_frame(-1), m_type(DIRECT) {}

      inline bool operator == (const ValuePass & that) const;

      Valuable::Node * m_listener;
      ListenerFunc m_func;
      ListenerFunc2 m_func2;
#ifdef CORNERSTONE_JS
      v8::Persistent<v8::Function> m_funcv8;
#endif
      Radiant::BinaryData   m_defaultData;
      QByteArray m_from;
      QByteArray m_to;
      int         m_frame;
      ListenerType m_type;
    };

    typedef std::list<ValuePass> Listeners;
    Listeners m_elisteners; // Event listeners

    typedef std::set<Valuable::Node *> Sources;
    Sources m_eventSources;
    bool m_eventsEnabled;

    // set of all valueobjects that this Node is listening to
    QSet<Attribute*> m_valueListening;

    Valuable::AttributeIntT<Uuid> m_id;

    /// List of all listeners added in onReady()
    QList<CallbackType> m_readyCallbacks;
    QList<CallbackType> m_readyOnceCallbacks;
    /// Protects m_readyCallbacks and isReady() call in onReady()
    std::shared_ptr<Radiant::Mutex> m_readyCallbacksMutex;
    /// Used by onReady to check if there already is "ready" event listener
    int m_hasReadyListener;

    // For invalidating the too new ValuePass objects
    int m_frame;

    QSet<QByteArray> m_eventSendNames;
    QSet<QByteArray> m_eventListenNames;
  };

}

#endif
