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
#include "ValueObject.hpp"

namespace Valuable
{
  /// Template class for all STL-like containers
  /**
    The container type can be nested STL-style container (like
    ValueContainer<std::map<int, std::vector<float> > >), it will be
    (de)serialized recursively.

    Example:
    \code
    typedef ValueContainer<std::list<int> > List;
    HasValue values;
    List list(values, "list");
    list->push_back(4);
    List::iterator it = list->begin();
    \endcode
  */
  template<typename T> class ValueContainer : public ValueObject
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

    ValueContainer() {}

    /// Constructs a new container
    /// @param parent parent object
    /// @param name name of the value
    ValueContainer(HasValues * parent, const std::string & name)
      : ValueObject(parent, name, false)
    {}

    virtual const char* type() const { return "container"; }

    virtual ArchiveElement & serialize(Archive & archive)
    {
      ArchiveElement & elem = archive.createElement((name().empty() ? type() : name()).c_str());
      for(iterator it = m_container.begin(); it != m_container.end(); it++) {
        elem.add(Serializer::serialize(archive, *it));
      }
      return elem;
    }

    virtual bool deserialize(ArchiveElement & element)
    {
      std::insert_iterator<T> inserter(m_container, m_container.end());
      for(ArchiveElement::Iterator & it = element.children(); it; ++it) {
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
    T * operator -> () { return &m_container; }
    /// @see operator->
    const T * operator -> () const { return &m_container; }

  protected:
    /// The actual container that this ValueContainer wraps.
    T m_container;
  };
}

#endif // VALUABLE_VALUE_CONTAINER_HPP
