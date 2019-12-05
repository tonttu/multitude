/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef VALUABLE_VALUE_CONTAINER_HPP
#define VALUABLE_VALUE_CONTAINER_HPP

#include "Serializer.hpp"
#include "Attribute.hpp"
#include <iterator>

namespace
{
  template <typename Container, typename T>
  struct NotNullInserter
  {
    static void insert(std::insert_iterator<Container> & inserter, const T & t)
    {
      *inserter = t;
    }
  };

  template <typename Container, typename T>
  struct NotNullInserter<Container, T*>
  {
    static void insert(std::insert_iterator<Container> & inserter, T * t)
    {
      if(t)
        *inserter = t;
    }
  };

  template <typename Container, typename T>
  struct NotNullInserter<Container, std::shared_ptr<T>>
  {
    static void insert(std::insert_iterator<Container> & inserter, const std::shared_ptr<T> & t)
    {
      if(t)
        *inserter = t;
    }
  };

  // Container elements that are not inherited from Serializable are always serializable
  template<typename T>
  typename std::enable_if<!std::is_base_of<Valuable::Serializable, T>::value, bool>::type
  isElementSerializable(const T& )
  {
    return true;
  }

  // Container elements that are inherited from Serializable have a flag we can check
  template<typename T>
  typename std::enable_if<std::is_base_of<Valuable::Serializable, T>::value, bool>::type
  isElementSerializable(const T& element)
  {
    return element.isSerializable();
  }

  // Utility function to pass pointer/reference parameters correctly (non-pointer)
  template<typename T>
  typename std::enable_if<!std::is_pointer<T>::value, bool>::type isElementSerializableForwarder(const T& element)
  {
    return isElementSerializable(element);
  }

  // Utility function to pass pointer/reference parameters correctly (pointer)
  template<typename T>
  bool isElementSerializableForwarder(const T* element)
  {
    return isElementSerializableForwarder(*element);
  }

  // Utility function to check both elements of the pair
  template<typename T, typename U>
  bool isElementSerializableForwarder(const std::pair<T,U>& pair)
  {
    return isElementSerializableForwarder(pair.first) &&
        isElementSerializableForwarder(pair.second);
  }


}

namespace Valuable
{

  /// This class is a base class for wrapping STL-like containers into attributes.
  /// @sa Valuable::AttributeContainer
  template<typename T> class AttributeContainerT : public Attribute
  {
  public:
    /// STL-compatible iterator
    typedef typename T::iterator iterator;
    /// STL-compatible const iterator
    typedef typename T::const_iterator const_iterator;
    /// STL-compatible reverse iterator
    typedef typename T::reverse_iterator reverse_iterator;
    /// STL-compatible const reverse iterator
    typedef typename T::const_reverse_iterator const_reverse_iterator;
    /// STL-like typedef for value type inside of T
    typedef typename T::value_type value_type;

    typedef T container_type;

    virtual ArchiveElement serialize(Archive & archive) const OVERRIDE
    {
      QString elementName = name().isEmpty() ? "AttributeContainerT" : name();

      ArchiveElement elem = archive.createElement(elementName);
      for(const_iterator it = m_container.begin(); it != m_container.end(); ++it) {

        if(isElementSerializableForwarder(*it)) {
          auto e = Serializer::serialize(archive, *it);
          if (!e.isNull())
            elem.add(e);
        }
      }
      return elem;
    }

    virtual bool deserialize(const ArchiveElement & element) OVERRIDE
    {
      if(m_clearOnDeserialize) m_container.clear();
      std::insert_iterator<T> inserter(m_container, m_container.end());
      for(ArchiveElement::Iterator it = element.children(); it; ++it) {
        NotNullInserter<T, typename T::value_type>::insert(inserter, Serializer::deserialize<typename T::value_type>(*it));
      }
      Attribute::emitChange();
      return true;
    }

    /// Typecast operator for the wrapped container
    operator T & () { return m_container; }
    /// Typecast operator for the const reference of the wrapped container
    operator const T & () const { return m_container; }

    /// Returns a reference to the wrapped container
    T & operator * () { return m_container; }
    /// Returns a const reference to the wrapped container
    const T & operator * () const { return m_container; }

    /// Use the arrow operator for accessing fields inside the wrapper container.
    /// Example: container->end();
    /// @return Pointer to the wrapped container
    T * operator -> () { return &m_container; }
    /// @copydoc operator->()
    const T * operator -> () const { return &m_container; }

    void setClearOnDeserialize(bool v) { m_clearOnDeserialize = v; }
    bool clearOnDeserialize() const { return m_clearOnDeserialize; }

    // Make public to allow container transmission to be controlled from
    // outside (to make AttributeContainers usable over Mushy 1.5)
    virtual void emitChange() OVERRIDE { Attribute::emitChange(); }

    void setValue(const T & t) { m_container = t; }
    const T & value() const { return m_container; }

    virtual QByteArray type() const override
    {
      return m_type.isNull() ? Radiant::StringUtils::type<T>() : m_type;
    }

  protected:
    AttributeContainerT(const QByteArray & type)
      : m_clearOnDeserialize(true)
      , m_type(type)
    {}

    /// Constructs a new container
    /// @param host host object
    /// @param name name of the value
    AttributeContainerT(Node * host, const QByteArray & name, const QByteArray & type)
      : Attribute(host, name)
      , m_clearOnDeserialize(true)
      , m_type(type)
    {}

    /// The actual container that this AttributeContainer wraps.
    T m_container;

    bool m_clearOnDeserialize;

    QByteArray m_type;
  };

  /// Template class for all STL-like containers
  /**
    The container type can be nested STL-style container (like
    AttributeContainer<std::map<int, std::vector<float> > >), it will be
    (de)serialized recursively.

    Example:
    \code
    typedef AttributeContainer<std::list<int> > List;
    HasValue values;
    List list(values, "list");
    list->push_back(4);
    List::iterator it = list->begin();
    \endcode
  */
  template<typename T> class AttributeContainer : public AttributeContainerT<T>
  {
  public:
    AttributeContainer(const QByteArray & type = QByteArray())
      : AttributeContainerT<T>(type)
    {}

    /// Constructs a new container
    /// @param host host object
    /// @param name name of the value
    /// @param type see Attribute::type
    AttributeContainer(Node * host, const QByteArray & name, const QByteArray & type = QByteArray())
      : AttributeContainerT<T>(host, name, type)
    {}
  };

  /// This class handles container attributes. It allows us to use
  /// std-containers and Qt containers as attributes.
  template <typename Key, typename T, typename Compare, typename Allocator>
  class AttributeContainer<std::map<Key, T, Compare, Allocator> >
    : public AttributeContainerT<std::map<Key, T, Compare, Allocator> >
  {
  public:
    typedef AttributeContainerT<std::map<Key, T, Compare, Allocator> > Container;

    /// Constructs a new container
    AttributeContainer(const QByteArray & type = QByteArray())
      : Container(type)
    {}

    /// Constructs a new container
    /// @param host host object
    /// @param name name of the value
    AttributeContainer(Node * host, const QByteArray & name, const QByteArray & type = QByteArray())
      : Container(host, name, type)
    {}

    virtual bool deserialize(const ArchiveElement & element) OVERRIDE
    {
      if(Container::m_clearOnDeserialize) Container::m_container.clear();
      for(ArchiveElement::Iterator it = element.children(); it; ++it) {
        typename Container::value_type p = Serializer::deserialize<typename Container::value_type>(*it);
        Container::m_container[p.first] = std::move(p.second);
      }
      Attribute::emitChange();
      return true;
    }
  };
}

#endif // VALUABLE_VALUE_CONTAINER_HPP
