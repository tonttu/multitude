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
#include <Valuable/ValueObject.hpp>

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
  class VALUABLE_API ValueVector : public ValueObjectT<VectorType>
  {
    typedef ValueObjectT<VectorType> Base;
    typedef typename VectorType::type ElementType;
    enum { N = VectorType::Elements };

    public:
      ValueVector() : Base() {}
      /// @copydoc ValueObject::ValueObject(HasValues *, const QString &, bool transit)
      /// @param v The value of this object
      ValueVector(HasValues * parent, const QString & name, const VectorType & v = VectorType(), bool transit = false)
        : Base(parent, name, v, transit) {}

      virtual ~ValueVector();

      /// Assigns a vector
      ValueVector<VectorType> & operator = (const VectorType & v) {
        if(Base::m_value != v) {
          Base::m_value = v;
          this->emitChange();
        }
        return *this;
      }

      /// Assigns by addition
      ValueVector<VectorType> & operator += (const VectorType & v) { return (*this = Base::m_value + v); }
      /// Assigns by subtraction
      ValueVector<VectorType> & operator -= (const VectorType & v) { return (*this = Base::m_value - v); }

      /// Subtraction operator
      VectorType operator -
      (const VectorType & v) const { return Base::m_value - v; }
      /// Addition operator
      VectorType operator +
      (const VectorType & v) const { return Base::m_value + v; }

      /** Access vector elements by their index.

        @return Returns the ith element. */
      ElementType operator [] (int i) const { return Base::m_value[i]; }

      /// Returns the data in its native format
      const ElementType * data() const
      { return Base::m_value.data(); }

      virtual void processMessage(const char * id, Radiant::BinaryData & data);
      virtual bool deserialize(ArchiveElement & element);

      const char * type() const;

      /// Sets the value
      virtual bool set(const VectorType & v);

      /** Returns the internal vector object as a constant reference. */
      const VectorType & asVector() const { return Base::m_value; }

      QString asString(bool * const ok = 0) const;

      /// Returns the ith element
      inline const ElementType & get(int i) const { return Base::m_value[i]; }

      /// Returns the first component
      inline const ElementType & x() const { return Base::m_value[0]; }
      /// Returns the second component
      inline const ElementType & y() const { return Base::m_value[1]; }

      /// Normalizes the vector
      inline void normalize(ElementType len = 1.0)
      {
        VectorType vector = Base::m_value;
        vector.normalize(len);
        *this = vector;
      }
  };

  /// An integer vector2 value object
  typedef ValueVector<Nimble::Vector2i> ValueVector2i;
  /// An integer vector3 value object
  typedef ValueVector<Nimble::Vector3i> ValueVector3i;
  /// An integer vector4 value object
  typedef ValueVector<Nimble::Vector4i> ValueVector4i;

  /// A float vector2 value object
  typedef ValueVector<Nimble::Vector2f> ValueVector2f;
  /// A float vector3 value object
  typedef ValueVector<Nimble::Vector3f> ValueVector3f;
  /// A float vector4 value object
  typedef ValueVector<Nimble::Vector4f> ValueVector4f;

}

#endif
