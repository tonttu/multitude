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

#define VALUEMIT_STD_OP emitChange(); return *this;

namespace Valuable
{

  /** A template class for vevctor values.

      This class is used to implement all the normal vector value
      objects.
   */
  template<class VectorType, typename ElementType, int N>
  class VALUABLE_API ValueVector : public ValueObjectT<VectorType>
  {
    typedef ValueObjectT<VectorType> Base;
    public:
      ValueVector() : Base() {}
      /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
      /// @param v The value of this object
      ValueVector(HasValues * parent, const std::string & name, const VectorType & v = VectorType(), bool transit = false)
        : Base(parent, name, v, transit) {}

      virtual ~ValueVector();

      /// Assigns a vector
      ValueVector<VectorType, ElementType, N> & operator =
      (const VectorType & v) { Base::m_value = v; this->emitChange(); return *this; }

      /// Assigns by addition
      ValueVector<VectorType, ElementType, N> & operator +=
      (const VectorType & v) { Base::m_value += v; this->emitChange(); return *this; }
      /// Assigns by subtraction
      ValueVector<VectorType, ElementType, N> & operator -=
      (const VectorType & v) { Base::m_value -= v; this->emitChange(); return *this; }

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
     const ElementType * native() const
    { return Base::m_value.data(); }

    virtual void processMessage(const char * id, Radiant::BinaryData & data);
    virtual bool deserialize(ArchiveElement & element);

    const char * type() const;

    /// Sets the value
      virtual bool set(const VectorType & v);

      /** Returns the internal vector object as a constant reference. */
      const VectorType & asVector() const { return Base::m_value; }
      /** Returns the internal vector object as a constant reference. */
      const VectorType & operator * () const { return Base::m_value; }

      std::string asString(bool * const ok = 0) const;

      /// Returns the ith element
      inline const ElementType & get(int i) const { return Base::m_value[i]; }
      /// Returns a pointer to the first element
      inline const ElementType * data() const { return Base::m_value.data(); }

      /// Returns the first component
      inline const ElementType & x() const { return Base::m_value[0]; }
      /// Returns the second component
      inline const ElementType & y() const { return Base::m_value[1]; }

      /// Normalizes the vector
      inline void normalize(ElementType len = 1.0)
      {
        Base::m_value.normalize(len);
        this->emitChange();
      }
  };

  /// An integer vector2 value object
  typedef ValueVector<Nimble::Vector2i, int, 2> ValueVector2i;
  /// An integer vector3 value object
  typedef ValueVector<Nimble::Vector3i, int, 3> ValueVector3i;
  /// An integer vector4 value object
  typedef ValueVector<Nimble::Vector4i, int, 4> ValueVector4i;

  /// A float vector2 value object
  typedef ValueVector<Nimble::Vector2f, float, 2> ValueVector2f;
  /// A float vector3 value object
  typedef ValueVector<Nimble::Vector3f, float, 3> ValueVector3f;
  /// A float vector4 value object
  typedef ValueVector<Nimble::Vector4f, float, 4> ValueVector4f;

}

#undef VALUEMIT_STD_OP

#endif
