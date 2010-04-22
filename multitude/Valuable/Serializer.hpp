#ifndef VALUABLE_SERIALIZER_HPP
#define VALUABLE_SERIALIZER_HPP

#include "DOMElement.hpp"
#include "DOMDocument.hpp"
#include "ValueObject.hpp"
#include "XMLArchive.hpp"

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

  /// XML Serializer namespace that has handles the (de)serialize dispatching
  /** Correct way to save/load object state to/from XML is to use static
      serialize/deserialize methods.
  */
  namespace Serializer
  {
    /// Trait class for compile time separation of different kinds of serializable objects
    /** \code
        Trait<int>::type == Type::other
        Trait<ValueInt>::type == Type::serializable
        Trait<std::list<int> >::type == Type::container
        Trait<std::pair<int, int> >::type == Type::pair
        \endcode
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
    ArchiveElement & serialize(Archive &archive, T & t);

    template <typename T>
    typename remove_const<T>::Type deserialize(ArchiveElement & element);

    template <typename T>
    typename remove_const<T>::Type deserialize(DOMElement & element);

    /// Default implementation for "other" types.
    /// Implementations need to be inside of a struct because of partial template specialization
    template <typename T, int type_id = Trait<T>::type>
    struct Impl
    {
      static ArchiveElement & serialize(Archive &archive, T & t)
      {
        ArchiveElement & elem = archive.createElement(typeid(t).name());
        elem.set(Radiant::StringUtils::stringify(t));
        return elem;
      }
      static typename remove_const<T>::Type deserialize(ArchiveElement & element)
      {
        std::istringstream is(element.get());
        typename remove_const<T>::Type t;
        is >> t;
        return t;
      }
    };

    template <typename T>
    struct Impl<T, Type::serializable>
    {
      static ArchiveElement & serialize(Archive &doc, T & t)
      {
        return t.serialize(doc);
      }
      static T deserialize(ArchiveElement & element)
      {
        T t;
        t.deserialize(element);
        return t;
      }
    };

    template <typename T>
    struct Impl<T*, Type::serializable>
    {
      static ArchiveElement & serialize(Archive &doc, T * t)
      {
        return t->serialize(doc);
      }
      static T * deserialize(ArchiveElement & element)
      {
        T * t = new T;
        t->deserialize(element);
        return t;
      }
    };

    template <typename T>
    struct Impl<T, Type::pair>
    {
      static ArchiveElement & serialize(Archive &archive, T & pair)
      {
        ArchiveElement & elem = archive.createElement("pair");
        elem.add(Serializer::serialize(archive, pair.first));
        elem.add(Serializer::serialize(archive, pair.second));
        return elem;
      }
      static T deserialize(ArchiveElement & element)
      {
        typedef typename T::first_type A;
        typedef typename T::second_type B;
        /// @todo do not use xml()
        DOMElement::NodeList nodes = element.xml()->getChildNodes();
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
    ArchiveElement & serialize(Archive &archive, T & t)
    {
      return Impl<T>::serialize(archive, t);
    }

    template <typename T>
    typename remove_const<T>::Type deserialize(ArchiveElement & element)
    {
      return Impl<T>::deserialize(element);
    }

    template <typename T>
    typename remove_const<T>::Type deserialize(DOMElement & element)
    {
      XMLArchiveElement e(element);
      return deserialize<T>(e);
    }

    /// Serialize object to file
    template <typename T>
    bool serialize(const std::string & filename, T t)
    {
      XMLArchive archive;
      ArchiveElement & e = serialize(archive, t);
      if(e.isNull()) {
        return false;
      }
      archive.add(e);
      return archive.writeToFile(filename.c_str());
    }

    /// Deserialize object from file
    template <typename T>
    T deserialize(const std::string & filename)
    {
      XMLArchive archive;

      if(!archive.readFromFile(filename.c_str()))
        return T();

      ArchiveElement & e = archive.root();
      return deserialize<T>(e);
    }
  }
}
#endif // VALUABLE_SERIALIZER_HPP
