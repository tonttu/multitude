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
#include <Radiant/Trace.hpp>

#include <v8.h>

#include <map>
#include <set>

#include <QString>
#include <QSet>

#define VO_TYPE_HASVALUES "Node"

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

    enum ListenerType
    {
      DIRECT,
      AFTER_UPDATE,
      AFTER_UPDATE_ONCE
    };

    Node();
    /** Constructs a new Node and adds it under the given host
      @param host host
      @param name name of the object
      @param transit should the object changes be transmitted
    */
    Node(Node * host, const QString & name = "", bool transit = false);
    virtual ~Node();

    /// Adds new Attribute to the list of values
    bool addValue(const QString & name, Attribute * const value);
    /// Gets a Attribute with the given name
    /// @param name Value object name to search for
    /// @return Null if no object can be found
    Attribute * getValue(const QString & name);
    /// Removes a Attribute from the list of value.
    void removeValue(Attribute * const value);

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
    template<class T>
    bool setValue(const QString & name, const T & v)
    {
      int cut = name.indexOf("/");
      QString next, rest;
      if(cut > 0) {
        next = name.left(cut);
        rest = name.mid(cut + 1);

        if(next == QString("..")) {
          if(!m_host) {
            Radiant::error(
                "Node::setValue # node '%s' has no host", m_name.toUtf8().data());
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
            "Node::setValue # property '%s' not found", next.toUtf8().data());
        return false;
      }

      if(cut > 0) {
        Node * hv = dynamic_cast<Node *> (it->second);
        if(hv) return hv->setValue(rest, v);
      }

      Attribute * vo = it->second;
      return vo->set(v);
    }

    bool setValue(const QString & name, v8::Handle<v8::Value> v);
    bool setValue(const QString & name, v8::Local<v8::Value> v) {
      return setValue(name, static_cast<v8::Handle<v8::Value> >(v));
    }

    /// Saves this object (and its children) to an XML file
    bool saveToFileXML(const QString & filename);
    /// Saves this object (and its children) to binary data buffer
    bool saveToMemoryXML(QByteArray & buffer);

    /// Reads this object (and its children) from an XML file
    bool loadFromFileXML(const QString & filename);

    /// Returns the typename of this object.
    virtual const char * type() const { return VO_TYPE_HASVALUES; }

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
    typedef std::map<QString, Attribute *> container;
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
        framework.

        @param from The event id to match when in the eventSend.

        @param to The event id to to use when delivering the event

        @param obj The listening object

        @param defaultData The default binary data to be used when
        delivering the message.

    */
    void eventAddListener(const QString & from,
                          const QString & to,
                          Valuable::Node * obj,
                          ListenerType listenerType,
                          const Radiant::BinaryData * defaultData = 0);
    void eventAddListener(const QString & from,
                          const QString & to,
                          Valuable::Node * obj,
                          const Radiant::BinaryData * defaultData = 0)
    {
      eventAddListener(from, to, obj, DIRECT, defaultData);
    }

    void eventAddListener(const QString & from,
                          const QString & to,
                          v8::Persistent<v8::Function> func,
                          const Radiant::BinaryData * defaultData = 0);
    void eventAddListener(const QString & from,
                          v8::Persistent<v8::Function> func,
                          const Radiant::BinaryData * defaultData = 0)
    {
      eventAddListener(from, from, func, defaultData);
    }

    /** Removes event listeners from this object.

      @code
      // Remove all event links between two widgets:
      myWidget1->eventRemoveListener(myWidget2);

      // Remove selected event links between two widgets:
      myWidget1->eventRemoveListener("interactionbegin", myWidget3);
      myWidget1->eventRemoveListener(QString(), "clear", myWidget4);

      // Remove all selected events to any other widgets
      myWidget1->eventRemoveListener("singletap");
      @endcode


      @param from The name of the originating event that should be cleared. If this parameter
      is null (QString()), then all originating events are matched.

      @param to The name of of the destination event that should be cleared. If this parameter
      is null (QString()), then all destination events are matched.

      @param obj The target object for which the events should be cleared. If
                 this parameter is null, then all objects are matched.

      @return number of event listener links removed
      */
    int eventRemoveListener(const QString & from = QString(), const QString & to = QString(), Valuable::Node * obj = 0);
    int eventRemoveListener(Valuable::Node * obj)
    {
      return eventRemoveListener(QString(), QString(), obj);
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
    virtual void processMessage(const QString & type, Radiant::BinaryData & data);

    /// @endcond

    /// Generates a unique identifier
    /// @return New unique id
    static Uuid generateId();
    /// Returns the unique id
    Uuid id() const;

    /// Registers a new event this class can send with eventSend
    void eventAddOut(const QString & id);

    /// Registers a new event that this class handles in processMessage
    void eventAddIn(const QString & id);

    /// Returns true if this object accepts event 'id' in processMessage
    bool acceptsEvent(const QString & id) const;

    /// Returns set of all registered OUT events
    const QSet<QString> & eventOutNames() const { return m_eventSendNames; }

    /// Returns set of all registered IN events
    const QSet<QString> & eventInNames() const { return m_eventListenNames; }

    long addListener(const QString & name, v8::Persistent<v8::Function> func,
                     int role = Attribute::CHANGE_ROLE);

    static int processQueue();

    static bool copyValues(const Node & from, Node & to);

  protected:

    /// Sends an event to all listeners on this object
    void eventSend(const QString & id, Radiant::BinaryData &);
    void eventSend(const QString & id);

    void defineShortcut(const QString & name);

    /// The sender of the event, can be read in processMessage()
    Node * m_sender;

  private:
    friend class Attribute; // So that Attribute can call the function below.

    void valueRenamed(const QString & was, const QString & now);

    container m_values;

    class ValuePass {
    public:
      ValuePass() : m_listener(0), m_valid(true), m_frame(-1), m_type(DIRECT) {}

      inline bool operator == (const ValuePass & that) const;

      Valuable::Node * m_listener;
      v8::Persistent<v8::Function> m_func;
      Radiant::BinaryData   m_defaultData;
      QString m_from;
      QString m_to;
      bool        m_valid;
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
    // For invalidating the too new ValuePass objects
    int m_frame;

    QSet<QString> m_eventSendNames;
    QSet<QString> m_eventListenNames;
  };

}

#endif
