/* COPYRIGHT
 */

#ifndef VALUABLE_ATTRIBUTE_OBJECT_HPP
#define VALUABLE_ATTRIBUTE_OBJECT_HPP

#include "Export.hpp"
#include "Archive.hpp"
#include "DOMElement.hpp"

#include <Nimble/Vector4.hpp>

#include <Patterns/NotCopyable.hpp>

#include <Radiant/BinaryData.hpp>
#include <Radiant/Functional.hpp>
#include <Radiant/MemCheck.hpp>

#include <Valuable/Archive.hpp>
#include <Valuable/Export.hpp>

#include <QString>
#include <QList>
#include <QVariantList>

// new behavior: elements of array 'array' will be default initialized
#if RADIANT_WINDOWS
# if _MSC_VER >= 1310
#  pragma warning(disable: 4351)
# endif
#endif

#ifdef MULTI_DOCUMENTER
#include "Serializer.hpp"
#include "XMLArchive.hpp"

// some forward declarations to work around cyclic include problems
namespace Valuable
{
  class Serializable;
  namespace Serializer
  {
    template <typename T>
    ArchiveElement serialize(Archive & archive, const T & t);
  }
}

#endif

namespace Valuable
{
  class Node;
  //class DOMElement;
  class DOMDocument;


  /// The base class for all serializable objects.
  class VALUABLE_API Serializable
  {
    MEMCHECKED
  public:
    virtual ~Serializable() {}

    /// Serializes (writes) this object to an element.
    /// @param archive The serializer archive that is used to create the new
    ///                element and maintains the serialization state and options.
    /// @return The new serialized element.
    virtual ArchiveElement serialize(Archive & archive) const = 0;

    /// Deserializes (reads) this object from serializer element.
    /// @param element Serialized element that holds the data that should be deserialized.
    /// @return Returns true if the read process worked correctly, and false otherwise.
    virtual bool deserialize(const ArchiveElement & element) = 0;

    /// Deserializes (reads) this object from an XML element.
    /// This function is only for keeping backwards compatibility.
    /// @param element XML element that is deserialized
    /** @return Returns true if the read process worked correctly, and false otherwise. */
    virtual bool deserializeXML(const DOMElement & element);
  };


  /** The base class for value objects.

      Typical child classes include some POD (plain old data) elements
      (floats, ints, vector2) etc, that can be accessed through the
      API. The Attributes have names (QString), that can be used to access
      Attributes that are stored inside Node objects.

      It is also possible to add listeners to values, so that if a
      value is changed, then a call-back to some other object is
      followed. The listener-API is a bit hard-core, but it has plenty
      of power when you need to track the state of other objects.

      @see Node
  */

  /// @todo the "set" functions are duplicating the processMessage functionality
  /// @todo processMessage should be renamed to eventProcess (can be tricky to do)
  /// @todo Doc
  class VALUABLE_API Attribute : public Serializable, public Patterns::NotCopyable
  {
  public:
    enum Layer {
      ORIGINAL = 0,
      STYLE,
      MANUAL,
      STYLE_IMPORTANT,

      LAYER_COUNT
    };

    /// Units of a value
    enum ValueUnit
    {
      VU_UNKNOWN,
      VU_PXS,         ///< Value defined in pixels
      VU_PERCENTAGE,  ///< Value defined in percentage (100% == 1.0f)
      VU_EMS,         ///< Length value defined by font-size
      VU_EXS          ///< Length value defined by x-height of the current font
    };

    typedef std::function<void ()> ListenerFunc;
    enum ListenerRole {
      DELETE_ROLE = 1 << 0,
      CHANGE_ROLE = 1 << 1,
      ALL_ROLES = (CHANGE_ROLE << 1) -1
    };

    Attribute();

    /// Create a copy of the given Attribute WITHOUT the link to host,
    /// listeners, or the attribute name. So only the values and transit
    /// parameter are copied.
    Attribute(const Attribute & o);

    /// @copydoc Attribute(const Attribute & o);
    const Attribute & operator = (const Attribute &);

    /** Constructs a new value object and attaches it to its host.

    @param host The host object. This object is automatically
    added to the host.

    @param name The name (or id) of this value. Names are typically
    human-readable. The names should not contain white-spaces
    as they may be used in XML files etc.

    @param transit Should value changes be transmitted forward. This
    is related to future uses, and can be largely ignored at the
    moment.
    */
    Attribute(Node * host, const QString & name, bool transit = false);
    virtual ~Attribute();

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
        void MyClass::processMessage(const QString & type, Radiant::BinaryData & data)
        {
          if(type == "jump")
            doJump();
          else if(type == "crawl") {
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
    virtual void processMessage(const QString &id, Radiant::BinaryData &data);
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
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a float, the default implementation returns 0.0f
    virtual float       asFloat(bool * const ok = 0) const;
    /// Converts the value object in an integer
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a int, the default implementation returns zero
    virtual int         asInt(bool * const ok = 0) const;
    /// Converts the value object to a string
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a string, the default implementation returns an empty string
    virtual QString asString(bool * const ok = 0) const;

    /// Sets the value of the object
    virtual bool set(float v, Layer layer = MANUAL, ValueUnit unit = VU_UNKNOWN);
    /// Sets the value of the object
    virtual bool set(int v, Layer layer = MANUAL, ValueUnit unit = VU_UNKNOWN);
    /// Sets the value of the object
    virtual bool set(const QString & v, Layer layer = MANUAL, ValueUnit unit = VU_UNKNOWN);
    /// Sets the value of the object
    virtual bool set(const Nimble::Vector2f & v, Layer layer = MANUAL, QList<ValueUnit> units = QList<ValueUnit>());
    /// Sets the value of the object
    virtual bool set(const Nimble::Vector3f & v, Layer layer = MANUAL, QList<ValueUnit> units = QList<ValueUnit>());
    /// Sets the value of the object
    virtual bool set(const Nimble::Vector4f & v, Layer layer = MANUAL, QList<ValueUnit> units = QList<ValueUnit>());
    /// Sets the value of the object
    virtual bool set(const QVariantList & v, QList<ValueUnit> unit, Layer layer = MANUAL);

    /// Get the type id of the type
    virtual const char * type() const = 0;

    /// The object is serialized using its name as a tag name.
    /// @param archive Serialization archive that is used to create new elements.
    /// @return Serialized object as an ArchiveElement
    virtual ArchiveElement serialize(Archive & archive) const OVERRIDE;

    /// The host object of the value object (is any).
    /// @return Pointer to the host
    Node * host() const { return m_host; }
    /** Sets the host pointer to zero and removes this object from the host. */
    void removeHost();

    /// Adds a listener that is invoked whenever the value is changed
    long addListener(ListenerFunc func, int role = CHANGE_ROLE);
    /// Adds a listener that is invoked whenever the value is changed
    /// The listener is removed when the listener object is deleted
    long addListener(Node * listener, ListenerFunc func, int role = CHANGE_ROLE);
#ifdef MULTI_WITH_V8
    long addListener(v8::Persistent<v8::Function> func, int role = CHANGE_ROLE);
#endif
    /// Removes listeners from the listener list
    void removeListeners(int role = ALL_ROLES);
    /// Removes a listener from the listener list
    void removeListener(Node * listener, int role = ALL_ROLES);
    void removeListener(long id);

    /// Returns true if the current value of the object is different from the original value.
    virtual bool isChanged() const;

    virtual void clearValue(Layer layout);

    virtual bool shortcut() const;

#ifdef MULTI_DOCUMENTER
    struct Doc
    {
      QString class_name;
      QString orig_str;
      Node * obj;
      Attribute * vo;
    };

    static std::list<Doc> doc;
#endif

    /// Gets an Attribute with the given name
    /// @param name Attribute name to search for
    /// @return Null if no object can be found
    virtual Attribute * getAttribute(const QString & name);
    /// @deprecated This function will be removed in Cornerstone 2.1. Use getAttribute instead.
    virtual Attribute * getValue(const QString & name);

  protected:

    /// Invokes the change valueChanged function of all listeners
    virtual void emitChange();
    /// Invokes the change valueDeleted function of all listeners
    virtual void emitDelete();

  private:
    // The object that holds this object
    Node * m_host;
    bool m_changed;
    QString m_name;
    bool m_transit;

    struct AttributeListener
    {
      AttributeListener(ListenerFunc func_, int role_, Node * listener_ = 0)
        : func(func_), role(role_), listener(listener_) {}

#ifdef MULTI_WITH_V8
      AttributeListener(v8::Persistent<v8::Function> func_, int role_)
        : func(), scriptFunc(func_), role(role_), listener(0) {}
#endif

      AttributeListener() : func(), role(), listener() {}

      ListenerFunc func;
#ifdef MULTI_WITH_V8
      v8::Persistent<v8::Function> scriptFunc;
#endif
      int role;
      Node * listener;
    };
    QMap<long, AttributeListener> m_listeners;
    long m_listenersId;

    friend class Node;
  };

  /// Every Attribute is some kind of AttributeT<T> object.
  /// Common functionality should be in either here or in Attribute
  template <typename T> class AttributeT : public Attribute
  {
  public:
    /// Creates a new AttributeT and stores the original and current value as a separate variables.
    /// @param host host object
    /// @param name name of the value
    /// @param v the default/original value of the object
    /// @param transit ignored
    AttributeT(Node * host, const QString & name, const T & v = T(), bool transit = false)
      : Attribute(host, name, transit),
      m_current(ORIGINAL),
      m_values(),
      m_valueSet()
    {
      m_values[ORIGINAL] = v;
      m_valueSet[ORIGINAL] = true;
#ifdef MULTI_DOCUMENTER
      Doc & d = doc.back();
      XMLArchive archive;
      ArchiveElement e = Serializer::serialize<T>(archive, orig());
      if(!e.isNull()) {
        d.orig_str = e.get();
      }
#endif
    }

    AttributeT()
      : Attribute(),
      m_current(ORIGINAL),
      m_values(),
      m_valueSet()
    {
      m_valueSet[ORIGINAL] = true;
    }

    virtual ~AttributeT() {}

    /// Access the wrapped object with the dereference operator
    inline const T & operator * () const { return value(); }

    /// Typecast operator for the wrapped value
    inline operator const T & () const { return value(); }
    /// Use the arrow operator to access fields inside the wrapped object.
    inline const T * operator->() const { return &value(); }

    /// The original value (the value given in constructor) of the Attribute.
    inline const T & orig() const { return m_values[ORIGINAL]; }

    inline const T & value(Layer layer) const { return m_values[layer]; }

    inline const T & value() const { return m_values[m_current]; }

    inline void setValue(const T & t, Layer layer = MANUAL)
    {
      bool top = layer >= m_current;
      bool sendSignal = top && value() != t;
      if(top) m_current = layer;
      m_values[layer] = t;
      m_valueSet[layer] = true;
      if (sendSignal) this->emitChange();
    }

    /// @todo should return the derived type, not AttributeT
    inline AttributeT<T> & operator = (const T & t)
    {
      setValue(t);
      return *this;
    }

    virtual void clearValue(Layer layout = MANUAL)
    {
      assert(layout > ORIGINAL);
      m_valueSet[layout] = false;
      if(m_current == layout) {
        assert(m_valueSet[ORIGINAL]);
        int l = int(layout) - 1;
        while(!m_valueSet[l]) --l;
        m_current = l;
        if(m_values[l] != m_values[layout])
          this->emitChange();
      }
    }

  protected:
    int m_current;
    T m_values[LAYER_COUNT];
    bool m_valueSet[LAYER_COUNT];
  };


}

#endif
