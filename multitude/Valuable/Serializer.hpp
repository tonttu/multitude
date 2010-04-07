#ifndef VALUABLE_SERIALIZER_HPP
#define VALUABLE_SERIALIZER_HPP

#include "DOMElement.hpp"
#include "DOMDocument.hpp"
#include "ValueObject.hpp"

#include "Radiant/StringUtils.hpp"

#include <typeinfo>

namespace Valuable
{
  /// helper structs and enum for template specializations
  /** Boost & TR1 have is_base_of & similar template hacks that basically do this */
  namespace Type
  {
    enum { pair = 1, container = 2, serializable = 3, other = 4 };
    // All structs have same _ element with different size, so we can use
    // sizeof operator to make decisions in templates
    struct pair_trait { char _[1]; };
    struct container_trait { char _[2]; };
    struct serializable_trait { char _[3]; };
    struct default_trait { char _[4]; };
  }

  /// Removes const from type: remove_const<const Foo>::Type == Foo
  template <typename T> struct remove_const { typedef T Type; };
  template <typename T> struct remove_const<const T> { typedef T Type; };

  /// XML Serializer namespace that has handles the (de)serializeXML dispatching
  /** Correct way to save/load object state to/from XML is to use static
      serializeXML/deserializeXML methods.
  */
  namespace Serializer
  {
    /// Trait class for compile time separation of different kinds of serializable objects
    /** Usage: Trait<int>::type == Type::other
               Trait<ValueInt>::type == Type::serializable
               Trait<std::list<int> >::type == Type::container
               Trait<std::pair<int, int> >::type == Type::pair
    */
    template <typename T> class Trait
    {
      static const T & t();

      template <typename Y> static Type::pair_trait test(typename Y::first_type*);
      template <typename Y> static Type::container_trait test(typename Y::value_type*);
      // the default one, if nothing matches
      template <typename Y> static Type::default_trait test(...);

      template <typename Y> static Type::serializable_trait s_test(const Serializable&);
      template <typename Y> static Type::serializable_trait s_test(const Serializable*);
      template <typename Y> static Type::default_trait s_test(...);

    public:
      enum { type = sizeof(s_test<T>(t())._) == sizeof(Type::serializable_trait)
                    ? sizeof(Type::serializable_trait) : sizeof(test<T>(0)._) };
    };

    template <typename T>
    DOMElement serializeXML(DOMDocument *doc, T & t);

    template <typename T>
    T deserializeXML(const DOMElement & element);


    /// Default implementation for "other" types.
    /// Implementations need to be inside of a struct because of partial template specialization
    template <typename T, int type_id = Trait<T>::type>
    struct Impl
    {
      static DOMElement serializeXML(DOMDocument * doc, T & t)
      {
        DOMElement elem = doc->createElement(typeid(t).name());
        elem.setTextContent(Radiant::StringUtils::stringify(t));
        return elem;
      }
      static T deserializeXML(const DOMElement & element)
      {
        std::istringstream is(element.getTextContent());
        typename remove_const<T>::Type t;
        is >> t;
        return t;
      }
    };

    template <typename T>
    struct Impl<T, Type::serializable>
    {
      static DOMElement serializeXML(DOMDocument * doc, T & t)
      {
        return t.serializeXML(doc);
      }
      static T deserializeXML(const DOMElement & element)
      {
        T t;
        t.deserializeXML(element);
        return t;
      }
    };

    template <typename T>
    struct Impl<T*, Type::serializable>
    {
      static DOMElement serializeXML(DOMDocument * doc, T * t)
      {
        return t->serializeXML(doc);
      }
      static T * deserializeXML(const DOMElement & element)
      {
        T * t = new T;
        t->deserializeXML(element);
        return t;
      }
    };

    template <typename T>
    struct Impl<T, Type::pair>
    {
      static DOMElement serializeXML(DOMDocument * doc, T & pair)
      {
        DOMElement elem = doc->createElement("pair");
        elem.appendChild(Serializer::serializeXML(doc, pair.first));
        elem.appendChild(Serializer::serializeXML(doc, pair.second));
        return elem;
      }
      static T deserializeXML(const DOMElement & element)
      {
        typedef typename T::first_type A;
        typedef typename T::second_type B;
        DOMElement::NodeList nodes = element.getChildNodes();
        if(nodes.size() == 2) {
          return std::make_pair(Serializer::deserializeXML<A>(nodes.front()),
                                Serializer::deserializeXML<B>(nodes.back()));
        } else {
          Radiant::error("pair size is not 2");
          return T();
        }
      }
    };

    template <typename T>
    DOMElement serializeXML(DOMDocument *doc, T & t)
    {
      return Impl<T>::serializeXML(doc, t);
    }

    template <typename T>
    T deserializeXML(const DOMElement & element)
    {
      return Impl<T>::deserializeXML(element);
    }

    /// Serialize object to file
    template <typename T>
    bool serializeXML(const std::string & filename, T t)
    {
      Radiant::RefPtr<DOMDocument> doc = DOMDocument::createDocument();
      DOMElement e = serializeXML(doc.ptr(), t);
      if(e.isNull()) {
        return false;
      }
      doc->appendChild(e);
      return doc->writeToFile(filename.c_str());
    }

    /// Deserialize object from file
    template <typename T>
    T deserializeXML(const std::string & filename)
    {
      Radiant::RefPtr<DOMDocument> doc = DOMDocument::createDocument();

      if(!doc->readFromFile(filename.c_str()))
        return T();

      DOMElement e = doc->getDocumentElement();
      return deserializeXML<T>(e);
    }
  }
}
#endif // VALUABLE_SERIALIZER_HPP
