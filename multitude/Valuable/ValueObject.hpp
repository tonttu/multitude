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

#ifndef VALUABLE_VALUE_OBJECT_HPP
#define VALUABLE_VALUE_OBJECT_HPP

#include <Nimble/Vector4.hpp>

#include <Patterns/NotCopyable.hpp>

#include <Radiant/BinaryData.hpp>

#include <Valuable/Export.hpp>
#include <Valuable/ValueListener.hpp>

#include <set>
#include <string>

namespace Valuable
{
  class HasValues;
  class DOMElement;
  class DOMDocument;


  /// Base class for values
  /** Typical child classes include some POD (plain old data) elements
      (floats, ints, vector2) etc, that can be accessed through the
      API. The ValueObjects have names (std::string), that can be used to access
      ValueObjects that are stored inside HasValues objects.

      It is also possible to add listeners to values, so that if a
      value is changed, then a call-back to soem other object is
      followed. The listener-API is a bit hard-core, but it has plenty
      of power when you need to track the state of other objects.

      @see HasValues
  */

  /// @todo the "set" functions are duplicating the processMessage functionality
  /// @todo processMessage should be renamed to eventProcess (can be tricky to do)
  /// @todo Doc
  class VALUABLE_API ValueObject
  {
  public:
    ValueObject();
    /// The copy constructor creates a copy of the ValueObject WITHOUT the
    /// link to parent
    ValueObject(const ValueObject & o);
    /// The most usual constructor
    /** This constructor is typically used when attaching the value
    object to its parent.

    @arg parent The parent object. This object is automatically
    added to the parent.

    @arg name The name (or id) of this value. Names are typically
    semi human readable. The names should not contain white-spaces
    as they may be used in XML files etc.

    @arg transit Should value changes be transmitted forward. This
    is related to future uses, and can be largely ignored at the
    moment.
    */
    ValueObject(HasValues * parent, const std::string & name, bool transit = false);
    virtual ~ValueObject();

    /// Returns the name of the object.
    const std::string & name() const { return m_name; }
    void setName(const std::string & s);

    std::string path() const;

    /// Process a message
    /** This method is a key element in the event-passing system.
        It is used to deliver information between objects. The information contains
        two parts:

        <OL>
        <LI>Identifier: A character string that gives the address for the adjustment</LI>
        <LI>Data: A binary blob that contains information for the message</LI>
        </OL>

        This function is overridden in number of classes that need to receive and
        process events. In a typical case, when overriding this function, you should
        either process the message, or call the function of the parent class.
    */
    virtual void processMessage(const char * id, Radiant::BinaryData & data);
    /// Utility function for sending string message to the object
    void processMessageString(const char * id, const char * str);
    /// Utility function for sending a float message to the object
    void processMessageFloat(const char * id, float v);
    /// Utility function for sending an int message to the object
    void processMessageInt(const char * id, int v);
    /// Utility function for sending a Vector2 message to the object
    void processMessageVector2(const char * id, Nimble::Vector2);
    /// Utility function for sending a Vector3 message to the object
    void processMessageVector3(const char * id, Nimble::Vector3);
    /// Utility function for sending a Vector4 message to the object
    void processMessageVector4(const char * id, Nimble::Vector4);

    /// Converts the value object in a floating point number
    /** The default implementation returns zero, and sets the
        ok pointer to false (if it is non-null). */
    virtual float       asFloat(bool * const ok = 0) const;
    /// Converts the value object in an integer
    /** The default implementation returns zero, and sets the
        ok pointer to false (if it is non-null). */
    virtual int         asInt(bool * const ok = 0) const;
    /// Converts the value object to a string
    /** The default implementation returns an empty string, and sets the
        ok pointer to false (if it is non-null). */
    virtual std::string asString(bool * const ok = 0) const;

    virtual bool set(float v);
    virtual bool set(int v);
    virtual bool set(const std::string & v);
    virtual bool set(const Nimble::Vector2f & v);
    virtual bool set(const Nimble::Vector4f & v);

    /// Get the type id of the type
    virtual const char * type() const = 0;

    /// Serializes (writes) this ValueObject to an XML element, and returns the new element.
    virtual DOMElement serializeXML(DOMDocument * doc);
    /// Deserializes (reads) this object from an XML element.
    /** @return Returns true if the read process worked correctly, and false otherwise. */
    virtual bool deserializeXML(DOMElement element) = 0;

    /** The parent object of the value object (is any). */
    HasValues * parent() { return m_parent; }
    /** Sets the parent pointer to zero and removes this object from the parent. */
    void removeParent();

    /// Adds a listener that is invoked whenever the value is changed
    void addListener(ValueListener * l) { m_listeners.push_back(l); }
    /// Removes a listener from the listener list
    void removeListener(ValueListener * l) { m_listeners.remove(l); }

  protected:

    /// Invokes the change valueChanged function of all listeners
    virtual void emitChange();
    /// Invokes the change valueDeleted function of all listeners
    virtual void emitDelete();

  private:
    // The object that holds this object
    HasValues * m_parent;
    std::string m_name;
    bool m_transit;

    ValueListeners m_listeners;

    friend class HasValues;
  };

}

#endif
