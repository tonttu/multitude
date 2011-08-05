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

#ifndef VALUABLE_VALUE_VECTOR_HPP
#define VALUABLE_VALUE_VECTOR_HPP

#include <Valuable/Export.hpp>
#include <Valuable/AttributeObject.hpp>

#include <Nimble/Vector4.hpp>

#include <Radiant/StringUtils.hpp>

#include <sstream>

namespace Valuable
{

  /** A template class for vector values.

      This class is used to implement all the normal vector value
      objects.
   */
  template<class VectorType>
  class VALUABLE_API AttributeVector : public AttributeT<VectorType>
  {
    typedef AttributeT<VectorType> Base;
    typedef typename VectorType::type ElementType;
    enum { N = VectorType::Elements };

    public:
      using Base::operator =;
      using Base::value;

      AttributeVector() : Base() {}
      /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
      /// @param v The value of this object
      AttributeVector(Node * host, const QString & name, const VectorType & v = VectorType(), bool transit = false)
        : Base(host, name, v, transit) {}

      virtual ~AttributeVector();

      /// Assigns by addition
      AttributeVector<VectorType> & operator += (const VectorType & v) { *this = value() + v; return *this; }
      /// Assigns by subtraction
      AttributeVector<VectorType> & operator -= (const VectorType & v) { *this = value() - v; return *this; }

      /// Subtraction operator
      VectorType operator -
      (const VectorType & v) const { return value() - v; }
      /// Addition operator
      VectorType operator +
      (const VectorType & v) const { return value() + v; }

      /// Access vector elements by their index.
      /// @param i Index, starting from zero
      /// @return Returns the ith element.
      ElementType operator [] (int i) const { return value()[i]; }

      /// Returns the data in its native format
      const ElementType * data() const
      { return value().data(); }

      virtual void processMessage(const char * id, Radiant::BinaryData & data);
      virtual bool deserialize(const ArchiveElement & element);

      const char * type() const;

      /// Sets the value
      virtual bool set(const VectorType & v, Attribute::Layer layer = Attribute::OVERRIDE);

      /// Returns the internal vector object as a constant reference.
      /// @return The wrapped vector value
      const VectorType & asVector() const { return value(); }

      QString asString(bool * const ok = 0) const;

      /// Returns the ith element
      inline const ElementType & get(int i) const { return value()[i]; }

      /// Returns the first component
      inline const ElementType & x() const { return value()[0]; }
      /// Returns the second component
      inline const ElementType & y() const { return value()[1]; }

      /// Normalizes the vector
      inline void normalize(ElementType len = 1.0)
      {
        VectorType vector = value();
        vector.normalize(len);
        *this = vector;
      }
  };

  /// An integer vector2 value object
  typedef AttributeVector<Nimble::Vector2i> AttributeVector2i;
  /// An integer vector3 value object
  typedef AttributeVector<Nimble::Vector3i> AttributeVector3i;
  /// An integer vector4 value object
  typedef AttributeVector<Nimble::Vector4i> AttributeVector4i;

  /// A float vector2 value object
  typedef AttributeVector<Nimble::Vector2f> AttributeVector2f;
  /// A float vector3 value object
  typedef AttributeVector<Nimble::Vector3f> AttributeVector3f;
  /// A float vector4 value object
  typedef AttributeVector<Nimble::Vector4f> AttributeVector4f;

}

#endif
