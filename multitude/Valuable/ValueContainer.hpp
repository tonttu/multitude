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
    typedef typename T::iterator iterator;
    typedef typename T::const_iterator const_iterator;
    typedef typename T::reverse_iterator reverse_iterator;
    typedef typename T::value_type value_type;

    ValueContainer() {}

    ValueContainer(HasValues * parent, const std::string & name)
      : ValueObject(parent, name, false)
    {}

    virtual const char* type() const { return "container"; }

    virtual ArchiveElement & serialize(Archive &doc)
    {
/*      DOMElement elem = doc->createElement((name().empty() ? type() : name()).c_str());
      for(iterator it = m_container.begin(); it != m_container.end(); it++) {
        elem.appendChild(Serializer::serialize(doc, *it));
      }
      return elem;*/
      return doc.emptyElement();
    }

    virtual bool deserialize(ArchiveElement & element)
    {
      DOMElement::NodeList list = element.xml()->getChildNodes();
      std::insert_iterator<T> inserter(m_container, m_container.end());
      for(DOMElement::NodeList::iterator it = list.begin(); it != list.end(); it++) {
        //*inserter = Serializer::deserialize<typename T::value_type>(*it);
      }
      return true;
    }

    /// @todo should be in ValueTyped
    operator T & () { return m_container; }
    operator const T & () const { return m_container; }

    T & operator * () { return m_container; }
    const T & operator * () const { return m_container; }

    T * operator -> () { return &m_container; }
    const T * operator -> () const { return &m_container; }

  protected:
    T m_container;
  };
}

#endif // VALUABLE_VALUE_CONTAINER_HPP
