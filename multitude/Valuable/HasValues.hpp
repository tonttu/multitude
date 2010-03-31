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

      This base class has a list of #ValueObject child objects (aka
      member variables) that are named with unique string.

      Deleting the child objects is the responsibility of the child
      classes, HasValues simply maintains a list of children.
  */
  /// @todo Doc, Examples
  class VALUABLE_API HasValues : public ValueObject
  {
  public:
    typedef int64_t Uuid;

    HasValues();
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
    bool setValue(const std::string & name, const T & v);

    /// Saves this object (and its children) to an XML file
    bool saveToFileXML(const char * filename);
    /// Saves this object (and its children) to binary data buffer
    bool saveToMemoryXML(std::vector<char> & buffer);

    /// Reads this object (and its children) from an XML file
    bool loadFromFileXML(const char * filename);

    /// Returns the typename of this object.
    virtual const char * type() const { return VO_TYPE_HASVALUES; }

    /// Serializes this object (and its children) to a DOM node
    virtual DOMElement serializeXML(DOMDocument * doc);
    /// De-serializes this object (and its children) from a DOM node
    virtual bool deserializeXML(DOMElement element);
    /** Handles a DOM element that lacks automatic handlers. */
    virtual bool readElement(DOMElement element);

    /// Prints the contents of this ValueObject to the terinal
    void debugDump();

    typedef std::map<std::string, ValueObject *> container;
    typedef container::iterator iterator;

    iterator valuesBegin() { return m_children.begin(); }
    iterator valuesEnd() { return m_children.end(); }

    /** Add an event listener to this object.

        This function is part of the experimental event passing
        framework.

        @arg from The event id to match when in the eventSend.

        @arg to The event id to to use when delivering the event

        @arg obj The listening object

        @arg defaultData The default binary data to be used when
        delivering the message.

    */
    void eventAddListener(const char * from,
                          const char * to,
                          Valuable::HasValues * obj,
                          const Radiant::BinaryData * defaultData = 0);
    /**

      @return number of listeners removed
      */
    int eventRemoveListener(Valuable::HasValues * obj, const char * from = 0, const char * to = 0);
    /** Adds */
    void eventAddSource(Valuable::HasValues * source);
    void eventRemoveSource(Valuable::HasValues * source);

    unsigned eventSourceCount() const {  return (unsigned) m_eventSources.size(); }
    unsigned eventListenerCount() const { return (unsigned) m_elisteners.size(); }


    void eventPassingEnable(bool enable) { m_eventsEnabled = enable; }

    /// The main event processing function
    /** To capture and process events, you need to override this function. The default implemendation
        tries to find a child object with the name found in the "type" argument,
        and passes the arguments to that object.

        Typical implementation of a custom processMessage function could look like:

        \code
        void MyClass::processMessage(const char * type, Radiant::BinaryData & data)
        {
          if(strcmp(type, "jump") == 0)
            doJump();
          else if(strcmp(type, "crawl") == 0) {
            bool ok;
            int speed = data.readInt32(&ok);
            if(ok)
              doCrawl(speed);
          }
          else
            HasValues::processMessage(type, data);
        }
        \endcode

    */
    virtual void processMessage(const char * type, Radiant::BinaryData & data);

    static Uuid generateId();
    Uuid id() const;

  protected:

    void eventSend(const std::string & id, Radiant::BinaryData &);
    void eventSend(const char *, Radiant::BinaryData &);
    void eventSend(const char *);

  private:
    friend class ValueObject; // So that ValueObject can call the function below.

    void childRenamed(const std::string & was, const std::string & now);

    container m_children;

    class ValuePass {
    public:
      ValuePass() : m_listener(0) {}

      inline bool operator == (const ValuePass & that) const
      { return (m_listener == that.m_listener) && (m_from == that.m_from) &&
      (m_to == that.m_to); }

      Valuable::HasValues * m_listener;
      Radiant::BinaryData   m_defaultData;
      std::string m_from;
      std::string m_to;
    };

    typedef std::list<ValuePass> Listeners;
    Listeners m_elisteners; // Event listeners

    typedef std::set<Valuable::HasValues *> Sources;
    Sources m_eventSources;
    bool m_eventsEnabled;

    Valuable::ValueIntT<Uuid> m_id;
  };

}

#endif
