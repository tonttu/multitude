/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_SERIALIZER_HPP
#define VALUABLE_SERIALIZER_HPP

#include "DOMElement.hpp"
#include "DOMDocument.hpp"
#include "XMLArchive.hpp"

#include <Radiant/IntrusivePtr.hpp>
#include <Radiant/StringUtils.hpp>
#include <Radiant/Trace.hpp>

#include <QMap>
#include <QStringList>

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
      typedef Radiant::IntrusivePtr<T> (*Func2)(const ArchiveElement &);
      typedef std::shared_ptr<T> (*Func3)(const ArchiveElement &);
      template <Func> struct Test {};
      template <Func2> struct Test2 {};
      template <Func3> struct Test3 {};
      template <class C> static char test(Test<&C::create>*);
      template <class C> static char test(Test2<&C::create>*);
      template <class C> static char test(Test3<&C::create>*);
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

    // This is used for the same reason as Nimble::Decltype
    template <typename T>
    struct CreateType
    {
      typedef decltype(&T::create) type;
    };

    template <typename T> struct Creator<T, true>
    {
      inline static typename CreateType<T>::type func() { return &T::create; }
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
    typename std::remove_cv<T>::type deserialize(const ArchiveElement & element);

    /// Compatibility function that deserializes DOMElement.
    /// Use deserialize(const ArchiveElement&) instead.
    /// @param element XML element that is deserialized
    /// @return Deserialized type
    template <typename T>
    typename std::remove_cv<T>::type deserializeXML(const DOMElement & element);

    /// @cond

    /// Default implementation for "other" types.
    /// Implementations need to be inside of a struct because of partial template specialization
    template <typename T, int type_id = Trait<T>::type>
    struct Impl
    {
      inline static ArchiveElement serialize(Archive &archive, const T & t)
      {
        ArchiveElement elem = archive.createElement(tagName<T>());
        elem.set(Radiant::StringUtils::toString(t));
        return elem;
      }

      inline static typename std::remove_cv<T>::type deserialize(const ArchiveElement & element)
      {
        return Radiant::StringUtils::fromString<typename std::remove_cv<T>::type>(element.get().toUtf8());
      }
    };

    /// Template specialization for QString.
    template < >
    struct Impl<QString>
    {
      inline static ArchiveElement serialize(Archive & archive, const QString & t)
      {
        ArchiveElement elem = archive.createElement("string");
        elem.set(t);
        return elem;
      }

      inline static QString deserialize(const ArchiveElement & element)
      {
        return element.get();
      }
    };

    /// Template specialization for QStringList.
    template < >
    struct Impl<QStringList>
    {
      inline static ArchiveElement serialize(Archive & archive, const QStringList & t)
      {
        Valuable::ArchiveElement strlist = archive.createElement("string-list");
        for (const QString & str: t) {
          Valuable::ArchiveElement e = archive.createElement("string");
          e.set(str);
          strlist.add(e);
        }
        return strlist;
      }

      inline static QStringList deserialize(const ArchiveElement & element)
      {
        QStringList lst;
        for (auto it = element.children(); it; ++it) {
          if ((*it).name() == "string") {
            lst << (*it).get();
          } else {
            Radiant::warning("deserialize # Unknown tag %s", (*it).name().toUtf8().data());
          }
        }
        return lst;
      }
    };

    /// Template specialization for QMap<QString, QString>.
    template < >
    struct Impl<QMap<QString, QString> >
    {
      inline static ArchiveElement serialize(Archive & archive, const QMap<QString, QString> & t)
      {
        Valuable::ArchiveElement strmap = archive.createElement("string-map");
        for (auto it = t.constBegin(); it != t.constEnd(); ++it) {
          Valuable::ArchiveElement e = archive.createElement("pair");

          Valuable::ArchiveElement k = archive.createElement("string");
          k.set(it.key());
          e.add(k);

          Valuable::ArchiveElement v = archive.createElement("string");
          v.set(it.value());
          e.add(v);

          strmap.add(e);
        }
        return strmap;
      }

      inline static QMap<QString, QString> deserialize(const ArchiveElement & element)
      {
        QMap<QString, QString> map;
        for (auto it = element.children(); it; ++it) {
          if ((*it).name() == "pair") {
            auto cIt = (*it).children();
            QString key = (*cIt).get();
            ++cIt;
            map[key] = (*cIt).get();
          } else {
            Radiant::warning("deserialize # Unknown tag %s", (*it).name().toUtf8().data());
          }
        }
        return map;
      }
    };

    template <typename T>
    struct Impl<T*, Type::other>
    {
      inline static ArchiveElement serialize(Archive & archive, const T * t)
      {
        if(!t) return ArchiveElement();
        ArchiveElement elem = archive.createElement(tagName<T>());
        elem.set(Radiant::StringUtils::toString(*t));
        return elem;
      }

      inline static T * deserialize(const ArchiveElement & element)
      {
        typedef typename std::remove_cv<T>::type T2;
        std::istringstream is(element.get().toStdString());
        T2 * t = new T2;
        is >> *t;
        return t;
      }
    };

    template <typename T, bool have_create = FactoryInfo<typename T::element_type>::have_create> struct SmartPtrCreate
    {
      inline static typename std::remove_cv<T>::type deserialize(const ArchiveElement & element)
      {
        return T(Serializer::deserialize<typename T::element_type*>(element));
      }
    };

    template <typename T> struct SmartPtrCreate<T, true>
    {
      inline static typename std::remove_cv<T>::type deserialize(const ArchiveElement & element)
      {
        return T::element_type::create(element);
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

      inline static typename std::remove_cv<T>::type deserialize(const ArchiveElement & element)
      {
        return SmartPtrCreate<T>::deserialize(element);
      }
    };

    // This is used for the same reason as Nimble::Decltype
    template <typename T>
    struct DeserializerType
    {
      typedef decltype(T::create(ArchiveElement())) type;
    };

    template <typename T>
    struct Impl<Radiant::IntrusivePtr<T>, Type::smart_ptr>
    {
      inline static ArchiveElement serialize(Archive & archive, const Radiant::IntrusivePtr<T> & t)
      {
        if(!t) return ArchiveElement();
        return t->serialize(archive);
      }

      inline static typename DeserializerType<T>::type deserialize (const ArchiveElement & element)
      {
        return T::create(element);
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
        auto firstElement = Serializer::serialize(archive, pair.first);
        auto secondElement = Serializer::serialize(archive, pair.second);

        // If either element of the pair is a null element (e.g.
        // isSerializable() is false), do not serialize anything
        if(firstElement.isNull() || secondElement.isNull())
          return ArchiveElement();

        // Both elements are valid, serialize the pair
        ArchiveElement elem = archive.createElement("pair");
        elem.add(firstElement);
        elem.add(secondElement);

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
          Radiant::error("Serializer:deserialize # failed to deserialize a pair. Could not deserialize two elements.");
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
      if(!t) return ArchiveElement();
      return Impl<T>::serialize(archive, *t);
    }

    template <typename T>
    inline typename std::remove_cv<T>::type deserialize(const ArchiveElement & element)
    {
      return Impl<typename std::remove_cv<T>::type>::deserialize(element);
    }

    template <typename T>
    inline typename std::remove_cv<T>::type deserializeXML(const DOMElement & element)
    {
      XMLArchiveElement e(element);
      return deserialize<T>(e);
    }
    /// @endcond

    /// Serialize object to a XML file. Example usage:
    /// @code
    /// Serializer::serializeXML("widget.xml", widget);
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
      return archive.writeToFile(filename);
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

      if(!archive.readFromFile(filename))
        return T();

      ArchiveElement e = archive.root();
      return deserialize<T>(e);
    }
  }
}

#endif // VALUABLE_SERIALIZER_HPP
