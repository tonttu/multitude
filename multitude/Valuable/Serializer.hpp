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

#ifndef VALUABLE_SERIALIZER_HPP
#define VALUABLE_SERIALIZER_HPP

#include "DOMElement.hpp"
#include "DOMDocument.hpp"
#include "AttributeObject.hpp"
#include "XMLArchive.hpp"

#include <Radiant/StringUtils.hpp>
#include <Radiant/Trace.hpp>

#include <typeinfo>

namespace Valuable
{
  class Serializable;

  /// XML Serializer namespace that has handles the (de)serialize dispatching
  /** Correct way to save/load object state to/from XML is to use static
      serialize/deserialize methods.
  */
  namespace Serializer
  {

    /// @cond

    VALUABLE_API QString tagName(const std::type_info & typeinfo);

    template <typename T>
    QString tagName()
    {
      return tagName(typeid(T));
    }

    /// helper structs and enum for template specializations
    /// Boost & TR1 have is_base_of & similar template hacks that basically do this
    namespace Type
    {
      /// Every object is one of these types, see Serializer::Trait
      enum { pair = 1, container = 2, serializable = 3, smart_ptr = 4, other = 5 };
      // All structs have same _ element with different size, so we can use
      // sizeof operator to make decisions in templates
      struct pair_trait { char _[1]; };
      struct container_trait { char _[2]; };
      struct serializable_trait { char _[3]; };
      struct smart_ptr_trait { char _[4]; };
      struct default_trait { char _[5]; };
    }

    /// @endcond

    /// Removes const from type: remove_const<const Foo>::Type == Foo
    /// Works also with non-const types, remove_const<Foo>::Type == Foo
    template <typename T> struct remove_const {
      /// The original type without const
      typedef T Type;
    };
    /// Removes const from type: remove_const<const Foo>::Type == Foo
    /// Works also with non-const types, remove_const<Foo>::Type == Foo
    template <typename T> struct remove_const<const T> {
      /// The original type without const
      typedef T Type;
    };

    /// Trait class for compile time separation of different kinds of serializable objects
    /** \code
        Trait<int>::type == Type::other
        Trait<AttributeInt>::type == Type::serializable
        Trait<std::list<int> >::type == Type::container
        Trait<std::pair<int, int> >::type == Type::pair
        Trait<Radiant::IntrusivePtr<T> >::type == Type::smart_ptr
        \endcode
    */
    template <typename T> class Trait
    {
      static const T & t();

      template <typename Y> static Type::pair_trait test(typename Y::first_type*);
      template <typename Y> static Type::container_trait test(typename Y::value_type*);
      template <typename Y> static Type::smart_ptr_trait test(typename Y::element_type*);
      // the default one, if nothing matches
      template <typename Y> static Type::default_trait test(...);

      template <typename Y> static Type::serializable_trait s_test(const Serializable&);
      template <typename Y> static Type::serializable_trait s_test(const Serializable*);
      template <typename Y> static Type::default_trait s_test(...);

    public:
      /// @cond
      enum { type = sizeof(test<T>(t())._) == sizeof(Type::smart_ptr_trait)
             ? sizeof(Type::smart_ptr_trait)
             : sizeof(s_test<T>(t())._) == sizeof(Type::serializable_trait)
             ? sizeof(Type::serializable_trait)
             : sizeof(test<T>(0)._) };
      /// @endcond
    };

    /// @cond

    /// FactoryInfo<T>::have_create is true, if there is T* T::create(const ArchiveElement &)
    template <typename T> struct FactoryInfo {
      typedef T * (*Func)(const ArchiveElement &);
      //typedef Radiant::IntrusivePtr<T> (*Func2)(ArchiveElement &);
      template <Func> struct Test {};
      //template <Func2> struct Test2 {};
      template <class C> static char test(Test<&C::create>*);
      //template <class C> static char test(Test2<&C::create>*);
      template <class C> static long test(...);
      static const bool have_create = sizeof(test<T>(0)) == sizeof(char);
    };

    /// Creator<T>::func() will return a pointer to a function of type T * (*)(const ArchiveElement &)
    /// that can create and deserialize instances of T. If there is a static method T::create
    /// that is correct type, then it is used. Otherwise we have a default implementation that
    /// just calls default constuctor and deserialize(element).
    ///
    /// Typical use: T * t = Creator<T>::func()(element);
    template <typename T, bool have_create = FactoryInfo<T>::have_create> struct Creator
    {
      inline static typename FactoryInfo<T>::Func func() { return &create; }
      inline static T * create(const ArchiveElement & element)
      {
        T * t = new T();
        t->deserialize(element);
        return t;
      }
    };

    template <typename T> struct Creator<T, true>
    {
      inline static typename FactoryInfo<T>::Func func() { return &T::create; }
    };

    /// @endcond

    /// Serializes object t to new element that is added to the archive.
    /// @param archive The serializer archive that is used to create the new
    ///                element and maintains the serialization state and options.
    /// @param t Object to be serialized
    /// @return The new serialized element.
    template <typename T>
    ArchiveElement serialize(Archive & archive, const T & t);

    /// Deserializes an element. If deserialization fails or the template type
    /// is not compatible with the data in the element, an object of T created
    /// by its default constructor or NULL is returned.
    /// @param element Serialized element that holds the data that should be deserialized.
    /// @return Deserialized type
    template <typename T>
    typename remove_const<T>::Type deserialize(const ArchiveElement & element);

    /// Compatibility function that deserializes DOMElement.
    /// Use deserialize(const ArchiveElement&) instead.
    /// @param element XML element that is deserialized
    /// @return Deserialized type
    template <typename T>
    typename remove_const<T>::Type deserializeXML(const DOMElement & element);

    /// @cond

    /// Default implementation for "other" types.
    /// Implementations need to be inside of a struct because of partial template specialization
    template <typename T, int type_id = Trait<T>::type>
    struct Impl
    {
      inline static ArchiveElement serialize(Archive &archive, const T & t)
      {
        ArchiveElement elem = archive.createElement(tagName<T>());
        elem.set(Radiant::StringUtils::stringify(t));
        return elem;
      }

      inline static typename remove_const<T>::Type deserialize(const ArchiveElement & element)
      {
        /// @todo should use something else than stringstream
        std::istringstream is(element.get().toUtf8().data());
        typename remove_const<T>::Type t;
        is >> t;
        return t;
      }
    };

    /// Template specialization for QString.
    template < >
    struct Impl<QString>
    {
      inline static ArchiveElement serialize(Archive &archive, const QString & t)
      {
        ArchiveElement elem = archive.createElement("QString");
        elem.set(t);
        return elem;
      }

      inline static QString deserialize(const ArchiveElement & element)
      {
        return element.get();
      }
    };

    template <typename T>
    struct Impl<T*, Type::other>
    {
      inline static ArchiveElement serialize(Archive & archive, const T * t)
      {
        if(!t) return ArchiveElement();
        ArchiveElement elem = archive.createElement(tagName<T>());
        elem.set(Radiant::StringUtils::stringify(*t));
        return elem;
      }

      inline static T * deserialize(const ArchiveElement & element)
      {
        typedef typename remove_const<T>::Type T2;
        std::istringstream is(element.get().toStdString());
        T2 * t = new T2;
        is >> *t;
        return t;
      }
    };

    template <typename T>
    struct Impl<T, Type::smart_ptr>
    {
      inline static ArchiveElement serialize(Archive & archive, const T & t)
      {
        if(!t) return ArchiveElement();
        return Serializer::serialize(archive, t.get());
      }

      inline static typename remove_const<T>::Type deserialize(const ArchiveElement & element)
      {
        return T(Serializer::deserialize<typename T::element_type*>(element));
      }
    };

    template <typename T>
    struct Impl<T, Type::serializable>
    {
      inline static ArchiveElement serialize(Archive & doc, const T & t)
      {
        return t.serialize(doc);
      }

      inline static T deserialize(const ArchiveElement & element)
      {
        T t;
        t.deserialize(element);
        return t;
      }
    };

    template <typename T>
    struct Impl<T*, Type::serializable>
    {
      inline static ArchiveElement serialize(Archive & archive, const T * t)
      {
        if(!t) return ArchiveElement();
        return t->serialize(archive);
      }

      inline static T * deserialize(const ArchiveElement & element)
      {
        return Creator<T>::func()(element);
      }
    };

    template <typename T>
    struct Impl<T, Type::pair>
    {
      inline static ArchiveElement serialize(Archive & archive, const T & pair)
      {
        ArchiveElement elem = archive.createElement("pair");
        elem.add(Serializer::serialize(archive, pair.first));
        elem.add(Serializer::serialize(archive, pair.second));
        return elem;
      }

      inline static T deserialize(const ArchiveElement & element)
      {
        typedef typename T::first_type A;
        typedef typename T::second_type B;

        ArchiveElement::Iterator it = element.children();
        ArchiveElement a = *it;
        ArchiveElement b = *(++it);

        if (!it || ++it) {
          Radiant::error("pair size is not 2");
          return T();
        }

        return std::make_pair(Serializer::deserialize<A>(a),
                              Serializer::deserialize<B>(b));
      }
    };

    template <typename T>
    inline ArchiveElement serialize(Archive & archive, const T & t)
    {
      return Impl<T>::serialize(archive, t);
    }

    template <typename T>
    inline ArchiveElement serialize(Archive & archive, const T * t)
    {
      return Impl<T*>::serialize(archive, t);
    }

    template <typename T>
    inline typename remove_const<T>::Type deserialize(const ArchiveElement & element)
    {
      return Impl<typename remove_const<T>::Type>::deserialize(element);
    }

    template <typename T>
    inline typename remove_const<T>::Type deserializeXML(const DOMElement & element)
    {
      XMLArchiveElement e(element);
      return deserialize<T>(e);
    }
    /// @endcond

    /// Serialize object to a XML file. Example usage:
    /// @code
    /// Serializer::serializeXML("widget.xml", widget, SerializationOptions::ONLY_CHANGED);
    /// @endcode
    /// @param filename output xml filename
    /// @param t Object to be serialized
    /// @param opts Bitmask of SerializationOptions::Options
    /// @return True on success
    template <typename T>
    inline bool serializeXML(const QString & filename, const T & t,
                             unsigned int opts = SerializationOptions::DEFAULTS)
    {
      XMLArchive archive(opts);
      ArchiveElement e = serialize<T>(archive, t);
      if(e.isNull()) {
        return false;
      }
      archive.setRoot(e);
      return archive.writeToFile(filename.toUtf8().data());
    }

    /// Deserialize object from a XML file. Example usage:
    /// @code
    /// Widget * widget = Serializer::deserializeXML<Widget*>("widget.xml");
    /// @endcode
    /// @param filename Name of the XML file
    /// @return Serialized object
    template <typename T>
    inline T deserializeXML(const QString & filename)
    {
      XMLArchive archive;

      if(!archive.readFromFile(filename.toUtf8().data()))
        return T();

      ArchiveElement e = archive.root();
      return deserialize<T>(e);
    }
  }
}

#endif // VALUABLE_SERIALIZER_HPP
