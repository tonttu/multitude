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

#include <Valuable/Export.hpp>
#include <Valuable/ValueInt.hpp>
#include <Valuable/ValueObject.hpp>

#include <Patterns/NotCopyable.hpp>

#include <Radiant/Color.hpp>

#include <map>
#include <string>

#define VO_TYPE_HASVALUES "HasValues"

namespace Valuable
{
  /** Base class for objects that include member variables with automatic IO.

      This base class has a list of #Valuable::ValueObject child objects (aka
      member variables) that are named with unique string.

      Deleting the child objects is the responsibility of the child
      classes, HasValues simply maintains a list of children.
  */
  /// @todo Examples
  class VALUABLE_API HasValues : public ValueObject
  {
  public:
    /// Universally unique identifier type
    typedef int64_t Uuid;

    HasValues();
    /** Constructs a new HasValues and adds it under the given parent
      @param parent parent
      @param name name of the object
      @param transit should the object changes be transmitted
    */
    HasValues(HasValues * parent, const std::string & name, bool transit = false);
    virtual ~HasValues();

    /// Adds new ValueObject to the list of values
    bool addValue(const std::string & name, ValueObject * const value);
    /// Gets a ValueObject with the given name
    /** If no object can be found, then this method return zero. */
    ValueObject * getValue(const std::string & name);
    /// Removes a ValueObject from the list of value.
    void removeValue(ValueObject * const value);

    /// @todo add 'shortcut' API
    // float getValueFloat(const std::string & name, bool * ok = 0, float default = 0.f)
    // ...

    template<class T>
    bool setValue(const std::string & name, const T & v)
    {
      size_t cut = name.find("/");
      std::string next = name.substr(0, cut);
      std::string rest = name.substr(cut + 1);

      if(next == std::string("..")) {
        if(!m_parent) {
          Radiant::error(
              "HasValues::setValue # node '%s' has no parent", m_name.c_str());
          return false;
        }

        return m_parent->setValue(rest, v);
      }

      container::iterator it = m_children.find(next);
      if(it == m_children.end()) {
        Radiant::error(
            "HasValues::setValue # property '%s' not found", next.c_str());
        return false;
      }

      HasValues * hv = dynamic_cast<HasValues *> (it->second);
      if(hv)
        return hv->setValue(rest, v);
      else {
        ValueObject * vo = it->second;
        return vo->set(v);
      }
    }

    /// Saves this object (and its children) to an XML file
    bool saveToFileXML(const char * filename);
    /// Saves this object (and its children) to binary data buffer
    bool saveToMemoryXML(std::vector<char> & buffer);

    /// Reads this object (and its children) from an XML file
    bool loadFromFileXML(const char * filename);

    /// Returns the typename of this object.
    virtual const char * type() const { return VO_TYPE_HASVALUES; }

    /// Serializes this object (and its children) to a DOM node
    virtual ArchiveElement & serialize(Archive &doc) const;
    /// De-serializes this object (and its children) from a DOM node
    virtual bool deserialize(ArchiveElement & element);

    /** Handles a DOM element that lacks automatic handlers. */
    virtual bool readElement(DOMElement element);

    /// Prints the contents of this ValueObject to the terinal
    void debugDump();

    /// Container for key-value object pairs
    typedef std::map<std::string, ValueObject *> container;
    /// Iterator for the container
    typedef container::iterator iterator;

    /// Returns an iterator to the beginning of the values
    iterator valuesBegin() { return m_children.begin(); }
    /// Returns an iterator to the end of the values
    iterator valuesEnd() { return m_children.end(); }

    /** Add an event listener to this object.

        This function is part of the experimental event passing
        framework.

        @param from The event id to match when in the eventSend.

        @param to The event id to to use when delivering the event

        @param obj The listening object

        @param defaultData The default binary data to be used when
        delivering the message.

    */
    void eventAddListener(const char * from,
                          const char * to,
                          Valuable::HasValues * obj,
                          const Radiant::BinaryData * defaultData = 0);
    /** Removes event listeners from this object.

      @code
      // Remove all event links between two widgets:
      myWidget1->eventRemoveListener(myWidget2);

      // Remove selected event links between two widgets:
      myWidget1->eventRemoveListener(myWidget3, "interactionbegin");
      myWidget1->eventRemoveListener(myWidget4, 0, "clear");
      @endcode


      @param obj The target object for which the events should be cleared

      @param from The name of the originating event that should be cleared. If this parameter
      is null, then all all originating events are matched.

      @param to The name of of the destination event that should be cleared. If this parameter
      is null, then all all destination events are matched.

      @return number of event listener links removed
      */
    int eventRemoveListener(Valuable::HasValues * obj, const char * from = 0, const char * to = 0);
    /// Adds an event source
    void eventAddSource(Valuable::HasValues * source);
    /// Removes an event source
    void eventRemoveSource(Valuable::HasValues * source);

    /// Returns the number of event sources
    unsigned eventSourceCount() const {  return (unsigned) m_eventSources.size(); }
    /// Returns the number of event listeners
    unsigned eventListenerCount() const { return (unsigned) m_elisteners.size(); }

    /// Control whether events are passed
    void eventPassingEnable(bool enable) { m_eventsEnabled = enable; }

    /// @cond
    virtual void processMessage(const char * type, Radiant::BinaryData & data);

    /// @endcond

    /// Generates a unique identifier
    static Uuid generateId();
    /// Returns the unique id
    Uuid id() const;

  protected:

    /// Sends an event to all listeners on this object
    void eventSend(const std::string & id, Radiant::BinaryData &);
    /// @copydoc eventSend
    void eventSend(const char *, Radiant::BinaryData &);
    /// @copydoc eventSend
    void eventSend(const char *);

  private:
    friend class ValueObject; // So that ValueObject can call the function below.

    void childRenamed(const std::string & was, const std::string & now);
    void addNews();

    container m_children;

    class ValuePass {
    public:
      ValuePass() : m_listener(0), m_valid(true), m_frame(-1) {}

      inline bool operator == (const ValuePass & that) const;

      Valuable::HasValues * m_listener;
      Radiant::BinaryData   m_defaultData;
      std::string m_from;
      std::string m_to;
      bool        m_valid;
      int         m_frame;
    };

    typedef std::list<ValuePass> Listeners;
    Listeners m_elisteners; // Event listeners

    typedef std::set<Valuable::HasValues *> Sources;
    Sources m_eventSources;
    bool m_eventsEnabled;

    Valuable::ValueIntT<Uuid> m_id;
    // For invalidating the too new ValuePass objects
    int m_frame;
  };

}

#endif
