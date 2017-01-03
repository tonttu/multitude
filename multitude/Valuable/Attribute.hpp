/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_ATTRIBUTE_HPP
#define VALUABLE_ATTRIBUTE_HPP

#include "Export.hpp"
#include "Archive.hpp"
#include "DOMElement.hpp"
#include "TransitionAnim.hpp"
#include "TransitionManager.hpp"

#include <Nimble/Vector4.hpp>

#include <Patterns/NotCopyable.hpp>

#include <Radiant/ArrayMap.hpp>
#include <Radiant/BinaryData.hpp>
#include <Radiant/MemCheck.hpp>
#include <Radiant/Flags.hpp>

#ifdef ENABLE_THREAD_CHECKS
#include <Radiant/Thread.hpp>
#endif

#include <Valuable/Archive.hpp>
#include <Valuable/Export.hpp>
#include <Valuable/Serializer.hpp>

#include <QString>
#include <QList>
#include <QMap>

#include <functional>

#ifdef CORNERSTONE_JS
#include <Valuable/v8.hpp>
#endif

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
  class StyleValue;

  /// The base class for all serializable objects.
  class VALUABLE_API Serializable
  {
    MEMCHECKED
  public:
    Serializable();
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

    /// @param v should this attribute be serialized with its host
    void setSerializable(bool v);
    /// @returns true if this attribute should be serialized with its host
    bool isSerializable() const;

  private:
    bool m_serializable;
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

  /// @todo Doc
  class VALUABLE_API Attribute : public Serializable
  {
  public:
    /// Attribute has multiple independent attribute values on LAYER_COUNT
    /// different layers. The real effective value of the attribute comes
    /// from an active layer that has the highest priority. By default only
    /// DEFAULT layer has value (is enabled). Layer numerical value also
    /// means the layer priority, DEFAULT layer has the lowest
    /// priority, while STYLE_IMPORTANT has the highest priority.
    enum Layer {
      DEFAULT = 0,       ///< Default, usually set in the constructor
      STYLE,             ///< Set from a CSS file
      USER,              ///< Set from a C++/JS code or by interaction / animators
      STYLE_IMPORTANT,   ///< !important rules from CSS file

      LAYER_COUNT,

      CURRENT_LAYER,     ///< This layer corresponds to the layer that has the
                         ///  highest priority among the layers having value set
      CURRENT_VALUE      ///< This pseudo-layer should used only for reading. It is
                         ///  different from the value in current layer only in the
                         ///  case where value is currently under the transition.
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

    /// Callback function type in the listener API
    typedef std::function<void ()> ListenerFunc;
    /// Different listener roles, used when adding a new listener
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

    @param name The name (or id) of this attribute. Names are typically
    human-readable. The names should not contain white-spaces
    as they may be used in XML files etc.

    @param transit Should value changes be transmitted forward. This
    is related to future uses, and can be largely ignored at the
    moment.
    */
    Attribute(Node * host, const QByteArray & name);
    virtual ~Attribute();

    /// Returns the name of the object.
    const QByteArray & name() const { return m_name; }
    /// Sets the name of the object
    void setName(const QByteArray & s);
    /// Returns the path (separated by '/'s) from the root
    QByteArray path() const;

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
        void MyClass::eventProcess(const QByteArray & type, Radiant::BinaryData & data)
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
            Parent::eventProcess(type, data);
        }
        \endcode

        @param id The indentifier for the message. Typically this is quite human-readable

        @param data Binary blob that contains the argument data in easily parseable format.

    */
    virtual void eventProcess(const QByteArray &id, Radiant::BinaryData &data);
    /// Utility function for sending string message to the object
    void eventProcessString(const QByteArray & id, const QString & str);
    /// Utility function for sending a float message to the object
    void eventProcessFloat(const char * id, float v);
    /// Utility function for sending an int message to the object
    void eventProcessInt(const char * id, int v);
    /// Utility function for sending a Nimble::Vector2f message to the object
    void eventProcessVector2(const char * id, Nimble::Vector2);
    /// Utility function for sending a Vector3 message to the object
    void eventProcessVector3(const char * id, Nimble::Vector3);
    /// Utility function for sending a Vector4 message to the object
    void eventProcessVector4(const char * id, Nimble::Vector4);

    /// Converts the value object in a floating point number
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a float, the default implementation returns 0.0f
    virtual float       asFloat(bool * const ok = 0, Layer layer = CURRENT_VALUE) const;
    /// Converts the value object in an integer
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a int, the default implementation returns zero
    virtual int         asInt(bool * const ok = 0, Layer layer = CURRENT_VALUE) const;
    /// Converts the value object to a string
    /// @param ok If non-null, *ok is set to true/false on success/error
    /// @return Object as a string, the default implementation returns an empty string
    virtual QString asString(bool * const ok = 0, Layer layer = CURRENT_VALUE) const;

    /// Sets the value of the object
    virtual bool set(float v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN);
    /// Sets the value of the object
    virtual bool set(int v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN);
    /// Sets the value of the object
    virtual bool set(const QString & v, Layer layer = USER, ValueUnit unit = VU_UNKNOWN);
    /// Sets the value of the object
    virtual bool set(const Nimble::Vector2f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>());
    /// Sets the value of the object
    virtual bool set(const Nimble::Vector3f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>());
    /// Sets the value of the object
    virtual bool set(const Nimble::Vector4f & v, Layer layer = USER, QList<ValueUnit> units = QList<ValueUnit>());
    /// Sets the value of the object
    virtual bool set(const StyleValue & value, Layer layer = USER);

    /// Get the type id of the attribute
    /// @return type of the attribute
    virtual QByteArray type() const { return QByteArray(); }

    virtual void copyValueFromLayer(Layer from, Layer to);

    /// The object is serialized using its name as a tag name.
    /// @param archive Serialization archive that is used to create new elements.
    /// @return Serialized object as an ArchiveElement
    virtual ArchiveElement serialize(Archive & archive) const OVERRIDE;

    /// The host object of the value object (is any).
    /// @return Pointer to the host
    Node * host() const { return m_host; }
    /** Sets the host pointer to zero and removes this object from the host. */
    void removeHost();

    /// Adds a listener that is invoked whenever the value is changed. Returns
    /// an id that can be used to remove the listener later.
    /// @param func listener function
    /// @param role when should the listener function be called
    /// @return listener id
    /// @sa removeListener
    long addListener(ListenerFunc func, int role = CHANGE_ROLE);

    /// Adds a listener that is invoked whenever the value is changed. Returns
    /// an id that can be used to remove the listener later.  The listener is
    /// removed when the listener object is deleted. This function can fail and
    /// return -1 if the target listener is being destroyed when this function
    /// is called.
    /// @param listener listener that owns the callback
    /// @param func listener function
    /// @param role when should the listener function be called
    /// @return listener id or -1 on failure
    /// @sa removeListener
    long addListener(Node * listener, ListenerFunc func, int role = CHANGE_ROLE);

#ifdef CORNERSTONE_JS
    /// Adds a JavaScript listener that is invoked whenever the value is changed
    /// @param func listener function
    /// @param role when should the listener function be called
    /// @return listener id that can be used to remove the listener with removeListener
    /// @sa removeListener
    long addListener(v8::Persistent<v8::Function> func, int role = CHANGE_ROLE);
#endif

    /// Removes listeners from the listener list
    void removeListeners(int role = ALL_ROLES);
    /// Removes a listener from the listener list
    /// @returns True if any items from the list were removed, otherwise false
    bool removeListener(Node * listener, int role = ALL_ROLES);
    /// Removes a listener from the listener list
    /// @param id listener id, same as the return value in addListener
    /// @returns True if any items from the list were removed, otherwise false
    bool removeListener(long id);

    /// @returns true if the current value of the object is different from
    ///          the default value. Default implementation always returns false
    virtual bool isChanged() const;

    /// Unsets the value from a specific layer
    /// @param layer layer to clear, must not be DEFAULT, since DEFAULT layer should always be set
    virtual void clearValue(Layer layer = USER);

    /// If attribute supports shorthand properties, this should be used to parse those.
    /// For example "background: url('image.png') repeat" is a shorhand for setting
    /// background-image to "url('image.png')" and background-repeat to "repeat".
    /// @param[in] value shorthand value
    /// @param[out] expanded shorthand property split to parts. With the previous
    ///             background-example map should have two values.
    /// @returns true if shorthand was successfully handled
    /// @sa http://www.w3.org/TR/CSS21/about.html#shorthand
    virtual bool handleShorthand(const Valuable::StyleValue & value,
                                 Radiant::ArrayMap<Valuable::Attribute*, Valuable::StyleValue> & expanded);


    /// Check if the given layer defines a value
    /// @param layer layer to check
    /// @return true if the given value defines a layer; otherwise false
    virtual bool isValueDefinedOnLayer(Layer layer) const;

    void setOwnerShorthand(Attribute * owner);
    Attribute * ownerShorthand() const;

#ifdef ENABLE_THREAD_CHECKS
    virtual void setOwnerThread(Radiant::Thread::id_t owner);
#endif

#ifdef MULTI_DOCUMENTER
    struct Doc
    {
      QString class_name;
      QString default_str;
      Node * obj;
      Attribute * vo;
    };

    static std::list<Doc> doc;
#endif

    /// Gets an Attribute with the given name
    /// @param name Attribute name to search for
    /// @return Null if no object can be found
    virtual Attribute * attribute(const QByteArray & name) const;

    /// Sets the current USER attribute value as the default value
    /// and clears the USER value.
    virtual void setAsDefaults() {}

    bool layerForSerialization(SerializationOptions flags, Layer & layer) const
    {
      if (flags.checkFlags(SerializationOptions::LAYER_STYLE_IMPORTANT) && isValueDefinedOnLayer(STYLE_IMPORTANT))
        layer = STYLE_IMPORTANT;
      else if (flags.checkFlags(SerializationOptions::LAYER_USER) && isValueDefinedOnLayer(USER))
        layer = USER;
      else if (flags.checkFlags(SerializationOptions::LAYER_STYLE) && isValueDefinedOnLayer(STYLE))
        layer = STYLE;
      else if (flags.checkFlags(SerializationOptions::LAYER_DEFAULT) && isValueDefinedOnLayer(DEFAULT))
        layer = DEFAULT;
      else
        return false;
      return true;
    }

    virtual void setTransitionAnim(float durationSeconds, float delaySeconds);

    /// Invokes the change valueChanged function of all listeners
    virtual void emitChange();

    /// Invoked when animated value is set. Can dynamically be adjusted by setting
    /// callback using "onAnimatedValueSet". Meant to use with attributes that are
    /// created by composition from multiple attributes
    void animatedValueSet(float dt);
    void onAnimatedValueSet(const std::function<void(Attribute*, float)> &f);

  protected:

    /// Invokes the change valueDeleted function of all listeners
    virtual void emitDelete();

  private:
    // The object that holds this object
    Node * m_host;
    Attribute * m_ownerShorthand;
    QByteArray m_name;
    std::function<void(Attribute*,float)> m_onAnimation;

    struct AttributeListener
    {
      AttributeListener(ListenerFunc func_, int role_, Node * listener_ = 0)
        : func(func_), role(role_), listener(listener_) {}

#ifdef CORNERSTONE_JS
      AttributeListener(v8::Persistent<v8::Function> func_, int role_)
        : func(), scriptFunc(func_), role(role_), listener(0) {}
#endif

      AttributeListener() : func(), role(), listener() {}

      ListenerFunc func;
#ifdef CORNERSTONE_JS
      v8::Persistent<v8::Function> scriptFunc;
#endif
      int role;
      Node * listener;
    };
    QMap<long, AttributeListener> m_listeners;
    long m_listenersId;

    friend class Node;

#ifdef ENABLE_THREAD_CHECKS
  public:
    Radiant::Thread::id_t m_ownerThread = nullptr;
#endif
  };

  /// Every Attribute is some kind of AttributeT<T> object.
  /// Common functionality should be in either here or in Attribute
  template <typename T>
  class AttributeBaseT : public Attribute
  {
  public:
    /// Creates a new AttributeT and stores the default and current value as a separate variables.
    /// @param host host object
    /// @param name name of the value
    /// @param v the default value of the object
    /// @param transit ignored
    AttributeBaseT(Node * host, const QByteArray & name, const T & v = T())
      : Attribute(host, name),
      m_transition(nullptr),
      m_currentValue(v),
      m_currentLayer(DEFAULT),
      m_values(),
      m_valueSet()
    {
      m_values[DEFAULT] = v;
      m_valueSet[DEFAULT] = true;
#ifdef MULTI_DOCUMENTER
      Doc & d = doc.back();
      XMLArchive archive;
      ArchiveElement e = Serializer::serialize<T>(archive, defaultValue());
      if(!e.isNull()) {
        d.default_str = e.get();
      }
#endif
    }

    AttributeBaseT()
      : Attribute(),
      m_transition(nullptr),
      m_currentValue(),
      m_currentLayer(DEFAULT),
      m_values(),
      m_valueSet()
    {
      m_valueSet[DEFAULT] = true;
    }

    virtual ~AttributeBaseT() {
      if (m_transition)
        m_transition->remove();
    }

    /// Access the wrapped object with the dereference operator
    inline const T & operator * () const { return value(); }

    /// Typecast operator for the wrapped value
    inline operator const T & () const { return value(); }
    /// Use the arrow operator to access fields inside the wrapped object.
    inline const T * operator->() const { return &value(); }

    /// The default value (the value given in constructor) of the Attribute.
    inline const T & defaultValue() const { return m_values[DEFAULT]; }

    /// @param layer layer to use
    /// @returns attribute value on given layer
    inline const T & value(Layer layer) const {
      if(layer == CURRENT_VALUE) return value();
      return m_values[layer == CURRENT_LAYER ? currentLayer() : layer];
    }

    /// @returns attribute active value
    inline const T & value() const { return m_currentValue; }

    /// @returns the active layer that has the highest priority
    Layer currentLayer() const { return m_currentLayer; }


    /// Sets a new value for a specific layer. If currentLayer() is more important
    /// than the layer given as a parameter, the active value of the attribute
    /// won't change. Otherwise, if the value does change, this function will
    /// trigger all listeners callbacks.
    /// @param t new value for the attribute
    /// @param layer value will be set to this layer
    inline void setValue(const T & t, Layer layer = USER)
    {
      layer = layer == CURRENT_LAYER ? currentLayer() : layer;
      bool top = layer >= m_currentLayer;
      bool sendSignal = top && value() != t;
      if(top) m_currentLayer = layer;
      m_values[layer] = t;
      m_valueSet[layer] = true;
      if (sendSignal) this->emitChange();
    }

    /// Calls setValue with USER layer
    /// @param t new value for the attribute
    /// @returns reference to this
    inline AttributeBaseT<T> & operator = (const T & t)
    {
      setValue(t);
      return *this;
    }

    virtual bool isChanged() const OVERRIDE
    {
      return m_currentLayer > DEFAULT && value() != value(DEFAULT);
    }

    /// Unsets the value from a specific layer
    /// @param layer layer to clear, must not be DEFAULT, since DEFAULT layer should always be set
    virtual void clearValue(Layer layout = USER) OVERRIDE
    {
      assert(layout > DEFAULT);
      m_valueSet[layout] = false;
      if(m_currentLayer == layout) {
        assert(m_valueSet[DEFAULT]);
        int l = int(layout) - 1;
        while(!m_valueSet[l]) --l;
        m_currentLayer = Layer(l);
        if(m_values[l] != m_values[layout])
          this->emitChange();
      }
    }

    virtual void setAsDefaults() OVERRIDE
    {
      if (!m_valueSet[USER])
        return;
      setValue(value(USER), DEFAULT);
      clearValue(USER);
    }

    virtual QString asString(bool * const ok, Layer layer) const OVERRIDE = 0;

    virtual ArchiveElement serialize(Archive & archive) const OVERRIDE
    {
      Layer layer;
      if (!layerForSerialization(archive, layer))
        return ArchiveElement();
      ArchiveElement e = Serializer::serialize(archive, value(layer));
      if (e.isNull())
        return e;
      if (!name().isEmpty())
        e.setName(name());
      e.add("type", type());
      return e;
    }

    virtual bool deserialize(const ArchiveElement & element) OVERRIDE
    {
      setValue(Serializer::deserialize<T>(element));
      return true;
    }

    /// @param layer layer to check
    /// @returns true if layer is active
    virtual bool isValueDefinedOnLayer(Layer layer) const FINAL
    {
      return m_valueSet[layer];
    }

    virtual void setTransitionAnim(float durationSeconds, float delaySeconds) FINAL
    {
      m_transitionCurve.duration = durationSeconds;
      m_transitionCurve.delay = delaySeconds;
      if(m_transition) {
        m_transition->setParameters(m_transitionCurve);
      }
    }

    void setAnimatedValue(T t, float dt)
    {
      if (m_currentValue != t) {
        m_currentValue = t;
        Attribute::animatedValueSet(dt);
      }
    }

    virtual void copyValueFromLayer(Layer from, Layer to) OVERRIDE
    {
      const T & val = value(from);
      setValue(val, to);
    }

    inline void updateTransitionPointer(TransitionAnim* ptr)
    {
      m_transition = ptr;
    }

    bool hasActiveAnimation(float dt) const
    {
      if(!m_transition)
        return false;
      auto *tData = m_transition->typeData();
      if(!tData)
        return false;
      return m_transition->normalizedPos() + dt >= 0.f && !tData->deleted;
    }

  protected:
    virtual void emitChange() FINAL
    {
      if(m_transitionCurve.isValid()) {

        if(!m_transition) {
          m_transition = TransitionManagerT<T>::create(this, m_transitionCurve);
        }
        TransitionManagerT<T>::setTarget(m_transition, m_values[m_currentLayer]);

      } else {
        m_currentValue = m_values[m_currentLayer];
        Attribute::emitChange();
      }
    }

  private:
    TransitionAnim* m_transition;
    TransitionCurve m_transitionCurve;
    T m_currentValue;
    Layer m_currentLayer;
    T m_values[LAYER_COUNT];
    bool m_valueSet[LAYER_COUNT];
  };

  template <typename T, typename Select = void>
  class AttributeT;

#ifdef CORNERSTONE_JS

  /// @cond

  /// @todo This should be removed and all v8 related things should be implemented
  ///       the same way as Scripting::eventAddListener is implemented.
  VALUABLE_API extern v8::Persistent<v8::Context> s_defaultV8Context;

  /// @endcond

#endif
}

#include "TransitionImpl.hpp"

#endif
