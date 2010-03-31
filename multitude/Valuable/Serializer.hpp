#ifndef VALUABLE_SERIALIZER_HPP
#define VALUABLE_SERIALIZER_HPP

#include "DOMElement.hpp"
#include "DOMDocument.hpp"
#include "ValueObject.hpp"

#include "Radiant/StringUtils.hpp"

#include <typeinfo>

namespace Valuable
{
  // Boost & TR1 have is_base_of & similar template hacks that basically do this
  namespace Type
  {
    enum { pair = 1, container = 2, value_object = 3 };
    struct pair_trait { char _[1]; };
    struct container_trait { char _[2]; };
    struct value_object_trait { char _[3]; };
    struct default_trait { char _[4]; };
  }

  template <typename T> struct remove_const { typedef T Type; };
  template <typename T> struct remove_const<const T> { typedef T Type; };

  class Serializer
  {
  public:

    // Temporary solution
    struct Struct
    {
      virtual DOMElement serializeXML(DOMDocument * doc) = 0;
      virtual bool deserializeXML(DOMElement element) = 0;
    };

    template <typename T> class Trait
    {
      static const T & t();

      template <typename Y> static Type::pair_trait test(typename Y::first_type*);
      template <typename Y> static Type::container_trait test(typename Y::value_type*);

      // the default one, if nothing matches
      template <typename Y> static Type::default_trait test(...);

      template <typename Y> static Type::value_object_trait vo_test(const ValueObject&);
      template <typename Y> static Type::value_object_trait vo_test(const ValueObject*);
      template <typename Y> static Type::value_object_trait vo_test(const Struct&);
      template <typename Y> static Type::value_object_trait vo_test(const Struct*);
      template <typename Y> static Type::default_trait vo_test(...);

    public:
      enum { type = sizeof(vo_test<T>(t())._) == 3 ? 3 : sizeof(test<T>(0)._) };
    };

    template <typename T, int type_id = Trait<T>::type>
    struct ImplSer
    {
      static DOMElement serializeXML(DOMDocument * doc, T & t)
      {
        DOMElement elem = doc->createElement(typeid(t).name());
        elem.setTextContent(Radiant::StringUtils::stringify(t));
        return elem;
      }
    };

    template <typename T>
    struct ImplSer<T, Type::value_object>
    {
      static DOMElement serializeXML(DOMDocument * doc, T & t)
      {
        return t.serializeXML(doc);
      }
    };

    template <typename T>
    struct ImplSer<T*, Type::value_object>
    {
      static DOMElement serializeXML(DOMDocument * doc, T * t)
      {
        return t->serializeXML(doc);
      }
    };

    template <typename T>
    struct ImplSer<T, Type::pair>
    {
      static DOMElement serializeXML(DOMDocument * doc, T & pair)
      {
        DOMElement elem = doc->createElement("pair");
        elem.appendChild(Serializer::serializeXML(doc, pair.first));
        elem.appendChild(Serializer::serializeXML(doc, pair.second));
        return elem;
      }
    };

    template <typename T>
    static DOMElement serializeXML(DOMDocument *doc, T & t)
    {
      return ImplSer<T>::serializeXML(doc, t);
    }

    template <typename T>
    static bool serializeXML(const std::string & filename, T & t)
    {
      Radiant::RefPtr<DOMDocument> doc = DOMDocument::createDocument();
      DOMElement e = serializeXML(doc.ptr(), t);
      if(e.isNull()) {
        return false;
      }
      doc->appendChild(e);
      return doc->writeToFile(filename.c_str());
    }

    ///////////////////////////////////////////////////////////////////////////

    template <typename T>
    static T deserializeXML(const std::string & filename)
    {
      Radiant::RefPtr<DOMDocument> doc = DOMDocument::createDocument();

      if(!doc->readFromFile(filename.c_str()))
        return T();

      DOMElement e = doc->getDocumentElement();
      return deserialize<T>(e);
    }

    template <typename T, int type_id = Trait<T>::type>
    struct ImplDe
    {
      static T deserialize(const DOMElement & element)
      {
        std::istringstream is(element.getTextContent());
        typename remove_const<T>::Type t;
        is >> t;
        return t;
      }
    };

    template <typename T>
    struct ImplDe<T, Type::value_object>
    {
      static T deserialize(const DOMElement & element)
      {
        T t;
        t.deserializeXML(element);
        return t;
      }
    };

    template <typename T>
    struct ImplDe<T*, Type::value_object>
    {
      static T * deserialize(const DOMElement & element)
      {
        T * t = new T;
        t->deserializeXML(element);
        return t;
      }
    };

    template<typename T>
    struct ImplDe<T, Type::pair>
    {
      static T deserialize(const DOMElement & element)
      {
        typedef typename T::first_type A;
        typedef typename T::second_type B;
        DOMElement::NodeList nodes = element.getChildNodes();
        if(nodes.size() == 2) {
          return std::make_pair(Serializer::deserialize<A>(nodes.front()),
                                Serializer::deserialize<B>(nodes.back()));
        } else {
          Radiant::error("pair size is not 2");
          return T();
        }
      }
    };

    template <typename T>
    static T deserialize(const DOMElement & element)
    {
      return ImplDe<T>::deserialize(element);
    }
  };


}
#endif // VALUABLE_SERIALIZER_HPP
