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

#ifndef VALUABLE_VALUE_CONTAINER_HPP
#define VALUABLE_VALUE_CONTAINER_HPP

#include "Serializer.hpp"
#include "AttributeObject.hpp"
#include <iterator>

namespace Valuable
{
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
  template<typename T> class AttributeContainer : public Attribute
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

    AttributeContainer() {}

    /// Constructs a new container
    /// @param host host object
    /// @param name name of the value
    AttributeContainer(Node * host, const QString & name)
      : Attribute(host, name, false)
    {}

    virtual const char* type() const { return "container"; }

    virtual ArchiveElement serialize(Archive & archive) const
    {
      ArchiveElement elem = archive.createElement((name().isEmpty() ? type() : name()).toUtf8().data());
      for(const_iterator it = m_container.begin(); it != m_container.end(); it++) {
        elem.add(Serializer::serialize(archive, *it));
      }
      return elem;
    }

    virtual bool deserialize(const ArchiveElement & element)
    {
      std::insert_iterator<T> inserter(m_container, m_container.end());
      for(ArchiveElement::Iterator it = element.children(); it; ++it) {
        *inserter = Serializer::deserialize<typename T::value_type>(*it);
      }
      return true;
    }

    virtual bool isChanged() const { return !m_container.empty(); }

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

  protected:
    /// The actual container that this AttributeContainer wraps.
    T m_container;
  };
}

#endif // VALUABLE_VALUE_CONTAINER_HPP
