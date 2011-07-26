/* COPYRIGHT
 */

#ifndef VALUABLE_VALUE_RECT_HPP
#define VALUABLE_VALUE_RECT_HPP

#include <Valuable/Export.hpp>
#include <Valuable/ValueObject.hpp>

#include <Nimble/Rect.hpp>

#define VALUEMIT_STD_OP this->emitChange(); return *this;

namespace Valuable
{

  /// A valuable object holding a Nimble::Rect object
  template <class T>
  class VALUABLE_API ValueRectT : public ValueObjectT<Nimble::RectT<T> >
  {
    typedef ValueObjectT<Nimble::RectT<T> > Base;
  public:
    /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
    /// @param r The rectangle to be stored in the ValueRect
    ValueRectT(HasValues * host, const std::string & name, const Nimble::RectT<T> & r, bool transit = false);

    /// Copies a rectangle
    ValueRectT & operator = (const Nimble::RectT<T> & r) { this->m_value = r; VALUEMIT_STD_OP }

    const char * type() const;

    std::string asString(bool * const ok = 0) const;

    bool deserialize(ArchiveElement & element);

    /// Converts the object to rectangle
    Nimble::RectT<T> asRect() const { return this->m_value; }
  };

  /// Default floating point ValueRectT typedef
  typedef ValueRectT<float> ValueRect;
  /// ValueRectT of floats
  typedef ValueRectT<float> ValueRectf;
  /// ValueRectT of doubles
  typedef ValueRectT<double> ValueRectd;
  /// ValueRectT of ints
  typedef ValueRectT<int> ValueRecti;
}

#undef VALUEMIT_STD_OP

#endif
