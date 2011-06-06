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
#include <Radiant/Functional.hpp>

#include <Valuable/Archive.hpp>
#include <Valuable/Export.hpp>

#include <QString>
#include <QList>

#include <Valuable/DOMElement.hpp>
#include <Radiant/MemCheck.hpp>

namespace Valuable
{
  class HasValues;
  //class DOMElement;
  class DOMDocument;


  /// The base class for all serializable objects.
  class VALUABLE_API Serializable
#ifdef MULTI_MEMCHECK
    : public Radiant::MemCheck
#endif
  {
  public:
    virtual ~Serializable() {}

    /// Serializes (writes) this object to an XML element, and returns the new element.
    virtual ArchiveElement & serialize(Archive & archive) const = 0;

    /// Deserializes (reads) this object from an XML element.
    /** @return Returns true if the read process worked correctly, and false otherwise. */
    virtual bool deserialize(ArchiveElement & element) = 0;

    /// Deserializes (reads) this object from an XML element.
    /** @return Returns true if the read process worked correctly, and false otherwise. */
    virtual bool deserializeXML(DOMElement & element);
  };


  /** The base class for value objects.

      Typical child classes include some POD (plain old data) elements
      (floats, ints, vector2) etc, that can be accessed through the
      API. The ValueObjects have names (QString), that can be used to access
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
  class VALUABLE_API ValueObject : public Serializable
  {
  public:
    enum Layer {
      ORIGINAL = 0,
      STYLE,
      OVERRIDE
    };

    typedef std::function<void ()> ListenerFunc;
    enum ListenerRole {
      DELETE = 1 << 0,
      CHANGE = 1 << 1,
      ALL = (CHANGE << 1) -1
    };

    ValueObject();
    /// The copy constructor creates a copy of the ValueObject WITHOUT the
    /// link to parent
    ValueObject(const ValueObject & o);
    /** Constructs a new value object and attaches it to its parent.

    @param parent The parent object. This object is automatically
    added to the parent.

    @param name The name (or id) of this value. Names are typically
    human-readable. The names should not contain white-spaces
    as they may be used in XML files etc.

    @param transit Should value changes be transmitted forward. This
    is related to future uses, and can be largely ignored at the
    moment.
    */
    ValueObject(HasValues * parent, const QString & name, bool transit = false);
    virtual ~ValueObject();

    /// Returns the name of the object.
    const QString & name() const { return m_name; }
    /// Sets the name of the object
    void setName(const QString & s);
    /// Returns the path (separated by '/'s) from the root
    QString path() const;

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
            Parent::processMessage(type, data);
        }
        \endcode

        @param id The indentifier for the message. Typically this is quite human-readable

        @param data Binary blob that contains the argument data in easily parseable format.

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
    virtual QString asString(bool * const ok = 0) const;

    /// Sets the value of the object
    virtual bool set(float v, Layer layer = OVERRIDE);
    /// Sets the value of the object
    virtual bool set(int v, Layer layer = OVERRIDE);
    /// Sets the value of the object
    virtual bool set(const QString & v, Layer layer = OVERRIDE);
    /// Sets the value of the object
    virtual bool set(const Nimble::Vector2f & v, Layer layer = OVERRIDE);
    /// Sets the value of the object
    virtual bool set(const Nimble::Vector4f & v, Layer layer = OVERRIDE);

    /// Get the type id of the type
    virtual const char * type() const = 0;

    /** The object is serialized using its name as a tag name. */
    virtual ArchiveElement & serialize(Archive &archive) const;

    /** The parent object of the value object (is any). */
    HasValues * parent() const { return m_parent; }
    /** Sets the parent pointer to zero and removes this object from the parent. */
    void removeParent();

    /// Adds a listener that is invoked whenever the value is changed
    void addListener(ListenerFunc func, int role = CHANGE);
    /// Adds a listener that is invoked whenever the value is changed
    /// The listener is removed when the listener object is deleted
    void addListener(HasValues * listener, ListenerFunc func, int role = CHANGE);
    /// Removes listeners from the listener list
    void removeListeners(int role = ALL);
    /// Removes a listener from the listener list
    void removeListener(HasValues * listener, int role = ALL);

    /// Returns true if the current value of the object is different from the original value.
    virtual bool isChanged() const;

    virtual void clearValue(Layer layout);

#ifdef MULTI_DOCUMENTER
    struct Doc
    {
      QString class_name;
      HasValues * obj;
      ValueObject * vo;
    };

    static std::list<Doc> doc;
#endif

  protected:

    /// Invokes the change valueChanged function of all listeners
    virtual void emitChange();
    /// Invokes the change valueDeleted function of all listeners
    virtual void emitDelete();

  private:
    // The object that holds this object
    HasValues * m_parent;
    bool m_changed;
    QString m_name;
    bool m_transit;

    struct ValueListener
    {
      ValueListener(ListenerFunc func_, int role_, HasValues * listener_ = 0)
        : func(func_), role(role_), listener(listener_) {}

      ListenerFunc func;
      int role;
      HasValues * listener;
    };
    QList<ValueListener> m_listeners;

    friend class HasValues;
  };

  /// Every ValueObject is some kind of ValueObjectT<T> object.
  /// Common functionality should be in either here or in ValueObject
  template <typename T> class ValueObjectT : public ValueObject
  {
  public:
    /// Creates a new ValueObjectT and stores the original and current value as a separate variables.
    /// @param parent parent object
    /// @param name name of the value
    /// @param v the default/original value of the object
    /// @param transit ignored
    ValueObjectT(HasValues * parent, const QString & name, const T & v = T(), bool transit = false)
      : ValueObject(parent, name, transit),
      m_current(ORIGINAL),
      m_valueSet()
    {
      m_values[ORIGINAL] = v;
      m_valueSet[ORIGINAL] = true;
    }

    ValueObjectT()
      : ValueObject(),
      m_current(ORIGINAL),
      m_values(),
      m_valueSet()
    {
      m_valueSet[ORIGINAL] = true;
    }

    virtual ~ValueObjectT() {}

    /// Access the wrapped object with the dereference operator
    inline const T & operator * () const { return value(); }
    /// Typecast operator for the wrapped value
    inline operator const T & () const { return value(); }
    /// Use the arrow operator to access fields inside the wrapped object.
    inline const T * operator->() const { return &value(); }

    /// The original value (the value given in constructor) of the ValueObject.
    inline const T & orig() const { return m_values[ORIGINAL]; }

    inline const T & value(Layer layer) const { return m_values[layer]; }

    inline const T & value() const { return m_values[m_current]; }

    inline void setValue(const T & t, Layer layer)
    {
      bool top = layer >= m_current;
      bool sendSignal = top && value() != t;
      if(top) m_current = layer;
      m_values[layer] = t;
      m_valueSet[layer] = true;
      if (sendSignal) this->emitChange();
    }

    /// @todo should return the derived type, not ValueObjectT
    inline ValueObjectT<T> & operator = (const T & t)
    {
      setValue(t, OVERRIDE);
      return *this;
    }

    /// Is the value different from the original value
    // use !( == ) instead of != because != isn't always implemented
    virtual bool isChanged() const { return !(m_values[m_current] == m_values[ORIGINAL]); }


    void clearValue(Layer layout)
    {
      assert(layout > ORIGINAL);
      m_valueSet[layout] = false;
      if(m_current == layout) {
        assert(m_valueSet[ORIGINAL]);
        int l = int(layout) - 1;
        while(!m_valueSet[l]) --l;
        m_current = l;
      }
    }


  protected:
    int m_current;
    T m_values[3];
    bool m_valueSet[3];
  };


}

#endif
